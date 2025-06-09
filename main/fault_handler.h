#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define FAULT_HANDLER_BACKUP_RAM_ADDR 0x2000f800UL

typedef struct __attribute__((packed)) crash_states
{
    uint32_t magic; // 0x096c6472
    uint32_t crash_ctr;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
    uint32_t scb_cfsr;
    uint32_t scb_hfsr;
    uint32_t scb_mmfar;
    uint32_t scb_bfar;
    uint32_t rcc_csr;
} fault_crash_state_t;


void handle_hard_fault(volatile uint32_t *stack_ptr);
bool fault_handler_check_crash();


#ifdef __cplusplus
}
#endif