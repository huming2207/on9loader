#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void stm32wl_iwdg_init();
void stm32wl_iwdg_feed();
void stm32wl_iwdg_enable();
bool stm32wl_is_reset_by_iwdg();
void stm32wl_iwdg_reconfig_reload(uint16_t reload_val);

#ifdef __cplusplus
}
#endif