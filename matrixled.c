#include "config.h"

#include QMK_KEYBOARD_H
#include "rgblight.h"
#include "matrixled.h"

#if defined(RGBLIGHT_ANIMATIONS)
# error please disable RGBLIGHT_ANIMATIONS, check ./rules.mk: LED_ANIMATIONS = no
#endif

// configure
#define DRAW_EXCLUSIVE_TIME     10  /* ms */
#define REFRESH_EXCLUSIVE_TIME  20  /* ms */
#define DECAY_TIME              500 /* ms */
#define TRACING_LEN             5   /* cell */

#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define SQUARE(x)           ((x) * (x))
#define HUE_BIN2DEG(x)      ((x) * (360.f/256.f))
#define HUE_DEG2BIN(x)      ((x) * (256.f/360.f))

// External parameter from matrix.c
extern uint8_t is_master;

// External parameter from rgblight.c
extern rgblight_config_t rgblight_config;
extern LED_TYPE led[RGBLED_NUM];
#define rgblight_led        (*rgblight_led_ptr)
static LED_TYPE (* const rgblight_led_ptr)[RGBLED_NUM] = &led;

// Lighting pattern name
enum as_led_mode {
    LM_SWITCH = 1,
    LM_SWITCH_RB,
    LM_DIMLY,
    LM_DIMLY_RB,
    LM_RIPPLE,
    LM_RIPPLE_RB,
    LM_CROSS,
    LM_CROSS_RB,
    LM_NUM
};

struct as_led_status {
  struct {
    uint8_t hue_bin;
    uint8_t val;
  } led_hv[RGBLED_NUM];
  uint8_t hue_rnd;
  enum as_led_mode mode;
} matled_status;

#define PRESSED_LIST_NUM      (8)
static struct as_pressed {
  keypos_t key;
  uint8_t hue_bin;
  uint16_t count;
} pressed_list[PRESSED_LIST_NUM];
static uint8_t pressed_end;

struct task_timing {
  uint16_t const EXCLUSIVE_TIME;
  uint16_t last_time;
};
static struct task_timing draw_task = {
  .EXCLUSIVE_TIME = DRAW_EXCLUSIVE_TIME,
};
static struct task_timing refresh_task = {
  .EXCLUSIVE_TIME = REFRESH_EXCLUSIVE_TIME,
};

static bool task_timing_check( struct task_timing* );

static void update_color_random(void);

static void post_keypos_to_matled(const keypos_t key_pos);
static void post_keypos_to_queueing(const keypos_t key_pos);

static void matled_draw(void);

static void matled_refresh_SWITCH(void);
static void matled_refresh_DIMLY(void);
static void matled_refresh_RIPPLE(void);
static void matled_refresh_CROSS(void);

static int get_ledidx_from_keypos( keypos_t keypos );

static struct {
  void (*update_color)(void);
  void (*post_keypos)(keypos_t key_pos);
  void (*matled_refresh)(void);
} function_table[LM_NUM] = {
  [LM_SWITCH]     = { NULL,                post_keypos_to_matled,   matled_refresh_SWITCH },
  [LM_SWITCH_RB]  = { update_color_random, post_keypos_to_matled,   matled_refresh_SWITCH },
  [LM_DIMLY]      = { NULL,                post_keypos_to_matled,   matled_refresh_DIMLY },
  [LM_DIMLY_RB]   = { update_color_random, post_keypos_to_matled,   matled_refresh_DIMLY },
  [LM_RIPPLE]     = { NULL,                post_keypos_to_queueing, matled_refresh_RIPPLE },
  [LM_RIPPLE_RB]  = { update_color_random, post_keypos_to_queueing, matled_refresh_RIPPLE },
  [LM_CROSS]      = { NULL,                post_keypos_to_queueing, matled_refresh_CROSS },
  [LM_CROSS_RB]   = { update_color_random, post_keypos_to_queueing, matled_refresh_CROSS },
};

void matled_init(void)
{
  rgblight_config.raw = eeconfig_read_rgblight();
  matled_status.mode = rgblight_config.mode;
  matled_draw();
}

void matled_eeconfig_update(void)
{
  if (matled_status.mode != rgblight_config.mode) {
    rgblight_config.mode = matled_status.mode;
    eeconfig_update_rgblight(rgblight_config.raw);
  }
}

int matled_get_mode(void)
{
  return matled_status.mode;
}

bool matled_get_enable(void)
{
  return rgblight_config.enable;
}

int matled_get_hue(void)
{
  return rgblight_config.hue;
}

int matled_get_sat(void)
{
  return rgblight_config.sat;
}

int matled_get_val(void)
{
  return rgblight_config.val;
}

