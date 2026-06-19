#include "timer_manager.h"
#include "timer_window.h"

#include <string.h>

#define MAX_TIMERS 4
#define PERSIST_KEY_COUNT 100
#define PERSIST_KEY_WAKEUP_IDS 101
#define PERSIST_KEY_NAMES 102
#define PERSIST_KEY_FIRE_TIMES 103
#define TIMER_NAME_SIZE 32

typedef struct {
  WakeupId wakeup_id;
  time_t fire_time;
  char name[TIMER_NAME_SIZE];
} StoredTimer;

static StoredTimer s_timers[MAX_TIMERS];
static uint8_t s_timer_count = 0;

static void prv_vibe_timer(void) {
  static const uint32_t segments[] = {200, 100, 200, 100, 400};
  VibeInfo info = {
    .durations = segments,
    .num_segments = 5,
  };
  vibes_enqueue_custom(info);
}

static void prv_save_timers(void) {
  WakeupId ids[MAX_TIMERS];
  time_t fire_times[MAX_TIMERS];
  char names[MAX_TIMERS][TIMER_NAME_SIZE];

  memset(names, 0, sizeof(names));
  for (uint8_t i = 0; i < s_timer_count; i++) {
    ids[i] = s_timers[i].wakeup_id;
    fire_times[i] = s_timers[i].fire_time;
    strncpy(names[i], s_timers[i].name, TIMER_NAME_SIZE - 1);
  }

  persist_write_int(PERSIST_KEY_COUNT, s_timer_count);
  persist_write_data(PERSIST_KEY_WAKEUP_IDS, ids, sizeof(ids));
  persist_write_data(PERSIST_KEY_FIRE_TIMES, fire_times, sizeof(fire_times));
  persist_write_data(PERSIST_KEY_NAMES, names, sizeof(names));
}

static void prv_load_timers(void) {
  s_timer_count = persist_read_int(PERSIST_KEY_COUNT);
  if (s_timer_count > MAX_TIMERS) {
    s_timer_count = 0;
    return;
  }

  WakeupId ids[MAX_TIMERS];
  time_t fire_times[MAX_TIMERS];
  char names[MAX_TIMERS][TIMER_NAME_SIZE];

  persist_read_data(PERSIST_KEY_WAKEUP_IDS, ids, sizeof(ids));
  persist_read_data(PERSIST_KEY_FIRE_TIMES, fire_times, sizeof(fire_times));
  persist_read_data(PERSIST_KEY_NAMES, names, sizeof(names));

  uint8_t valid = 0;
  for (uint8_t i = 0; i < s_timer_count; i++) {
    if (wakeup_query(ids[i], NULL)) {
      s_timers[valid].wakeup_id = ids[i];
      s_timers[valid].fire_time = fire_times[i];
      strncpy(s_timers[valid].name, names[i], TIMER_NAME_SIZE - 1);
      s_timers[valid].name[TIMER_NAME_SIZE - 1] = '\0';
      valid++;
    }
  }
  s_timer_count = valid;
  if (s_timer_count == 0) {
    persist_delete(PERSIST_KEY_COUNT);
  } else {
    prv_save_timers();
  }
}

static void prv_wakeup_handler(WakeupId wakeup_id, int32_t cookie) {
  for (uint8_t i = 0; i < s_timer_count; i++) {
    if (s_timers[i].wakeup_id == wakeup_id) {
      prv_vibe_timer();
      timer_window_push(s_timers[i].name);
      for (uint8_t j = i; j + 1 < s_timer_count; j++) {
        s_timers[j] = s_timers[j + 1];
      }
      s_timer_count--;
      prv_save_timers();
      return;
    }
  }
}

void timer_manager_init(void) {
  wakeup_service_subscribe(prv_wakeup_handler);
  prv_load_timers();
}

void timer_manager_schedule(int32_t duration_seconds, const char *name) {
  if (duration_seconds <= 0 || s_timer_count >= MAX_TIMERS) {
    return;
  }

  time_t fire_time = time(NULL) + duration_seconds;
  WakeupId id = wakeup_schedule(fire_time, (int32_t)fire_time, true);
  if (id < 0) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "wakeup_schedule failed: %d", (int)id);
    return;
  }

  StoredTimer *timer = &s_timers[s_timer_count++];
  timer->wakeup_id = id;
  timer->fire_time = fire_time;
  if (name) {
    strncpy(timer->name, name, TIMER_NAME_SIZE - 1);
    timer->name[TIMER_NAME_SIZE - 1] = '\0';
  } else {
    timer->name[0] = '\0';
  }

  prv_save_timers();
}

bool timer_manager_handle_wakeup_launch(void) {
  if (launch_reason() != APP_LAUNCH_WAKEUP) {
    return false;
  }

  WakeupId launch_id = 0;
  int32_t cookie = 0;
  if (!wakeup_get_launch_event(&launch_id, &cookie)) {
    return false;
  }

  for (uint8_t i = 0; i < s_timer_count; i++) {
    if (s_timers[i].wakeup_id == launch_id) {
      prv_vibe_timer();
      timer_window_push(s_timers[i].name);
      for (uint8_t j = i; j + 1 < s_timer_count; j++) {
        s_timers[j] = s_timers[j + 1];
      }
      s_timer_count--;
      prv_save_timers();
      return true;
    }
  }

  return false;
}
