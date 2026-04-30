/*
 * Custom peripheral status screen — replaces nice!view's default
 * peripheral display with the tamagotchi widget.
 *
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>
#include "tamagotchi_widget.h"

lv_obj_t *zmk_display_status_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    tamagotchi_widget_init(screen);
    return screen;
}
