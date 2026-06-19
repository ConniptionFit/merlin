#include "wizard_layer.h"

struct WizardLayer {
  Layer *layer;
  BitmapLayer *bitmap_layer;
  GBitmap *bitmap;
};

static void prv_update_proc(Layer *layer, GContext *ctx) {
  (void)layer;
  (void)ctx;
}

WizardLayer *wizard_layer_create(GRect frame) {
  WizardLayer *wizard = malloc(sizeof(WizardLayer));
  if (!wizard) {
    return NULL;
  }

  wizard->bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WIZARD);
  wizard->layer = layer_create(frame);
  wizard->bitmap_layer = bitmap_layer_create(gbitmap_get_bounds(wizard->bitmap));
  bitmap_layer_set_bitmap(wizard->bitmap_layer, wizard->bitmap);
  bitmap_layer_set_compositing_mode(wizard->bitmap_layer, GCompOpSet);

  layer_set_update_proc(wizard->layer, prv_update_proc);
  layer_add_child(wizard->layer, bitmap_layer_get_layer(wizard->bitmap_layer));

  return wizard;
}

Layer *wizard_layer_get_layer(WizardLayer *wizard) {
  return wizard ? wizard->layer : NULL;
}

void wizard_layer_destroy(WizardLayer *wizard) {
  if (!wizard) {
    return;
  }

  if (wizard->bitmap_layer) {
    bitmap_layer_destroy(wizard->bitmap_layer);
  }
  if (wizard->bitmap) {
    gbitmap_destroy(wizard->bitmap);
  }
  if (wizard->layer) {
    layer_destroy(wizard->layer);
  }
  free(wizard);
}
