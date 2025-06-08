#pragma once

#include <cstdint>

class bl_cfg
{
public:
    static const constexpr uint32_t MAGIC_NUM = 0x096c6472; // "09" and "LDR"

    struct __attribute__((packed)) fw_slots
    {
        uint32_t slot_crc32;
        uint32_t slot_addr;
        uint32_t version_major;
        uint32_t version_minor;
    };

    struct __attribute__((packed)) lora_sos_cfg
    {
        uint8_t sf;
        uint8_t bw;
        uint8_t cr;
        uint8_t ldro_en;
        uint32_t freq_hz;
    };

    struct __attribute__((packed)) config
    {
        uint32_t magic; // 0x096c6472
        uint32_t version; // Version set to 0x100 for now
        uint32_t active_slot; // The slot address that is active
        uint32_t known_good_slot; // Last-known good slot that successfully boot up
        lora_sos_cfg lora_sos; // LoRa emergency SOS mode (TBD)
        fw_slots slots[2]; // We only uses two slots for now
        uint32_t config_crc;
    };

public:
    static bl_cfg &instance()
    {
        static bl_cfg _instance;
        return _instance;
    }

    void operator=(bl_cfg const &) =delete;
    bl_cfg(bl_cfg const &) = delete;

private:
    bl_cfg() = default;

public:
    bool load_config();
    bl_cfg::config *get_config();
    bool set_config(bl_cfg::config *cfg_in);
    static uint32_t crc32(const uint8_t *data, size_t len)
    {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < len; i++) {
            crc ^= (uint32_t)data[i] << 24;
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint32_t mask = (uint32_t)(-(int32_t)(crc >> 31)) & 0x04C11DB7;
                crc = (crc << 1) ^ mask;
            }
        }
        crc ^= 0xFFFFFFFF;
        return crc;
    }

private:
    bl_cfg::config cfg = {};
};

