#include "flash_ll.h"

#define STM32WL_FLASH_KEY1              0x45670123UL
#define STM32WL_FLASH_KEY2              0xCDEF89ABUL
#define STM32WL_FLASH_BASE_ADDR         0x08000000UL
#define STM32WL_FLASH_SECTOR_SIZE       2048UL
#define STM32WL_FLASH_WAIT_TIMEOUT_CNT  1600000UL

FLASH_RAM_FUNC void stm32wl_flash_unlock()
{
    if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0) {
        WRITE_REG(FLASH->KEYR, STM32WL_FLASH_KEY1);
        WRITE_REG(FLASH->KEYR, STM32WL_FLASH_KEY2);
    }
}

FLASH_RAM_FUNC void stm32wl_flash_lock()
{
    SET_BIT(FLASH->CR, FLASH_CR_LOCK);
}

FLASH_RAM_FUNC void stm32wl_icache_disable()
{
    CLEAR_BIT(FLASH->ACR, FLASH_ACR_ICEN);
}

FLASH_RAM_FUNC void stm32wl_icache_enable()
{
    SET_BIT(FLASH->ACR, FLASH_ACR_ICEN);
}

FLASH_RAM_FUNC void stm32wl_dcache_disable()
{
    CLEAR_BIT(FLASH->ACR, FLASH_ACR_DCEN);
}

FLASH_RAM_FUNC void stm32wl_dcache_enable()
{
    SET_BIT(FLASH->ACR, FLASH_ACR_DCEN);
}

FLASH_RAM_FUNC void stm32wl_dcache_reset()
{
    SET_BIT(FLASH->ACR, FLASH_ACR_DCRST);
}

FLASH_RAM_FUNC void stm32wl_icache_reset()
{
    SET_BIT(FLASH->ACR, FLASH_ACR_ICRST);
}

FLASH_RAM_FUNC bool stm32wl_flash_wait()
{
    volatile uint32_t retry_ctr = STM32WL_FLASH_WAIT_TIMEOUT_CNT; // Wait for a few seconds (3 to 6??) at most
    while(READ_BIT(FLASH->SR, FLASH_SR_BSY) != 0 && retry_ctr > 0) {
        retry_ctr -= 1;
    }

    if (retry_ctr < 1) {
        return false;
    }

    retry_ctr = STM32WL_FLASH_WAIT_TIMEOUT_CNT;
    while(READ_BIT(FLASH->SR, FLASH_SR_CFGBSY) != 0 && retry_ctr > 0) {
        retry_ctr -= 1;
    }

    if (retry_ctr < 1) {
        return false;
    }

    retry_ctr = STM32WL_FLASH_WAIT_TIMEOUT_CNT;
    while(READ_BIT(FLASH->SR, FLASH_SR_PESD) != 0 && retry_ctr > 0) {
        retry_ctr -= 1;
    }

    if (retry_ctr < 1) {
        return false;
    }

    return true;
}

FLASH_RAM_FUNC bool stm32wl_flash_check_error()
{
    volatile bool has_err = false;

    if(READ_BIT(FLASH->SR, FLASH_SR_PROGERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_PROGERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_WRPERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_WRPERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_PGAERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_PGAERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_SIZERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_SIZERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_PGSERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_PGSERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_MISERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_MISERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_FASTERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_FASTERR);
        has_err = true;
    }

    if (READ_BIT(FLASH->SR, FLASH_SR_RDERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_RDERR);
        has_err = true;
    }

    return has_err;
}

