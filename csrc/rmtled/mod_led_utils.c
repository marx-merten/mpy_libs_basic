#include "mod_led.h"
#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/objarray.h"
#include "./led/rmt_hal.h"

extern void ledmodule_rmtled_checkError(rmtled_result_t code, mp_obj_type_t *alertType, mp_rom_error_text_t msg)
{
    if (code != ESP_OK)
    {
        mp_raise_msg(alertType, msg);
    }
}
extern void ledmodule_rmtled_storePixel(ledmodule_rmtled_obj_t *type, uint16_t index, uint8_t colors[])
{
    assert(index < type->led_count);
    uint8_t *currentBuffer = ((mp_obj_array_t *)MP_OBJ_TO_PTR(type->led_buffer))->items;

    for (uint8_t cidx = 0; cidx < type->bpp; cidx++)
    {
        // printf("DB1[%u] for %u %u\n",index*type->bpp+type->byteorder[cidx],index,cidx);
        currentBuffer[index * type->bpp + type->byteorder[cidx]] = colors[cidx];
    }
    type->dirty = true;
}

extern void ledmodule_rmtled_storePixelRGB(ledmodule_rmtled_obj_t *type, uint16_t index, const ledpixel_CRGB_t color)
{

    if (type->bpp == 3)
    {
        uint8_t cols[3];
        memcpy(cols, color.raw, 3);
        ledmodule_rmtled_storePixel(type, index, cols);
    }
    else
    {
        if (type->autoconvertToRGBW)
        {
            ledpixel_CRGBW_t rgbw;
            ledmodule_rmtled_color_convert_rgb_to_rgbw(color, &rgbw);
            // printf("Converted (%u,%u,%u) -> (%u,%u,%u,%u)\n",color.r,color.g,color.b,rgbw.r,rgbw.g,rgbw.b,rgbw.w);
            ledmodule_rmtled_storePixelRGBW(type, index, rgbw);
        }
        else
        {
            uint8_t cols[4];
            memcpy(cols, color.raw, 3);
            cols[3] = 0;
            ledmodule_rmtled_storePixel(type, index, cols);
        }
    }
}
extern void ledmodule_rmtled_storePixelRGBW(ledmodule_rmtled_obj_t *type, uint16_t index, const ledpixel_CRGBW_t color)
{
    assert(type->bpp == 4);
    uint8_t cols[4];
    memcpy(cols, color.raw, 4);
    ledmodule_rmtled_storePixel(type, index, cols);
}

extern rmtled_result_t ledmodule_rmtled_mpy_convertToRGB(const mp_obj_t pyColors, ledpixel_CRGB_t *target)
{
    if (mp_obj_is_type(pyColors, &mp_type_tuple))
    {
        mp_obj_tuple_t *c = MP_OBJ_TO_PTR(pyColors);
        if (c->len != 3)
        {
            mp_raise_msg(&mp_type_AttributeError, "Incorrect bpp count while converting to rgb. Expecting 3 colors.");
            return RMTLED_STATUS_NOK;
        }
        else
        {
            target->r = (uint8_t)mp_obj_get_int(c->items[0]);
            target->g = (uint8_t)mp_obj_get_int(c->items[1]);
            target->b = (uint8_t)mp_obj_get_int(c->items[2]);
        }
    }
    else if (mp_obj_is_integer(pyColors))
    {
        uint32_t val = mp_obj_get_int(pyColors) & 0xffffff;
        target->r = (val & 0xff0000) >> 16;
        target->g = (val & 0xff00) >> 8;
        target->b = (val & 0xff);
    }
    return RMTLED_STATUS_OK;
}

extern rmtled_result_t ledmodule_rmtled_mpy_convertToRGBW(const mp_obj_t pyColors, ledpixel_CRGBW_t *target)
{
    if (mp_obj_is_type(pyColors, &mp_type_tuple))
    {
        mp_obj_tuple_t *c = MP_OBJ_TO_PTR(pyColors);
        if (c->len != 4)
        {
            mp_raise_msg(&mp_type_AttributeError, "Incorrect bpp count while converting to rgbw. Expecting 4 colors.");
            return RMTLED_STATUS_NOK;
        }
        else
        {
            target->r = (uint8_t)mp_obj_get_int(c->items[0]);
            target->g = (uint8_t)mp_obj_get_int(c->items[1]);
            target->b = (uint8_t)mp_obj_get_int(c->items[2]);
            target->w = (uint8_t)mp_obj_get_int(c->items[3]);
        }
    }
    return RMTLED_STATUS_OK;
}

extern rmtled_result_t ledmodule_rmtled_mpy_convertToHSV(const mp_obj_t pyColors, ledpixel_CHSV_t *target)
{
    if (mp_obj_is_type(pyColors, &mp_type_tuple))
    {
        mp_obj_tuple_t *c = MP_OBJ_TO_PTR(pyColors);
        if (c->len != 3)
        {
            mp_raise_msg(&mp_type_AttributeError, "Incorrect bpp count while converting to rgb. Expecting 3 colors.");
            return RMTLED_STATUS_NOK;
        }
        else
        {
            target->hue = (uint8_t)mp_obj_get_int(c->items[0]);
            target->saturation = (uint8_t)mp_obj_get_int(c->items[1]);
            target->value = (uint8_t)mp_obj_get_int(c->items[2]);
        }
    }
    else if (mp_obj_is_integer(pyColors))
    {
        uint32_t val = mp_obj_get_int(pyColors) & 0xffffff;
        target->hue = (val & 0xff0000) >> 16;
        target->saturation = (val & 0xff00) >> 8;
        target->value = (val & 0xff);
    }
    return RMTLED_STATUS_OK;
}

extern void ledmodule_rmtled_rotate(ledmodule_rmtled_obj_t *self, int steps)
{
    if (steps == 0)
    {
        return;
    } else {
        steps*=self->bpp;
    }
    uint16_t totalsize = self->led_count * self->bpp;
    uint8_t buffer[totalsize];
    uint8_t *currentBuffer = ((mp_obj_array_t *)MP_OBJ_TO_PTR(self->led_buffer))->items;

    memcpy(buffer, currentBuffer, totalsize);

    for (uint16_t x = 0;x < totalsize;x++){

        uint16_t targetpos=((x+steps+totalsize)%totalsize);
        currentBuffer[targetpos] = buffer[x];
    }

}