#pragma once

#include <lvgl.h>

/* Internal states are defined in the .c file.
 * This header exposes only the init entry point. */

void tamagotchi_widget_init(lv_obj_t *parent);
