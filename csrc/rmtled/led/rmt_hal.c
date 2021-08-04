#include "./rmt_hal.h"
#include "py/mphal.h"

static rmtled_transpose_context_t rmtled_static_context;

/**
 * @brief Conver RGB data to RMT format.
 *
 * @note For WS2812, R,G,B each contains 256 different choices (i.e. uint8_t)
 *
 * @param[in] src: source data, to converted to RMT format
 * @param[in] dest: place where to store the convert result
 * @param[in] src_size: size of source data
 * @param[in] wanted_num: number of RMT items that want to get
 * @param[out] translated_size: number of source data that got converted
 * @param[out] item_num: number of RMT items which are converted from source data
 */
static void IRAM_ATTR rmtled_hal_transform_cb(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_num, size_t *translated_size, size_t *item_num)
{
    if (src == NULL || dest == NULL) {
        *translated_size = 0;
        *item_num = 0;
        return;
    }
    rmtled_transpose_context_t ctx = rmtled_static_context;

    const rmt_item32_t bit0 = {{{ ctx.t0h_ticks, 1, ctx.t0l_ticks, 0 }}}; //Logical 0
    const rmt_item32_t bit1 = {{{ ctx.t1h_ticks, 1, ctx.t1l_ticks, 0 }}}; //Logical 1
    size_t size = 0;
    size_t num = 0;
    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;
    while (size < src_size && num < wanted_num) {
        for (int i = 0; i < 8; i++) {
            // MSB first
            if (*psrc & (1 << (7 - i))) {
                pdest->val =  bit1.val;
            } else {
                pdest->val =  bit0.val;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}



extern rmtled_hal_t *rmtled_hal_init(const uint8_t bpp, const uint16_t ledcount,
                                    const uint8_t ledpin, const rmt_channel_t channel,
                                    const rmtled_hal_timings_t timings)
{

    rmtled_hal_t *hal = malloc(sizeof(rmtled_hal_t));

    hal->channel=channel;
    hal->bpp = bpp;
    hal->led_count = ledcount;
    hal->led_pin = ledpin;

    hal->config.rmt_mode = RMT_MODE_TX;
    hal->config.channel = channel;
    hal->config.gpio_num = hal->led_pin;
    hal->config.mem_block_num = 1;
    hal->config.tx_config.loop_en = 0;

    hal->config.tx_config.carrier_en = false;
    hal->config.tx_config.idle_output_en = 1;
    hal->config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    hal->config.tx_config.carrier_duty_percent = 50;
    hal->config.tx_config.carrier_freq_hz = 0;
    hal->config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;

    hal->config.clk_div = 4;

    check_esp_err(rmt_config(&hal->config));
    check_esp_err(rmt_driver_install(hal->config.channel, 0, 0));

    // example calc for ws2812b
    // # Neopixel timing in RMT clock counts.
    // T_0H = const(35 // 5)  # 0.35 microseconds / 50 nanoseconds 7
    // T_1H = const(70 // 5)  # 0.70 microseconds / 50 nanoseconds 14
    // T_0L = const(80 // 5)  # 0.80 microseconds / 50 nanoseconds 16
    // T_1L = const(60 // 5)  # 0.60 microseconds / 50 nanoseconds 12

    hal->context.t0h_ticks = (uint16_t)timings.t0h_ns/RMTLED_NS_PER_CYCLE;
    hal->context.t0l_ticks = (uint16_t)timings.t0l_ns/RMTLED_NS_PER_CYCLE;
    hal->context.t1h_ticks = (uint16_t)timings.t1h_ns/RMTLED_NS_PER_CYCLE;
    hal->context.t1l_ticks = (uint16_t)timings.t1l_ns/RMTLED_NS_PER_CYCLE;

    // TODO Fix this IF support for multiple channels with mixing timings is required !!
    rmtled_static_context = hal->context;



    rmt_translator_init(channel,rmtled_hal_transform_cb);

    return hal;


}


extern void rmtled_hal_deinit(const rmtled_hal_t hal)
{
    check_esp_err(rmt_driver_uninstall(hal.channel));
}

extern uint8_t rmtled_hal_send(rmtled_hal_t *hal, uint8_t *led_buffer)
{
    // let's make sure the previous run is done
    rmt_wait_tx_done(hal->config.channel,200);
    rmt_write_sample(hal->config.channel,led_buffer , hal->led_count * hal->bpp,false);
    return RMTLED_STATUS_OK;
}
