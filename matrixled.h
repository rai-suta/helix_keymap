#ifndef MATRIXLED_H
#define MATRIXLED_H

#include "action.h"

#ifdef RGBLIGHT_ENABLE
  void matled_init(void);
  void matled_eeconfig_update(void);
  int matled_get_mode(void);
  void matled_mode_forward(void);
  void matled_toggle(void);
  void matled_refresh(void);
  void matled_event_pressed(keyrecord_t *record);
#else
# define matled_init()
# define matled_eeconfig_update()
# define matled_get_mode()
# define matled_mode_forward()
# define matled_toggle()
# define matled_refresh()
# define matled_event_pressed(a)
#endif

#endif
