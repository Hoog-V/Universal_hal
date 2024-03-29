/**
* \file            irq_typedefs.h
* \brief           Header file which implements the struct which is passed to and from ISR's
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
*/

#ifndef IRQ_TYPEDEFS_H
#define IRQ_TYPEDEFS_H
#include <stdint.h>

typedef struct {
    uint8_t transaction_type;
    uint8_t instance_num;
    const uint8_t *write_buffer;
    uint8_t *read_buffer;
    uint8_t buf_size;
    uint8_t buf_cnt;
    uint8_t status;
} bustransaction_t;

#endif /* IRQ_TYPEDEFS_H */