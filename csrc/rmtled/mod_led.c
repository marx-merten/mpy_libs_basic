#include "mod_led.h"
#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/objarray.h"
#include "./led/rmt_hal.h"
#include "freertos/queue.h"

// Defines a basic LED Module using advanced timing on esp32 and esp32 only !!!
// IMPORTANT, currently it only supports 1 Instance at any given time, this one needs to be canceled OR it will
// create a error
// ALSO. it needs to be closed before GC otherwise it'll create a potentially memory leak outside
// of the Python heap

// ------------------------
// Function implementation
// ------------------------

// ---- User Functions
// -------------------------
mp_obj_t ledmodule_rmtled_clear(mp_obj_t self_in)
{
    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t *currentBuffer = ((mp_obj_array_t *)MP_OBJ_TO_PTR(self->led_buffer))->items;
    for (int led = 0; led < self->led_count; led++)
    {
        for (uint8_t i = 0; i < self->bpp; i++)
        {
            int idx = (led * self->bpp) + i;
            currentBuffer[idx] = 0;
        }
    }
    self->dirty = true;

    return mp_const_none;
}

mp_obj_t getOptionString(char *str)
{
    return mp_obj_new_str(str, strlen(str));
}

mp_obj_t ledmodule_rmtled_options(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        /* args[0] */ {MP_QSTR_self, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = mp_const_none}}};

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(args[0].u_obj);

    mp_obj_t opts = mp_obj_new_dict(2);

    mp_obj_dict_store(opts, getOptionString("enabledRainbowTables"), mp_obj_new_bool(self->useRainbow));
    mp_obj_dict_store(opts, getOptionString("enabledAutoconvertToRGBW"), mp_obj_new_bool(self->autoconvertToRGBW));

    return opts;
}

mp_obj_t ledmodule_rmtled_fill(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        /* args[0] */ {MP_QSTR_self, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = mp_const_none}},
        /* args[1] */ {MP_QSTR_rgb, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        /* args[2] */ {MP_QSTR_rgbw, MP_ARG_OBJ, {.u_obj = mp_const_none}}};

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(args[0].u_obj);

    uint8_t cols[self->bpp];
    if (args[1].u_obj != mp_const_none)
    {

        ledpixel_CRGB_t rgb = {};
        ledmodule_rmtled_mpy_convertToRGB(args[1].u_obj, &rgb);

        if (self->autoconvertToRGBW && self->bpp == 4)
        {
            ledpixel_CRGBW_t rgbw;
            ledmodule_rmtled_color_convert_rgb_to_rgbw(rgb, &rgbw);
            cols[0] = rgbw.raw[0];
            cols[1] = rgbw.raw[1];
            cols[2] = rgbw.raw[2];
            cols[3] = rgbw.raw[3];
        }
        else
        {
            cols[0] = rgb.raw[0];
            cols[1] = rgb.raw[1];
            cols[2] = rgb.raw[2];
            if (self->bpp == RMT_LED_BPP_4)
            {
                cols[3] = 0;
            }
        }
    }

    if (args[2].u_obj != mp_const_none)
    {

        ledpixel_CRGBW_t rgb = {};
        ledmodule_rmtled_mpy_convertToRGBW(args[2].u_obj, &rgb);
        cols[0] = rgb.raw[0];
        cols[1] = rgb.raw[1];
        cols[2] = rgb.raw[2];
        if (self->bpp == RMT_LED_BPP_4)
        {
            cols[3] = rgb.raw[3];
        }
    }

    for (int i = 0; i < self->led_count; i++)
    {
        ledmodule_rmtled_storePixel(self, i, cols);
    }
    return mp_const_none;
}

mp_obj_t ledmodule_rmtled_restore(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        /* args[0] */ {MP_QSTR_self, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = mp_const_none}},
        /* args[1] */ {MP_QSTR_leds, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = mp_const_none}},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];

    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(args[0].u_obj);

    assert(mp_obj_is_type(args[1].u_obj, &mp_type_bytearray));

    mp_obj_array_t *src = MP_OBJ_TO_PTR(args[1].u_obj);
    mp_obj_array_t *current = ((mp_obj_array_t *)MP_OBJ_TO_PTR(self->led_buffer));
    if (current->len == src->len)
    {
        memcpy(current->items, src->items, src->len);
    }

    return mp_const_none;
}

