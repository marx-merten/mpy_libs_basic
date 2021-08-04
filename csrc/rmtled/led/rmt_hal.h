#include <py/obj.h>
#include <py/runtime.h>

#include <driver/rmt.h>
#include <soc/rmt_struct.h>

#ifndef LED_HAL_H
#define LED_HAL_H

//        rmtled_hal_init(self->bpp, self->led_count, self->led_pin);

typedef struct _rmtled_hal_timings_t
{
    uint16_t t0h_ns;
    uint16_t t0l_ns;
    uint16_t t1h_ns;
    uint16_t t1l_ns;

} rmtled_hal_timings_t;

typedef struct _rmtled_transpose_context_t
{
    uint16_t t0h_ticks;
    uint16_t t0l_ticks;
    uint16_t t1h_ticks;
    uint16_t t1l_ticks;
} rmtled_transpose_context_t;


typedef struct _rmtled_hal_t
{
    uint8_t led_pin;
    uint16_t led_count;
    uint8_t bpp;
    rmt_item32_t *hal_buffer;
    rmt_config_t config;
    rmtled_transpose_context_t context;
    rmt_channel_t channel;
} rmtled_hal_t;

extern rmtled_hal_t *rmtled_hal_init(const uint8_t bpp, const uint16_t ledcount,
                                    const uint8_t ledpin, const rmt_channel_t channel,
                                    const rmtled_hal_timings_t timings);
extern void rmtled_hal_deinit(const rmtled_hal_t hal);

extern uint8_t rmtled_hal_send(rmtled_hal_t *hal, uint8_t *led_buffer);

#define RMTLED_STATUS_OK 0
#define RMTLED_STATUS_NOK -1
typedef uint8_t rmtled_result_t;


#define RMTLED_NS_PER_CYCLE 50

// # The peripheral clock is 80MHz or 12.5 nanoseconds per clock.
// # The smallest precision of timing requried for neopixels is
// # 0.35us, but I've decided to go with 0.05 microseconds or
// # 50 nanoseconds. 50 nanoseconds = 12.5 * 4 clocks.
// # By dividing the 80MHz clock by 4 we get a clock every 50 nanoseconds.

// # Neopixel timing in RMT clock counts.
// T_0H = const(35 // 5)  # 0.35 microseconds / 50 nanoseconds 7
// T_1H = const(70 // 5)  # 0.70 microseconds / 50 nanoseconds 14
// T_0L = const(80 // 5)  # 0.80 microseconds / 50 nanoseconds 16
// T_1L = const(60 // 5)  # 0.60 microseconds / 50 nanoseconds 12

#define RMTLED_TIMING_1H 14
#define RMTLED_TIMING_1L 12
#define RMTLED_TIMING_0H 7
#define RMTLED_TIMING_0L 16
#define RMTLED_TIMING_LATCH 110

#define RMT_LEVEL_H 1
#define RMT_LEVEL_L 0

#endif //LED_HAL_H