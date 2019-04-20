#include "config.h"

#include QMK_KEYBOARD_H
#include "rgblight.h"
#include "matrixled.h"

// configure
#define DECAY_TIME              200 // ms
#define MATLED_TASK_TIME        10  // ms
#define TRACING_LEN             5   // cell

#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define SQUARE(x)           ((x) * (x))
#define HUE_BIN_MAX         255
#define HUE_BIN_QN          256
#define HUE_BIN2DEG(x)      ((x) * (360.f/HUE_BIN_QN))
#define HUE_DEG2BIN(x)      ((x) * (HUE_BIN_QN/360.f))

// External parameter from matrix.c
extern uint8_t is_master;

// External parameter from rgblight.c
extern rgblight_config_t rgblight_config;
extern LED_TYPE led[RGBLED_NUM];
#define rgblight_led        (*rgblight_led_ptr)
static LED_TYPE (* const rgblight_led_ptr)[RGBLED_NUM] = &led;

// Lighting pattern name
enum LightingPattern {
  LP_STATIC = 0,
  #ifdef ENABLE_MATLED_SWITCH_PATTERN
    LP_SWITCH,
    LP_SWITCH_RB,
  #endif
  #ifdef ENABLE_MATLED_DIMLY_PATTERN
    LP_DIMLY,
    LP_DIMLY_RB,
  #endif
  #ifdef ENABLE_MATLED_RIPPLE_PATTERN
    LP_RIPPLE,
    LP_RIPPLE_RB,
  #endif
  #ifdef ENABLE_MATLED_CROSS_PATTERN
    LP_CROSS,
    LP_CROSS_RB,
  #endif
  #ifdef ENABLE_MATLED_WAVE_PATTERN
    LP_WAVE,
    LP_WAVE_RB,
  #endif
  LP_NUM
};

struct {
  struct {
    uint8_t hue_bin;
    uint8_t val;
  } led_hv[RGBLED_NUM];
  uint8_t hue_rnd;
  enum LightingPattern mode;
  bool is_refreshed;
} matled_status;

#define PRESSED_LIST_NUM      (8)
static struct PressedRecord {
  keypos_t key;
  uint8_t hue_bin;
  int count;
} pressed_list[PRESSED_LIST_NUM];
static uint8_t pressed_end = 0;

struct TaskTiming {
  uint16_t const EXCLUSIVE_TIME;
  uint16_t last_time;
};
static struct TaskTiming draw_task = {
  .EXCLUSIVE_TIME = MATLED_TASK_TIME,
};
static struct TaskTiming refresh_task = {
  .EXCLUSIVE_TIME = MATLED_TASK_TIME,
};

static bool task_timing_check( struct TaskTiming* );

static void update_color_random(void);

static void post_keypos_to_matled(const keypos_t key_pos);
static void post_keypos_to_queueing(const keypos_t key_pos);

static void matled_draw(void);
static void matled_clear(void);
static void matled_clear_led_hv(void);
static void matled_toggle(void);
static void matled_mode_forward(void);
static void matled_event_pressed(keyrecord_t *record);

#ifdef ENABLE_MATLED_SWITCH_PATTERN
static void matled_refresh_SWITCH(void);
#endif
#ifdef ENABLE_MATLED_DIMLY_PATTERN
static void matled_refresh_DIMLY(void);
#endif
#ifdef ENABLE_MATLED_RIPPLE_PATTERN
static void matled_refresh_RIPPLE(void);
#endif
#ifdef ENABLE_MATLED_CROSS_PATTERN
static void matled_refresh_CROSS(void);
#endif
#ifdef ENABLE_MATLED_WAVE_PATTERN
static void matled_refresh_WAVE(void);
static void matled_refresh_WAVE_RB(void);
#endif

static int get_ledidx_from_keypos( keypos_t keypos );
static int distance(int x, int y);
static int distance_from_line(int x, int y, int m, int n);