FLASH_RAM_FUNC bool stm32wl_flash_sector_erase(uint8_t idx, uint8_t count)
{
    // See RM0461 Section 3.3.7 and KVED reference code
    // 0. Apply workaround
    stm32wl_flash_force_reset_cfgbsy();

    // 1. Wait first
    if (!stm32wl_flash_wait()) {
        return false;
    }

    // 2. Check if there's any suspend ops
    if (READ_BIT(FLASH->SR, FLASH_SR_PESD) != 0) {
        return false;
    }

    // 3. Unlock, disable I/D-Cache
    stm32wl_flash_unlock();
    stm32wl_icache_disable();
    stm32wl_dcache_disable();

    for (volatile uint8_t cnt = 0; cnt < count; cnt += 1) {
        // 4. Wait and check error
        if (!stm32wl_flash_wait()) {
            return false;
        }

        if (stm32wl_flash_check_error()) {
            return false;
        }

        // 5. Set the page that needs to be erased
        MODIFY_REG(FLASH->CR, FLASH_CR_PNB, (((cnt + idx) & 0xFFU) << FLASH_CR_PNB_Pos));

        // 6. Enable page erase
        SET_BIT(FLASH->CR, FLASH_CR_PER);

        // 7. Nuke the page
        SET_BIT(FLASH->CR, FLASH_CR_STRT);

        // 8. Wait for the nuke
        if (!stm32wl_flash_wait()) {
            return false;
        }

        // 9. Clear page erase request
        // JMH: this seems not necessary
        CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));
        if (stm32wl_flash_check_error()) {
            return false;
        }
    }

    // 10. Put back I/D-cache and flash lock
    stm32wl_icache_enable();
    stm32wl_dcache_enable();
    stm32wl_flash_lock();

    return true;
}

FLASH_RAM_FUNC bool stm32wl_flash_sector_erase_addr(uint32_t addr, uint32_t size)
{
    uint8_t sector_idx = (uint8_t)(((addr - STM32WL_FLASH_BASE_ADDR) / STM32WL_FLASH_SECTOR_SIZE) & 0xff);
    uint8_t sector_cnt = (size / STM32WL_FLASH_SECTOR_SIZE) + ((size % STM32WL_FLASH_SECTOR_SIZE != 0) ? 1 : 0);

    return stm32wl_flash_sector_erase(sector_idx, sector_cnt);
}

FLASH_RAM_FUNC bool stm32wl_flash_write_u64(uint32_t addr, uint64_t data)
{
    // See RM0461 Section 3.3.8 and KVED reference code
    // 0. Disable all cache
    stm32wl_flash_force_reset_cfgbsy();
    stm32wl_icache_disable();
    stm32wl_dcache_disable();

    stm32wl_icache_reset();
    stm32wl_dcache_reset();

    // 1. Wait first
    if (!stm32wl_flash_wait()) {
        return false;
    }

    // 2. Check if there's any suspend ops
    if (READ_BIT(FLASH->SR, FLASH_SR_PESD) != 0) {
        return false;
    }

    // 3. Unlock
    stm32wl_flash_unlock();

    // 4. Wait and check error
    if (!stm32wl_flash_wait()) {
        return false;
    }

    if (stm32wl_flash_check_error()) {
        return false;
    }

    // We also need to set EOPIE bit, otherwise EOP will be always 0
    // This is not clearly documented at the programming procedure on RM0461 ref manual.
    SET_BIT(FLASH->CR, FLASH_CR_EOPIE);

    // 5. Enable programming
    SET_BIT(FLASH->CR, FLASH_CR_PG);

    // 6. Write the stuff
    if (!stm32wl_flash_wait()) {
        return false;
    }

    *(volatile uint32_t*)addr = (uint32_t)data;
    __ISB(); // Instruction Synchronization Barrier - make sure the program is in order
    *(volatile uint32_t*)(addr + 4U) = (uint32_t)(data >> 32ull);

    // 7. Wait
    if (!stm32wl_flash_wait()) {
        return false;
    }

    // 8. Check EOP
    bool success = false;
    if (READ_BIT(FLASH->SR, FLASH_SR_EOP) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_EOP);
        success = true;
    } else {
        SET_BIT(FLASH->SR, FLASH_SR_EOP);
        success = false;
    }

    CLEAR_BIT(FLASH->CR, FLASH_CR_PG);
    CLEAR_BIT(FLASH->CR, FLASH_CR_EOPIE);

    stm32wl_icache_enable();
    stm32wl_dcache_enable();
    stm32wl_flash_lock();

    return success;
}

