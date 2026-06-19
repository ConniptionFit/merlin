#include "session_window.h"
#include "app_message.h"
#include "merlin_theme.h"

#include <string.h>

#define DICTATION_BUF_SIZE 512
#define PROMPT_BUF_SIZE 512
#define RESPONSE_BUF_SIZE 256
#define STATUS_BUF_SIZE 96
#define SCROLL_PADDING 6
#define LABEL_HEIGHT 18

typedef struct SessionWindow {
  Window *window;
  DictationSession *dictation;
  ScrollLayer *scroll_layer;
  Layer *scroll_content;
  TextLayer *you_label_layer;
  TextLayer *prompt_layer;
  TextLayer *merlin_label_layer;
  TextLayer *response_layer;
  TextLayer *status_layer;
  Layer *divider_layer;
  bool dictation_pending;
  bool feedback_mode;
  bool awaiting_response;
  bool pending_submit;
  bool pending_feedback;
  char prompt_text[PROMPT_BUF_SIZE];
  char response_text[RESPONSE_BUF_SIZE];
  char status_text[STATUS_BUF_SIZE];
} SessionWindow;

static SessionWindow *s_session = NULL;

static void prv_refresh_scroll(SessionWindow *sw);
static void prv_set_status(SessionWindow *sw, const char *status);
static void prv_start_dictation(SessionWindow *sw, bool feedback);
static void prv_submit_prompt(SessionWindow *sw, const char *prompt, bool feedback);
static void prv_dictation_status_callback(DictationSession *session, DictationSessionStatus status,
                                          char *transcription, void *context);

static void prv_divider_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_stroke_color(ctx, MERLIN_TEXT_PRIMARY);
  for (int16_t x = 0; x < bounds.size.w; x += 4) {
    graphics_draw_pixel(ctx, GPoint(x, bounds.size.h / 2));
  }
}

static void prv_vibe_done(void) {
  static uint32_t segments[] = {50, 50, 100};
  VibePattern pattern = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pattern);
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

static void prv_scroll_to_bottom(ScrollLayer *scroll_layer) {
  GSize content_size = scroll_layer_get_content_size(scroll_layer);
  GRect frame = layer_get_bounds(scroll_layer_get_layer(scroll_layer));
  int16_t max_offset_y = content_size.h - frame.size.h;
  if (max_offset_y < 0) {
    max_offset_y = 0;
  }
  scroll_layer_set_content_offset(scroll_layer, GPoint(0, max_offset_y), false);
}