mp_obj_t ledmodule_rmtled_set(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        /* args[0] */ {MP_QSTR_self, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = mp_const_none}},
        /* args[1] */ {MP_QSTR_led, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_int = 0}},
        /* args[2] */ {MP_QSTR_rgb, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none}},
        /* args[3] */ {MP_QSTR_rgbw, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none}},
        /* args[4] */ {MP_QSTR_hsv_plain, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none}},
        /* args[5] */ {MP_QSTR_hsv_rainbow, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none}},
        /* args[6] */ {MP_QSTR_hsv, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none}}};

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(args[0].u_obj);
    uint16_t led = mp_obj_get_int(args[1].u_obj);

    if (args[2].u_obj != mp_const_none)
    {

        ledpixel_CRGB_t rgb = {};
        ledmodule_rmtled_mpy_convertToRGB(args[2].u_obj, &rgb);
        ledmodule_rmtled_storePixelRGB(self, led, rgb);
    }

    if (args[4].u_obj != mp_const_none)
    {

        ledpixel_CRGB_t rgb = {};
        ledpixel_CHSV_t hsv = {};
        ledmodule_rmtled_mpy_convertToHSV(args[4].u_obj, &hsv);
        ledmodule_rmtled_color_hsv_plain(hsv, &rgb);
        ledmodule_rmtled_storePixelRGB(self, led, rgb);
    }
    if (args[5].u_obj != mp_const_none)
    {

        ledpixel_CRGB_t rgb = {};
        ledpixel_CHSV_t hsv = {};
        ledmodule_rmtled_mpy_convertToHSV(args[5].u_obj, &hsv);
        ledmodule_rmtled_color_hsv_rainbow(hsv, &rgb);
        ledmodule_rmtled_storePixelRGB(self, led, rgb);
    }
    if (args[6].u_obj != mp_const_none)
    {

        ledpixel_CRGB_t rgb = {};
        ledpixel_CHSV_t hsv = {};
        ledmodule_rmtled_mpy_convertToHSV(args[6].u_obj, &hsv);
        if (self->useRainbow)
        {
            ledmodule_rmtled_color_hsv_rainbow(hsv, &rgb);
        }
        else
        {
            ledmodule_rmtled_color_hsv_plain(hsv, &rgb);
        }
        ledmodule_rmtled_storePixelRGB(self, led, rgb);
    }

    if (args[3].u_obj != mp_const_none)
    {
        ledpixel_CRGBW_t rgbw = {};
        ledmodule_rmtled_mpy_convertToRGBW(args[3].u_obj, &rgbw);
        ledmodule_rmtled_storePixelRGBW(self, led, rgbw);
    }

    return args[0].u_obj;
}

mp_obj_t ledmodule_rmtled_resize(mp_obj_t self_in, mp_obj_t newSize)
{
    mp_int_t size = mp_obj_get_int(newSize);
    ledmodule_rmtled_deinit(self_in);

    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // set new Size to leds
    self->led_count = size;
    // create led value buffer
    uint8_t *led_values = m_new(uint8_t, self->led_count * self->bpp);
    self->led_buffer = mp_obj_new_bytearray_by_ref(self->led_count * self->bpp, led_values);

    // old buffer is discarded by GC eventually as it's in python space

    return mp_const_none;
}

mp_obj_t ledmodule_rmtled_leds(mp_obj_t self_in)
{
    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(self_in);

    void *currentBuffer = ((mp_obj_array_t *)MP_OBJ_TO_PTR(self->led_buffer))->items;
    int buffersize = self->led_count * self->bpp;
    uint8_t *led_values = m_new(uint8_t, buffersize);

    memcpy(led_values, currentBuffer, buffersize);
    mp_obj_t led_buffer = mp_obj_new_bytearray_by_ref(buffersize, led_values);

    return led_buffer;
}

// ---- System Functions
// -------------------------

STATIC mp_obj_t ledmodule_rmtled_unary_op(mp_unary_op_t op, mp_obj_t self_in)
{
    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op)
    {
    // case MP_UNARY_OP_BOOL: return mp_obj_new_bool((self->a > 0) && (self->b > 0));
    case MP_UNARY_OP_LEN:
        return mp_obj_new_int(self->led_count);

    default:
        return MP_OBJ_NULL; // operator not supported
    }
}

