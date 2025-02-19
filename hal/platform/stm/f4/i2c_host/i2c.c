/**
* \file            i2c.c
* \brief           Source file which implements the standard I2C API functions
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
*/

#ifndef DISABLE_I2C_HOST_MODULE

#include <hal_i2c_host.h>
#include <stdbool.h>
#include "error_handling.h"

uhal_status_t i2c_host_init(const i2c_periph_inst_t i2c_peripheral_num,
                            const i2c_clock_sources_t clock_sources,
                            const uint32_t periph_clk_freq,
                            const uint32_t baud_rate_freq,
                            const i2c_extra_opt_t extra_configuration_options) {
    
    return HAL_I2C_Init(i2c_peripheral_num);
}

uhal_status_t i2c_host_deinit(const i2c_periph_inst_t i2c_peripheral_num) {
    return HAL_I2C_DeInit(i2c_peripheral_num);
}

uhal_status_t i2c_host_write_non_blocking(const i2c_periph_inst_t i2c_peripheral_num,
                                          const uint16_t addr,
                                          const uint8_t *write_buff,
                                          const size_t size,
                                          const i2c_stop_bit_t stop_bit) {
    return HAL_I2C_Master_Transmit(i2c_peripheral_num, addr << 1, write_buff, size, 1000);
}

uhal_status_t i2c_host_write_blocking(const i2c_periph_inst_t i2c_peripheral_num, const uint16_t addr,
                                      const uint8_t *write_buff, const size_t size,
                                      const i2c_stop_bit_t stop_bit) {
    uhal_status_t status = i2c_host_write_non_blocking(i2c_peripheral_num, addr, write_buff, size, stop_bit);
    return status;
}

uhal_status_t i2c_host_read_blocking(const i2c_periph_inst_t i2c_peripheral_num,
                                     const uint16_t addr, uint8_t *read_buff,
                                     const size_t amount_of_bytes) {

    return HAL_I2C_Master_Receive(i2c_peripheral_num, addr << 1, read_buff, amount_of_bytes, 1000);
}

uhal_status_t i2c_host_read_non_blocking(const i2c_periph_inst_t i2c_peripheral_num,
                                         const uint16_t addr, uint8_t *read_buff,
                                         const size_t amount_of_bytes) {


    return  i2c_host_read_blocking(i2c_peripheral_num, addr, read_buff, amount_of_bytes);
}

#endif /* DISABLE_I2C_HOST_MODULE */