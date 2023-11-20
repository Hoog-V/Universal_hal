/**
* \file            hal_uart.h
* \brief           UART module include file
*/
/*
*  Copyright 2023 (C) Victor Hogeweij <hogeweyv@gmail.com>
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

#ifndef HAL_UART_H
#define HAL_UART_H
#include <stdint.h>
#include <string.h>
#include "error_handling.h"
#include "uart/uart_platform_specific.h"

/* Extern c for compiling with c++*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

uhal_status_t uart_init(const uart_inst_t uart_inst, const uint32_t uart_clock_source,
                        const uint32_t uart_clock_source_freq,
                        const unsigned long uart_baudrate, const uart_opt_t uart_extra_configuration_opt);

uhal_status_t uart_write_blocking(const uart_inst_t uart_inst, const uint8_t* buffer, const size_t size);

uhal_status_t uart_write_non_blocking(const uart_inst_t uart_inst, const uint8_t* buffer, const size_t size);

uhal_status_t uart_read_blocking(const uart_inst_t uart_inst, uint8_t* buffer, const size_t size);

uhal_status_t uart_read_non_blocking(const uart_inst_t uart_inst, uint8_t* buffer, const size_t size);

uhal_status_t uart_deinit(const uart_inst_t uart_inst);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif //HAL_UART_H