STATIC mp_obj_t ledmodule_rmtled_binary_op(mp_binary_op_t op, mp_obj_t lhs, mp_obj_t rhs)
{
    ledmodule_rmtled_obj_t *left_hand_side = MP_OBJ_TO_PTR(lhs);
    // ledmodule_rmtled_obj_t *right_hand_side = MP_OBJ_TO_PTR(rhs);

    switch (op)
    {
    // case MP_BINARY_OP_EQUAL:
    //     return mp_obj_new_bool((left_hand_side->a == right_hand_side->a) && (left_hand_side->b == right_hand_side->b));
    // case MP_BINARY_OP_ADD:
    //     return create_new_myclass(left_hand_side->a + right_hand_side->a, left_hand_side->b + right_hand_side->b);
    // case MP_BINARY_OP_MULTIPLY:
    //     return create_new_myclass(left_hand_side->a * right_hand_side->a, left_hand_side->b * right_hand_side->b);
    case MP_BINARY_OP_INPLACE_LSHIFT:
        ledmodule_rmtled_rotate(left_hand_side, mp_obj_get_int(rhs) * -1);
        return left_hand_side;
    case MP_BINARY_OP_INPLACE_RSHIFT:
        ledmodule_rmtled_rotate(left_hand_side, mp_obj_get_int(rhs));
        return left_hand_side;
    default:
        printf("DEBUG %u \n", op);
        return MP_OBJ_NULL; // operator not supported
    }
}

void ledmodule_rmtled_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    // get a ptr to the C-struct of the object
    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "LED(%u) with RMT on Pin %u, count:%u",
              self->bpp, self->led_pin, self->led_count);
}

mp_obj_t ledmodule_rmtled_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    static const mp_arg_t allowed_args[] = {
        /* args[0] */ {MP_QSTR_pin, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none}},
        /* args[1] */ {MP_QSTR_leds, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8}},
        /* args[2] */ {MP_QSTR_bpp, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = RMT_LED_BPP_3}},
        /* args[3] */ {MP_QSTR_channel, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = RMT_CHANNEL_0}},

    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint8_t pin = machine_pin_get_id(args[0].u_obj);
    mp_uint_t leds = args[1].u_int;
    mp_uint_t bpp = args[2].u_int;
    rmt_channel_t channel = args[3].u_int;
    // create a new object of our C-struct type
    ledmodule_rmtled_obj_t *self = m_new_obj(ledmodule_rmtled_obj_t);

    self->led_pin = pin;
    self->led_count = leds;
    self->bpp = bpp;
    self->channel = channel;
    self->dirty = true;
    self->hal_initialized = false;
    self->useRainbow = false; // use rainbow conversion by default !!!
    self->autoconvertToRGBW = true;
    self->loopCore = 1;

    // TODO, Make sure this is configurable
    self->byteorder = malloc(4);
    // IN RGBW OUT GRBW
    self->byteorder[0] = (uint)1;
    self->byteorder[1] = (uint)0;
    self->byteorder[2] = (uint)2;
    self->byteorder[3] = (uint)3;

    // create led value buffer
    uint8_t *led_values = m_new(uint8_t, leds * bpp);
    self->led_buffer = mp_obj_new_bytearray_by_ref(leds * bpp, led_values);
    // give it a type
    self->base.type = &ledmodule_rmtledType;
    return MP_OBJ_FROM_PTR(self);
}

static void ledmodule_task_hal(void *module)
{
    ledmodule_rmtled_obj_t *self = module;
    ledmodule_rmtled_loopmsg_t queueMsg;

    for (;;)
    {
        if (xQueueReceive(self->loopQueue, &(queueMsg), pdMS_TO_TICKS(5000)))
        {
            switch (queueMsg.type)
            {
            case INIT:;
                rmtled_hal_t *hal = rmtled_hal_init(self->bpp, self->led_count, self->led_pin, self->channel, LEDMODULE_TIMINGS_WS2812B);
                self->hal = hal;
                self->hal_initialized = true;
                LED_SEND_CMD(self->loopReturnQueue, DONE, 500);
                break;
            case DEINIT:
                // wait first-making sure we don't interrupt an ongoing transmit
                rmt_wait_tx_done(self->hal->config.channel, 200);
                rmtled_hal_deinit(*self->hal);
                LED_SEND_CMD(self->loopReturnQueue, DONE, 500);
                break;
            case DISPLAY:;
                uint8_t *currentBuffer = ((mp_obj_array_t *)MP_OBJ_TO_PTR(self->led_buffer))->items;
                rmtled_hal_send(self->hal, currentBuffer);
                LED_SEND_CMD(self->loopReturnQueue, DONE, 500);
                break;
            case MSG:
                printf("%s\n", queueMsg.msg);
                break;
            default:
                break;
            }
        }
    }
}

