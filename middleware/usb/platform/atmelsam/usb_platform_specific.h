/**
* \file            usb_platform_specific.h
* \brief           USB module platform defines include file
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
#ifndef ATMELSAMD21_USB_PLATFORM_SPECIFIC_H
#define ATMELSAMD21_USB_PLATFORM_SPECIFIC_H
#include <sam.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    USB_CLK_SOURCE_USE_DEFAULT = 0x00,
    USB_CLK_SOURCE_FAST_CLKGEN0 = 0x01,
    USB_CLK_SOURCE_FAST_CLKGEN1 = 0x02,
    USB_CLK_SOURCE_FAST_CLKGEN2 = 0x03,
    USB_CLK_SOURCE_FAST_CLKGEN3 = 0x04,
    USB_CLK_SOURCE_FAST_CLKGEN4 = 0x05,
    USB_CLK_SOURCE_FAST_CLKGEN5 = 0x06,
    USB_CLK_SOURCE_FAST_CLKGEN6 = 0x07,
    USB_CLK_SOURCE_FAST_CLKGEN7 = 0x08
} usb_clock_sources_t;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //ATMELSAMD21_USB_PLATFORM_SPECIFIC_H
