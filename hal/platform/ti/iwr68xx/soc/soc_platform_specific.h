/**
* \file            soc_platform_specific.h
* \brief           Include file with peripheral specific settings for the SOC module
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
#ifndef SOC_PLATFORM_SPECIFIC
#define SOC_PLATFORM_SPECIFIC

#include "error_handling.h"

/**
 * @brief soc_init() on this platform takes no configuration -- it brings up
 *        the BSS/APLL clock and (on secure parts only) disables the
 *        JTAG/logger debug firewalls, neither of which is optional or
 *        configurable per this HAL's current usage. Nothing to declare here
 *        yet; kept as its own header to match the other peripheral modules'
 *        include pattern (<soc/soc_platform_specific.h>).
 */

#endif /* SOC_PLATFORM_SPECIFIC */
