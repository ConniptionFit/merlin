#include "session_window.h"
#include "app_message.h"
#include "wizard_layer.h"

#include <string.h>

#define DICTATION_BUF_SIZE 512
#define PROMPT_BUF_SIZE 512
#define RESPONSE_BUF_SIZE 256
#define STATUS_BUF_SIZE 96
#define WIZARD_WIDTH 72
#define WIZARD_HEIGHT 84
#define SCROLL_PADDING 4

typedef struct SessionWindow {
  Window *window;
  DictationSession *dictation;
  ScrollLayer *scroll_layer;
  Layer *scroll_content;
  TextLayer *prompt_layer;
  TextLayer *response_layer;
  TextLayer *status_layer;
  WizardLayer *wizard;
  bool dictation_pending;
  bool feedback_mode;
  bool awaiting_response;
  char prompt_text[PROMPT_BUF_SIZE];
  char response_text[RESPONSE_BUF_SIZE];
  char status_text[STATUS_BUF_SIZE];
} SessionWindow;

static SessionWindow *s_session = NULL;

static void prv_refresh_scroll(SessionWindow *sw);
static void prv_set_status(SessionWindow *sw, const char *status);
static void prv_start_dictation(SessionWindow *sw, bool feedback);
static void prv_dictation_status_callback(DictationSession *session, DictationSessionStatus status,
                                          char *transcription, void *context);

static void prv_vibe_done(void) {
  static const uint32_t segments[] = {50, 50, 100};
  VibeInfo info = {
    .durations = segments,
    .num_segments = 3,
  };
  vibes_enqueue_custom(info);
}

static void prv_heap_warmup(void) {
  void *block = malloc(2048);
  if (block) {
    free(block);
  }
}

static void prv_update_response_text(SessionWindow *sw) {
  text_layer_set_text(sw->response_layer, sw->response_text);
  prv_refresh_scroll(sw);
}

static void prv_refresh_scroll(SessionWindow *sw) {
  Layer *prompt = text_layer_get_layer(sw->prompt_layer);
  Layer *response = text_layer_get_layer(sw->response_layer);
  Layer *status = text_layer_get_layer(sw->status_layer);

  GRect prompt_frame = layer_get_frame(prompt);
  GRect response_frame = layer_get_frame(response);
  GRect status_frame = layer_get_frame(status);

  response_frame.origin.y = prompt_frame.origin.y + prompt_frame.size.h + SCROLL_PADDING;
  layer_set_frame(response, response_frame);

  status_frame.origin.y = response_frame.origin.y + response_frame.size.h + SCROLL_PADDING;
  layer_set_frame(status, status_frame);

  GRect bounds = layer_get_bounds(sw->scroll_content);
  int content_height = status_frame.origin.y + status_frame.size.h + SCROLL_PADDING;
  if (content_height < bounds.size.h) {
    content_height = bounds.size.h;
  }
  layer_set_frame(sw->scroll_content, GRect(0, 0, bounds.size.w, content_height));
  scroll_layer_set_content_size(sw->scroll_layer, GSize(bounds.size.w, content_height));
  scroll_layer_scroll_to_bottom(sw->scroll_layer, false);
}

static void prv_set_status(SessionWindow *sw, const char *status) {
  if (!status) {
    sw->status_text[0] = '\0';
  } else {
    strncpy(sw->status_text, status, sizeof(sw->status_text) - 1);
    sw->status_text[sizeof(sw->status_text) - 1] = '\0';
  }
  text_layer_set_text(sw->status_layer, sw->status_text);
  prv_refresh_scroll(sw);
}

static void prv_start_dictation(SessionWindow *sw, bool feedback) {
  if (sw->dictation_pending || sw->awaiting_response) {
    return;
  }

  prv_heap_warmup();
  sw->feedback_mode = feedback;
  sw->dictation_pending = true;
  prv_set_status(sw, feedback ? "Speak feedback..." : "Listening...");
  dictation_session_start(sw->dictation);
}

static void prv_dictation_status_callback(DictationSession *session, DictationSessionStatus status,
                                          char *transcription, void *context) {
  SessionWindow *sw = context;
  sw->dictation_pending = false;

  if (status != DictationSessionStatusSuccess || !transcription || transcription[0] == '\0') {
    prv_set_status(sw, "Dictation failed");
    return;
  }

  strncpy(sw->prompt_text, transcription, sizeof(sw->prompt_text) - 1);
  sw->prompt_text[sizeof(sw->prompt_text) - 1] = '\0';
  text_layer_set_text(sw->prompt_layer, sw->prompt_text);

  sw->response_text[0] = '\0';
  prv_update_response_text(sw);
  sw->awaiting_response = true;
  prv_set_status(sw, "Thinking...");

  app_message_send_dictation(transcription, sw->feedback_mode);
}

static void prv_select_click(ClickRecognizerRef recognizer, void *context) {
  SessionWindow *sw = context;
  if (!sw->dictation_pending && !sw->awaiting_response) {
    prv_start_dictation(sw, false);
  }
}

static void prv_select_long(ClickRecognizerRef recognizer, void *context) {
  SessionWindow *sw = context;
  if (!sw->dictation_pending) {
    sw->awaiting_response = false;
    prv_start_dictation(sw, true);
  }
}