static const struct {
  void (*update_color)(void);
  void (*post_keypos)(keypos_t key_pos);
  void (*matled_refresh)(void);
} function_table[LP_NUM] = {
  [LP_STATIC]        = { 0 },
  #ifdef ENABLE_MATLED_SWITCH_PATTERN
    [LP_SWITCH]     = { NULL,                post_keypos_to_matled,   matled_refresh_SWITCH },
    [LP_SWITCH_RB]  = { update_color_random, post_keypos_to_matled,   matled_refresh_SWITCH },
  #endif
  #ifdef ENABLE_MATLED_DIMLY_PATTERN
    [LP_DIMLY]      = { NULL,                post_keypos_to_matled,   matled_refresh_DIMLY },
    [LP_DIMLY_RB]   = { update_color_random, post_keypos_to_matled,   matled_refresh_DIMLY },
  #endif
  #ifdef ENABLE_MATLED_RIPPLE_PATTERN
    [LP_RIPPLE]     = { NULL,                post_keypos_to_queueing, matled_refresh_RIPPLE },
    [LP_RIPPLE_RB]  = { update_color_random, post_keypos_to_queueing, matled_refresh_RIPPLE },
  #endif
  #ifdef ENABLE_MATLED_CROSS_PATTERN
    [LP_CROSS]      = { NULL,                post_keypos_to_queueing, matled_refresh_CROSS },
    [LP_CROSS_RB]   = { update_color_random, post_keypos_to_queueing, matled_refresh_CROSS },
  #endif
  #ifdef ENABLE_MATLED_WAVE_PATTERN
    [LP_WAVE]       = { NULL,                NULL,                    matled_refresh_WAVE },
    [LP_WAVE_RB]    = { NULL,                NULL,                    matled_refresh_WAVE_RB },
  #endif
};

void matled_init(void)
{
  rgblight_config.raw = eeconfig_read_rgblight();
  matled_status.mode = rgblight_config.mode;

  matled_clear();
}

int matled_get_mode(void)
{
  return matled_status.mode;
}

void matled_refresh_task(void)
{
  if ( !task_timing_check(&refresh_task) ) {
    return;
  }

  uint8_t led_mode = matled_status.mode;
  if (led_mode >= LP_NUM) {
    // nothing mode
  }
  else {
    if ( function_table[led_mode].matled_refresh != NULL ) {
      function_table[led_mode].matled_refresh();
    }
  }

  matled_draw();
}

#define PROCESS_OVERRIDE_BEHAVIOR   (false)
#define PROCESS_USUAL_BEHAVIOR      (true)

bool matled_record_event(uint16_t keycode, keyrecord_t *record)
{
  switch (keycode) {
    case RGB_TOG: if (record->event.pressed) {
      matled_toggle();
      return PROCESS_OVERRIDE_BEHAVIOR;
    } break;

    case RGB_MOD: if (record->event.pressed) {
      matled_mode_forward();
      return PROCESS_OVERRIDE_BEHAVIOR;
    } break;
  }

  if (record->event.pressed) {
    matled_event_pressed(record);
  }

  return PROCESS_USUAL_BEHAVIOR;
}

__attribute__ ((unused))
static void matled_toggle(void)
{
  if (rgblight_config.enable) {
    rgblight_disable();
  }
  else {
    rgblight_enable();
  }

  matled_clear();
}

__attribute__ ((unused))
static void matled_mode_forward(void)
{
  matled_status.mode = (matled_status.mode + 1) % LP_NUM;
  rgblight_config.mode = matled_status.mode;
  eeconfig_update_rgblight(rgblight_config.raw);

  matled_clear();
}

__attribute__ ((unused))
static void matled_event_pressed(keyrecord_t *record)
{
  if ( (!rgblight_config.enable) || (matled_status.mode == LP_STATIC) ) {
    return;
  }

  uint8_t led_mode = matled_status.mode;
  if ( led_mode >= LP_NUM ) {
    // nothing mode
  }
  else {
    if ( function_table[led_mode].update_color != NULL ) {
      function_table[led_mode].update_color();
    }
    if ( function_table[led_mode].post_keypos != NULL ) {
      function_table[led_mode].post_keypos(record->event.key);
    }
  }

  matled_draw();
}

__attribute__ ((unused))
static void update_color_random(void)
{
  matled_status.hue_rnd += rand();
}

__attribute__ ((unused))
static void post_keypos_to_matled(keypos_t keypos)
{
  int led_idx = get_ledidx_from_keypos(keypos);
  if ( led_idx < 0 ) {
    // nothing led
  }
  else {
    uint8_t hue_bin = HUE_DEG2BIN(rgblight_config.hue) + matled_status.hue_rnd;
    uint8_t led_val = rgblight_config.val;

    matled_status.led_hv[led_idx].hue_bin = hue_bin;
    matled_status.led_hv[led_idx].val = led_val;
    matled_status.is_refreshed = true;
  }
}

