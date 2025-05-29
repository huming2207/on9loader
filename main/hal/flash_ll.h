#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stm32wlxx_ll_system.h>

#pragma GCC push_options
#pragma GCC optimize ("O0")

#define FLASH_RAM_FUNC __attribute__((long_call, section (".code_in_ram")))
#define STM32WL_FLASH_SECTOR_SIZE    2048UL
#define STM32WL_FLASH_WORD_SIZE      8UL

void stm32wl_flash_unlock();
void stm32wl_flash_lock();
void stm32wl_icache_disable();
void stm32wl_icache_enable();
void stm32wl_dcache_disable();
void stm32wl_dcache_enable();
void stm32wl_dcache_reset();
void stm32wl_icache_reset();
bool stm32wl_flash_wait();
void stm32wl_set_vcore_reg_range_1();
void stm32wl_set_vcore_reg_range_2();
void stm32wl_flash_force_reset_cfgbsy();
bool stm32wl_flash_check_error();
bool stm32wl_flash_sector_erase(uint8_t idx, uint8_t count);
bool stm32wl_flash_sector_erase_addr(uint32_t addr, uint32_t size);
bool stm32wl_flash_write_u64(uint32_t addr, uint64_t data);
bool stm32wl_flash_write_mass(uint32_t addr, uint8_t *buf, uint32_t size);
bool stm32wl_flash_erase_then_write_sector(uint32_t addr, uint8_t *buf, uint32_t size);
uint64_t stm32wl_flash_read_u64(uint32_t addr);

#pragma GCC pop_options

#ifdef __cplusplus
}
#endif