static void prv_back_click(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
}

static void prv_click_config(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click);
  window_long_click_subscribe(BUTTON_ID_SELECT, 700, prv_select_long, NULL);
  window_single_click_subscribe(BUTTON_ID_BACK, prv_back_click);
}

static void prv_window_load(Window *window) {
  SessionWindow *sw = window_get_user_data(window);
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  sw->dictation = dictation_session_create(DICTATION_BUF_SIZE, prv_dictation_status_callback, sw);
  dictation_session_enable_confirmation(sw->dictation, false);
  dictation_session_enable_error_dialogs(sw->dictation, false);

  GRect wizard_frame = GRect(0, 0, WIZARD_WIDTH, WIZARD_HEIGHT);
  sw->wizard = wizard_layer_create(wizard_frame);
  layer_add_child(root, wizard_layer_get_layer(sw->wizard));

  GRect scroll_frame = GRect(WIZARD_WIDTH + 2, 0, bounds.size.w - WIZARD_WIDTH - 2, bounds.size.h);
  sw->scroll_layer = scroll_layer_create(scroll_frame);
  scroll_layer_set_shadow_hidden(sw->scroll_layer, true);

  sw->scroll_content = layer_create(GRect(0, 0, scroll_frame.size.w, scroll_frame.size.h));
  scroll_layer_add_child(sw->scroll_layer, sw->scroll_content);

  sw->prompt_layer = text_layer_create(GRect(0, 0, scroll_frame.size.w - 4, 72));
  text_layer_set_font(sw->prompt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(sw->prompt_layer, "Tap Select to speak");
  text_layer_set_background_color(sw->prompt_layer, GColorClear);
  layer_add_child(sw->scroll_content, text_layer_get_layer(sw->prompt_layer));

  sw->response_layer = text_layer_create(GRect(0, 76, scroll_frame.size.w - 4, 120));
  text_layer_set_font(sw->response_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(sw->response_layer, GColorClear);
  layer_add_child(sw->scroll_content, text_layer_get_layer(sw->response_layer));

  sw->status_layer = text_layer_create(GRect(0, 200, scroll_frame.size.w - 4, 24));
  text_layer_set_font(sw->status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(sw->status_layer, GColorDarkGray);
  text_layer_set_background_color(sw->status_layer, GColorClear);
  layer_add_child(sw->scroll_content, text_layer_get_layer(sw->status_layer));

  layer_add_child(root, scroll_layer_get_layer(sw->scroll_layer));
  prv_refresh_scroll(sw);
}

static void prv_window_unload(Window *window) {
  SessionWindow *sw = window_get_user_data(window);
  if (!sw) {
    return;
  }

  if (sw->dictation) {
    dictation_session_destroy(sw->dictation);
    sw->dictation = NULL;
  }
  if (sw->wizard) {
    wizard_layer_destroy(sw->wizard);
    sw->wizard = NULL;
  }
  if (sw->prompt_layer) {
    text_layer_destroy(sw->prompt_layer);
    sw->prompt_layer = NULL;
  }
  if (sw->response_layer) {
    text_layer_destroy(sw->response_layer);
    sw->response_layer = NULL;
  }
  if (sw->status_layer) {
    text_layer_destroy(sw->status_layer);
    sw->status_layer = NULL;
  }
  if (sw->scroll_content) {
    layer_destroy(sw->scroll_content);
    sw->scroll_content = NULL;
  }
  if (sw->scroll_layer) {
    scroll_layer_destroy(sw->scroll_layer);
    sw->scroll_layer = NULL;
  }
}

static void prv_window_appear(Window *window) {
  SessionWindow *sw = window_get_user_data(window);
  window_set_click_config_provider_with_context(window, prv_click_config, sw);
  prv_start_dictation(sw, false);
}

void session_window_push(void) {
  if (s_session) {
    window_stack_push(s_session->window, true);
    return;
  }

  s_session = malloc(sizeof(SessionWindow));
  memset(s_session, 0, sizeof(SessionWindow));

  s_session->window = window_create();
  window_set_user_data(s_session->window, s_session);
  window_set_window_handlers(s_session->window, (WindowHandlers){
    .load = prv_window_load,
    .unload = prv_window_unload,
    .appear = prv_window_appear,
  });

  window_stack_push(s_session->window, true);
}

void session_window_handle_response_chunk(const char *chunk) {
  if (!s_session || !chunk) {
    return;
  }

  size_t current_len = strlen(s_session->response_text);
  size_t chunk_len = strlen(chunk);
  if (current_len + chunk_len >= sizeof(s_session->response_text)) {
    chunk_len = sizeof(s_session->response_text) - current_len - 1;
  }
  strncat(s_session->response_text, chunk, chunk_len);
  prv_update_response_text(s_session);
}

void session_window_handle_response_done(void) {
  if (!s_session) {
    return;
  }

  s_session->awaiting_response = false;
  prv_set_status(s_session, "");
  prv_vibe_done();
}

void session_window_handle_status(const char *status) {
  if (!s_session) {
    return;
  }

  s_session->awaiting_response = false;
  prv_set_status(s_session, status ? status : "");
}
