#include "badge_reminder.h"

#include <stddef.h>

static uint64_t interval_ms(void)
{
    return (uint64_t)BADGE_REMINDER_INTERVAL_MIN * 60U * 1000U;
}

void badge_reminder_init(badge_reminder_t *state, bool default_enabled)
{
    if (!state) return;
    *state = (badge_reminder_t){
        .reminders_enabled = default_enabled,
        .calendar_day = -1,
    };
}

void badge_reminder_set_enabled(badge_reminder_t *state, bool enabled)
{
    if (!state || state->reminders_enabled == enabled) return;
    state->reminders_enabled = enabled;
    state->active = false;
    state->pending = false;
    state->calendar_day = -1;
}

bool badge_reminder_tick(badge_reminder_t *state, bool clock_valid,
                         int calendar_day, int hour, int minute, uint64_t now_ms)
{
    if (!state) return false;
    bool in_hours = state->reminders_enabled && clock_valid && hour >= 9 && hour < 21 &&
                    minute >= 0 && minute < 60;
    if (!in_hours) {
        state->active = false;
        state->pending = false;
        return false;
    }
    if (!state->active || state->calendar_day != calendar_day) {
        state->active = true;
        state->calendar_day = calendar_day;
        state->next_due_ms = now_ms + interval_ms();
        state->pending = false;
        return false;
    }
    if (!state->pending && now_ms >= state->next_due_ms) {
        state->pending = true;
        return true;
    }
    return false;
}

void badge_reminder_confirm(badge_reminder_t *state, uint64_t now_ms)
{
    if (!state || !state->pending) return;
    state->pending = false;
    state->next_due_ms = now_ms + interval_ms();
}
