#include "timer_window.h"
#include "merlin_theme.h"
#include "wizard_layer.h"

#include <string.h>

static Window *s_window = NULL;
static TextLayer *s_title_layer = NULL;
static TextLayer *s_body_layer = NULL;
static WizardLayer *s_wizard = NULL;
static char s_name_buffer[32];

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  window_set_background_color(window, MERLIN_BG_PRIMARY);

  s_title_layer = text_layer_create(GRect(0, 36, bounds.size.w, 28));
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_color(s_title_layer, MERLIN_TEXT_PRIMARY);
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_text(s_title_layer, "Timer!");
  layer_add_child(root, text_layer_get_layer(s_title_layer));

  s_body_layer = text_layer_create(GRect(8, 72, bounds.size.w - 16, 60));
  text_layer_set_text_alignment(s_body_layer, GTextAlignmentCenter);
  text_layer_set_font(s_body_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_color(s_body_layer, MERLIN_TEXT_PRIMARY);
  text_layer_set_background_color(s_body_layer, GColorClear);
  text_layer_set_text(s_body_layer, s_name_buffer[0] ? s_name_buffer : "Time is up!");
  layer_add_child(root, text_layer_get_layer(s_body_layer));

  s_wizard = wizard_layer_create(
      GRect((bounds.size.w - MERLIN_HEAD_WIDTH) / 2, bounds.size.h - MERLIN_HEAD_HEIGHT - 8,
            MERLIN_HEAD_WIDTH, MERLIN_HEAD_HEIGHT),
      RESOURCE_ID_IMAGE_MERLIN_SLEEP);
  layer_add_child(root, wizard_layer_get_layer(s_wizard));
}

static void prv_window_unload(Window *window) {
  if (s_wizard) {
    wizard_layer_destroy(s_wizard);
    s_wizard = NULL;
  }
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_body_layer);
  s_title_layer = NULL;
  s_body_layer = NULL;
}

void timer_window_push(const char *name) {
  if (name) {
    strncpy(s_name_buffer, name, sizeof(s_name_buffer) - 1);
    s_name_buffer[sizeof(s_name_buffer) - 1] = '\0';
  } else {
    s_name_buffer[0] = '\0';
  }

  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = prv_window_load,
      .unload = prv_window_unload,
    });
  }

  window_stack_push(s_window, true);
}
