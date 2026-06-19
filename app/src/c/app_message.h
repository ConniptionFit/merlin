#pragma once

#include <pebble.h>

void app_message_init(void);
void app_message_send_dictation(const char *text, bool feedback);
StatusCode app_message_send_with_retry(DictionaryIterator *iter);

typedef struct AppMessageCallbacks {
  void (*on_response_chunk)(const char *chunk);
  void (*on_response_done)(void);
  void (*on_status)(const char *status);
  void (*on_timer)(int32_t duration, const char *name);
} AppMessageCallbacks;

void app_message_set_callbacks(const AppMessageCallbacks *callbacks);
