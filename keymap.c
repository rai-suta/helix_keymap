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

#include "matrixled.h"

extern uint8_t is_master;

// Keymap layer names
#define APPLY_LAYER_NAMES( func ) \
    func(QWERTY),   \
    func(CURSOR),   \
    func(MEDIA),    \
    func(CONFIG),   \
    func(LAYER_SW)

// Index of keymap layer
// e.g.: keymaps[KL_(<NAME>)]
#define KL_( name )   KL_##name
enum keymap_layer {
  APPLY_LAYER_NAMES( KL_ ),
  KL_NUM
};

enum custom_keycodes {
  KC_LAYER = SAFE_RANGE,
  KC_MIRROR,
  KC_SLIDE,
  KC_ADJUST
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
#define MO_LSW    MO(KL_(LAYER_SW))

#if HELIX_ROWS == 5
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [KL_(QWERTY)] = LAYOUT( \
      KC_GRV,        KC_1,     KC_2,   KC_3,      KC_4,    KC_5,                      KC_6,    KC_7,    KC_8,    KC_9,    KC_0, _______, \
      KC_TAB,        KC_Q,     KC_W,   KC_E,      KC_R,    KC_T,                      KC_Y,    KC_U,    KC_I,    KC_O,    KC_P, _______, \
      OSM_LCTL,      KC_A,     KC_S,   KC_D,      KC_F,    KC_G,                      KC_H,    KC_J,    KC_K,    KC_L, KC_SCLN, _______, \
      OSM_LSFT,      KC_Z,     KC_X,   KC_C,      KC_V,    KC_B, KC_LBRC, KC_RBRC,    KC_N,    KC_M, KC_COMM,  KC_DOT, KC_SLSH, _______, \
      KC_ADJUST, OSM_LALT, OSM_LGUI, MO_LSW, KC_MIRROR,  MT_SAS,  KC_ENT, _______, _______, _______, _______, _______, _______, _______ \
      ),

  [KL_(CURSOR)] = LAYOUT( \
      _______, XXXXXXX, XXXXXXX, XXXXXXX,  XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, _______, \
      _______,  KC_ESC,  KC_TOP, KC_BTTM,  XXXXXXX, XXXXXXX,                   XXXXXXX, KC_HOME,  KC_END, XXXXXXX, XXXXXXX, _______, \
      _______, KC_LEFT,   KC_UP, KC_DOWN,  KC_RGHT, XXXXXXX,                    KC_MBW, KC_PGUP, KC_PGDN,  KC_MFW, XXXXXXX, _______, \
      _______, XXXXXXX, XXXXXXX, XXXXXXX,  XXXXXXX, XXXXXXX, XXXXXXX, _______, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, _______, \
      _______, _______, _______,  MO_LSW, KC_SLIDE, _______, _______, _______, _______, _______, _______, _______, _______, _______ \
      ),

  [KL_(MEDIA)] = LAYOUT( \
      XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
       KC_TAB, XXXXXXX, KC_MPRV, KC_MNXT, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, KC_MRWD, KC_MSTP, KC_MPLY, KC_MFFD, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, KC_MUTE, KC_VOLD, KC_VOLU, KC_EJCT, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, XXXXXXX, XXXXXXX,  MO_LSW, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX \
      ),

  [KL_(CONFIG)] = LAYOUT( \
      XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
       KC_TAB, RGB_TOG, RGB_HUI, RGB_SAI, RGB_VAI, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, RGB_SMOD,RGB_HUD, RGB_SAD, RGB_VAD, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, XXXXXXX, XXXXXXX,  MO_LSW, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX \
      ),

