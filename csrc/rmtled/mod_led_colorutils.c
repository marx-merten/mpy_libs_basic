#include "mod_led.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "py/mphal.h"
#include "py/objarray.h"
#include "./led/rmt_hal.h"

#define APPLY_DIMMING(X) (X)
#define HSV_SECTION_6 (0x20)
#define HSV_SECTION_3 (0x40)
#define K255 255
#define K171 171
#define K170 170
#define K85 85

#define FORCE_REFERENCE(var) asm volatile("" \
                                          :  \
                                          : "r"(var))

uint8_t scale8(uint8_t i, uint8_t scale)
{
    return (((uint16_t)i) * (1 + (uint16_t)(scale))) >> 8;
}

uint8_t scale8_video(uint8_t i, uint8_t scale)
{
    uint8_t j = (((int)i * (int)scale) >> 8) + ((i && scale) ? 1 : 0);
    // uint8_t nonzeroscale = (scale != 0) ? 1 : 0;
    // uint8_t j = (i == 0) ? 0 : (((int)i * (int)(scale) ) >> 8) + nonzeroscale;
    return j;
}

extern void ledmodule_rmtled_color_hsv_rainbow(const ledpixel_CHSV_t hsv, ledpixel_CRGB_t *rgb)
{
    // Yellow has a higher inherent brightness than
    // any other color; 'pure' yellow is perceived to
    // be 93% as bright as white.  In order to make
    // yellow appear the correct relative brightness,
    // it has to be rendered brighter than all other
    // colors.
    // Level Y1 is a moderate boost, the default.
    // Level Y2 is a strong boost.
    const uint8_t Y1 = 1;
    const uint8_t Y2 = 0;

    // G2: Whether to divide all greens by two.
    // Depends GREATLY on your particular LEDs
    const uint8_t G2 = 0;

    // Gscale: what to scale green down by.
    // Depends GREATLY on your particular LEDs
    const uint8_t Gscale = 0;

    uint8_t hue = hsv.hue;
    uint8_t sat = hsv.sat;
    uint8_t val = hsv.val;

    uint8_t offset = hue & 0x1F; // 0..31

    // offset8 = offset * 8
    uint8_t offset8 = offset;
    {
        offset8 <<= 3;
    }

    uint8_t third = scale8(offset8, (256 / 3)); // max = 85

    uint8_t r, g, b;

    if (!(hue & 0x80))
    {
        // 0XX
        if (!(hue & 0x40))
        {
            // 00X
            //section 0-1
            if (!(hue & 0x20))
            {
                // 000
                //case 0: // R -> O
                r = K255 - third;
                g = third;
                b = 0;
                FORCE_REFERENCE(b);
            }
            else
            {
                // 001
                //case 1: // O -> Y
                if (Y1)
                {
                    r = K171;
                    g = K85 + third;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
                if (Y2)
                {
                    r = K170 + third;
                    //uint8_t twothirds = (third << 1);
                    uint8_t twothirds = scale8(offset8, ((256 * 2) / 3)); // max=170
                    g = K85 + twothirds;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
            }
        }
        else
        {
            //01X
            // section 2-3
            if (!(hue & 0x20))
            {
                // 010
                //case 2: // Y -> G
                if (Y1)
                {
                    //uint8_t twothirds = (third << 1);
                    uint8_t twothirds = scale8(offset8, ((256 * 2) / 3)); // max=170
                    r = K171 - twothirds;
                    g = K170 + third;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
                if (Y2)
                {
                    r = K255 - offset8;
                    g = K255;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
            }
            else
            {
                // 011
                // case 3: // G -> A
                r = 0;
                FORCE_REFERENCE(r);
                g = K255 - third;
                b = third;
            }
        }
    }
    else
    {
        // section 4-7
        // 1XX
        if (!(hue & 0x40))
        {
            // 10X
            if (!(hue & 0x20))
            {
                // 100
                //case 4: // A -> B
                r = 0;
                FORCE_REFERENCE(r);
                //uint8_t twothirds = (third << 1);
                uint8_t twothirds = scale8(offset8, ((256 * 2) / 3)); // max=170
                g = K171 - twothirds;                                 //K170?
                b = K85 + twothirds;
            }
            else
            {
                // 101
                //case 5: // B -> P
                r = third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K255 - third;
            }
        }
        else
        {
            if (!(hue & 0x20))
            {
                // 110
                //case 6: // P -- K
                r = K85 + third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K171 - third;
            }
            else
            {
                // 111
                //case 7: // K -> R
                r = K170 + third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K85 - third;
            }
        }
    }

    // This is one of the good places to scale the green down,
    // although the client can scale green down as well.
    if (G2)
        g = g >> 1;
    if (Gscale)
        g = scale8(g, Gscale);

    // Scale down colors if we're desaturated at all
    // and add the brightness_floor to r, g, and b.
    if (sat != 255)
    {
        if (sat == 0)
        {
            r = 255;
            b = 255;
            g = 255;
        }
        else
        {
            uint8_t desat = 255 - sat;
            desat = scale8_video(desat, desat);

            uint8_t satscale = 255 - desat;
            //satscale = sat; // uncomment to revert to pre-2021 saturation behavior

            //nscale8x3_video( r, g, b, sat);
#if (FASTLED_SCALE8_FIXED == 1)
            r = scale8_LEAVING_R1_DIRTY(r, satscale);
            g = scale8_LEAVING_R1_DIRTY(g, satscale);
            b = scale8_LEAVING_R1_DIRTY(b, satscale);
            cleanup_R1();
#else
            if (r)
                r = scale8(r, satscale) + 1;
            if (g)
                g = scale8(g, satscale) + 1;
            if (b)
                b = scale8(b, satscale) + 1;
#endif
            uint8_t brightness_floor = desat;
            r += brightness_floor;
            g += brightness_floor;
            b += brightness_floor;
        }
    }

    // Now scale everything down if we're at value < 255.
    if (val != 255)
    {

        val = scale8(val, val);
        if (val == 0)
        {
            r = 0;
            g = 0;
            b = 0;
        }
        else
        {
            if (r)
                r = scale8(r, val) + 1;
            if (g)
                g = scale8(g, val) + 1;
            if (b)
                b = scale8(b, val) + 1;
        }
    }

    // Here we have the old AVR "missing std X+n" problem again
    // It turns out that fixing it winds up costing more than
    // not fixing it.
    // To paraphrase Dr Bronner, profile! profile! profile!
    //asm volatile(  ""  :  :  : "r26", "r27" );
    //asm volatile (" movw r30, r26 \n" : : : "r30", "r31");
    rgb->r = r;
    rgb->g = g;
    rgb->b = b;
}

extern void ledmodule_rmtled_color_hsv_plain(const ledpixel_CHSV_t hsvin, ledpixel_CRGB_t *rgb)
{
    // Convert hue, saturation and brightness ( HSV/HSB ) to RGB
    // "Dimming" is used on saturation and brightness to make
    // the output more visually linear.

    ledpixel_CHSV_t hsv = {
        .h = scale8(hsvin.h, 191),
        .s = hsvin.s,
        .v = hsvin.v};
    // Apply dimming curves
    uint8_t value = APPLY_DIMMING(hsv.val);
    uint8_t saturation = hsv.sat;

    // The brightness floor is minimum number that all of
    // R, G, and B will be set to.
    uint8_t invsat = APPLY_DIMMING(255 - saturation);
    uint8_t brightness_floor = (value * invsat) / 256;

    // The color amplitude is the maximum amount of R, G, and B
    // that will be added on top of the brightness_floor to
    // create the specific hue desired.
    uint8_t color_amplitude = value - brightness_floor;

    // Figure out which section of the hue wheel we're in,
    // and how far offset we are withing that section
    uint8_t section = hsv.hue / HSV_SECTION_3; // 0..2
    uint8_t offset = hsv.hue % HSV_SECTION_3;  // 0..63

    uint8_t rampup = offset;                         // 0..63
    uint8_t rampdown = (HSV_SECTION_3 - 1) - offset; // 63..0

    // We now scale rampup and rampdown to a 0-255 range -- at least
    // in theory, but here's where architecture-specific decsions
    // come in to play:
    // To scale them up to 0-255, we'd want to multiply by 4.
    // But in the very next step, we multiply the ramps by other
    // values and then divide the resulting product by 256.
    // So which is faster?
    //   ((ramp * 4) * othervalue) / 256
    // or
    //   ((ramp    ) * othervalue) /  64
    // It depends on your processor architecture.
    // On 8-bit AVR, the "/ 256" is just a one-cycle register move,
    // but the "/ 64" might be a multicycle shift process. So on AVR
    // it's faster do multiply the ramp values by four, and then
    // divide by 256.
    // On ARM, the "/ 256" and "/ 64" are one cycle each, so it's
    // faster to NOT multiply the ramp values by four, and just to
    // divide the resulting product by 64 (instead of 256).
    // Moral of the story: trust your profiler, not your insticts.

    // Since there's an AVR assembly version elsewhere, we'll
    // assume what we're on an architecture where any number of
    // bit shifts has roughly the same cost, and we'll remove the
    // redundant math at the source level:

    //  // scale up to 255 range
    //  //rampup *= 4; // 0..252
    //  //rampdown *= 4; // 0..252

    // compute color-amplitude-scaled-down versions of rampup and rampdown
    uint8_t rampup_amp_adj = (rampup * color_amplitude) / (256 / 4);
    uint8_t rampdown_amp_adj = (rampdown * color_amplitude) / (256 / 4);

    // add brightness_floor offset to everything
    uint8_t rampup_adj_with_floor = rampup_amp_adj + brightness_floor;
    uint8_t rampdown_adj_with_floor = rampdown_amp_adj + brightness_floor;

    // printf("D1: Section %u, --> %u %u %u\n",section,brightness_floor,rampdown_adj_with_floor,rampup_adj_with_floor);
    if (section)
    {
        if (section == 1)
        {
            // section 1: 0x40..0x7F
            rgb->r = brightness_floor;
            rgb->g = rampdown_adj_with_floor;
            rgb->b = rampup_adj_with_floor;
        }
        else
        {
            // section 2; 0x80..0xBF
            rgb->r = rampup_adj_with_floor;
            rgb->g = brightness_floor;
            rgb->b = rampdown_adj_with_floor;
        }
    }
    else
    {
        // section 0: 0x00..0x3F
        rgb->r = rampdown_adj_with_floor;
        rgb->g = rampup_adj_with_floor;
        rgb->b = brightness_floor;
    }
}

extern void ledmodule_rmtled_color_convert_rgb_to_rgbw(ledpixel_CRGB_t rgb, ledpixel_CRGBW_t *target)
{
    //Get the maximum between R, G, and B
    float tM = fmaxf(rgb.r, fmaxf(rgb.g, rgb.b));
    float Ri = rgb.r;
    float Gi = rgb.g;
    float Bi = rgb.b;

    //If the maximum value is 0, immediately return pure black.
    if (tM == 0)
    {
        target->r = target->g = target->b = target->w = 0;
        return;
    }

    //This section serves to figure out what the color with 100% hue is
    float multiplier = 255.0f / tM;
    float hR = Ri * multiplier;
    float hG = Gi * multiplier;
    float hB = Bi * multiplier;

    //This calculates the Whiteness (not strictly speaking Luminance) of the color
    float M = fmaxf(hR, fmaxf(hG, hB));
    float m = fminf(hR, fminf(hG, hB));
    float Luminance = ((M + m) / 2.0f - 127.5f) * (255.0f / 127.5f) / multiplier;

    //Calculate the output values
    int Wo = (int)round(Luminance);
    int Bo = (int)round(Bi - Luminance);
    int Ro = (int)round(Ri - Luminance);
    int Go = (int)round(Gi - Luminance);

    //Trim them so that they are all between 0 and 255
    if (Wo < 0)
        Wo = 0;
    if (Bo < 0)
        Bo = 0;
    if (Ro < 0)
        Ro = 0;
    if (Go < 0)
        Go = 0;
    if (Wo > 255)
        Wo = 255;
    if (Bo > 255)
        Bo = 255;
    if (Ro > 255)
        Ro = 255;
    if (Go > 255)
        Go = 255;
    target->r = Ro;
    target->g = Go;
    target->b = Bo;
    target->w = Wo;
    return;
}