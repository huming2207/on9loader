#include <log.h>
#include <on9ldr_memory.hpp>
#include <cstring>
#include <flash_ll.h>
#include "bl_cfg.hpp"

bool bl_cfg::load_config()
{
    if (__on9_blcfg_size__ < sizeof(bl_cfg::config)) {
        ON9_LOGN("bl_cfg: config section too small: %lu vs %lu", __on9_blcfg_size__, sizeof(bl_cfg::config));
        return false;
    }

    auto *cfg_in_flash = (bl_cfg::config *)(__on9_blcfg_start__);
    if (cfg_in_flash->magic != bl_cfg::MAGIC_NUM) {
        ON9_LOGN("bl_cfg: invalid magic 0x%x", cfg_in_flash->magic);
        return false;
    }

    memcpy(&cfg, cfg_in_flash, sizeof(bl_cfg::config));

    uint32_t expected_crc = cfg.config_crc;
    cfg.config_crc = 0;
    uint32_t crc = bl_cfg::crc32((uint8_t *)&cfg, sizeof(bl_cfg::config));

    if (expected_crc == crc) {
        return true;
    } else {
        ON9_LOGN("bl_cfg: checksum failed: 0x%lx vs 0x%lx", expected_crc, crc);
        return false;
    }
}

bl_cfg::config *bl_cfg::get_config()
{
    return &cfg;
}

bool bl_cfg::set_config(bl_cfg::config *cfg_in)
{
    if (cfg_in == nullptr) {
        return false;
    }

    memcpy(&cfg, cfg_in, sizeof(bl_cfg::config));
    cfg.config_crc = 0;
    cfg.magic = MAGIC_NUM;

    uint32_t crc = bl_cfg::crc32((uint8_t *)&cfg, sizeof(bl_cfg::config));
    cfg.config_crc = crc;

    if (!stm32wl_flash_sector_erase_addr(__on9_blcfg_start__, __on9_blcfg_size__)) {
        ON9_LOGN("bl_cfg: erase old cfg failed!!");
        return false;
    }

    if (!stm32wl_flash_write_mass(__on9_blcfg_start__, (uint8_t *)&cfg, sizeof(bl_cfg::config))) {
        ON9_LOGN("bl_cfg: write cfg failed!!");
        return false;
    }

    return true;
}
