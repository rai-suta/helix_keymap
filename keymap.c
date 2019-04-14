#include "config.h"

#include QMK_KEYBOARD_H
#include "bootloader.h"
#ifdef PROTOCOL_LUFA
#include "lufa.h"
#include "split_util.h"
#endif
#ifdef SSD1306OLED
  #include "ssd1306.h"
#endif


#ifdef RGBLIGHT_ENABLE
  #include "matrixled.h"
  // Following line allows macro to read current RGB settings
  extern rgblight_config_t rgblight_config;
#endif

// Keymap layer names
#define APPLY_LAYER_NAMES( func ) \
    func(QWERTY),   \
    func(CURSOR),   \
    func(MEDIA),    \
    func(CONFIG)

// Index of keymap layer
// e.g.: keymaps[KL_(<NAME>)]
#define KL_( name )   KL_##name
enum keymap_layer {
  APPLY_LAYER_NAMES( KL_ ),
  KL_NUM
};

enum custom_keycodes {
  KC_LAYER = SAFE_RANGE,
  KC_ADJUST,
  RGBRST
};

#define _______ KC_TRNS
#define XXXXXXX KC_NO
// Combination keycode
#define KC_TOP    LCTL(KC_HOME)    // move to top
#define KC_BTTM   LCTL(KC_END)     // move to bottom
#define KC_MBW    LCTL(KC_LEFT)    // move to backward-word
#define KC_MFW    LCTL(KC_RGHT)    // move to forward-word
#define KC_UNDO   LCTL(KC_Z)
#define KC_CUT    LCTL(KC_X)
#define KC_COPY   LCTL(KC_C)
#define KC_PST    LCTL(KC_V)
#define KC_REDO   LCTL(KC_Y)
// Modifier keycode
#define MT_SAS    MT(MOD_RSFT, KC_SPACE)
#define OSM_LSFT  OSM(MOD_LSFT)
#define OSM_RSFT  OSM(MOD_RSFT)
#define OSM_LCTL  OSM(MOD_LCTL)
#define OSM_RCTL  OSM(MOD_RCTL)
#define OSM_LALT  OSM(MOD_LALT)
#define OSM_RALT  OSM(MOD_RALT)
#define OSM_LGUI  OSM(MOD_LGUI)
#define OSM_RGUI  OSM(MOD_RGUI)
// Set default_layer_state
#define DF_QWRT   DF(KL_(QWERTY))
#define DF_CURS   DF(KL_(CURSOR))
#define DF_MEDI   DF(KL_(MEDIA))
// Set layer_state
#define TO_CONF   TO(KL_(CONFIG))
#define MO_CONF   MO(KL_(CONFIG))

#if HELIX_ROWS == 5
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [KL_(QWERTY)] = LAYOUT( \
      KC_GRV,        KC_1,     KC_2,   KC_3,      KC_4,    KC_5,                      KC_6,    KC_7,    KC_8,    KC_9,    KC_0, _______, \
      KC_TAB,        KC_Q,     KC_W,   KC_E,      KC_R,    KC_T,                      KC_Y,    KC_U,    KC_I,    KC_O,    KC_P, _______, \
      OSM_LCTL,      KC_A,     KC_S,   KC_D,      KC_F,    KC_G,                      KC_H,    KC_J,    KC_K,    KC_L, KC_SCLN, _______, \
      OSM_LSFT,      KC_Z,     KC_X,   KC_C,      KC_V,    KC_B, KC_LBRC, KC_RBRC,    KC_N,    KC_M, KC_COMM,  KC_DOT, KC_SLSH, _______, \
      KC_ADJUST, OSM_LALT, OSM_LGUI,MO_CONF,    MT_SAS,  MT_SAS,  KC_ENT, _______, _______, _______, _______, _______, _______, _______ \
      ),

  [KL_(CURSOR)] = LAYOUT( \
      _______, XXXXXXX, XXXXXXX, XXXXXXX,  XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, _______, \
      _______,  KC_ESC,  KC_TOP, KC_BTTM,  XXXXXXX, XXXXXXX,                   XXXXXXX, KC_HOME,  KC_END, XXXXXXX, XXXXXXX, _______, \
      _______, KC_LEFT,   KC_UP, KC_DOWN,  KC_RGHT, XXXXXXX,                    KC_MBW, KC_PGUP, KC_PGDN,  KC_MFW, XXXXXXX, _______, \
      _______, XXXXXXX, XXXXXXX, XXXXXXX,  XXXXXXX, XXXXXXX, XXXXXXX, _______, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, _______, \
      _______, _______, _______, MO_CONF,  _______, _______, _______, _______, _______, _______, _______, _______, _______, _______ \
      ),

  [KL_(MEDIA)] = LAYOUT( \
      XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
       KC_TAB, XXXXXXX, KC_MPRV, KC_MNXT, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, KC_MRWD, KC_MSTP, KC_MPLY, KC_MFFD, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, KC_MUTE, KC_VOLD, KC_VOLU, KC_EJCT, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, XXXXXXX, XXXXXXX, MO_CONF, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX \
      ),

  [KL_(CONFIG)] = LAYOUT( \
      XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
       KC_TAB, RGB_TOG, RGB_HUI, RGB_SAI, RGB_VAI,  RGBRST,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, RGB_MOD, RGB_HUD, RGB_SAD, RGB_VAD, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, DF_QWRT, DF_CURS, DF_MEDI, TO_CONF, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, XXXXXXX, XXXXXXX, MO_CONF, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX \
      ),

};
#else
# error "undefined keymaps"
#endif

