/**
* \file            soc_iwr68xx.c
* \brief           Source file which implements the standard SOC API functions
*/
/*
*  Copyright 2024 (C) Victor Hogeweij <hogeweyv@gmail.com>
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
* This file is part of the Universal Hal Framework.
*
* Author:          Victor Hogeweij <hogeweyv@gmail.com>
*/
#include <soc/soc_platform_specific.h>
#include <hal_soc.h>
#include <AWR6843.h>
#include <AWR6843_TOPRCM.h>

/* Bounded poll count for the APLL calibration wait -- large enough to never
 * trip on real hardware (calibration is sub-millisecond) while still turning
 * a stuck/absent BSS into a reported error instead of a silent hang. */
#define SOC_APLL_CAL_TIMEOUT_ITERATIONS 0xFFFFFU

/* CP15 c6 (MPU) access helpers -- see soc_mpu_iwr68xx.asm. */
extern void uhal_mpu_enable(void);
extern void uhal_mpu_disable(void);
extern void uhal_mpu_enable_background_region(void);
extern void uhal_mpu_disable_background_region(void);
extern void uhal_mpu_set_region(uint32_t region);
extern void uhal_mpu_set_region_base_address(uint32_t address);
extern void uhal_mpu_set_region_type_and_permission(uint32_t type, uint32_t permission);
extern void uhal_mpu_set_region_size_register(uint32_t value);

/* Region type (DRACR[4:0]) */
#define MPU_TYPE_STRONGLYORDERED_SHAREABLE 0x0000U
#define MPU_TYPE_NORMAL_OINC_NONSHARED     0x0008U
#define MPU_TYPE_NORMAL_OINC_SHARED        0x000CU

/* Region access permission (DRACR[15:8]) */
#define MPU_PERM_NA_USER_NA_NOEXEC   0x1000U
#define MPU_PERM_RW_USER_RW_EXEC     0x0300U
#define MPU_PERM_RW_USER_RW_NOEXEC   0x1300U
#define MPU_PERM_RW_USER_RO_NOEXEC   0x1200U

/* Region size (DRSR[5:1]); OR with MPU_REGION_ENABLE (DRSR[0]) */
#define MPU_REGION_ENABLE 1U
#define MPU_SIZE_128_BYTES (0x06U << 1U)
#define MPU_SIZE_8_KB       (0x0CU << 1U)
#define MPU_SIZE_32_KB      (0x0EU << 1U)
#define MPU_SIZE_512_KB     (0x12U << 1U)
#define MPU_SIZE_1_MB       (0x13U << 1U)
#define MPU_SIZE_2_MB       (0x14U << 1U)
#define MPU_SIZE_8_MB       (0x16U << 1U)
#define MPU_SIZE_256_MB     (0x1BU << 1U)
#define MPU_SIZE_4_GB       (0x1FU << 1U)

#define MPU_ALIGN(addr, size) ((addr) & ~((size) - 1U))

/* MSS memory map, ported from the mmWave SDK's sys_common_xwr68xx_mss.h --
 * only the base addresses SOC_mpu_config() actually references. */
#define MSS_TCMA_BASE_ADDRESS           0x00000000U
#define MSS_TCMB_BASE_ADDRESS           0x08000000U
#define MSS_SW_BUFFER_BASE_ADDRESS      0x0C200000U
#define MSS_EDMA_TPTC0_BASE_ADDRESS     0x50000000U
#define MSS_L3RAM_BASE_ADDRESS          0x51000000U
#define MSS_ADCBUF_BASE_ADDRESS         0x52000000U
#define MSS_HSRAM_BASE_ADDRESS          0x52080000U
#define MSS_EXT_FLASH_BASE_ADDRESS      0xC0000000U
#define MSS_QSPI_BASE_ADDRESS           0xC0800000U
#define MSS_MBOX_BSS_MSS_MEM_BASE_ADDRESS 0xF0601000U

static void mpu_configure_region(uint32_t region, uint32_t base_address,
                                  uint32_t type, uint32_t permission, uint32_t size) {
    uhal_mpu_set_region(region);
    uhal_mpu_set_region_base_address(base_address);
    uhal_mpu_set_region_type_and_permission(type, permission);
    uhal_mpu_set_region_size_register(MPU_REGION_ENABLE | size);
}

/* Ported from the mmWave SDK's SOC_mpu_config() (soc_xwr68xx_mss.c) -- same
 * 12 regions, same base addresses/types/permissions/sizes, same order (later
 * regions win on overlap, e.g. region 7 carves the mailbox memory back out
 * of region 6's peripheral span). Assumes a CCS-downloaded image (TCMA
 * region kept executable+writable), matching this project's DOWNLOAD_FROM_CCS
 * build definition -- see mss_program.xer4f's target_compile_definitions. */