  [KL_(LAYER_SW)] = LAYOUT( \
      XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, DF_QWRT, DF_CURS, DF_MEDI, TO_CONF, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
      XXXXXXX, XXXXXXX, XXXXXXX,  MO_LSW, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX \
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

static bool
process_record_usermod(uint16_t keycode, keyrecord_t *record);
static bool
process_record_event(uint16_t keycode, keyrecord_t *record);
static keypos_t
get_keypos_converted_usermod(keyrecord_t *record);
static keypos_t
get_keypos_mirror(keypos_t key);
static keypos_t
get_keypos_slide(keypos_t key);

// override the behavior of an existing key,
// called by QMK during key processing before the actual key event is handled.
bool
process_record_user(uint16_t keycode, keyrecord_t *record)
{
  bool result_process;

  // notice keypos to matled
  if (record->event.pressed) {
    matled_event_pressed(record);
  }

  // check user modifier key
  result_process = process_record_usermod(keycode, record);
  if (result_process == PROCESS_OVERRIDE_BEHAVIOR) {
    return PROCESS_OVERRIDE_BEHAVIOR;
  }

  // get keypos converted by user modifiler
  keypos_t event_key = record->event.key;
  uint8_t layer = layer_switch_get_layer(event_key);
  keypos_t keypos_converted = get_keypos_converted_usermod(record);
  uint16_t keycode_converted = keymap_key_to_keycode(layer, keypos_converted);
  if (keycode_converted == KC_TRNS) {
    keycode_converted = keycode;
  }

  // check the event to be overridden
  result_process = process_record_event(keycode_converted, record);
  if (result_process == PROCESS_OVERRIDE_BEHAVIOR) {
    return PROCESS_OVERRIDE_BEHAVIOR;
  }

  // store action from keypos converted
  if (   (keypos_converted.row != event_key.row)
      || (keypos_converted.col != event_key.col) ){
    action_t action = store_or_get_action(record->event.pressed, keypos_converted);
    process_action(record, action);
    return PROCESS_OVERRIDE_BEHAVIOR;
  }

  return PROCESS_USUAL_BEHAVIOR;
}

static bool
process_record_usermod(uint16_t keycode, keyrecord_t *record)
{
  switch (keycode) {

    case KC_MIRROR: {
      if (record->event.pressed) {
        user_modifier_on(UM_(MIRROR));
      }
      else {
        user_modifier_off(UM_(MIRROR));
        clear_keyboard();
      }
      return PROCESS_OVERRIDE_BEHAVIOR;
    } break;

    case KC_SLIDE: {
      if (record->event.pressed) {
        user_modifier_on(UM_(SLIDE));
      }
      else {
        user_modifier_off(UM_(SLIDE));
        clear_keyboard();
      }
      return PROCESS_OVERRIDE_BEHAVIOR;
    } break;

    default:
      break;
  }

  return PROCESS_USUAL_BEHAVIOR;
}

static bool
process_record_event(uint16_t keycode, keyrecord_t *record)
{
  switch (keycode) {

#   if defined(MATRIXLED_H)
    case RGB_TOG: if (record->event.pressed) {
      matled_toggle();
      return PROCESS_OVERRIDE_BEHAVIOR;
    } break;

    case RGB_SMOD: if (record->event.pressed) {
      matled_mode_forward();
      return PROCESS_OVERRIDE_BEHAVIOR;
    } break;
#   endif

    case MO_LSW: {
      static uint32_t before_default_layer_state;
      if (record->event.pressed) {
        before_default_layer_state = default_layer_state;
      }
      else {
        layer_clear();
        matled_eeconfig_update();
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

static keypos_t
get_keypos_converted_usermod(keyrecord_t *record)
{
  keypos_t keypos_converted;

  if (!user_modifier_bits) {
    keypos_converted = record->event.key;
  }
  else if ( user_modifiler_contains_idx(UM_(MIRROR)) ) {
    keypos_converted = get_keypos_mirror(record->event.key);
  }
  else if ( user_modifiler_contains_idx(UM_(SLIDE)) ) {
    keypos_converted = get_keypos_slide(record->event.key);
  }
  else {
    keypos_converted = record->event.key;
  }

  return keypos_converted;
}

static keypos_t
get_keypos_mirror(keypos_t key)
{
  keypos_t convert_key = {
    .row = (key.row + HELIX_ROWS) % MATRIX_ROWS,
#ifndef FLIP_HALF
    .col = key.col,
#else
    .col = (MATRIX_COLS-1) - key.col,
#endif
  };
  return convert_key;
}

static keypos_t
get_keypos_slide(keypos_t key)
{
  keypos_t convert_key = {
    .row = (key.row + HELIX_ROWS) % MATRIX_ROWS,
#ifndef FLIP_HALF
    .col = (MATRIX_COLS-1) - key.col,
#else
    .col = key.col,
#endif
  };
  return convert_key;
}

//keyboard start-up code. Runs once when the firmware starts up.
void matrix_init_user(void) {
  matled_init();
  //SSD1306 OLED init, make sure to add #define SSD1306OLED in config.h
  #ifdef SSD1306OLED
    iota_gfx_init(!has_usb());   // turns on the display
  #endif
}

void matrix_scan_user(void) {
  matled_refresh();
  #ifdef SSD1306OLED
    iota_gfx_task();  // this is what updates the display continuously
  #endif
}

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
static void
render_status_LedMode(struct CharacterMatrix *matrix);

static void
matrix_update(struct CharacterMatrix *dest,
              const struct CharacterMatrix *source);

// be called from iota_gfx_task
void iota_gfx_task_user(void)
{
  struct CharacterMatrix matrix;

  matrix_clear(&matrix);
  if(is_master){
    render_status(&matrix);
  }
  matrix_update(&display, &matrix);
}

static void
render_status(struct CharacterMatrix *matrix)
{
  render_status_Layer(matrix);

  matrix_write_P(matrix, PSTR("\n"));
  render_status_UserMod(matrix);

  uint32_t layer = layer_state | default_layer_state;
  if ( layer & (1<<KL_(CONFIG)) ) {
    matrix_write_P(matrix, PSTR("\n"));
    render_status_LedMode(matrix);
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
      matrix_write_P(matrix, PSTR(" "));
      matrix_write_P(matrix, layerNameStr_P(0));
  }
  else {
    for ( int layer_idx = 0; layer_idx < KL_NUM; layer_idx++ ) {
      if ( layer & (1<<layer_idx) ) {
        matrix_write_P(matrix, PSTR(" "));
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
      matrix_write_P(matrix, PSTR(" "));
      matrix_write_P(matrix, userModNameStr_P(mod_idx));
    }
  }
}

static void
render_status_LedMode(struct CharacterMatrix *matrix)
{
  char buf[16];

  matrix_write_P(matrix, PSTR("LedMode:"));
  itoa(matled_get_mode(), buf, 10);
  matrix_write(matrix, buf);
}

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
