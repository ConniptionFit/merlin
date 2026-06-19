#pragma once

#include <pebble.h>

void session_window_push(void);
void session_window_handle_response_chunk(const char *chunk);
void session_window_handle_response_done(void);
void session_window_handle_status(const char *status);
