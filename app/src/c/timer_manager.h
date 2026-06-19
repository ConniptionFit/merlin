#pragma once

#include <pebble.h>

void timer_manager_init(void);
bool timer_manager_handle_wakeup_launch(void);
void timer_manager_schedule(int32_t duration_seconds, const char *name);