extern mp_obj_t ttt_testcore(mp_obj_t self_in)
{
    printf("Running task 1\n");
    printf("Hello world from core: %d!\n", xPortGetCoreID());
    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(self_in);
    LED_SEND_MSG(self->loopQueue, "NOPE", 1000);
    return mp_const_none;
}
extern mp_obj_t ledmodule_rmtled_deinit(mp_obj_t self_in)
{
    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->hal_initialized)
    {

        LED_SEND_CMD(self->loopQueue, DEINIT, 1000);
        ledmodule_rmtled_loopmsg_t receive;
        xQueueReceive(self->loopReturnQueue, &receive, pdMS_TO_TICKS(500));
        if (receive.type != DONE)
        {
            // add some security Delay
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        vTaskDelete(self->loopHandle);
        vQueueDelete(self->loopQueue);
        vQueueDelete(self->loopReturnQueue);
        free(self->hal);
        self->hal = NULL;
        self->hal_initialized = false;
    }
    return mp_const_none;
}

extern mp_obj_t ledmodule_rmtled_init(mp_obj_t self_in)
{
    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->hal_initialized)
    {
        // Start Background Loop
        // ----------------------
        self->loopQueue = xQueueCreate(5, sizeof(ledmodule_rmtled_loopmsg_t));
        self->loopReturnQueue = xQueueCreate(5, sizeof(ledmodule_rmtled_loopmsg_t));
        xTaskCreatePinnedToCore(ledmodule_task_hal, "isr_task", 2048, MP_OBJ_TO_PTR(self_in), 5, &self->loopHandle, self->loopCore);

        LED_SEND_CMD(self->loopQueue, INIT, 1000);
        ledmodule_rmtled_loopmsg_t receive;

        xQueueReceive(self->loopReturnQueue, &receive, pdMS_TO_TICKS(500));
        if (receive.type != DONE)
        {
            // add some security Delay
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (self->hal == NULL)
        {
            mp_raise_msg(&mp_type_Exception, "unable to initialize RMT-HAL Layer");
            self->hal_initialized = false;
        }
    }
    return mp_const_none;
}

extern mp_obj_t ledmodule_rmtled_display(mp_obj_t self_in)
{
    // INIT if needed
    ledmodule_rmtled_init(self_in);

    ledmodule_rmtled_obj_t *self = MP_OBJ_TO_PTR(self_in);

    LED_SEND_CMD(self->loopQueue, DISPLAY, 1000);
    ledmodule_rmtled_loopmsg_t receive;
    xQueueReceive(self->loopReturnQueue, &receive, pdMS_TO_TICKS(500));

    self->dirty = false;
    // TODO Check Status
    return mp_const_none;
}

// --------------------------
// Python Module Boilerplate
// --------------------------

