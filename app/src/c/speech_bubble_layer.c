#include "speech_bubble_layer.h"
#include "merlin_theme.h"

#define BUBBLE_CHAMFER 8
#define BUBBLE_TAIL_DEPTH 8

struct SpeechBubbleLayer {
  Layer *layer;
  TextLayer *text_layer;
};

static void prv_fill_triangle(GContext *ctx, GPoint p0, GPoint p1, GPoint p2) {
  GPoint top = p0;
  GPoint left = p1;
  GPoint right = p2;

  if (left.y > top.y) {
    GPoint tmp = top;
    top = left;
    left = tmp;
  }
  if (right.y > top.y) {
    GPoint tmp = top;
    top = right;
    right = tmp;
  }
  if (left.y > right.y) {
    GPoint tmp = left;
    left = right;
    right = tmp;
  }

  for (int16_t y = top.y; y <= right.y; y++) {
    int16_t x_start;
    int16_t x_end;

    if (y <= left.y) {
      int16_t dy = left.y - top.y;
      int16_t dx_left = left.x - top.x;
      int16_t dx_right = right.x - top.x;
      int16_t step = y - top.y;
      x_start = top.x + (dy ? (dx_left * step) / dy : 0);
      x_end = top.x + (dy ? (dx_right * step) / dy : 0);
    } else {
      int16_t dy = right.y - left.y;
      int16_t dx_left = right.x - left.x;
      int16_t step = y - left.y;
      x_start = left.x;
      x_end = left.x + (dy ? (dx_left * step) / dy : 0);
    }

    if (x_start > x_end) {
      int16_t tmp = x_start;
      x_start = x_end;
      x_end = tmp;
    }
    graphics_draw_line(ctx, GPoint(x_start, y), GPoint(x_end, y));
  }
}

static void prv_draw_bubble(GContext *ctx, GRect bounds) {
  const int16_t w = bounds.size.w;
  const int16_t h = bounds.size.h - BUBBLE_TAIL_DEPTH;
  const int16_t c = BUBBLE_CHAMFER;

  graphics_context_set_fill_color(ctx, MERLIN_BG_SURFACE);
  graphics_fill_rect(ctx, GRect(c, 0, w - (2 * c), h), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0, c, w, h - (2 * c)), 0, GCornerNone);

  GPoint tail[] = {
    GPoint(c, h),
    GPoint(0, h + BUBBLE_TAIL_DEPTH),
    GPoint(c + 12, h),
  };
  graphics_context_set_stroke_color(ctx, MERLIN_BG_SURFACE);
  prv_fill_triangle(ctx, tail[0], tail[1], tail[2]);

  graphics_context_set_stroke_color(ctx, MERLIN_TEXT_PRIMARY);
  graphics_draw_line(ctx, GPoint(c, 0), GPoint(w - c, 0));
  graphics_draw_line(ctx, GPoint(w - c, 0), GPoint(w, c));
  graphics_draw_line(ctx, GPoint(w, c), GPoint(w, h - c));
  graphics_draw_line(ctx, GPoint(w, h - c), GPoint(w - c, h));
  graphics_draw_line(ctx, GPoint(w - c, h), GPoint(c, h));
  graphics_draw_line(ctx, GPoint(c, h), GPoint(0, h - c));
  graphics_draw_line(ctx, GPoint(0, h - c), GPoint(0, c));
  graphics_draw_line(ctx, GPoint(0, c), GPoint(c, 0));
  graphics_draw_line(ctx, tail[0], tail[1]);
  graphics_draw_line(ctx, tail[1], tail[2]);
  graphics_draw_line(ctx, tail[2], tail[0]);
}

static void prv_update_proc(Layer *layer, GContext *ctx) {
  prv_draw_bubble(ctx, layer_get_bounds(layer));
}

SpeechBubbleLayer *speech_bubble_layer_create(GRect frame) {
  SpeechBubbleLayer *bubble = malloc(sizeof(SpeechBubbleLayer));
  if (!bubble) {
    return NULL;
  }

  bubble->layer = layer_create(frame);
  layer_set_update_proc(bubble->layer, prv_update_proc);

  GRect text_frame = GRect(BUBBLE_CHAMFER, BUBBLE_CHAMFER, frame.size.w - (BUBBLE_CHAMFER * 2),
                           frame.size.h - BUBBLE_CHAMFER - BUBBLE_TAIL_DEPTH);
  bubble->text_layer = text_layer_create(text_frame);
  text_layer_set_font(bubble->text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_color(bubble->text_layer, MERLIN_TEXT_PRIMARY);
  text_layer_set_background_color(bubble->text_layer, GColorClear);
  text_layer_set_overflow_mode(bubble->text_layer, GTextOverflowModeWordWrap);
  layer_add_child(bubble->layer, text_layer_get_layer(bubble->text_layer));

  return bubble;
}

Layer *speech_bubble_layer_get_layer(SpeechBubbleLayer *bubble) {
  return bubble ? bubble->layer : NULL;
}

TextLayer *speech_bubble_layer_get_text_layer(SpeechBubbleLayer *bubble) {
  return bubble ? bubble->text_layer : NULL;
}

void speech_bubble_layer_set_text(SpeechBubbleLayer *bubble, const char *text) {
  if (bubble && bubble->text_layer) {
    text_layer_set_text(bubble->text_layer, text ? text : "");
  }
}

void speech_bubble_layer_destroy(SpeechBubbleLayer *bubble) {
  if (!bubble) {
    return;
  }
  if (bubble->text_layer) {
    text_layer_destroy(bubble->text_layer);
  }
  if (bubble->layer) {
    layer_destroy(bubble->layer);
  }
  free(bubble);
}
