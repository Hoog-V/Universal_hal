/**
* \file            gpio_platform_specific.h
* \brief           Include file with peripheral specific settings for the GPIO module
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

#ifndef PINS_H
#define PINS_H


#define GPIO_PIN_PA0 0x100
#define GPIO_PIN_PA1 0x101
#define GPIO_PIN_PA2 0x102
#define GPIO_PIN_PA3 0x103
#define GPIO_PIN_PA4 0x104
#define GPIO_PIN_PA5 0x105
#define GPIO_PIN_PA6 0x106
#define GPIO_PIN_PA7 0x107
#define GPIO_PIN_PA8 0x108
#define GPIO_PIN_PA9 0x109
#define GPIO_PIN_PA10 0x10A
#define GPIO_PIN_PA11 0x10B
#define GPIO_PIN_PA12 0x10C
#define GPIO_PIN_PA13 0x10D
#define GPIO_PIN_PA14 0x10E
#define GPIO_PIN_PA15 0x10F
#define GPIO_PIN_PA16 0x110
#define GPIO_PIN_PA17 0x111
#define GPIO_PIN_PA18 0x112
#define GPIO_PIN_PA19 0x113
#define GPIO_PIN_PA20 0x114
#define GPIO_PIN_PA21 0x115
#define GPIO_PIN_PA22 0x116
#define GPIO_PIN_PA23 0x117
#define GPIO_PIN_PA24 0x118
#define GPIO_PIN_PA25 0x119
#define GPIO_PIN_PA26 0x11A
#define GPIO_PIN_PA27 0x11B
#define GPIO_PIN_PA28 0x11C
#define GPIO_PIN_PA29 0x11D
#define GPIO_PIN_PA30 0x11E
#define GPIO_PIN_PA31 0x11F

#if PORT_GROUPS >= 2
#define GPIO_PIN_PB0 0x200
#define GPIO_PIN_PB1 0x201
#define GPIO_PIN_PB2 0x202
#define GPIO_PIN_PB3 0x203
#define GPIO_PIN_PB4 0x204
#define GPIO_PIN_PB5 0x205
#define GPIO_PIN_PB6 0x206
#define GPIO_PIN_PB7 0x207
#define GPIO_PIN_PB8 0x208
#define GPIO_PIN_PB9 0x209
#define GPIO_PIN_PB10 0x20A
#define GPIO_PIN_PB11 0x20B
#define GPIO_PIN_PB12 0x20C
#define GPIO_PIN_PB13 0x20D
#define GPIO_PIN_PB14 0x20E
#define GPIO_PIN_PB15 0x20F
#define GPIO_PIN_PB16 0x210
#define GPIO_PIN_PB17 0x211
#define GPIO_PIN_PB18 0x212
#define GPIO_PIN_PB19 0x213
#define GPIO_PIN_PB20 0x214
#define GPIO_PIN_PB21 0x215
#define GPIO_PIN_PB22 0x216
#define GPIO_PIN_PB23 0x217
#define GPIO_PIN_PB24 0x218
#define GPIO_PIN_PB25 0x219
#define GPIO_PIN_PB26 0x21A
#define GPIO_PIN_PB27 0x21B
#define GPIO_PIN_PB28 0x21C
#define GPIO_PIN_PB29 0x21D
#define GPIO_PIN_PB30 0x21E
#define GPIO_PIN_PB31 0x21F
#endif

#if PORT_GROUPS >= 3
#define GPIO_PIN_PC0 0x300
#define GPIO_PIN_PC1 0x301
#define GPIO_PIN_PC2 0x302
#define GPIO_PIN_PC3 0x303
#define GPIO_PIN_PC4 0x304
#define GPIO_PIN_PC5 0x305
#define GPIO_PIN_PC6 0x306
#define GPIO_PIN_PC7 0x307
#define GPIO_PIN_PC8 0x308
#define GPIO_PIN_PC9 0x309
#define GPIO_PIN_PC10 0x30A
#define GPIO_PIN_PC11 0x30B
#define GPIO_PIN_PC12 0x30C
#define GPIO_PIN_PC13 0x30D
#define GPIO_PIN_PC14 0x30E
#define GPIO_PIN_PC15 0x30F
#define GPIO_PIN_PC16 0x310
#define GPIO_PIN_PC17 0x311
#define GPIO_PIN_PC18 0x312
#define GPIO_PIN_PC19 0x313
#define GPIO_PIN_PC20 0x314
#define GPIO_PIN_PC21 0x315
#define GPIO_PIN_PC22 0x316
#define GPIO_PIN_PC23 0x317
#define GPIO_PIN_PC24 0x318
#define GPIO_PIN_PC25 0x319
#define GPIO_PIN_PC26 0x31A
#define GPIO_PIN_PC27 0x31B
#define GPIO_PIN_PC28 0x31C
#define GPIO_PIN_PC29 0x31D
#define GPIO_PIN_PC30 0x31E
#define GPIO_PIN_PC31 0x31F
# endif

#if PORT_GROUPS >= 4
#define GPIO_PIN_PC0 0x400
#define GPIO_PIN_PC1 0x401
#define GPIO_PIN_PC2 0x402
#define GPIO_PIN_PC3 0x403
#define GPIO_PIN_PC4 0x404
#define GPIO_PIN_PC5 0x405
#define GPIO_PIN_PC6 0x406
#define GPIO_PIN_PC7 0x407
#define GPIO_PIN_PC8 0x408
#define GPIO_PIN_PC9 0x409
#define GPIO_PIN_PC10 040A
#define GPIO_PIN_PC11 040B
#define GPIO_PIN_PC12 040C
#define GPIO_PIN_PC13 040D
#define GPIO_PIN_PC14 040E
#define GPIO_PIN_PC15 0x40F
#define GPIO_PIN_PC16 0x410
#define GPIO_PIN_PC17 0x411
#define GPIO_PIN_PC18 0x412
#define GPIO_PIN_PC19 0x413
#define GPIO_PIN_PC20 0x414
#define GPIO_PIN_PC21 0x415
#define GPIO_PIN_PC22 0x416
#define GPIO_PIN_PC23 0x417
#define GPIO_PIN_PC24 0x418
#define GPIO_PIN_PC25 0x419
#define GPIO_PIN_PC26 0x41A
#define GPIO_PIN_PC27 0x41B
#define GPIO_PIN_PC28 0x41C
#define GPIO_PIN_PC29 0x41D
#define GPIO_PIN_PC30 0x41E
#define GPIO_PIN_PC31 0x41F
# endif

#endif