#ifndef MATRIXLED_H
#define MATRIXLED_H

#if !defined(RGBLIGHT_ENABLE)
# error please enable RGBLIGHT_ENABLE, check ./rules.mk: LED_BACK_ENABLE = yes
#endif
#if defined(RGBLIGHT_ANIMATIONS)
# error please disable RGBLIGHT_ANIMATIONS, check ./rules.mk: LED_ANIMATIONS = no
#endif

#include "action.h"

#define ENABLE_SWITCH_PATTERN
//#define ENABLE_DIMLY_PATTERN
#define ENABLE_RIPPLE_PATTERN
#define ENABLE_CROSS_PATTERN
#define ENABLE_WAVE_PATTERN

void matled_init(void);
int matled_get_mode(void);
void matled_refresh_task(void);
bool matled_record_event(uint16_t keycode, keyrecord_t *record);

#endif // MATRIXLED