FLASH_RAM_FUNC uint64_t stm32wl_flash_read_u64(uint32_t addr)
{
    return *((uint64_t *)addr);
}

FLASH_RAM_FUNC bool stm32wl_flash_write_mass(uint32_t addr, uint8_t *buf, uint32_t size)
{
    // 0. Make sure the address is aligned to double word, disable all cache
    stm32wl_flash_force_reset_cfgbsy();
    stm32wl_icache_disable();
    stm32wl_dcache_disable();

    stm32wl_icache_reset();
    stm32wl_dcache_reset();

    if (addr % 8 != 0) {
        return false;
    }

    // See RM0461 Section 3.3.8 and KVED reference code
    // 1. Wait first
    if (!stm32wl_flash_wait()) {
        return false;
    }

    // 2. Check if there's any suspend ops
    if (READ_BIT(FLASH->SR, FLASH_SR_PESD) != 0) {
        return false;
    }

    // 3. Unlock
    stm32wl_flash_unlock();

    // 4. Wait and check error
    if (!stm32wl_flash_wait()) {
        return false;
    }

    if (stm32wl_flash_check_error()) {
        return false;
    }

    // We also need to set EOPIE bit, otherwise EOP will be always 0
    // This is not clearly documented at the programming procedure on RM0461 ref manual.
    SET_BIT(FLASH->CR, FLASH_CR_EOPIE);

    // 5. Enable programming
    SET_BIT(FLASH->CR, FLASH_CR_PG);

    // 6. Write the stuff
    volatile uint32_t size_left = size;
    volatile uint32_t curr_addr = addr;
    volatile uint8_t *buf_ptr = buf;
    while (size_left > 0) {
        if (size_left >= 8) {
            // 6.1. Wait
            if (!stm32wl_flash_wait()) {
                return false;
            }

            // 6.2. Write in double word
            *(volatile uint32_t*)curr_addr = *((uint32_t *)buf_ptr);
            __ISB(); // Instruction Synchronization Barrier - make sure the program is in order
            *(volatile uint32_t*)(curr_addr + 4U) = *((uint32_t *)(buf_ptr + 4));

            curr_addr += 8;
            buf_ptr += 8;
            size_left -= 8;

            // 6.3. Wait
            if (!stm32wl_flash_wait()) {
                return false;
            }
        } else {

            // Code sourced from STM32WL programming algorithm (FlashPrg.c)
            // See RM0461 Section 3.3.8
            volatile uint8_t temp_buf[8] = {};
            for (uint32_t idx = 0; idx < size_left; idx += 1) {
                temp_buf[idx] = *((uint8_t *)buf_ptr);
                buf_ptr += 1;
            }

            for (uint32_t idx = 0; idx < (8 - size_left); idx += 1) {
                temp_buf[idx + size_left] = 0xff; // Fill 0xff for unused bytes
            }

            // 6.1. Wait
            if (!stm32wl_flash_wait()) {
                return false;
            }

            // 6.2. Write in double word
            *(volatile uint32_t*)curr_addr = *((uint32_t *)temp_buf);
            __ISB(); // Instruction Synchronization Barrier - make sure the program is in order
            *(volatile uint32_t*)(curr_addr + 4U) = *((uint32_t *)(temp_buf + 4));

            // 6.3. Wait
            if (!stm32wl_flash_wait()) {
                return false;
            }

            size_left = 0;
        }
    }

    // 7. Wait
    if (!stm32wl_flash_wait()) {
        return false;
    }

    // 8. Check EOP
    bool success = false;
    if (READ_BIT(FLASH->SR, FLASH_SR_EOP) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_EOP);
        success = true;
    } else {
        SET_BIT(FLASH->SR, FLASH_SR_EOP);
        success = false;
    }

    CLEAR_BIT(FLASH->CR, FLASH_CR_PG);
    CLEAR_BIT(FLASH->CR, FLASH_CR_EOPIE);

    stm32wl_icache_enable();
    stm32wl_dcache_enable();
    stm32wl_flash_lock();

    return success;
}

