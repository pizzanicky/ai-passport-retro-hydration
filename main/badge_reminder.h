#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BADGE_REMINDER_INTERVAL_MIN 30U

typedef struct {
    bool reminders_enabled;
    uint64_t next_due_ms;
    int calendar_day;
    bool active;
    bool pending;
} badge_reminder_t;

void badge_reminder_init(badge_reminder_t *state, bool default_enabled);
void badge_reminder_set_enabled(badge_reminder_t *state, bool enabled);
bool badge_reminder_tick(badge_reminder_t *state, bool clock_valid,
                         int calendar_day, int hour, int minute, uint64_t now_ms);
void badge_reminder_confirm(badge_reminder_t *state, uint64_t now_ms);
