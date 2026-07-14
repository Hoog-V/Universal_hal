/**
* \file            dma_platform_specific.h
* \brief           DMA module platform defines include file
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
* This platform's DMA driver (dma_iwr68xx.c) is deliberately scoped to
* software-triggered transfers, polling completion -- the same bare-metal,
* no-VIM-interrupts-enabled convention the rest of this project's iwr68xx
* HAL follows (see uart_iwr68xx.c/soc_iwr68xx.c). hal_dma.h has no
* blocking-wait or status-query function, so callers who need to know when
* a software-triggered transfer finished should poll the relevant
* completion flag directly, e.g. (DMA_1->BTCFLAG >> DMA_CHANNEL_0) & 1
* (AWR6843_DMA.h) -- write the same bit back to clear it once observed.
*/
#ifndef IWR68XX_DMA_PLATFORM_SPECIFIC_H
#define IWR68XX_DMA_PLATFORM_SPECIFIC_H

#include "error_handling.h"
#include <AWR6843.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief The IWR68xx MSS has two independent DMA controller instances
 *        (each with its own 32 channels, register block, and packet RAM).
 */
typedef enum {
    DMA_PERIPHERAL_1,
    DMA_PERIPHERAL_2
} dma_peripheral_t;

/**
 * @brief 32 channels per DMA instance -- see
 *        SOC_XWR68XX_NUM_DMA_CHANNELS_PER_INSTANCE (AWR6843_DMA.h).
 */
typedef enum {
    DMA_CHANNEL_0,  DMA_CHANNEL_1,  DMA_CHANNEL_2,  DMA_CHANNEL_3,
    DMA_CHANNEL_4,  DMA_CHANNEL_5,  DMA_CHANNEL_6,  DMA_CHANNEL_7,
    DMA_CHANNEL_8,  DMA_CHANNEL_9,  DMA_CHANNEL_10, DMA_CHANNEL_11,
    DMA_CHANNEL_12, DMA_CHANNEL_13, DMA_CHANNEL_14, DMA_CHANNEL_15,
    DMA_CHANNEL_16, DMA_CHANNEL_17, DMA_CHANNEL_18, DMA_CHANNEL_19,
    DMA_CHANNEL_20, DMA_CHANNEL_21, DMA_CHANNEL_22, DMA_CHANNEL_23,
    DMA_CHANNEL_24, DMA_CHANNEL_25, DMA_CHANNEL_26, DMA_CHANNEL_27,
    DMA_CHANNEL_28, DMA_CHANNEL_29, DMA_CHANNEL_30, DMA_CHANNEL_31
} dma_channel_t;

/**
 * @brief Element size for both source and destination -- the one transfer
 *        parameter this minimal driver exposes (matches DMA_ElemSize_e's
 *        numeric values in AWR6843_DMA.h directly, see dma_iwr68xx.c).
 *        Indexed/chained transfers and per-completion-type interrupts
 *        aren't implemented yet -- there's nothing else to OR in here today.
 */
typedef enum {
    DMA_OPT_USE_DEFAULT = 0,     /* 8-bit elements */
    DMA_OPT_ELEM_SIZE_8_BIT = 0,
    DMA_OPT_ELEM_SIZE_16_BIT = 1,
    DMA_OPT_ELEM_SIZE_32_BIT = 2,
    DMA_OPT_ELEM_SIZE_64_BIT = 3
} dma_opt_t;

/**
 * @brief Only software-triggered channels are implemented -- see this
 *        file's header. dma_set_trigger(peripheral, channel,
 *        DMA_TRIGGER_SOFTWARE) fires a channel already configured by
 *        dma_set_transfer_mem()/_peripheral_to_mem()/_mem_to_peripheral();
 *        dma_reset_trigger() disables the channel.
 */
typedef enum {
    DMA_TRIGGER_SOFTWARE
} dma_trigger_t;

typedef enum {
    DMA_INIT_OPT_USE_DEFAULT = 0
} dma_init_opt_t;

/**
 * @brief Fixed (non-incrementing) peripheral register addresses usable as
 *        a DMA transfer endpoint. Values are plain MMIO addresses (see
 *        AWR6843.h/AWR6843_SCI.h/AWR6843_MIBSPI.h for the base addresses
 *        and offsets these are built from), not an index into a lookup
 *        table -- dma_iwr68xx.c uses them directly as ISADDR/IDADDR.
 *        Extend this list as more peripherals gain DMA-driven use.
 *
 * @note A plain uint32_t typedef + #defines, not an enum: these values
 *       (e.g. SCI_A's base 0xFFF7E500) exceed INT32_MAX, and C doesn't
 *       guarantee an enum's underlying type is wide/unsigned enough to
 *       hold them without a sign-changing conversion (confirmed as a real
 *       warning under TI armcl, not just a lint nit).
 */
typedef uint32_t dma_peripheral_location_t;
#define DMA_PERIPHERAL_LOCATION_SCIA_TX    (SOC_XWR68XX_MSS_SCI_A_BASE_ADDRESS + 0x038U)    /* SCITD */
#define DMA_PERIPHERAL_LOCATION_SCIA_RX    (SOC_XWR68XX_MSS_SCI_A_BASE_ADDRESS + 0x034U)    /* SCIRD */
#define DMA_PERIPHERAL_LOCATION_MIBSPIA_TX (SOC_XWR68XX_MSS_MIBSPIA_BASE_ADDRESS + 0x03CU)  /* SPIDAT1 */
#define DMA_PERIPHERAL_LOCATION_MIBSPIA_RX (SOC_XWR68XX_MSS_MIBSPIA_BASE_ADDRESS + 0x040U)  /* SPIBUF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* No compile-time parameter validation on this platform -- matches
 * atmelsam's dma_platform_specific.h precedent. */
#define DMA_INIT_FUNC_PARAMETER_CHECK(dma_peripheral, dma_init_options)                                                                                     \
    do {                                                                                                                                                    \
    } while (0)

#define DMA_SET_TRANSFER_MEM_FUNC_PARAMETER_CHECK(dma_peripheral, dma_channel, src, dst, size, dma_options, do_software_trigger)                            \
    do {                                                                                                                                                    \
    } while (0)

#define DMA_SET_TRANSFER_PERIPHERAL_TO_MEM_FUNC_PARAMETER_CHECK(dma_peripheral, dma_channel, src, dst, size, dma_options, do_software_trigger)              \
    do {                                                                                                                                                    \
    } while (0)

#define DMA_SET_TRANSFER_MEM_TO_PERIPHERAL_FUNC_PARAMETER_CHECK(dma_peripheral, dma_channel, src, dst, size, dma_options, do_software_trigger)              \
    do {                                                                                                                                                    \
    } while (0)

#define DMA_TRIGGER_FUNC_PARAMETER_CHECK(dma_peripheral, dma_channel, trigger)                                                                               \
    do {                                                                                                                                                    \
    } while (0)

#define DMA_DEINIT_FUNC_PARAMETER_CHECK(dma_peripheral)                                                                                                      \
    do {                                                                                                                                                    \
    } while (0)

#endif /* IWR68XX_DMA_PLATFORM_SPECIFIC_H */