FLASH_RAM_FUNC bool stm32wl_flash_erase_then_write_sector(uint32_t addr, uint8_t *buf, uint32_t size)
{
    // 0. Make sure the address is aligned to sector
    stm32wl_flash_force_reset_cfgbsy();
    if (addr % 2048 != 0) {
        return false;
    }

    // See RM0461 Section 3.3.8 and KVED reference code
    // 1. Wait first
    if (!stm32wl_flash_wait()) {
        return false;
    }

    uint32_t remain_size = size;
    uint32_t curr_addr = addr;
    uint8_t *curr_buf_ptr = buf;
    while (remain_size > 0) {
        uint8_t sector_idx = (uint8_t)(((curr_addr - STM32WL_FLASH_BASE_ADDR) / STM32WL_FLASH_SECTOR_SIZE) & 0xff);

        // 2. We erase a sector first (2048 bytes)
        if (!stm32wl_flash_sector_erase(sector_idx, 1)) {
            return false;
        }

        // 3. ...then write a sector
        uint32_t write_len = (remain_size > STM32WL_FLASH_SECTOR_SIZE ? STM32WL_FLASH_SECTOR_SIZE : remain_size);
        if (!stm32wl_flash_write_mass(curr_addr, curr_buf_ptr, write_len)) {
            return false;
        }

        curr_addr += STM32WL_FLASH_SECTOR_SIZE;
        curr_buf_ptr += STM32WL_FLASH_SECTOR_SIZE;
        remain_size -= write_len;
    }

    return true;
}

FLASH_RAM_FUNC void stm32wl_set_vcore_reg_range_1()
{
    // Check RM0461 Section 5.1.4
    // Set Vcore = 1.2v to do flash write without random lockup
    MODIFY_REG(PWR->CR1, PWR_CR1_VOS, PWR_CR1_VOS_0);

    // Wait VOSF to be cleared
    while ((READ_BIT(PWR->SR2, PWR_SR2_VOSF) != 0)) {}

    // Decrease flash wait state
    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, 0);
    while ((READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY)) != 0) {}
}

FLASH_RAM_FUNC void stm32wl_set_vcore_reg_range_2()
{
    // Increase flash wait state
    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_1);
    while ((READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY)) != FLASH_ACR_LATENCY_1) {}

    // Set to Range 2, no need to poll VOSF according to ST
    MODIFY_REG(PWR->CR1, PWR_CR1_VOS, PWR_CR1_VOS_1);
}

FLASH_RAM_FUNC void stm32wl_flash_force_reset_cfgbsy()
{
    // See https://blog.csdn.net/fangjiaze444/article/details/125790180
    // In case someone can't read Chinese:
    //    - This is a workaround for CFGBSY lockup issue. Somehow before writing, STM32WL's flash may
    //      locks itself up with CFGBSY always 1. If we ignore CFGBSY and write stuff to flash, it may
    //      end up in hard fault.
    //    - To fix this, we firstly try clearing error, then write some random crap to the flash BEFORE
    //      unlock, then clear the error again.
    // Update on 2 May 2023: As per discussed, we try a few more times for this hack, cuz 1 time may not work...
    // Update on 15 May 2023: Add in some ISB instructions to really make sure there's no weird optimisations

    volatile uint8_t retry_cnt = 3;
    while (retry_cnt > 0) {
        if ((READ_BIT(FLASH->SR, FLASH_SR_CFGBSY)) != 0) {
            stm32wl_flash_check_error();
            __ISB();
            *(volatile uint32_t *)(0x08020000) = 12323; // Write garbage
            __ISB();
            stm32wl_flash_check_error();
            __ISB();
        } else {
            return;
        }

        retry_cnt -= 1;
    }
}
