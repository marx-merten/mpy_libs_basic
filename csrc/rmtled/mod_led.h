#include "py/obj.h"
#include "py/runtime.h"
#include "./led/rmt_hal.h"
#include "./led/pixel.h"

#ifndef MOD_LED_H
#define MOD_LED_H

// -------------------------------------
//  User Code Defines and Declarations
// -------------------------------------

#define RMT_LED_MODE_WS2812B 1
#define RMT_LED_MODE_SK6812 1
#define RMT_LED_BPP_3 3
#define RMT_LED_BPP_4 4

// -----------------------------
// UPython Usermod Boilerplate
// -----------------------------

// Python TYPE Structure
// used to announce internal fields and share with python classes
// C TYPE
typedef struct _ledmodule_rmtled_obj_t
{
    mp_obj_base_t base; // base represents some basic information, like type
    uint16_t led_count;
    uint8_t led_pin;
    uint8_t bpp;
    mp_obj_t led_buffer;
    bool hal_initialized;
    rmtled_hal_t *hal;
    bool dirty;
    bool useRainbow;
    bool autoconvertToRGBW;
    uint8_t *byteorder;
    rmt_channel_t channel;

} ledmodule_rmtled_obj_t;

#define LEDMODULE_TIMINGS_WS2812B \
    (rmtled_hal_timings_t) { .t0h_ns = 350, .t0l_ns = 800, .t1h_ns = 700, .t1l_ns = 600 }

// PYTHON TYPE
extern const mp_obj_type_t ledmodule_rmtledType;

// ---- Class Functions
// define class functions down here as declarations
// prefix with modulename_classname
extern mp_obj_t ledmodule_rmtled_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
extern void ledmodule_rmtled_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
extern mp_obj_t ledmodule_rmtled_fill(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
extern mp_obj_t ledmodule_rmtled_restore(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
extern mp_obj_t ledmodule_rmtled_clear(mp_obj_t self_in);
extern mp_obj_t ledmodule_rmtled_resize(mp_obj_t self_in,mp_obj_t newSize);

extern mp_obj_t ledmodule_rmtled_set(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
extern mp_obj_t ledmodule_rmtled_leds(mp_obj_t self_in);
extern mp_obj_t ledmodule_rmtled_display(mp_obj_t self_in);
extern mp_obj_t ledmodule_rmtled_init(mp_obj_t self_in);
extern mp_obj_t ledmodule_rmtled_deinit(mp_obj_t self_in);

extern mp_obj_t ledmodule_rmtled_options(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);

// Define C++ Module Entry
extern void doinit_pixels(mp_obj_module_t *mod);

// -------------------------------------
//  Utils Functions
// -------------------------------------

extern void ledmodule_rmtled_storePixel(ledmodule_rmtled_obj_t *type, uint16_t index, uint8_t colors[]);
extern void ledmodule_rmtled_storePixelRGB(ledmodule_rmtled_obj_t *type, uint16_t index, const ledpixel_CRGB_t color);
extern void ledmodule_rmtled_storePixelRGBW(ledmodule_rmtled_obj_t *type, uint16_t index, const ledpixel_CRGBW_t color);
extern void ledmodule_rmtled_checkError(rmtled_result_t code, mp_obj_type_t *alertType, mp_rom_error_text_t msg);
extern rmtled_result_t ledmodule_rmtled_mpy_convertToRGB(const mp_obj_t pyColors, ledpixel_CRGB_t *target);
extern rmtled_result_t ledmodule_rmtled_mpy_convertToRGBW(const mp_obj_t pyColors, ledpixel_CRGBW_t *target);
extern rmtled_result_t ledmodule_rmtled_mpy_convertToHSV(const mp_obj_t pyColors, ledpixel_CHSV_t* target);
extern void ledmodule_rmtled_rotate(ledmodule_rmtled_obj_t *self, int steps);


// Color Functions
extern void ledmodule_rmtled_color_hsv_plain (const ledpixel_CHSV_t hsv, ledpixel_CRGB_t * rgb);
extern void ledmodule_rmtled_color_hsv_rainbow (const ledpixel_CHSV_t hsv, ledpixel_CRGB_t *rgb);
extern void  ledmodule_rmtled_color_convert_rgb_to_rgbw(ledpixel_CRGB_t rgb, ledpixel_CRGBW_t * target);
#endif // MOD_LED_H