__attribute__ ((unused))
static void post_keypos_to_queueing(keypos_t keypos)
{
    post_keypos_to_matled(keypos);

    uint8_t hue_bin = HUE_DEG2BIN(rgblight_config.hue) + matled_status.hue_rnd;

    pressed_list[pressed_end].key = keypos;
    pressed_list[pressed_end].hue_bin = hue_bin;
    pressed_list[pressed_end].count = 1u;
    pressed_end = (pressed_end + 1) % PRESSED_LIST_NUM;
}

__attribute__ ((unused))
static bool task_timing_check( struct TaskTiming *task )
{
  uint16_t current_time = timer_read();
  uint16_t diff_time = TIMER_DIFF_16(current_time, task->last_time);
  if ( diff_time < task->EXCLUSIVE_TIME ){
    return false;
  }

  task->last_time = current_time;
  return true;
}

__attribute__ ((unused))
static void matled_draw(void)
{
  if (!matled_status.is_refreshed) {
    return;
  }
  if ( (matled_status.mode == LP_STATIC) || !task_timing_check(&draw_task) ) {
    return;
  }

  for ( int idx = 0; idx < RGBLED_NUM; idx++ ) {
    uint16_t led_hue = HUE_BIN2DEG(matled_status.led_hv[idx].hue_bin);
    uint8_t led_sat  = rgblight_config.sat;
    uint8_t led_val  = matled_status.led_hv[idx].val;
    sethsv(led_hue, led_sat, led_val, &rgblight_led[idx]);
  }
  matled_status.is_refreshed = false;

  rgblight_set();
}

__attribute__ ((unused))
static void matled_clear(void)
{
  for ( int idx = 0; idx < PRESSED_LIST_NUM; idx++ ) {
    pressed_list[idx].count = 0u;
  }

  if (matled_status.mode == LP_STATIC) {
    rgblight_sethsv(rgblight_config.hue, rgblight_config.sat, rgblight_config.val);
  }
  else {
    matled_status.hue_rnd = 0u;
    matled_clear_led_hv();
    matled_status.is_refreshed = true;
    matled_draw();
  }
}

__attribute__ ((unused))
static void matled_clear_led_hv(void)
{
  for ( int idx = 0; idx < RGBLED_NUM; idx++ ) {
    matled_status.led_hv[idx].val = 0u;
  }
}

