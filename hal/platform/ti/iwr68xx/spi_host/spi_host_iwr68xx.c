/**
* \file            spi_host_iwr68xx.c
* \brief           Source file which implements the standard SPI host API functions
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
*
* Register sequence ported from the mmWave SDK's MIBSPI_initMaster()/
* MIBSPI_setMasterClockRate() (ti/drivers/spi/src/mibspi_dma.c), keeping
* only the classic (non-multibuffered, non-DMA) polling master-mode path --
* MIBSPIE (multibuffered/RAM mode) is deliberately never set, so each
* SPIDAT1 write/SPIBUF read moves exactly one character, the same way the
* UART driver polls SCIFLR one byte at a time. Bit positions confirmed
* against the SDK's own reg_mibspi.h, the authoritative register map for
* this peripheral.
*
* Chip select is driven as a plain GPIO pin by spi_host_start/end_transaction
* (matching this HAL's atmelsam/raspberrypi convention, see hal_spi_host.h),
* not MibSPI's own hardware CS lines -- SPIDAT1.CSNR is always written as
* "no hardware CS asserted" (see AWR6843_MIBSPI.h's SPIDAT1_CSNR_NONE).
*/
#include <hal_spi_host.h>
#include <AWR6843.h>

static volatile MIBSPI_Type* get_mibspi_inst(const spi_host_inst_t spi_peripheral) {
    switch (spi_peripheral) {
        case SPI_PERIPHERAL_MIBSPIA:
            return MIBSPI_A;
        default:
            return (volatile MIBSPI_Type*)0;
    }
}