void matled_mode_forward(void)
{
  // reset current parameters
  matled_status.hue_rnd = 0u;
  for ( int idx = 0; idx < PRESSED_LIST_NUM; idx++ ) {
    pressed_list[idx].count = 0u;
  }
  for ( int idx = 0; idx < RGBLED_NUM; idx++ ) {
    matled_status.led_hv[idx].val = 0u;
    rgblight_led[idx].r = 0u;
    rgblight_led[idx].g = 0u;
    rgblight_led[idx].b = 0u;
  }
  rgblight_set();

  // increment mode
  matled_status.mode = (matled_status.mode + 1) % LM_NUM;
  if ( matled_status.mode == 0 ) {
    matled_status.mode++;
  }
}

void matled_toggle(void)
{
  for ( int idx = 0; idx < PRESSED_LIST_NUM; idx++ ) {
    pressed_list[idx].count = 0u;
  }
  for ( int idx = 0; idx < RGBLED_NUM; idx++ ) {
    matled_status.led_hv[idx].val = 0u;
    rgblight_led[idx].r = 0u;
    rgblight_led[idx].g = 0u;
    rgblight_led[idx].b = 0u;
  }
  rgblight_set();

  if (rgblight_config.enable) {
    matled_status.hue_rnd = 0u;
    rgblight_disable();
  }
  else {
    rgblight_enable();
  }
}

void matled_event_pressed(keyrecord_t *record)
{
  if ( !rgblight_config.enable ) {
    return;
  }

  uint8_t led_mode = matled_status.mode;
  if ( led_mode >= LM_NUM ) {
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

static void update_color_random(void)
{
  matled_status.hue_rnd += rand();
}

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
  }
}

static void post_keypos_to_queueing(keypos_t keypos)
{
    post_keypos_to_matled(keypos);

    uint8_t hue_bin = HUE_DEG2BIN(rgblight_config.hue) + matled_status.hue_rnd;

    pressed_list[pressed_end].key = keypos;
    pressed_list[pressed_end].hue_bin = hue_bin;
    pressed_list[pressed_end].count = 1u;
    pressed_end = (pressed_end + 1) % PRESSED_LIST_NUM;
}

static bool task_timing_check( struct task_timing *task )
{
  uint16_t current_time = timer_read();
  uint16_t diff_time = TIMER_DIFF_16(current_time, task->last_time);
  if ( diff_time < task->EXCLUSIVE_TIME ){
    return false;
  }
  task->last_time = ( diff_time > (2 * task->EXCLUSIVE_TIME) )
                      ? current_time
                      : task->last_time + task->EXCLUSIVE_TIME;
  return true;
}

static void matled_draw(void)
{
  if ( !task_timing_check(&draw_task) ) {
    return;
  }

  for ( int idx = 0; idx < RGBLED_NUM; idx++ ) {
    uint16_t led_hue = HUE_BIN2DEG(matled_status.led_hv[idx].hue_bin);
    uint8_t led_sat  = rgblight_config.sat;
    uint8_t led_val  = matled_status.led_hv[idx].val;
    sethsv(led_hue, led_sat, led_val, &rgblight_led[idx]);
  }
  rgblight_set();
}

void matled_refresh(void)
{
  if ( !task_timing_check(&refresh_task) ) {
    return;
  }

  uint8_t led_mode = matled_status.mode;
  if (led_mode >= LM_NUM) {
    // nothing mode
  }
  else {
    if ( function_table[led_mode].matled_refresh != NULL ) {
      function_table[led_mode].matled_refresh();
    }
  }

  matled_draw();
}

