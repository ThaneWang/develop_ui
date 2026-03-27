#include "ui_bt_state.h"

static bool s_bt_enabled;
static bool s_low_power = true;

bool ui_bt_is_enabled(void)
{
    return s_bt_enabled;
}

void ui_bt_set_enabled(bool on)
{
    s_bt_enabled = on;
}

bool ui_bt_low_power_is_on(void)
{
    return s_low_power;
}

void ui_bt_set_low_power(bool on)
{
    s_low_power = on;
}
