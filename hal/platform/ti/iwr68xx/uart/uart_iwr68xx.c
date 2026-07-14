/**
* \file            uart_iwr68xx.c
* \brief           Source file which implements the standard UART API functions
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
* Register sequence ported from the mmWave SDK's UartSci_open()/
* UartSci_writePolling() (ti/drivers/uart/src/uartsci.c), keeping only what
* a single fixed-instance polling UART needs -- no DMA, no interrupts, no
* OS semaphores. Bit positions confirmed against the SDK's own reg_sci.h,
* the authoritative register map for this peripheral.
*/
#include <hal_uart.h>
#include <AWR6843.h>
#include <AWR6843_SCI.h>

/**
 * @brief SCIA and SCIB are register-identical peripherals at different base
 *        addresses (AWR6843.h's SCI_A/SCI_B), both on the default
 *        always-enabled peripheral clock -- no RCM clock-gating step is
 *        needed for either (confirmed against the mmWave SDK's own
 *        UartSci driver, which has none), unlike e.g. MSS_RTIB's separate
 *        watchdog clock domain (see AWR6843.h's RTIA/RTIB comment).
 * @return The SCI instance's register block, or NULL for an unknown instance.
 */
static volatile SCI_Type* get_sci_inst(const uart_peripheral_inst_t uart_peripheral) {
    switch (uart_peripheral) {
        case UART_PERIPHERAL_MSS_SCIA:
            return SCI_A;
        case UART_PERIPHERAL_MSS_SCIB:
            return SCI_B;
        default:
            return (volatile SCI_Type*)0;
    }
}

uhal_status_t uhal_uart_init(const uart_peripheral_inst_t uart_peripheral, const uint32_t baudrate, const uart_clock_sources_t clock_source,
                              const uint32_t clock_source_freq, const uart_extra_config_opt_t uart_extra_opt) {
    volatile SCI_Type* const sci = get_sci_inst(uart_peripheral);
    uint32_t gcr1;
    (void)clock_source; /* only one clock source exists for this peripheral -- see uart_platform_specific.h */
    if (sci == (volatile SCI_Type*)0) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    if ((baudrate == 0U) || (clock_source_freq == 0U)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }

    /* Reset the SCI module, then bring it out of reset. */
    sci->SCIGCR0 = 0U;
    sci->SCIGCR0 = 1U;

    /* Sleep state while configuring. */
    sci->SCIGCR1 = 0U;
    sci->SCICLEARINT    = 0xFFFFFFFFU;
    sci->SCICLEARINTLVL = 0xFFFFFFFFU;

    /* Rx/Tx enabled, internal clock, asynchronous timing mode. */
    gcr1 = SCIGCR1_TXENA | SCIGCR1_RXENA | SCIGCR1_CLOCK | SCIGCR1_TIMING_MODE;
    if ((uart_extra_opt & UART_EXTRA_OPT_TWO_STOP_BITS) != 0U) {
        gcr1 |= SCIGCR1_STOP_2BIT;
    }
    if ((uart_extra_opt & UART_EXTRA_OPT_PARITY_EVEN) != 0U) {
        gcr1 |= SCIGCR1_PARITY_ENA | SCIGCR1_PARITY_EVEN;
    } else if ((uart_extra_opt & UART_EXTRA_OPT_PARITY_ODD) != 0U) {
        gcr1 |= SCIGCR1_PARITY_ENA;
    }
    sci->SCIGCR1 = gcr1;

    sci->SCIBAUD = clock_source_freq / (16U * (baudrate + 1U));
    sci->SCICHAR = 7U; /* 8 data bits -- see uart_platform_specific.h */

    /* Pins under SCI, not GPIO, control; driven by the SCI module, not sw;
     * push-pull, no open drain; pull up when idle. Caller is responsible
     * for muxing the TX/RX pads to their SCI function first (pinmux_set_pin_function()). */
    sci->SCIPIO0 = SCIPIO_RX_BIT | SCIPIO_TX_BIT;
    sci->SCIPIO1 = 0U;
    sci->SCIPIO3 = 0U;
    sci->SCIPIO6 = 0U;
    sci->SCIPIO7 = 0U;
    sci->SCIPIO8 = SCIPIO_RX_BIT | SCIPIO_TX_BIT;

    sci->SCIGCR1 |= SCIGCR1_SW_NRESET; /* start the SCI */

    return UHAL_STATUS_OK;
}

uhal_status_t uhal_uart_deinit(const uart_peripheral_inst_t uart_peripheral) {
    volatile SCI_Type* const sci = get_sci_inst(uart_peripheral);
    if (sci == (volatile SCI_Type*)0) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    sci->SCIGCR1 = 0U;
    sci->SCIGCR0 = 0U;
    return UHAL_STATUS_OK;
}

uhal_status_t uhal_uart_transmit(const uart_peripheral_inst_t uart_peripheral, const uint8_t* transmit_buffer, const size_t size) {
    volatile SCI_Type* const sci = get_sci_inst(uart_peripheral);
    size_t i;
    if ((sci == (volatile SCI_Type*)0) || (transmit_buffer == (const uint8_t*)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    for (i = 0; i < size; i++) {
        while ((sci->SCIFLR & SCIFLR_TXRDY_BIT) == 0U) {
        }
        sci->SCITD = transmit_buffer[i];
    }
    return UHAL_STATUS_OK;
}

uhal_status_t uhal_uart_receive(const uart_peripheral_inst_t uart_peripheral, uint8_t* receive_buffer, const size_t size) {
    volatile SCI_Type* const sci = get_sci_inst(uart_peripheral);
    size_t i;
    if ((sci == (volatile SCI_Type*)0) || (receive_buffer == (uint8_t*)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    for (i = 0; i < size; i++) {
        while ((sci->SCIFLR & SCIFLR_RXRDY_BIT) == 0U) {
        }
        receive_buffer[i] = (uint8_t)sci->SCIRD;
    }
    return UHAL_STATUS_OK;
}