uhal_status_t spi_host_init(const spi_host_inst_t spi_peripheral_num, const uint32_t spi_clock_source,
                            const uint32_t spi_clock_source_freq, const unsigned long spi_bus_frequency,
                            const spi_bus_opt_t spi_extra_configuration_opt) {
    volatile MIBSPI_Type* const mibspi = get_mibspi_inst(spi_peripheral_num);
    uint32_t fmt0;
    uint32_t prescale;
    (void)spi_clock_source; /* only one clock source exists for this peripheral -- see spi_platform_specific.h */
    if (mibspi == (volatile MIBSPI_Type*)0) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    if ((spi_clock_source_freq == 0U) || (spi_bus_frequency == 0UL)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    if ((spi_extra_configuration_opt & SPI_BUS_OPT_LSB_FIRST) != 0) {
        return UHAL_STATUS_INVALID_PARAMETERS; /* not implemented -- see spi_platform_specific.h */
    }

    /* Reset the MibSPI module, then bring it out of reset. */
    mibspi->SPIGCR0 = 0U;
    mibspi->SPIGCR0 = 1U;

    /* Master mode, internal clock; leave SPIEN (pin enable) off while configuring. */
    mibspi->SPIGCR1 = SPIGCR1_MASTER | SPIGCR1_CLKMOD;

    /* Pins under MibSPI, not GPIO, control for CLK/SIMO/SOMI; SCS pins are
     * left as GPIO (SCSFUN=0) since chip select is software-driven -- see
     * this file's header. Caller is responsible for muxing the CLK/SIMO/SOMI
     * pads to their MibSPI function first (pinmux_set_pin_function()). */
    mibspi->SPIPC0 = SPIPC0_CLKFUN | SPIPC0_SIMOFUN0 | SPIPC0_SOMIFUN0;

    prescale = (spi_clock_source_freq / (uint32_t)spi_bus_frequency);
    if (prescale == 0U) {
        prescale = 1U;
    }
    fmt0 = (7U << SPIFMT0_CHARLEN_SHIFT) |                    /* 8 data bits */
           ((prescale - 1U) << SPIFMT0_PRESCALE_SHIFT);
    /* spi_bus_opt_t's SPI_BUS_OPT_MODE_0..3 values are CPOL<<1|CPHA, matching
     * the common SPI mode numbering -- bit 0 is CPHA, bit 1 is CPOL. */
    if ((spi_extra_configuration_opt & 0x1U) != 0) {
        fmt0 |= SPIFMT0_PHASE;
    }
    if ((spi_extra_configuration_opt & 0x2U) != 0) {
        fmt0 |= SPIFMT0_POLARITY;
    }
    mibspi->SPIFMT0 = fmt0;

    mibspi->SPIGCR1 |= SPIGCR1_SPIEN;

    return UHAL_STATUS_OK;
}

uhal_status_t spi_host_deinit(const spi_host_inst_t spi_peripheral_num) {
    volatile MIBSPI_Type* const mibspi = get_mibspi_inst(spi_peripheral_num);
    if (mibspi == (volatile MIBSPI_Type*)0) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    mibspi->SPIGCR1 &= ~SPIGCR1_SPIEN;
    mibspi->SPIGCR0 = 0U;
    return UHAL_STATUS_OK;
}

uhal_status_t spi_host_start_transaction(const spi_host_inst_t spi_peripheral_num, const gpio_pin_t chip_select_pin,
                                         const spi_extra_dev_opt_t device_specific_config_opt) {
    (void)device_specific_config_opt;
    if (get_mibspi_inst(spi_peripheral_num) == (volatile MIBSPI_Type*)0) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    return gpio_set_pin_lvl(chip_select_pin, GPIO_LOW);
}

uhal_status_t spi_host_end_transaction(const spi_host_inst_t spi_peripheral_num, const gpio_pin_t chip_select_pin) {
    if (get_mibspi_inst(spi_peripheral_num) == (volatile MIBSPI_Type*)0) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    return gpio_set_pin_lvl(chip_select_pin, GPIO_HIGH);
}

/**
 * @brief Shift one character out (and in) over MibSPI. Even a write-only or
 *        read-only transfer must both write SPIDAT1 and read SPIBUF: this
 *        peripheral is inherently full-duplex, and (in this non-buffered
 *        mode) the next SPIDAT1 write isn't accepted until SPIBUF has been
 *        read for the current one.
 */
static void spi_transfer_byte(volatile MIBSPI_Type* const mibspi, const uint8_t tx_byte, uint8_t* const rx_byte) {
    uint32_t dat1 = ((uint32_t)tx_byte << SPIDAT1_TXDATA_SHIFT) & SPIDAT1_TXDATA_MASK;
    dat1 |= (SPIDAT1_CSNR_NONE << SPIDAT1_CSNR_SHIFT);
    mibspi->SPIDAT1 = dat1;
    while ((mibspi->SPIBUF & SPIBUF_RXEMPTY) != 0U) {
    }
    if (rx_byte != (uint8_t*)0) {
        *rx_byte = (uint8_t)(mibspi->SPIBUF & SPIBUF_RXDATA_MASK);
    } else {
        (void)mibspi->SPIBUF;
    }
}

uhal_status_t spi_host_write_blocking(const spi_host_inst_t spi_peripheral_num, const unsigned char* write_buff, const size_t size) {
    volatile MIBSPI_Type* const mibspi = get_mibspi_inst(spi_peripheral_num);
    size_t i;
    if ((mibspi == (volatile MIBSPI_Type*)0) || (write_buff == (const unsigned char*)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    for (i = 0; i < size; i++) {
        spi_transfer_byte(mibspi, write_buff[i], (uint8_t*)0);
    }
    return UHAL_STATUS_OK;
}

uhal_status_t spi_host_write_non_blocking(const spi_host_inst_t spi_peripheral_num, const unsigned char* write_buff, const size_t size) {
    /* No IRQ/DMA-driven path implemented yet -- see this file's header. */
    return spi_host_write_blocking(spi_peripheral_num, write_buff, size);
}

uhal_status_t spi_host_read_blocking(const spi_host_inst_t spi_peripheral_num, unsigned char* read_buff, size_t amount_of_bytes) {
    volatile MIBSPI_Type* const mibspi = get_mibspi_inst(spi_peripheral_num);
    size_t i;
    if ((mibspi == (volatile MIBSPI_Type*)0) || (read_buff == (unsigned char*)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    for (i = 0; i < amount_of_bytes; i++) {
        spi_transfer_byte(mibspi, 0xFFU, &read_buff[i]); /* 0xFF dummy TX -- generates clock while reading */
    }
    return UHAL_STATUS_OK;
}

uhal_status_t spi_host_read_non_blocking(const spi_host_inst_t spi_peripheral_num, unsigned char* read_buff, size_t amount_of_bytes) {
    /* No IRQ/DMA-driven path implemented yet -- see this file's header. */
    return spi_host_read_blocking(spi_peripheral_num, read_buff, amount_of_bytes);
}
