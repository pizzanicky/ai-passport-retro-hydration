#include "badge_reminder.h"

#include <assert.h>

int main(void)
{
    badge_reminder_t r;
    badge_reminder_init(&r, true);
    assert(r.reminders_enabled);
    assert(!badge_reminder_tick(&r, false, 20261003, 9, 0, 0));
    assert(!badge_reminder_tick(&r, true, 20261003, 8, 59, 0));

    // Saturday is treated the same as every other day.
    assert(!badge_reminder_tick(&r, true, 20261003, 9, 0, 0));
    assert(!badge_reminder_tick(&r, true, 20261003, 9, 29, 1799000));
    assert(badge_reminder_tick(&r, true, 20261003, 9, 30, 1800000));
    assert(r.pending);
    assert(!badge_reminder_tick(&r, true, 20261003, 9, 31, 1860000));

    badge_reminder_confirm(&r, 1860000);
    assert(!r.pending);
    assert(!badge_reminder_tick(&r, true, 20261003, 10, 0, 3600000));
    assert(badge_reminder_tick(&r, true, 20261003, 10, 1, 3660000));

    // The master switch clears a pending alert and rearms from the next tick.
    badge_reminder_set_enabled(&r, false);
    assert(!r.pending);
    assert(!badge_reminder_tick(&r, true, 20261003, 10, 2, 3720000));
    badge_reminder_set_enabled(&r, true);
    assert(!badge_reminder_tick(&r, true, 20261003, 10, 2, 3720000));
    assert(!badge_reminder_tick(&r, true, 20261003, 10, 31, 5460000));
    assert(badge_reminder_tick(&r, true, 20261003, 10, 32, 5520000));
    assert(!badge_reminder_tick(&r, true, 20261003, 21, 0, 36000000));
    assert(!r.pending);

    // Sunday starts a new 30-minute interval once the window opens.
    assert(!badge_reminder_tick(&r, true, 20261004, 9, 0, 50000000));
    assert(badge_reminder_tick(&r, true, 20261004, 9, 30, 51800000));

    badge_reminder_init(&r, false);
    assert(!r.reminders_enabled);
    assert(!badge_reminder_tick(&r, true, 20261004, 12, 0, 60000000));
    return 0;
}