// User modifier names
#define APPLY_USERMOD_NAMES( func ) \
    func(LAYER),  \
    func(MIRROR), \
    func(SLIDE)

// Index of user modifier
// e.g.: user_modifier_on(UM_(<NAME>))
#define UM_( name )   UM_##name
enum user_modifier {
  APPLY_USERMOD_NAMES( UM_ ),
  UM_NUM
};

// Mask of user modifier
// e.g.: user_modifiler_contains_mask(UM_MASK_(<NAME>))
#define UM_MASK_( name )  UM_MASK_##name
#define DEFINE_UM_MASK( name )  UM_MASK_(name) = 1 << UM_(name)
enum user_modifier_mask {
  APPLY_USERMOD_NAMES( DEFINE_UM_MASK ),
} user_modifier_bits;

// User modifier utilitys
static inline void
user_modifier_on( enum user_modifier mod_idx )
{
  user_modifier_bits = user_modifier_bits | (1u<<mod_idx);
}
static inline void
user_modifier_off( enum user_modifier mod_idx )
{
  user_modifier_bits = user_modifier_bits & ~(1u<<mod_idx);
}
static inline bool
user_modifiler_contains_mask( enum user_modifier_mask mod_mask )
{
  return ( (user_modifier_bits & mod_mask) == mod_mask );
}
static inline bool
user_modifiler_contains_idx( enum user_modifier mod_idx )
{
  enum user_modifier_mask mod_mask = 1 << mod_idx;
  return user_modifiler_contains_mask(mod_mask);
}

#define PROCESS_OVERRIDE_BEHAVIOR   (false)
#define PROCESS_USUAL_BEHAVIOR      (true)

static keyrecord_t last_keyrecord;
static bool
process_record_event(uint16_t keycode, keyrecord_t *record);

// override the behavior of an existing key,
// called by QMK during key processing before the actual key event is handled.
bool
process_record_user(uint16_t keycode, keyrecord_t *record)
{
  bool result_process;

  last_keyrecord = *record;

  // check the event to be overridden
  result_process = process_record_event(keycode, record);
  if (result_process == PROCESS_OVERRIDE_BEHAVIOR) {
    return PROCESS_OVERRIDE_BEHAVIOR;
  }

  #ifdef MATRIXLED_H
    // notice keypos to matled
    result_process = matled_record_event(keycode, record);
    if (result_process == PROCESS_OVERRIDE_BEHAVIOR) {
      return PROCESS_OVERRIDE_BEHAVIOR;
    }
  #endif

  return PROCESS_USUAL_BEHAVIOR;
}

static bool
process_record_event(uint16_t keycode, keyrecord_t *record)
{
  switch (keycode) {

    case RGBRST: if (record->event.pressed) {
      #ifdef RGBLIGHT_ENABLE
        eeconfig_update_rgblight_default();
        rgblight_enable();
      #endif
      #ifdef MATRIXLED_H
        matled_init();
      #endif
    } break;

    case MO_CONF: {
      static uint32_t before_default_layer_state;
      if (record->event.pressed) {
        before_default_layer_state = default_layer_state;
      }
      else {
        layer_clear();
        if (before_default_layer_state != default_layer_state) {
          eeconfig_update_default_layer(default_layer_state);
        }
      }
      return PROCESS_USUAL_BEHAVIOR;
    } break;

    default: {
    } break;
  }

  return PROCESS_USUAL_BEHAVIOR;
 }

