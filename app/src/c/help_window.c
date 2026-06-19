#include "help_window.h"
#include "merlin_theme.h"

static Window *s_window = NULL;

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  window_set_background_color(window, MERLIN_BG_SURFACE);

  TextLayer *title = text_layer_create(GRect(8, 24, bounds.size.w - 16, 28));
  text_layer_set_font(title, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text(title, "Merlin");
  text_layer_set_text_color(title, MERLIN_TEXT_PRIMARY);
  text_layer_set_background_color(title, GColorClear);
  layer_add_child(root, text_layer_get_layer(title));

  TextLayer *body = text_layer_create(GRect(8, 56, bounds.size.w - 16, bounds.size.h - 64));
  text_layer_set_font(body, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(body,
                      "? Help\n"
                      "Mic Speak\n"
                      "... History\n\n"
                      "Hold Select for feedback.");
  text_layer_set_text_color(body, MERLIN_TEXT_PRIMARY);
  text_layer_set_background_color(body, GColorClear);
  text_layer_set_overflow_mode(body, GTextOverflowModeWordWrap);
  layer_add_child(root, text_layer_get_layer(body));
}

static void prv_window_unload(Window *window) {
  layer_remove_child_layers(window_get_root_layer(window));
}

void help_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = prv_window_load,
      .unload = prv_window_unload,
    });
  }
  window_stack_push(s_window, true);
}
