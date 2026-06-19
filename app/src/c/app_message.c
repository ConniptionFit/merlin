#include "app_message.h"

#include <string.h>

#define OUTBOX_RETRY_MAX 5
#define OUTBOX_RETRY_BASE_MS 50

static AppMessageCallbacks s_callbacks;
static AppTimer *s_outbox_retry_timer = NULL;
static char s_pending_text[512];
static bool s_pending_feedback = false;
static int s_outbox_retry_attempt = 0;

static void prv_dispatch_inbox(DictionaryIterator *iter) {
  Tuple *tuple = dict_find(iter, MESSAGE_KEY_RESPONSE);
  if (tuple && tuple->value->cstring) {
    if (s_callbacks.on_response_chunk) {
      s_callbacks.on_response_chunk(tuple->value->cstring);
    }
  }

  tuple = dict_find(iter, MESSAGE_KEY_RESPONSE_DONE);
  if (tuple) {
    if (s_callbacks.on_response_done) {
      s_callbacks.on_response_done();
    }
  }

  tuple = dict_find(iter, MESSAGE_KEY_STATUS);
  if (tuple && tuple->value->cstring) {
    if (s_callbacks.on_status) {
      s_callbacks.on_status(tuple->value->cstring);
    }
  }

  tuple = dict_find(iter, MESSAGE_KEY_TIMER_DURATION);
  if (tuple) {
    const char *name = NULL;
    Tuple *name_tuple = dict_find(iter, MESSAGE_KEY_TIMER_NAME);
    if (name_tuple && name_tuple->value->cstring) {
      name = name_tuple->value->cstring;
    }
    if (s_callbacks.on_timer) {
      s_callbacks.on_timer(tuple->value->int32, name);
    }
  }
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  prv_dispatch_inbox(iter);
}

static void prv_inbox_dropped(AppMessageResult reason, DictionaryIterator *iter, void *context) {
  if (iter) {
    prv_dispatch_inbox(iter);
  }
}

static void prv_outbox_failed(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Outbox failed: %d", reason);
}

static void prv_outbox_sent(DictionaryIterator *iter, void *context) {
  s_outbox_retry_attempt = 0;
}

static bool prv_send_pending(void) {
  DictionaryIterator *iter = NULL;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK || !iter) {
    return false;
  }

  if (s_pending_feedback) {
    dict_write_cstring(iter, MESSAGE_KEY_FEEDBACK_TEXT, s_pending_text);
    dict_write_int8(iter, MESSAGE_KEY_FEEDBACK_FLAG, 1);
  } else {
    dict_write_cstring(iter, MESSAGE_KEY_DICTATION_TEXT, s_pending_text);
  }

  result = app_message_outbox_send();
  return result == APP_MSG_OK;
}

static void prv_outbox_retry(void *data) {
  s_outbox_retry_timer = NULL;
  if (prv_send_pending()) {
    return;
  }

  s_outbox_retry_attempt++;
  if (s_outbox_retry_attempt >= OUTBOX_RETRY_MAX) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox retry exhausted");
    return;
  }

  int delay = OUTBOX_RETRY_BASE_MS * (1 << s_outbox_retry_attempt);
  if (delay > 1000) {
    delay = 1000;
  }
  s_outbox_retry_timer = app_timer_register(delay, prv_outbox_retry, NULL);
}

void app_message_init(void) {
  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_failed(prv_outbox_failed);
  app_message_register_outbox_sent(prv_outbox_sent);

  const uint32_t size = 2048;
  app_message_inbox_size_maximum_set(size);
  app_message_outbox_size_maximum_set(size);
}

void app_message_set_callbacks(const AppMessageCallbacks *callbacks) {
  if (callbacks) {
    s_callbacks = *callbacks;
  }
}

void app_message_send_dictation(const char *text, bool feedback) {
  if (!text) {
    return;
  }

  strncpy(s_pending_text, text, sizeof(s_pending_text) - 1);
  s_pending_text[sizeof(s_pending_text) - 1] = '\0';
  s_pending_feedback = feedback;
  s_outbox_retry_attempt = 0;

  if (s_outbox_retry_timer) {
    app_timer_cancel(s_outbox_retry_timer);
    s_outbox_retry_timer = NULL;
  }

  if (prv_send_pending()) {
    return;
  }

  prv_outbox_retry(NULL);
}

StatusCode app_message_send_with_retry(DictionaryIterator *iter) {
  return app_message_outbox_send();
}
