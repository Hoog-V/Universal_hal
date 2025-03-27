/**
* \file            spi.c
* \brief           Source file which implements the standard SPI API functions
*/
/*
*  Copyright 2025 (C) Victor Hogeweij <hogeweyv@gmail.com>
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
*/
#include <hal_spi_host.h>
#include <stddef.h>
#include "bit_manipulation.h"


uhal_status_t spi_host_init(const spi_host_inst_t spi_peripheral_num, const uint32_t spi_clock_source, const uint32_t spi_clock_source_freq,
                            const unsigned long spi_bus_frequency, const spi_bus_opt_t spi_extra_configuration_opt) {

    return HAL_SPI_Init(spi_peripheral_num);
}

uhal_status_t spi_host_deinit(const spi_host_inst_t spi_peripheral_num) {
    return HAL_SPI_DeInit(spi_peripheral_num);
}

uhal_status_t spi_host_start_transaction(const spi_host_inst_t spi_peripheral_num, const gpio_pin_t chip_select_pin,
                                         const spi_extra_dev_opt_t device_specific_config_opt) {
    // if (chip_select_pin != -1) {
    //     return (HAL_GPIO_WritePin(chip_select_pin, GPIO_LOW));
    // } else {
        return UHAL_STATUS_OK;
    // }
}

uhal_status_t spi_host_end_transaction(const spi_host_inst_t spi_peripheral_num, const gpio_pin_t chip_select_pin) {
    // if (chip_select_pin != -1) {
    //     return (gpio_set_pin_lvl(chip_select_pin, GPIO_HIGH));
    // } else {
        return UHAL_STATUS_OK;
    // }
}

uhal_status_t spi_host_write_blocking(const spi_host_inst_t spi_peripheral_num, const unsigned char* write_buff, const size_t size) {
    return HAL_SPI_Transmit(spi_peripheral_num, write_buff, size, 1000);
}

uhal_status_t spi_host_write_non_blocking(const spi_host_inst_t spi_peripheral_num, const unsigned char* write_buff, const size_t size) {
    return HAL_SPI_Transmit(spi_peripheral_num, write_buff, size, 1000);
}

uhal_status_t spi_host_read_blocking(const spi_host_inst_t spi_peripheral_num, unsigned char* read_buff, size_t amount_of_bytes) {
    return HAL_SPI_Receive(spi_peripheral_num, read_buff, amount_of_bytes, 1000);
}

uhal_status_t spi_host_read_non_blocking(const spi_host_inst_t spi_peripheral_num, unsigned char* read_buff, size_t amount_of_bytes) {
    return HAL_SPI_Receive(spi_peripheral_num, read_buff, amount_of_bytes, 1000);
}