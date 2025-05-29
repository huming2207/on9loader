#include <stm32wlxx.h>
#include <stm32wle5xx.h>

#include "flash_ll.h"

#include "iwdg.h"
#include "stm32wlxx_ll_iwdg.h"

static volatile bool stm32wl_iwdg_enabled = false;

FLASH_RAM_FUNC void stm32wl_iwdg_init()
{
    if (stm32wl_iwdg_enabled) {
        return;
    } else {
        stm32wl_iwdg_enabled = true;
    }

    WRITE_REG(IWDG->KR, LL_IWDG_KEY_ENABLE);
    WRITE_REG(IWDG->KR, LL_IWDG_KEY_WR_ACCESS_ENABLE);
    WRITE_REG(IWDG->PR, (IWDG_PR_PR & LL_IWDG_PRESCALER_256)); // 32768Hz / 256 = 128Hz
    WRITE_REG(IWDG->RLR, (IWDG_RLR_RL & 0xfff)); // 0xfff / 128 => about 32 seconds
    while ((READ_BIT(IWDG->SR, (IWDG_SR_PVU | IWDG_SR_RVU | IWDG_SR_WVU)) == 0)) {} // Wait for ready

    WRITE_REG(IWDG->KR, LL_IWDG_KEY_RELOAD);
}

FLASH_RAM_FUNC void stm32wl_iwdg_feed()
{
    WRITE_REG(IWDG->KR, LL_IWDG_KEY_RELOAD);
}

FLASH_RAM_FUNC void stm32wl_iwdg_enable()
{
    WRITE_REG(IWDG->KR, LL_IWDG_KEY_ENABLE);
}

FLASH_RAM_FUNC bool stm32wl_is_reset_by_iwdg()
{
    if ((READ_BIT(RCC->CSR, RCC_CSR_IWDGRSTF) == (RCC_CSR_IWDGRSTF))) {
        SET_BIT(RCC->CSR, RCC_CSR_RMVF);
        return true;
    }

    return false;
}

FLASH_RAM_FUNC void stm32wl_iwdg_reconfig_reload(uint16_t reload_val)
{
    WRITE_REG(IWDG->KR, LL_IWDG_KEY_ENABLE);
    WRITE_REG(IWDG->KR, LL_IWDG_KEY_WR_ACCESS_ENABLE);
    WRITE_REG(IWDG->RLR, (IWDG_RLR_RL & reload_val)); // 0xfff / 128 => about 32 seconds
    while ((READ_BIT(IWDG->SR, IWDG_SR_RVU)) == 0) {} // Wait for ready
    WRITE_REG(IWDG->KR, LL_IWDG_KEY_RELOAD);
}