static void prv_refresh_scroll(SessionWindow *sw) {
  Layer *prompt = text_layer_get_layer(sw->prompt_layer);
  Layer *divider = sw->divider_layer;
  Layer *merlin_label = text_layer_get_layer(sw->merlin_label_layer);
  Layer *response = text_layer_get_layer(sw->response_layer);
  Layer *status = text_layer_get_layer(sw->status_layer);

  GRect prompt_frame = layer_get_frame(prompt);
  GRect response_frame = layer_get_frame(response);
  GRect status_frame = layer_get_frame(status);

  GRect divider_frame = layer_get_frame(divider);
  divider_frame.origin.y = prompt_frame.origin.y + prompt_frame.size.h + SCROLL_PADDING;
  layer_set_frame(divider, divider_frame);

  GRect merlin_frame = layer_get_frame(merlin_label);
  merlin_frame.origin.y = divider_frame.origin.y + divider_frame.size.h + SCROLL_PADDING;
  layer_set_frame(merlin_label, merlin_frame);

  response_frame.origin.y = merlin_frame.origin.y + merlin_frame.size.h + 2;
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
  prv_scroll_to_bottom(sw->scroll_layer);
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

static void prv_submit_prompt(SessionWindow *sw, const char *prompt, bool feedback) {
  if (!prompt || prompt[0] == '\0') {
    return;
  }

  strncpy(sw->prompt_text, prompt, sizeof(sw->prompt_text) - 1);
  sw->prompt_text[sizeof(sw->prompt_text) - 1] = '\0';
  text_layer_set_text(sw->prompt_layer, sw->prompt_text);

  sw->response_text[0] = '\0';
  prv_update_response_text(sw);
  sw->awaiting_response = true;
  sw->feedback_mode = feedback;
  prv_set_status(sw, "Thinking...");

  app_message_send_dictation(prompt, feedback);
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

  prv_submit_prompt(sw, transcription, sw->feedback_mode);
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

  window_set_background_color(window, MERLIN_BG_SURFACE);

  sw->dictation = dictation_session_create(DICTATION_BUF_SIZE, prv_dictation_status_callback, sw);
  dictation_session_enable_confirmation(sw->dictation, false);
  dictation_session_enable_error_dialogs(sw->dictation, false);

  sw->scroll_layer = scroll_layer_create(bounds);
  scroll_layer_set_shadow_hidden(sw->scroll_layer, true);

  sw->scroll_content = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  scroll_layer_add_child(sw->scroll_layer, sw->scroll_content);

  const int16_t text_w = bounds.size.w - 16;

  sw->you_label_layer = text_layer_create(GRect(8, 8, text_w, LABEL_HEIGHT));
  text_layer_set_font(sw->you_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text(sw->you_label_layer, "You");
  text_layer_set_text_color(sw->you_label_layer, MERLIN_TEXT_PRIMARY);
  text_layer_set_background_color(sw->you_label_layer, GColorClear);
  layer_add_child(sw->scroll_content, text_layer_get_layer(sw->you_label_layer));

  sw->prompt_layer = text_layer_create(GRect(8, 26, text_w, 72));
  text_layer_set_font(sw->prompt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(sw->prompt_layer, "Tap Select to speak");
  text_layer_set_text_color(sw->prompt_layer, MERLIN_TEXT_PRIMARY);
  text_layer_set_background_color(sw->prompt_layer, GColorClear);
  text_layer_set_overflow_mode(sw->prompt_layer, GTextOverflowModeWordWrap);
  layer_add_child(sw->scroll_content, text_layer_get_layer(sw->prompt_layer));

  sw->divider_layer = layer_create(GRect(8, 104, text_w, 4));
  layer_set_update_proc(sw->divider_layer, prv_divider_update);
  layer_add_child(sw->scroll_content, sw->divider_layer);

  sw->merlin_label_layer = text_layer_create(GRect(8, 112, text_w, LABEL_HEIGHT));
  text_layer_set_font(sw->merlin_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text(sw->merlin_label_layer, "Merlin");
  text_layer_set_text_color(sw->merlin_label_layer, MERLIN_TEXT_PRIMARY);
  text_layer_set_background_color(sw->merlin_label_layer, GColorClear);
  layer_add_child(sw->scroll_content, text_layer_get_layer(sw->merlin_label_layer));

  sw->response_layer = text_layer_create(GRect(8, 132, text_w, 120));
  text_layer_set_font(sw->response_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_color(sw->response_layer, MERLIN_TEXT_PRIMARY);
  text_layer_set_background_color(sw->response_layer, GColorClear);
  text_layer_set_overflow_mode(sw->response_layer, GTextOverflowModeWordWrap);
  layer_add_child(sw->scroll_content, text_layer_get_layer(sw->response_layer));

  sw->status_layer = text_layer_create(GRect(8, 256, text_w, 24));
  text_layer_set_font(sw->status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(sw->status_layer, MERLIN_TEXT_SECONDARY);
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
  if (sw->you_label_layer) {
    text_layer_destroy(sw->you_label_layer);
    sw->you_label_layer = NULL;
  }
  if (sw->prompt_layer) {
    text_layer_destroy(sw->prompt_layer);
    sw->prompt_layer = NULL;
  }
  if (sw->divider_layer) {
    layer_destroy(sw->divider_layer);
    sw->divider_layer = NULL;
  }
  if (sw->merlin_label_layer) {
    text_layer_destroy(sw->merlin_label_layer);
    sw->merlin_label_layer = NULL;
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

  if (sw->pending_submit && sw->prompt_text[0] != '\0') {
    sw->pending_submit = false;
    prv_submit_prompt(sw, sw->prompt_text, sw->pending_feedback);
  }
}

static void prv_ensure_session(void) {
  if (s_session) {
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
}

void session_window_push(void) {
  prv_ensure_session();
  window_stack_push(s_session->window, true);
}

void session_window_push_with_prompt(const char *prompt, bool feedback) {
  if (!prompt || prompt[0] == '\0') {
    session_window_push();
    return;
  }

  prv_ensure_session();

  strncpy(s_session->prompt_text, prompt, sizeof(s_session->prompt_text) - 1);
  s_session->prompt_text[sizeof(s_session->prompt_text) - 1] = '\0';
  s_session->pending_feedback = feedback;
  s_session->pending_submit = true;

  window_stack_push(s_session->window, true);

  if (s_session->prompt_layer) {
    s_session->pending_submit = false;
    prv_submit_prompt(s_session, prompt, feedback);
  }
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