//keyboard start-up code. Runs once when the firmware starts up.
void matrix_init_user(void) {
  #ifdef MATRIXLED_H
    matled_init();
  #endif
  //SSD1306 OLED init, make sure to add #define SSD1306OLED in config.h
  #ifdef SSD1306OLED
    iota_gfx_init(!has_usb());   // turns on the display
  #endif
}

//#define MATRIX_SCAN_RUN_TIME
#ifdef MATRIX_SCAN_RUN_TIME
static struct {
  uint32_t last_calc_time;
  uint32_t scan_num;
  uint32_t progress_sum;
  uint32_t progress_max;
  uint32_t cycle_time;
  uint32_t mean;
  uint32_t max;
} matrix_scan_run_time;
static inline void matrix_scan_run_time_end(uint32_t begin_time);
#endif

void matrix_scan_user(void) {
  __attribute__ ((unused))
  uint32_t begin_time = timer_read32();

  #ifdef MATRIXLED_H
    matled_refresh_task();
  #endif

  #ifdef MATRIX_SCAN_RUN_TIME
    matrix_scan_run_time_end(begin_time);
  #endif

  #ifdef SSD1306OLED
    iota_gfx_task();  // this is what updates the display continuously
  #endif
}

#ifdef MATRIX_SCAN_RUN_TIME
static inline void matrix_scan_run_time_end(uint32_t begin_time)
{
  uint32_t end_time = timer_read32();
  uint32_t run_time = TIMER_DIFF_32(end_time, begin_time);

  matrix_scan_run_time.scan_num++;
  matrix_scan_run_time.progress_sum += run_time;
  matrix_scan_run_time.progress_max = (matrix_scan_run_time.progress_max > run_time) ? matrix_scan_run_time.progress_max : run_time;

  if (TIMER_DIFF_32(begin_time, matrix_scan_run_time.last_calc_time) > 1000) {
    matrix_scan_run_time.cycle_time = TIMER_DIFF_32(begin_time, matrix_scan_run_time.last_calc_time) / matrix_scan_run_time.scan_num;
    matrix_scan_run_time.mean = matrix_scan_run_time.progress_sum / matrix_scan_run_time.scan_num;
    matrix_scan_run_time.max  = matrix_scan_run_time.progress_max;

    matrix_scan_run_time.last_calc_time = begin_time;
    matrix_scan_run_time.scan_num = 0u;
    matrix_scan_run_time.progress_sum = 0u;
    matrix_scan_run_time.progress_max = 0u;
  }
}
#endif

// OLED image characters
static const char PROGMEM
  matrix_HELIX[] = {
     0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94
    ,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4
    ,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4
    ,0
  };
enum MatrixIcon {
  MI_APPLE, MI_WINDOWS, MI_PENGUIN, MI_ANDROID
};
static const char PROGMEM
  matrix_Icons[][2][3] = {
    [MI_APPLE] = {
      { 0x95, 0x96, 0 },
      { 0xb5, 0xb6, 0 }
    },
    [MI_WINDOWS] = {
      { 0x97, 0x98, 0 },
      { 0xb7, 0xb8, 0 },
    },
    [MI_PENGUIN] = {
      { 0x99, 0x9A, 0 },
      { 0xb9, 0xbA, 0 },
    },
    [MI_ANDROID] = {
      { 0x9B, 0x9C, 0 },
      { 0xbB, 0xbC, 0 },
    }
  };

static void
render_status(struct CharacterMatrix *matrix);
static void
render_status_Layer(struct CharacterMatrix *matrix);
static void
render_status_UserMod(struct CharacterMatrix *matrix);
#ifdef RGBLIGHT_ENABLE
static void
render_status_LedParams(struct CharacterMatrix *matrix);
#endif
#ifdef MATRIX_SCAN_RUN_TIME
  static void
  render_status_RunTime(struct CharacterMatrix *matrix);
#endif

static void
matrix_update(struct CharacterMatrix *dest,
              const struct CharacterMatrix *source);

// be called from iota_gfx_task
void iota_gfx_task_user(void)
{
  struct CharacterMatrix matrix;

  matrix_clear(&matrix);

  render_status(&matrix);

  matrix_update(&display, &matrix);
}

static void
render_status(struct CharacterMatrix *matrix)
{
  render_status_Layer(matrix);

  matrix_write_P(matrix, "\n");
  render_status_UserMod(matrix);

  #ifdef MATRIX_SCAN_RUN_TIME
    matrix_write_P(matrix, "\n");
    render_status_RunTime(matrix);
  #endif

  uint32_t layer = layer_state | default_layer_state;
  if ( layer & (1<<KL_(CONFIG)) ) {
    #ifdef RGBLIGHT_ENABLE
      matrix_write_P(matrix, "\n");
      render_status_LedParams(matrix);
    #endif
  }
}

