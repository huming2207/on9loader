#include <stm32wle5xx.h>
#include "fault_handler.h"

volatile fault_crash_state_t * crash_log = (fault_crash_state_t *)FAULT_HANDLER_BACKUP_RAM_ADDR; // Example B

void handle_hard_fault(volatile uint32_t *stack_ptr)
{
    crash_log->r0 = stack_ptr[0];
    crash_log->r1 = stack_ptr[1];
    crash_log->r2 = stack_ptr[2];
    crash_log->r3 = stack_ptr[3];
    crash_log->r12 = stack_ptr[4];
    crash_log->lr = stack_ptr[5];
    crash_log->pc = stack_ptr[6];
    crash_log->xpsr = stack_ptr[7];

    NVIC_SystemReset();
}