#define FOREACH_MATRIX(row, col, LIMIT_ROW, LIMIT_COL)  \
  int const row##_begin = is_master ? 0 : LIMIT_ROW;    \
  int const row##_end   = row##_begin + LIMIT_ROW;      \
  for ( int row = row##_begin; row < row##_end; row++ ) \
  for ( int col = 0; col < LIMIT_COL; col++ )

#ifdef ENABLE_MATLED_SWITCH_PATTERN
static void matled_refresh_SWITCH(void)
{
  FOREACH_MATRIX(row, col, HELIX_ROWS, HELIX_COLS) {
    int led_idx = get_ledidx_from_keypos( (keypos_t){.row = row, .col = col} );
    if ( led_idx >= 0 && !matrix_is_on( row, col ) ) {
      matled_status.led_hv[led_idx].hue_bin = 0u;
      matled_status.led_hv[led_idx].val = 0u;
      matled_status.is_refreshed = true;
    }
  }
}
#endif // ENABLE_MATLED_SWITCH_PATTERN

#ifdef ENABLE_MATLED_DIMLY_PATTERN
static void matled_refresh_DIMLY(void)
{
  int led_decay_val = rgblight_config.val * (1.f * MATLED_TASK_TIME / DECAY_TIME);

  FOREACH_MATRIX(row, col, HELIX_ROWS, HELIX_COLS) {
    int led_idx = get_ledidx_from_keypos( (keypos_t){.row = row, .col = col} );
    if ( led_idx >= 0 && !matrix_is_on( row, col ) ) {
      matled_status.led_hv[led_idx].val = MAX(0, matled_status.led_hv[led_idx].val - led_decay_val);
      matled_status.is_refreshed = true;
    }
  }
}
#endif

#ifdef ENABLE_MATLED_RIPPLE_PATTERN
static void matled_refresh_RIPPLE(void)
{
  static const int factor_numer = RGBLIGHT_LIMIT_VAL;
  static const int factor_denom = TRACING_LEN;
  static const int factor       = factor_numer / factor_denom;
  static const int count_step   = factor * TRACING_LEN / (1.f * DECAY_TIME / MATLED_TASK_TIME);
  static const int near_max     = factor * (HELIX_ROWS + HELIX_COLS);

  int const idx_end = pressed_end;
  int idx = (idx_end + 1) % PRESSED_LIST_NUM;
  for ( ; idx != idx_end; idx = (idx + 1) % PRESSED_LIST_NUM ) {
    struct PressedRecord* it_source_pos = &pressed_list[idx];
    if ( it_source_pos->count <= 0u ) {
      continue;
    }
    else if (!matled_status.is_refreshed) {
      matled_clear_led_hv();
      matled_status.is_refreshed = true;
    }

    int far = it_source_pos->count;
    int near = MAX(0, far - (factor*TRACING_LEN));
    if (near >= near_max) {
      it_source_pos->count = 0u;
      continue;
    }
    int outline = far + factor*1;

    FOREACH_MATRIX(row, col, HELIX_ROWS, HELIX_COLS) {
      int led_idx = get_ledidx_from_keypos( (keypos_t){.col=col, .row=row} );
      if (led_idx < 0) {
        continue;
      }

      // led_val = (LIMIT_VAL / LIMIT_CELL) * cell_num;
      int x = factor * (it_source_pos->key.col - col);
      int y = factor * (it_source_pos->key.row - row);
      int d = distance(x, y);
      if ((d < near) || (outline < d)) {
        continue;
      }
      else if (d > far) {
        // led_val' = (LIMIT_VAL / 1) * cell_num
        //          = (LIMIT_VAL / LIMIT_CELL) * cell_num * LIMIT_CELL
        //          = led_val * LIMIT_CELL
        d = (far - d) * factor_denom + far;
      }

      int val = rgblight_config.val - (far - d);
      matled_status.led_hv[led_idx].hue_bin = it_source_pos->hue_bin;
      matled_status.led_hv[led_idx].val     = MIN(matled_status.led_hv[led_idx].val + val, RGBLIGHT_LIMIT_VAL);
    }
    it_source_pos->count += count_step;
  }
}
#endif // ENABLE_MATLED_RIPPLE_PATTERN

#ifdef ENABLE_MATLED_CROSS_PATTERN
static void matled_refresh_CROSS(void)
{
  static const int factor = RGBLIGHT_LIMIT_VAL / HELIX_COLS;
  static const int count_step = factor * TRACING_LEN / (1.f * DECAY_TIME / MATLED_TASK_TIME);
  static const int near_max = factor * (HELIX_ROWS + HELIX_COLS);

  int const idx_end = pressed_end;
  int idx = (idx_end + 1) % PRESSED_LIST_NUM;
  for ( ; idx != idx_end; idx = (idx + 1) % PRESSED_LIST_NUM ) {
    struct PressedRecord* it_source_pos = &pressed_list[idx];
    if ( it_source_pos->count <= 0u ) {
      continue;
    }
    else if (!matled_status.is_refreshed) {
      matled_clear_led_hv();
      matled_status.is_refreshed = true;
    }

    int far = it_source_pos->count;
    int near = MAX(0, far - (factor*TRACING_LEN));
    if (near >= near_max) {
      it_source_pos->count = 0u;
      continue;
    }

    FOREACH_MATRIX(row, col, HELIX_ROWS, HELIX_COLS) {
      if ( (it_source_pos->key.row != row) && (it_source_pos->key.col != col) ) {
        continue;
      }

      int led_idx = get_ledidx_from_keypos( (keypos_t){.col=col, .row=row} );
      if (led_idx < 0) {
        continue;
      }

      int x = factor * (it_source_pos->key.col - col);
      int y = factor * (it_source_pos->key.row - row);
      int d = distance(x, y);
      if ((d < near) || (far < d)) {
        continue;
      }

      int val = rgblight_config.val - (far - d);
      matled_status.led_hv[led_idx].hue_bin = it_source_pos->hue_bin;
      matled_status.led_hv[led_idx].val     = MIN(matled_status.led_hv[led_idx].val + val, RGBLIGHT_LIMIT_VAL);
    }
    it_source_pos->count += count_step;
  }
}
#endif // ENABLE_MATLED_CROSS_PATTERN

#ifdef ENABLE_MATLED_WAVE_PATTERN
static void matled_refresh_WAVE(void)
{
  static const int factor = RGBLIGHT_LIMIT_VAL / TRACING_LEN;
  static const int slope = -1;
  static const int ofst_step = 256 * (MATLED_TASK_TIME / 1000.f);
  static uint16_t ofst;

  ofst += ofst_step;

  FOREACH_MATRIX(row, col, HELIX_ROWS, HELIX_COLS) {
    int led_idx = get_ledidx_from_keypos( (keypos_t){.col=col, .row=row} );
    if (led_idx < 0) {
      continue;
    }

    int x = factor * col;
    int y = factor * row;
    unsigned int d     = distance_from_line(x, y, slope, ofst);
    unsigned int d_mod = d % RGBLIGHT_LIMIT_VAL;
    int value = (d / RGBLIGHT_LIMIT_VAL) & 1
                  ? d_mod
                  : rgblight_config.val - d_mod;
    matled_status.led_hv[led_idx].hue_bin = HUE_DEG2BIN(rgblight_config.hue);
    matled_status.led_hv[led_idx].val     = MAX(0, value);
  }
  matled_status.is_refreshed = true;
}

static void matled_refresh_WAVE_RB(void)
{
  static uint16_t count;
  static const uint16_t count_step = 256 * (MATLED_TASK_TIME / 1000.f);
  static const uint8_t factor = 128 / HELIX_COLS;

  FOREACH_MATRIX(row, col, HELIX_ROWS, HELIX_COLS) {
    int led_idx = get_ledidx_from_keypos( (keypos_t){.col=col, .row=row} );
    if (led_idx < 0) {
      continue;
    }

    matled_status.led_hv[led_idx].hue_bin = factor * (row + col) + count;
    matled_status.led_hv[led_idx].val     = rgblight_config.val;
  }
  matled_status.is_refreshed = true;

  count -= count_step;
}
#endif // ENABLE_MATLED_WAVE_PATTERN

static int get_ledidx_from_keypos( keypos_t keypos )
{
  static const uint8_t PROGMEM keypos2ledidx_lut[MATRIX_ROWS][MATRIX_COLS] = LAYOUT( \
       5,  4,  3,  2,  1, 32,         32,  1,  2,  3,  4,  5, \
       6,  7,  8,  9, 10, 11,         11, 10,  9,  8,  7,  6, \
      17, 16, 15, 14, 13, 12,         12, 13, 14, 15, 16, 17, \
      18, 19, 20, 21, 22, 23, 24, 24, 23, 22, 21, 20, 19, 18, \
      31, 30, 29, 28, 27, 26, 25, 25, 26, 27, 28, 29, 30, 31  \
  );
  bool keypos_is_valid;
  if ( is_master ) {
    keypos_is_valid = (keypos.row < HELIX_ROWS) && (keypos.col < HELIX_COLS);
  }
  else {
    keypos_is_valid = (HELIX_ROWS <= keypos.row) && (keypos.row < MATRIX_ROWS)
                                                 && (keypos.col < MATRIX_COLS);
  }
  if ( !keypos_is_valid ) {
    return -1;
  }
  else {
    int idx = pgm_read_byte(&keypos2ledidx_lut[keypos.row][keypos.col]);
    int idx_mod32 = idx & 31;
    return ( idx > 0 ) ? ( idx_mod32 ) : ( -1 );
  }
}

__attribute__ ((unused))
static int distance(int x, int y)
{
#if 1
  // [REF]Algorithms / Distance approximations - Octagonal, https://en.wikibooks.org/wiki/Algorithms/Distance_approximations
  int absx = abs(x);
  int absy = abs(y);
  int max, min;
  if (absx > absy) {
    max = absx;
    min = absy;
  }
  else {
    max = absy;
    min = absx;
  }

  return (964L * max + 420L * min) / 1024L;
#else
  return sqrt(x*x + y*y);
#endif
}

__attribute__ ((unused))
static int distance_from_line(int x, int y, int m, int n)
{
  int a = m, b = -1, c = n;
  int tmp_a = abs(a*x + b*y + c);
  int tmp_b = distance(a, b);
  return tmp_a / tmp_b;
}
