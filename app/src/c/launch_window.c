#include "launch_window.h"
#include "app_message.h"
#include "help_window.h"
#include "merlin_theme.h"
#include "session_window.h"
#include "speech_bubble_layer.h"
#include "wizard_layer.h"

#include <string.h>

#define DICTATION_BUF_SIZE 512
#define GREETING_BUF_SIZE 64

typedef struct LaunchWindow {
  Window *window;
  DictationSession *dictation;
  ActionBarLayer *action_bar;
  GBitmap *icon_help;
  GBitmap *icon_mic;
  GBitmap *icon_more;
  TextLayer *clock_layer;
  TextLayer *version_layer;
  SpeechBubbleLayer *speech_bubble;
  WizardLayer *wizard;
  bool dictation_pending;
} LaunchWindow;

static LaunchWindow *s_launch = NULL;

static void prv_update_greeting(LaunchWindow *lw);
static void prv_update_clock(LaunchWindow *lw);
static void prv_start_dictation(LaunchWindow *lw);
static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed);

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (!s_launch || !(units_changed & MINUTE_UNIT)) {
    return;
  }
  prv_update_clock(s_launch);
  prv_update_greeting(s_launch);
}

static void prv_update_clock(LaunchWindow *lw) {
  if (!lw || !lw->clock_layer) {
    return;
  }

  static char clock_buf[8];
  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  strftime(clock_buf, sizeof(clock_buf), clock_is_24h_style() ? "%H:%M" : "%I:%M", tm_now);
  text_layer_set_text(lw->clock_layer, clock_buf);
}

static void prv_update_greeting(LaunchWindow *lw) {
  if (!lw || !lw->speech_bubble) {
    return;
  }

  static char greeting[GREETING_BUF_SIZE];
  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  const char *text;

  if (tm_now->tm_hour < 12) {
    text = "Good morning!";
  } else if (tm_now->tm_hour < 17) {
    text = "Good afternoon!";
  } else {
    text = "Good evening!";
  }

  strncpy(greeting, text, sizeof(greeting) - 1);
  greeting[sizeof(greeting) - 1] = '\0';
  speech_bubble_layer_set_text(lw->speech_bubble, greeting);
}

static void prv_dictation_status_callback(DictationSession *session, DictationSessionStatus status,
                                          char *transcription, void *context) {
  LaunchWindow *lw = context;
  lw->dictation_pending = false;

  if (status != DictationSessionStatusSuccess || !transcription || transcription[0] == '\0') {
    speech_bubble_layer_set_text(lw->speech_bubble, "Try again?");
    return;
  }

  session_window_push_with_prompt(transcription, false);
}

static void prv_start_dictation(LaunchWindow *lw) {
  if (lw->dictation_pending) {
    return;
  }

  lw->dictation_pending = true;
  speech_bubble_layer_set_text(lw->speech_bubble, "Listening...");
  dictation_session_start(lw->dictation);
}

static void prv_action_bar_up(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  help_window_push();
}

static void prv_action_bar_select(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  prv_start_dictation(context);
}

static void prv_action_bar_down(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  session_window_push();
}

static void prv_action_bar_config(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, prv_action_bar_up);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_action_bar_select);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_action_bar_down);
}