static const char*
layerNameStr_P( enum keymap_layer layer );
static const char*
userModNameStr_P( enum user_modifier mod );

static void
render_status_Layer(struct CharacterMatrix *matrix)
{
  uint32_t layer = layer_state | default_layer_state;

  matrix_write_P(matrix, PSTR("Layer:"));
  if ( layer == 0u ) {
      matrix_write_P(matrix, " ");
      matrix_write_P(matrix, layerNameStr_P(0));
  }
  else {
    for ( int layer_idx = 0; layer_idx < KL_NUM; layer_idx++ ) {
      if ( layer & (1<<layer_idx) ) {
        matrix_write_P(matrix, " ");
        matrix_write_P(matrix, layerNameStr_P(layer_idx));
      }
    }
  }
}

static void
render_status_UserMod(struct CharacterMatrix *matrix)
{
  matrix_write_P(matrix, PSTR("UserMod:"));
  for ( int mod_idx = 0; mod_idx < UM_NUM; mod_idx++ ) {
    if ( user_modifier_bits & (1<<mod_idx) ) {
      matrix_write_P(matrix, " ");
      matrix_write_P(matrix, userModNameStr_P(mod_idx));
    }
  }
}


#ifdef RGBLIGHT_ENABLE
static void
render_status_LedParams(struct CharacterMatrix *matrix)
{
  char buf[16];
  const size_t sizeof_buf = sizeof(buf);

  matrix_write_P(matrix, PSTR("LedStt"));

  #ifdef MATRIXLED_H
    int led_mode = matled_get_mode();
  #else
    int led_mode = rgblight_config.mode;
  #endif
  if ( snprintf(buf, sizeof_buf, ":%c%d", (rgblight_config.enable ? ' ' : '!')
                                        , led_mode) > 0 ) {
    matrix_write(matrix, buf);
  }

  if ( snprintf(buf, sizeof_buf, ":%d", rgblight_config.hue) > 0 ) {
    matrix_write(matrix, buf);
  }

  if ( snprintf(buf, sizeof_buf, ":%d", rgblight_config.sat) > 0 ) {
    matrix_write(matrix, buf);
  }

  if ( snprintf(buf, sizeof_buf, ":%d", rgblight_config.val) > 0 ) {
    matrix_write(matrix, buf);
  }
}
#endif

#ifdef MATRIX_SCAN_RUN_TIME
static void
render_status_RunTime(struct CharacterMatrix *matrix)
{
  char buf[16];
  const size_t sizeof_buf = sizeof(buf);

  matrix_write_P(matrix, PSTR("RunTime:"));

  if (snprintf(buf, sizeof_buf, "%ld,", matrix_scan_run_time.cycle_time) > 0) {
    matrix_write(matrix, buf);
  }
  if (snprintf(buf, sizeof_buf, "%ld,", matrix_scan_run_time.mean) > 0) {
    matrix_write(matrix, buf);
  }
  if (snprintf(buf, sizeof_buf, "%ld,", matrix_scan_run_time.max) > 0) {
    matrix_write(matrix, buf);
  }
}
#endif

static void
matrix_update(struct CharacterMatrix *dest,
              const struct CharacterMatrix *source)
{
  if (memcmp(dest->display, source->display, sizeof(dest->display))) {
    memcpy(dest->display, source->display, sizeof(dest->display));
    dest->dirty = true;
  }
}

// Utility for define string data
#define DEFINE_STR_ITEM( name )  STR_##name[] PROGMEM = #name
#define INITIALIZE_KL_ITEM_TO_STR( name )  [KL_(name)] = STR_##name
#define INITIALIZE_UM_ITEM_TO_STR( name )  [UM_(name)] = STR_##name

static const char*
layerNameStr_P( enum keymap_layer layer )
{
  static const char
    APPLY_LAYER_NAMES( DEFINE_STR_ITEM ),
    * const layer_names_lut[KL_NUM] = { APPLY_LAYER_NAMES( INITIALIZE_KL_ITEM_TO_STR ) };

  return (layer_names_lut[layer]);
}

static const char*
userModNameStr_P( enum user_modifier mod )
{
  static const char
    APPLY_USERMOD_NAMES( DEFINE_STR_ITEM ),
    * const um_names_lut[UM_NUM] = { APPLY_USERMOD_NAMES( INITIALIZE_UM_ITEM_TO_STR ) };

  return (um_names_lut[mod]);
}
