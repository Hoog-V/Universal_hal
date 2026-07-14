/**
* \file            spi_platform_specific.h
* \brief           Include file with peripheral specific settings for the SPI host module
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
#ifndef SPI_PLATFORM_SPECIFIC
#define SPI_PLATFORM_SPECIFIC

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief The IWR68xx MSS has two MibSPI-based instances (MIBSPIA, MIBSPIB).
 *        Only MIBSPIA is implemented so far, matching uart_platform_specific.h's
 *        MSS_SCIA-only precedent -- MIBSPIB is reserved for when it's needed.
 */
typedef enum {
    SPI_PERIPHERAL_MIBSPIA,
    SPI_PERIPHERAL_MIBSPIB
} spi_host_inst_t;

/**
 * @brief This driver only has one clock source (MSS VCLK, passed in as
 *        spi_clock_source_freq) -- see uart_clock_sources_t's identical
 *        rationale in uart_platform_specific.h.
 */
typedef enum {
    SPI_CLK_SOURCE_USE_DEFAULT = 0x00
} spi_clock_sources_t;

/**
 * @brief SPI mode (clock polarity/phase) and bit order, set at
 *        spi_host_init() time. CPOL/CPHA follow the common SPI mode 0-3
 *        naming; MibSPI's native MSB-first shift direction is the only one
 *        wired up (SPI_BUS_OPT_LSB_FIRST is reserved, returns
 *        UHAL_STATUS_INVALID_PARAMETERS today).
 */
typedef enum {
    SPI_BUS_OPT_MODE_0 = 0,      /* CPOL=0, CPHA=0 (default) */
    SPI_BUS_OPT_MODE_1 = 1,      /* CPOL=0, CPHA=1 */
    SPI_BUS_OPT_MODE_2 = 2,      /* CPOL=1, CPHA=0 */
    SPI_BUS_OPT_MODE_3 = 3,      /* CPOL=1, CPHA=1 */
    SPI_BUS_OPT_LSB_FIRST = 1U << 4
} spi_bus_opt_t;

/**
 * @brief Per-transaction options. This driver drives chip select as a
 *        plain GPIO pin (see spi_host_start_transaction()/
 *        spi_host_end_transaction() in hal_spi_host.h) rather than using
 *        MibSPI's own hardware CS lines -- CSNR is always set to "none
 *        asserted" (see AWR6843_MIBSPI.h's SPIDAT1_CSNR_NONE). There is
 *        currently nothing to configure per-transaction, so this is a
 *        placeholder matching the generic API's shape.
 */
typedef enum {
    SPI_EXTRA_OPT_USE_DEFAULT = 0
} spi_extra_dev_opt_t;

/**
 * @brief Minimal transaction-info struct passed to the (unused, weak)
 *        IRQ hooks declared in hal_spi_host.h -- this driver is polling-
 *        only (see spi_host_iwr68xx.c), so nothing populates or reads
 *        this today; it exists to satisfy hal_spi_host.h's function
 *        signatures, mirroring raspberrypi/irq/irq_typedefs.h's shape.
 */
typedef struct {
    uint8_t transaction_type;
    uint8_t instance_num;
    const uint8_t *write_buffer;
    uint8_t *read_buffer;
    uint32_t buf_size;
    uint32_t buf_cnt;
    uint8_t status;
} bustransaction_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* No compile-time parameter validation on this platform -- matches
 * atmelsam's dma_platform_specific.h precedent for the *_PARAMETER_CHECK
 * macros hal_spi_host.h's wrapper macros expect to exist. */
#define SPI_HOST_INIT_PARAMETER_CHECK(spi_peripheral_num, peripheral_clock_source, peripheral_clock_freq, spi_bus_frequency, spi_extra_configuration_opt) \
    do {                                                                                                                                                  \
    } while (0)

#define SPI_HOST_DEINIT_PARAMETER_CHECK(spi_peripheral_num)                                                                                               \
    do {                                                                                                                                                  \
    } while (0)

#define SPI_HOST_START_TRANSACTION_PARAMETER_CHECK(spi_peripheral_num, chip_select_pin, device_specific_config_opt)                                      \
    do {                                                                                                                                                  \
    } while (0)

#define SPI_HOST_END_TRANSACTION_PARAMETER_CHECK(spi_peripheral_num, chip_select_pin)                                                                     \
    do {                                                                                                                                                  \
    } while (0)

#define SPI_HOST_WRITE_PARAMETER_CHECK(spi_peripheral_num, write_buff, size)                                                                              \
    do {                                                                                                                                                  \
    } while (0)

#define SPI_HOST_READ_PARAMETER_CHECK(spi_peripheral_num, read_buff, amount_of_bytes)                                                                     \
    do {                                                                                                                                                  \
    } while (0)

#endif /* SPI_PLATFORM_SPECIFIC */
