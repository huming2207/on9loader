#include <stm32wlxx_it.h>
#include <algorithm>
#include <cstring>
#include "subghz.hpp"

#include "subghz/sx126x.h"
#include "subghz/stm32wl_subghz_hal.h"
#include "log.h"

extern "C" void SUBGHZ_Radio_IRQHandler()
{
    sx126x_irq_mask_t irq_status = 0;
    if (sx126x_get_and_clear_irq_status(&irq_status) == SX126X_STATUS_OK) {
        subghz::last_irq_status = irq_status;
    }
}

volatile sx126x_irq_mask_t subghz::last_irq_status = 0;

bool subghz::init()
{
    if (!stm32wl_subghz_init()) {
        ON9_LOGN("SUBGHZ init fail");
        return false;
    }

    if (sx126x_set_sleep(SX126X_SLEEP_CFG_COLD_START) != SX126X_STATUS_OK) {
        ON9_LOGN("SUBGHZ sleep fail");
        return false;
    }

    ON9_LOGN("SUBGHZ init OK");
    return true;
}

bool subghz::setup_lora(uint32_t freq_hz, sx126x_lora_sf_t sf, sx126x_lora_bw_t bw, bool low_data_rate_opt, sx126x_lora_cr_t cr, uint8_t sync_word, uint16_t img_cal_start_mhz, uint16_t img_cal_end_mhz)
{
    // Step 1. Go standby
    if (pwr_mode != lora::STDBY_RC) {
        if (sx126x_set_standby(SX126X_STANDBY_CFG_RC) != SX126X_STATUS_OK) {
            ON9_LOGN("SUBGHZ set standby fail");
            return false;
        } else {
            pwr_mode = lora::STDBY_RC;
        }
    }

    // Step 2. Packet type = LoRa
    auto ret = sx126x_set_pkt_type(SX126X_PKT_TYPE_LORA);

    // Step 3. Recalibration
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_cal(SX126X_CAL_ALL));

    // Step 4. Set RF frequency & do image calibration
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_rf_freq(freq_hz));
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_cal_img_in_mhz(img_cal_start_mhz, img_cal_end_mhz));

    // Step 5: Buffer address (override to 0 for now?)
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_buffer_base_address(0, 0));

    // Step 6: Set modulation parameters
    sx126x_mod_params_lora_t mod_params = {};
    mod_params.bw = bw;
    mod_params.cr = cr;
    mod_params.sf = sf;
    mod_params.ldro = low_data_rate_opt ? 1 : 0;

    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_lora_mod_params(&mod_params));

    // Step 7: Set sync word
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_lora_sync_word(sync_word));
    return ret == SX126X_STATUS_OK;
}

void subghz::handle_task()
{
    volatile sx126x_irq_mask_t irq_status = last_irq_status;
    last_irq_status = 0;

    if ((irq_status & SX126X_IRQ_RX_DONE) != 0) {
        if (irq_handler) {
            irq_handler->on_subghz_rx_done();
        }

        pwr_mode = lora::STDBY_RC;

        if (((irq_status & SX126X_IRQ_CRC_ERROR) != 0) || ((irq_status & SX126X_IRQ_HEADER_ERROR) != 0)) {
            if ((irq_status & SX126X_IRQ_CRC_ERROR) != 0 && irq_handler != nullptr) {
                irq_handler->on_subghz_crc_error();
            }

            if ((irq_status & SX126X_IRQ_HEADER_ERROR) != 0 && irq_handler != nullptr) {
                irq_handler->on_subghz_header_error();
            }
        } else if (irq_handler != nullptr) {
            irq_handler->on_subghz_rx_done();
        }
    }

    if ((irq_status & SX126X_IRQ_TX_DONE) != 0) {
        if (irq_handler) {
            irq_handler->on_subghz_tx_done();
        }

        pwr_mode = lora::STDBY_RC;
    }

    if ((irq_status & SX126X_IRQ_TIMEOUT) != 0) {
        if (irq_handler) {
            irq_handler->on_subghz_timeout();
        }

        pwr_mode = lora::STDBY_RC;
    }
}