#define FOREACH_MATRIX(row, col, LIMIT_ROW, LIMIT_COL)  \
  int const row##_begin = is_master ? 0 : LIMIT_ROW;    \
  int const row##_end   = row##_begin + LIMIT_ROW;      \
  for ( int row = row##_begin; row < row##_end; row++ ) \
  for ( int col = 0; col < LIMIT_COL; col++ )

static void matled_refresh_SWITCH(void)
{
  FOREACH_MATRIX(row, col, HELIX_ROWS, HELIX_COLS) {
    int led_idx = get_ledidx_from_keypos( (keypos_t){.row = row, .col = col} );
    if ( led_idx >= 0 && !matrix_is_on( row, col ) ) {
      matled_status.led_hv[led_idx].hue_bin = 0u;
      matled_status.led_hv[led_idx].val = 0u;
    }
  }
}

static void matled_refresh_DIMLY(void)
{
  int led_decay_val = rgblight_config.val * (1.f * REFRESH_EXCLUSIVE_TIME / DECAY_TIME);

  FOREACH_MATRIX(row, col, HELIX_ROWS, HELIX_COLS) {
    int led_idx = get_ledidx_from_keypos( (keypos_t){.row = row, .col = col} );
    if ( led_idx >= 0 && !matrix_is_on( row, col ) ) {
      matled_status.led_hv[led_idx].val = MAX(0, matled_status.led_hv[led_idx].val - led_decay_val);
    }
  }
}

static void matled_refresh_RIPPLE(void)
{
  uint16_t const DECAY_COUNT  = 1.f * DECAY_TIME / REFRESH_EXCLUSIVE_TIME;
  float const LINER_SLOPE     = -(rgblight_config.val / TRACING_LEN);
  float const LINER_INTERCEPT = rgblight_config.val;

  for ( int idx = 0; idx < RGBLED_NUM; idx++ ) {
    matled_status.led_hv[idx].val = 0u;
  }

  int const END_IDX = pressed_end;
  int idx = (END_IDX + 1) % PRESSED_LIST_NUM;
  for ( ; idx != END_IDX; idx = (idx + 1) % PRESSED_LIST_NUM ) {
    struct as_pressed* it_source_pos = &pressed_list[idx];
    if ( it_source_pos->count > 0u ) {
      it_source_pos->count++;
      int top_dist = HELIX_COLS * (1.f * it_source_pos->count / DECAY_COUNT);
      int end_dist = MAX(0, top_dist - TRACING_LEN);
      bool redraw = false;
      FOREACH_MATRIX(row, col, HELIX_ROWS, HELIX_COLS) {
        int led_idx = get_ledidx_from_keypos( (keypos_t){.col=col, .row=row} );
        if (led_idx < 0) {
          continue;
        }
        int square_dist = SQUARE(it_source_pos->key.row - row) + SQUARE(it_source_pos->key.col - col);
        if ( (square_dist <= SQUARE(top_dist)) && (square_dist >= SQUARE(end_dist)) ) {
          int dist = sqrt(square_dist);
          int val  = LINER_SLOPE * (top_dist - dist) + LINER_INTERCEPT;
          matled_status.led_hv[led_idx].hue_bin = it_source_pos->hue_bin;
          matled_status.led_hv[led_idx].val     = MIN(matled_status.led_hv[led_idx].val + val, RGBLIGHT_LIMIT_VAL);
          redraw = true;
        }
      }
      if (!redraw) {
        it_source_pos->count = 0u;
      }
    }
  }
}

static void matled_refresh_CROSS(void)
{
  uint16_t const DECAY_COUNT  = 1.f * DECAY_TIME / REFRESH_EXCLUSIVE_TIME;
  float const LINER_SLOPE     = -(rgblight_config.val / TRACING_LEN);
  float const LINER_INTERCEPT = rgblight_config.val;

  for ( int idx = 0; idx < RGBLED_NUM; idx++ ) {
    matled_status.led_hv[idx].val = 0u;
  }

  int const END_IDX = pressed_end;
  int idx = (END_IDX + 1) % PRESSED_LIST_NUM;
  for ( ; idx != END_IDX; idx = (idx + 1) % PRESSED_LIST_NUM ) {
    struct as_pressed* it_source_pos = &pressed_list[idx];
    if ( it_source_pos->count > 0u ) {
      it_source_pos->count++;
      int top_dist = HELIX_COLS * (1.f * it_source_pos->count / DECAY_COUNT);
      int end_dist = MAX(0, top_dist - TRACING_LEN);
      bool redraw = false;
      FOREACH_MATRIX(row, col, HELIX_ROWS, HELIX_COLS) {
        if ( (it_source_pos->key.row != row) && (it_source_pos->key.col != col) ) {
          continue;
        }
        int led_idx = get_ledidx_from_keypos( (keypos_t){.col=col, .row=row} );
        if (led_idx < 0) {
          continue;
        }
        int square_dist = SQUARE(it_source_pos->key.row - row) + SQUARE(it_source_pos->key.col - col);
        if ( (square_dist <= SQUARE(top_dist)) && (square_dist >= SQUARE(end_dist)) ) {
          int dist = sqrt(square_dist);
          int val  = LINER_SLOPE * (top_dist - dist) + LINER_INTERCEPT;
          matled_status.led_hv[led_idx].hue_bin = it_source_pos->hue_bin;
          matled_status.led_hv[led_idx].val     = MIN(matled_status.led_hv[led_idx].val + val, RGBLIGHT_LIMIT_VAL);
          redraw = true;
        }
      }
      if (!redraw) {
        it_source_pos->count = 0u;
      }
    }
  }

}

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
    keypos_is_valid = (HELIX_COLS <= keypos.col)
                   && (keypos.row < MATRIX_ROWS) && (keypos.col < MATRIX_COLS);
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
