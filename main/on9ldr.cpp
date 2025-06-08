//
// Created by Jackson Hu on 7/6/2025.
//

#include <stm32wle5xx.h>
#include "on9ldr.hpp"


__attribute__((noinline, noreturn))
static void jump_to_application(uint32_t app_base) {
    __disable_irq();

    __DSB(); // Data Sync Barrier (ensure writes complete)
    __ISB(); // Instruction Sync Barrier (flush pipeline)

    volatile uint32_t *stack_ptr = (uint32_t *)app_base;
    volatile uint32_t app_sp = stack_ptr[0];
    volatile uint32_t app_entry_addr = stack_ptr[1];

    SCB->VTOR = app_base;
    __set_CONTROL(0);                     // Use MSP only (in case PSP is used, e.g. with RTOS)
    __set_MSP(app_sp);             // Set stack pointer

    __DSB(); // Data Sync Barrier (ensure writes complete)
    __ISB(); // Instruction Sync Barrier (flush pipeline)

    void (* volatile app_entry)(void) = (void (*)(void))app_entry_addr;
    app_entry();

    __builtin_unreachable();       // Compiler hint: this function does not return
}

