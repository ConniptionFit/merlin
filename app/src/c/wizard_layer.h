#pragma once

#include <pebble.h>

typedef struct WizardLayer WizardLayer;

WizardLayer *wizard_layer_create(GRect frame, uint32_t resource_id);
Layer *wizard_layer_get_layer(WizardLayer *wizard);
void wizard_layer_destroy(WizardLayer *wizard);
