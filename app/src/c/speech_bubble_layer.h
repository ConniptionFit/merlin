#pragma once

#include <pebble.h>

typedef struct SpeechBubbleLayer SpeechBubbleLayer;

SpeechBubbleLayer *speech_bubble_layer_create(GRect frame);
Layer *speech_bubble_layer_get_layer(SpeechBubbleLayer *bubble);
TextLayer *speech_bubble_layer_get_text_layer(SpeechBubbleLayer *bubble);
void speech_bubble_layer_set_text(SpeechBubbleLayer *bubble, const char *text);
void speech_bubble_layer_destroy(SpeechBubbleLayer *bubble);
