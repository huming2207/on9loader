#include <stm32wlxx_ll_pwr.h>
#include <stm32wlxx_ll_system.h>
#include <stm32wlxx_ll_rcc.h>
#include <stm32wlxx_ll_bus.h>
#include <stm32wlxx_ll_utils.h>

#include "clock_setup.h"
#include "log.h"

void setup_flash_latency()
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2)
    {
    }
}

void setup_high_speed_clk()
{
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
    LL_RCC_MSI_Enable();

    while(LL_RCC_MSI_IsReady() != 1) {}

    LL_RCC_MSI_EnableRangeSelection();
    LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_8); // 16MHz for now
    LL_RCC_MSI_SetCalibTrimming(0);
}

void setup_low_speed_clk()
{
    LL_PWR_EnableBkUpAccess();
    while (LL_PWR_IsEnabledBkUpAccess() == 0) {}

    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();

    LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_HIGH); // Suggested from RAK FAE, won't waste power at STOP mode
    LL_RCC_LSE_EnablePropagation(); // Enable LSE output to peripherals, like LPUART etc.
    LL_RCC_LSE_Enable();

    /* Wait till LSE is ready */
    while(LL_RCC_LSE_IsReady() != 1)
    {
    }

    LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_MSI); // Ensure HSI is the clock used after wake up
    LL_RCC_LSE_EnablePropagation(); // Enable LSE output to peripherals, like LPUART etc.
}

void setup_system_clk()
{
    LL_RCC_MSI_EnablePLLMode();
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI) {}

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAHB3Prescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

    /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
    LL_SetSystemCoreClock(16000000);
    LL_Init1msTick(16000000);
}

void clock_init()
{
    setup_flash_latency();
    setup_high_speed_clk();
    setup_low_speed_clk();
    setup_system_clk();

#ifndef DISABLE_LOG
    logger_init();
#endif
}
