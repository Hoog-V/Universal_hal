/**
* \file            uart_platform_specific.h
* \brief           Include file with peripheral specific settings for the UART module
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
#ifndef UART_PLATFORM_SPECIFIC
#define UART_PLATFORM_SPECIFIC

#include "error_handling.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief The IWR68xx MSS has two SCI-based UART instances (MSS_SCIA,
 *        MSS_SCIB), register-identical at different base addresses --
 *        both implemented in uart_iwr68xx.c's get_sci_inst(). MSS_SCIA is
 *        the one wired to the EVM's USB-UART bridge on every board this
 *        HAL has been used with; MSS_SCIB's physical routing (which pins,
 *        whether it reaches an accessible header/USB bridge at all) is
 *        board-specific and hasn't been cross-checked against a specific
 *        EVM schematic -- see GCC_FreeRTOS_VitalSigns_MSS/src/main.c's
 *        pinmux comment for the one place this HAL currently uses it.
 */
typedef enum {
    UART_PERIPHERAL_MSS_SCIA,
    UART_PERIPHERAL_MSS_SCIB
} uart_peripheral_inst_t;

/**
 * @brief The IWR68xx's SCI supports 8 data bits only in this driver (the
 *        hardware also supports 5/6/7, see reg_sci.h's SCICHAR field, but
 *        nothing in this HAL has needed anything but 8 yet). Stop bits and
 *        parity are configurable; OR these together, e.g.
 *        UART_EXTRA_OPT_PARITY_EVEN | UART_EXTRA_OPT_TWO_STOP_BITS.
 */
typedef enum {
    UART_EXTRA_OPT_USE_DEFAULT   = 0,
    UART_EXTRA_OPT_PARITY_EVEN   = 1U << 0,
    UART_EXTRA_OPT_PARITY_ODD    = 1U << 1,
    UART_EXTRA_OPT_TWO_STOP_BITS = 1U << 2,
} uart_extra_config_opt_t;

/**
 * @brief The SCI's baud rate divider is computed directly from the MSS
 *        VCLK frequency (passed in as clock_source_freq to
 *        uhal_uart_init()) -- there's no alternate clock source to select,
 *        so this only has one value, unlike platforms with multiple UART
 *        clock muxes.
 */
typedef enum {
    UART_CLK_SOURCE_USE_DEFAULT = 0x00
} uart_clock_sources_t;

#endif /* UART_PLATFORM_SPECIFIC */