static void prv_window_load(Window *window) {
  LaunchWindow *lw = window_get_user_data(window);
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  const int16_t content_w = MERLIN_CONTENT_WIDTH(bounds.size.w);

  window_set_background_color(window, MERLIN_BG_PRIMARY);

  lw->dictation = dictation_session_create(DICTATION_BUF_SIZE, prv_dictation_status_callback, lw);
  dictation_session_enable_confirmation(lw->dictation, false);
  dictation_session_enable_error_dialogs(lw->dictation, false);

  lw->clock_layer = text_layer_create(GRect(8, 2, content_w - 16, 44));
  text_layer_set_font(lw->clock_layer, fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS));
  text_layer_set_text_color(lw->clock_layer, MERLIN_TEXT_PRIMARY);
  text_layer_set_background_color(lw->clock_layer, GColorClear);
  layer_add_child(root, text_layer_get_layer(lw->clock_layer));

  lw->speech_bubble = speech_bubble_layer_create(GRect(12, 52, 136, 96));
  layer_add_child(root, speech_bubble_layer_get_layer(lw->speech_bubble));

  lw->wizard = wizard_layer_create(GRect(4, 160, MERLIN_HEAD_WIDTH, MERLIN_HEAD_HEIGHT),
                                   RESOURCE_ID_IMAGE_MERLIN_HEAD);
  layer_add_child(root, wizard_layer_get_layer(lw->wizard));

  lw->version_layer = text_layer_create(GRect(content_w - 68, 210, 60, 16));
  text_layer_set_font(lw->version_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(lw->version_layer, "v1.0.0");
  text_layer_set_text_color(lw->version_layer, MERLIN_TEXT_PRIMARY);
  text_layer_set_background_color(lw->version_layer, GColorClear);
  text_layer_set_text_alignment(lw->version_layer, GTextAlignmentRight);
  layer_add_child(root, text_layer_get_layer(lw->version_layer));

  lw->action_bar = action_bar_layer_create();
  action_bar_layer_set_background_color(lw->action_bar, MERLIN_ACTIONBAR);

  lw->icon_help = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ICON_HELP);
  lw->icon_mic = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ICON_MIC);
  lw->icon_more = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ICON_MORE);
  action_bar_layer_set_icon(lw->action_bar, BUTTON_ID_UP, lw->icon_help);
  action_bar_layer_set_icon(lw->action_bar, BUTTON_ID_SELECT, lw->icon_mic);
  action_bar_layer_set_icon(lw->action_bar, BUTTON_ID_DOWN, lw->icon_more);
  action_bar_layer_set_click_config_provider(lw->action_bar, prv_action_bar_config);
  action_bar_layer_set_context(lw->action_bar, lw);
  Layer *bar_layer = action_bar_layer_get_layer(lw->action_bar);
  GRect bar_frame = layer_get_frame(bar_layer);
  bar_frame.origin.x = bounds.size.w - ACTION_BAR_WIDTH;
  layer_set_frame(bar_layer, bar_frame);
  layer_add_child(root, bar_layer);

  prv_update_clock(lw);
  prv_update_greeting(lw);
}

static void prv_window_unload(Window *window) {
  LaunchWindow *lw = window_get_user_data(window);
  if (!lw) {
    return;
  }

  tick_timer_service_unsubscribe();

  if (lw->dictation) {
    dictation_session_destroy(lw->dictation);
    lw->dictation = NULL;
  }
  if (lw->action_bar) {
    action_bar_layer_destroy(lw->action_bar);
    lw->action_bar = NULL;
  }
  if (lw->icon_help) {
    gbitmap_destroy(lw->icon_help);
    lw->icon_help = NULL;
  }
  if (lw->icon_mic) {
    gbitmap_destroy(lw->icon_mic);
    lw->icon_mic = NULL;
  }
  if (lw->icon_more) {
    gbitmap_destroy(lw->icon_more);
    lw->icon_more = NULL;
  }
  if (lw->clock_layer) {
    text_layer_destroy(lw->clock_layer);
    lw->clock_layer = NULL;
  }
  if (lw->version_layer) {
    text_layer_destroy(lw->version_layer);
    lw->version_layer = NULL;
  }
  if (lw->speech_bubble) {
    speech_bubble_layer_destroy(lw->speech_bubble);
    lw->speech_bubble = NULL;
  }
  if (lw->wizard) {
    wizard_layer_destroy(lw->wizard);
    lw->wizard = NULL;
  }
}

static void prv_window_appear(Window *window) {
  LaunchWindow *lw = window_get_user_data(window);
  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
  prv_update_clock(lw);
  prv_update_greeting(lw);
}

void launch_window_push(void) {
  if (s_launch) {
    window_stack_push(s_launch->window, true);
    return;
  }

  s_launch = malloc(sizeof(LaunchWindow));
  if (!s_launch) {
    return;
  }
  memset(s_launch, 0, sizeof(LaunchWindow));

  s_launch->window = window_create();
  window_set_user_data(s_launch->window, s_launch);
  window_set_window_handlers(s_launch->window, (WindowHandlers){
    .load = prv_window_load,
    .unload = prv_window_unload,
    .appear = prv_window_appear,
  });

  window_stack_push(s_launch->window, true);
}