bool subghz::set_lora_tx(uint8_t *buf, uint8_t len, int8_t tx_power, uint32_t timeout_ms, uint16_t preamble_cnt, bool header_en, bool crc_on, bool invert_iq)
{
    if (buf == nullptr || len < 1) {
        return false;
    }

    auto ret = SX126X_STATUS_OK;
    auto tx_pwr_level = (int8_t)(tx_power > 14 ? tx_power : 14);
    for (uint32_t idx = 0; idx < (sizeof(pa_cfg_lut) / sizeof(lora::pa_cfg_lut_item)); idx += 1) {
        if (pa_cfg_lut[idx].tx_pwr == tx_pwr_level) {
            ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_pa_cfg(&pa_cfg_lut[idx].pa_cfg));
        }
    }

    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_tx_params(tx_power, SX126X_RAMP_3400_US)); // RampTime = 0x7 - need review

    sx126x_pkt_params_lora_t pkt_params = {};
    pkt_params.crc_is_on = crc_on;
    pkt_params.invert_iq_is_on = invert_iq;
    pkt_params.header_type = header_en ? SX126X_LORA_PKT_EXPLICIT : SX126X_LORA_PKT_IMPLICIT;
    pkt_params.preamble_len_in_symb = preamble_cnt;
    pkt_params.pld_len_in_bytes = len;
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_lora_pkt_params(&pkt_params));

    uint16_t dio_masks = SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT;
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_dio_irq_params(dio_masks, dio_masks, 0, 0));

    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_write_buffer(0, buf, len));

    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_tx(timeout_ms));
    pwr_mode = lora::TX;
    return ret == SX126X_STATUS_OK;
}

bool subghz::set_lora_rx(uint8_t len, uint32_t timeout_ms, uint16_t preamble_cnt, bool rx_boost, bool header_en, bool crc_on, bool invert_iq)
{
    sx126x_pkt_params_lora_t pkt_params = {};
    pkt_params.crc_is_on = crc_on;
    pkt_params.invert_iq_is_on = invert_iq;
    pkt_params.header_type = header_en ? SX126X_LORA_PKT_EXPLICIT : SX126X_LORA_PKT_IMPLICIT;
    pkt_params.preamble_len_in_symb = preamble_cnt;
    pkt_params.pld_len_in_bytes = len;
    auto ret = sx126x_set_lora_pkt_params(&pkt_params);

    uint16_t dio_masks = SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT;
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_dio_irq_params(dio_masks, dio_masks, 0, 0));

    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_cfg_rx_boosted(rx_boost));

    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_rx(timeout_ms));
    pwr_mode = lora::RX;
    return ret == SX126X_STATUS_OK;
}

bool subghz::read_rx_buf(uint8_t *buf, uint8_t len, uint8_t *actual_len)
{
    if (buf == nullptr || len < 1) {
        return false;
    }

    sx126x_rx_buffer_status_t buf_status = {};
    auto ret = sx126x_get_rx_buffer_status(&buf_status);
    if (ret != SX126X_STATUS_OK) {
        return false;
    }

    if (actual_len != nullptr) {
        *actual_len = buf_status.pld_len_in_bytes;
    }

    uint8_t read_len = std::min(len, buf_status.pld_len_in_bytes);
    ret = sx126x_read_buffer(buf_status.buffer_start_pointer, buf, read_len);

    return ret == SX126X_STATUS_OK;
}

void subghz::set_irq_handler(subghz_irq_notifiable *handler)
{
    irq_handler = handler;
}

bool subghz::get_lora_pkt_status(sx126x_pkt_status_lora_t *status)
{
    if (status == nullptr) {
        return false;
    }

    sx126x_get_lora_pkt_status(status);

    return true;
}
