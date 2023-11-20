/**
* \file            uart.c
* \brief           UART module source file
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
#include "hal_uart.h"

uhal_status_t uart_init(const uart_inst_t uart_inst, const uint32_t uart_clock_source,
                        const uint32_t uart_clock_source_freq,
                        const unsigned long uart_baudrate, const uart_opt_t uart_extra_configuration_opt) {
    return UHAL_STATUS_OK;
}

uhal_status_t uart_write_blocking(const uart_inst_t uart_inst, const uint8_t* buffer, const size_t size) {
    return UHAL_STATUS_OK;
}

uhal_status_t uart_write_non_blocking(const uart_inst_t uart_inst, const uint8_t* buffer, const size_t size) {
    return UHAL_STATUS_OK;
}

uhal_status_t uart_read_blocking(const uart_inst_t uart_inst, uint8_t* buffer, const size_t size) {
    return UHAL_STATUS_OK;
}

uhal_status_t uart_read_non_blocking(const uart_inst_t uart_inst, uint8_t* buffer, const size_t size) {
    return UHAL_STATUS_OK;
}

uhal_status_t uart_deinit(const uart_inst_t uart_inst) {
    return UHAL_STATUS_OK;
}