static void mpu_config(void) {
    uhal_mpu_disable();
    uhal_mpu_disable_background_region();

    /* Region 1: background, whole 4GB, no access. */
    mpu_configure_region(0U, 0x0U, MPU_TYPE_STRONGLYORDERED_SHAREABLE,
                          MPU_PERM_NA_USER_NA_NOEXEC, MPU_SIZE_4_GB);

    /* Region 2: TCMA IRAM, 512KB. */
    mpu_configure_region(1U, MSS_TCMA_BASE_ADDRESS, MPU_TYPE_NORMAL_OINC_NONSHARED,
                          MPU_PERM_RW_USER_RW_EXEC, MPU_SIZE_512_KB);

    /* Region 3: TCMB DRAM, 512KB. */
    mpu_configure_region(2U, MSS_TCMB_BASE_ADDRESS, MPU_TYPE_NORMAL_OINC_NONSHARED,
                          MPU_PERM_RW_USER_RW_NOEXEC, MPU_SIZE_512_KB);

    /* Region 4: Ext Flash (QSPI-mapped), 8MB. */
    mpu_configure_region(3U, MSS_EXT_FLASH_BASE_ADDRESS, MPU_TYPE_NORMAL_OINC_NONSHARED,
                          MPU_PERM_RW_USER_RW_NOEXEC, MPU_SIZE_8_MB);

    /* Region 5: QSPI controller registers, 128B. */
    mpu_configure_region(4U, MSS_QSPI_BASE_ADDRESS, MPU_TYPE_STRONGLYORDERED_SHAREABLE,
                          MPU_PERM_RW_USER_RO_NOEXEC, MPU_SIZE_128_BYTES);

    /* Region 6: Mailbox regs + MCRC + peripheral space, 256MB. */
    mpu_configure_region(5U, 0xF0000000U, MPU_TYPE_STRONGLYORDERED_SHAREABLE,
                          MPU_PERM_RW_USER_RO_NOEXEC, MPU_SIZE_256_MB);

    /* Region 7: Mailbox memory carve-out, 32KB (overrides part of region 6). */
    mpu_configure_region(6U, MPU_ALIGN(MSS_MBOX_BSS_MSS_MEM_BASE_ADDRESS, 32U * 1024U),
                          MPU_TYPE_NORMAL_OINC_SHARED, MPU_PERM_RW_USER_RO_NOEXEC, MPU_SIZE_32_KB);

    /* Region 8: HSRAM, 32KB. */
    mpu_configure_region(7U, MPU_ALIGN(MSS_HSRAM_BASE_ADDRESS, 32U * 1024U),
                          MPU_TYPE_NORMAL_OINC_NONSHARED, MPU_PERM_RW_USER_RW_NOEXEC, MPU_SIZE_32_KB);

    /* Region 9: DSS peripheral region, 2MB. */
    mpu_configure_region(8U, MPU_ALIGN(MSS_EDMA_TPTC0_BASE_ADDRESS, 2U * 1024U * 1024U),
                          MPU_TYPE_STRONGLYORDERED_SHAREABLE, MPU_PERM_RW_USER_RO_NOEXEC, MPU_SIZE_2_MB);

    /* Region 10: L3 memory, 1MB. */
    mpu_configure_region(9U, MPU_ALIGN(MSS_L3RAM_BASE_ADDRESS, 1U * 1024U * 1024U),
                          MPU_TYPE_NORMAL_OINC_NONSHARED, MPU_PERM_RW_USER_RW_NOEXEC, MPU_SIZE_1_MB);

    /* Region 11: ADC buffer + Chirp + FFTC + DSS peripheral region, 512KB. */
    mpu_configure_region(10U, MSS_ADCBUF_BASE_ADDRESS, MPU_TYPE_STRONGLYORDERED_SHAREABLE,
                          MPU_PERM_RW_USER_RW_NOEXEC, MPU_SIZE_512_KB);

    /* Region 12: software buffer, 8KB. */
    mpu_configure_region(11U, MSS_SW_BUFFER_BASE_ADDRESS, MPU_TYPE_NORMAL_OINC_NONSHARED,
                          MPU_PERM_RW_USER_RW_NOEXEC, MPU_SIZE_8_KB);

    uhal_mpu_enable_background_region();
    uhal_mpu_enable();
}

uhal_status_t soc_init(void) {
    volatile uint32_t timeout;
    uint32_t partNumber;

    mpu_config();

    /* Ungate + unhalt + de-reset the BSS (radar front-end) clock domain in
     * one step, and clear SPARE0's upper halfword so the APLL calibration
     * status bits below start from a known state. */
    TOP_RCM->BSSCTL = 0U;
    TOP_RCM->SPARE0 &= 0x0000FFFFU;

    timeout = SOC_APLL_CAL_TIMEOUT_ITERATIONS;
    while ((TOP_RCM->SPARE0 & TOPRCM_SPARE0_APLL_CAL_MASK) != TOPRCM_SPARE0_APLL_CAL_DONE) {
        if (timeout == 0U) {
            return UHAL_STATUS_PERIPHERAL_CLOCK_ERROR;
        }
        timeout--;
    }

    /* On secure parts, the JTAG and logger debug firewalls are enabled by
     * default and must be explicitly disabled (OR the 3-bit field to 0x7)
     * before those peripherals are usable; GP parts don't have them. */
    partNumber = (TOP_RCM->EFUSEREGROW10 & TOPRCM_EFUSEREGROW10_PART_NUMBER_MASK)
                 >> TOPRCM_EFUSEREGROW10_PART_NUMBER_SHIFT;
    if (partNumber == TOPRCM_SECURE_PART_NUMBER) {
        TOP_RCM->SECURECFGREG1 |= (TOPRCM_SECURECFGREG1_JTAGFIREWALLEN_MASK
                                    | TOPRCM_SECURECFGREG1_LOGGERFIREWALLEN_MASK);
    }

    return UHAL_STATUS_OK;
}
