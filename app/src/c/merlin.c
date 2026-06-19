#include <pebble.h>

#include "app_message.h"
#include "session_window.h"
#include "timer_manager.h"

static void prv_on_response_chunk(const char *chunk) {
  session_window_handle_response_chunk(chunk);
}

static void prv_on_response_done(void) {
  session_window_handle_response_done();
}

static void prv_on_status(const char *status) {
  session_window_handle_status(status);
}

static void prv_on_timer(int32_t duration, const char *name) {
  timer_manager_schedule(duration, name);
  session_window_handle_status("Timer set");
}

int main(void) {
  timer_manager_init();
  app_message_init();
  app_message_set_callbacks(&(AppMessageCallbacks){
    .on_response_chunk = prv_on_response_chunk,
    .on_response_done = prv_on_response_done,
    .on_status = prv_on_status,
    .on_timer = prv_on_timer,
  });

  if (timer_manager_handle_wakeup_launch()) {
    app_event_loop();
    return 0;
  }

  session_window_push();
  app_event_loop();
  return 0;
}