// ---- Function and globaal declarations
// EXAMPLES
// MP_DEFINE_CONST_FUN_OBJ_1(mymodule_hello_increment_obj, mymodule_hello_increment);
// MP_DEFINE_CONST_FUN_OBJ_2(mymodule_hello_display_obj, display);
// MP_DEFINE_CONST_FUN_OBJ_0(mymodule_mem_obj, memory_info);
MP_DEFINE_CONST_FUN_OBJ_KW(ledmodule_rmtled_fill_funcObj, 1, ledmodule_rmtled_fill);
MP_DEFINE_CONST_FUN_OBJ_KW(ledmodule_rmtled_option_funcObj, 1, ledmodule_rmtled_options);
MP_DEFINE_CONST_FUN_OBJ_1(ledmodule_rmtled_clear_funcObj, ledmodule_rmtled_clear);
MP_DEFINE_CONST_FUN_OBJ_2(ledmodule_rmtled_resize_funcObj, ledmodule_rmtled_resize);
MP_DEFINE_CONST_FUN_OBJ_1(ledmodule_rmtled_leds_funcObj, ledmodule_rmtled_leds);
MP_DEFINE_CONST_FUN_OBJ_KW(ledmodule_rmtled_set_funcObj, 2, ledmodule_rmtled_set);
MP_DEFINE_CONST_FUN_OBJ_1(ledmodule_rmtled_display_funcObj, ledmodule_rmtled_display);
MP_DEFINE_CONST_FUN_OBJ_1(ledmodule_rmtled_init_funcObj, ledmodule_rmtled_init);
MP_DEFINE_CONST_FUN_OBJ_1(ledmodule_rmtled_deinit_funcObj, ledmodule_rmtled_deinit);
MP_DEFINE_CONST_FUN_OBJ_1(ledmodule_rmtled_test_funcObj, ttt_testcore);
MP_DEFINE_CONST_FUN_OBJ_KW(ledmodule_rmtled_restore_funcObj, 2, ledmodule_rmtled_restore);
// ---- local class members
STATIC const mp_rom_map_elem_t ledmodule_rmtled_locals_dict_table[] = {
    // {MP_ROM_QSTR(MP_QSTR_inc), MP_ROM_PTR(&mymodule_hello_increment_obj)},
    // {MP_ROM_QSTR(MP_QSTR_disp), MP_ROM_PTR(&mymodule_hello_display_obj)},
    // {MP_ROM_QSTR(MP_QSTR_mem), MP_ROM_PTR(&mymodule_mem_obj)},
    {MP_ROM_QSTR(MP_QSTR_MODE_WS2812B), MP_ROM_INT(RMT_LED_MODE_WS2812B)},
    {MP_ROM_QSTR(MP_QSTR_BPP_3), MP_ROM_INT(RMT_LED_BPP_3)},
    {MP_ROM_QSTR(MP_QSTR_BPP_4), MP_ROM_INT(RMT_LED_BPP_4)},
    {MP_ROM_QSTR(MP_QSTR_RMT_CHANNEL_0), MP_ROM_INT(RMT_CHANNEL_0)},
    {MP_ROM_QSTR(MP_QSTR_RMT_CHANNEL_1), MP_ROM_INT(RMT_CHANNEL_1)},
    {MP_ROM_QSTR(MP_QSTR_RMT_CHANNEL_2), MP_ROM_INT(RMT_CHANNEL_2)},
    {MP_ROM_QSTR(MP_QSTR_RMT_CHANNEL_3), MP_ROM_INT(RMT_CHANNEL_3)},
    {MP_ROM_QSTR(MP_QSTR_RMT_CHANNEL_4), MP_ROM_INT(RMT_CHANNEL_4)},
    {MP_ROM_QSTR(MP_QSTR_RMT_CHANNEL_5), MP_ROM_INT(RMT_CHANNEL_5)},
    {MP_ROM_QSTR(MP_QSTR_RMT_CHANNEL_6), MP_ROM_INT(RMT_CHANNEL_6)},
    {MP_ROM_QSTR(MP_QSTR_RMT_CHANNEL_7), MP_ROM_INT(RMT_CHANNEL_7)},
    {MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&ledmodule_rmtled_fill_funcObj)},
    {MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&ledmodule_rmtled_clear_funcObj)},
    {MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&ledmodule_rmtled_set_funcObj)},
    {MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&ledmodule_rmtled_display_funcObj)},
    {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&ledmodule_rmtled_init_funcObj)},
    {MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&ledmodule_rmtled_deinit_funcObj)},
    {MP_ROM_QSTR(MP_QSTR_options), MP_ROM_PTR(&ledmodule_rmtled_option_funcObj)},
    {MP_ROM_QSTR(MP_QSTR_restore), MP_ROM_PTR(&ledmodule_rmtled_restore_funcObj)},
    {MP_ROM_QSTR(MP_QSTR_resize), MP_ROM_PTR(&ledmodule_rmtled_resize_funcObj)},
    {MP_ROM_QSTR(MP_QSTR_backup), MP_ROM_PTR(&ledmodule_rmtled_leds_funcObj)},
    {MP_ROM_QSTR(MP_QSTR_coretest), MP_ROM_PTR(&ledmodule_rmtled_test_funcObj)},

};

STATIC MP_DEFINE_CONST_DICT(ledmodule_rmtled_locals_dict, ledmodule_rmtled_locals_dict_table);

// create the class-object itself
const mp_obj_type_t ledmodule_rmtledType =
    {
        {&mp_type_type},                 // "inherit" the type "type"
        .name = MP_QSTR_RmtLed,          // give it a name
        .print = ledmodule_rmtled_print, // give it a print-function
        .unary_op = ledmodule_rmtled_unary_op,
        .binary_op = ledmodule_rmtled_binary_op,
        .make_new = ledmodule_rmtled_make_new,                         // give it a constructor
        .locals_dict = (mp_obj_dict_t *)&ledmodule_rmtled_locals_dict, // and the local members
};
