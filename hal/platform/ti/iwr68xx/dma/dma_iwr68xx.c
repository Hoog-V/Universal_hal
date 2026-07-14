/**
* \file            dma_iwr68xx.c
* \brief           Source file which implements the standard DMA API functions
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
* Register sequence ported from the mmWave SDK's DMA_open()/
* DMA_setChannelParams()/DMA_enableChannel() (ti/drivers/dma/src/dma.c),
* keeping only the software-triggered, polling-completion path -- no HW
* request-line triggering, no per-completion-type interrupts (this
* peripheral's IRQ registration needs SYS/BIOS's Hwi_create(), the same
* reason esm_iwr68xx.c is optional -- see UHAL_DISABLE_ESM_MODULE in
* CMakeLists.txt). Bit positions and the GCTRL reset/enable sequence
* confirmed against the SDK's own reg_dma.h/reg_dmaram.h, the authoritative
* register map for this peripheral.
*/
#include <hal_dma.h>
#include <AWR6843.h>

static volatile DMA_Type* get_dma_ctrl_inst(const dma_peripheral_t dma_peripheral) {
    switch (dma_peripheral) {
        case DMA_PERIPHERAL_1:
            return DMA_1;
        case DMA_PERIPHERAL_2:
            return DMA_2;
        default:
            return (volatile DMA_Type*)0;
    }
}

static volatile DMARAM_Type* get_dma_ram_inst(const dma_peripheral_t dma_peripheral) {
    switch (dma_peripheral) {
        case DMA_PERIPHERAL_1:
            return DMA_1_RAM;
        case DMA_PERIPHERAL_2:
            return DMA_2_RAM;
        default:
            return (volatile DMARAM_Type*)0;
    }
}

uhal_status_t dma_init(dma_peripheral_t dma_peripheral, dma_init_opt_t dma_init_options) {
    volatile DMA_Type* const ctrl = get_dma_ctrl_inst(dma_peripheral);
    uint32_t i;
    (void)dma_init_options; /* nothing to configure yet -- see dma_platform_specific.h */
    if (ctrl == (volatile DMA_Type*)0) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }

    /* Reset the DMA controller, then bring it out of reset. */
    ctrl->GCTRL = DMA_GCTRL_DMARES;
    ctrl->GCTRL = DMA_GCTRL_DMAEN;

    /* Assign every channel to the default port -- see AWR6843_DMA.h. */
    for (i = 0; i < 4U; i++) {
        ctrl->PAR[i] = DMA_PAR_DEFAULT_ALL_CHANNELS;
    }
    return UHAL_STATUS_OK;
}

/**
 * @brief Fill in one channel's primary control packet. Common to
 *        dma_set_transfer_mem()/_peripheral_to_mem()/_mem_to_peripheral() --
 *        they differ only in address-increment mode for each side.
 */
static void dma_configure_channel(volatile DMARAM_Type* const ram, const dma_channel_t channel, const uint32_t src, const uint32_t dst,
                                   const size_t elem_count, const uint32_t elem_size, const uint32_t src_addr_mode, const uint32_t dest_addr_mode) {
    volatile DMA_ChannelPacket_Type* const pkt = &ram->PRIMARYCONTROLPACKET[channel];
    pkt->ISADDR = src;
    pkt->IDADDR = dst;
    pkt->ITCOUNT = (1U << 16) | ((uint32_t)elem_count & 0xFFFFU); /* 1 frame, elem_count elements/frame */
    pkt->CHCTRL = (elem_size << DMA_CHCTRL_SRC_ELEM_SIZE_SHIFT) | (elem_size << DMA_CHCTRL_DEST_ELEM_SIZE_SHIFT) |
                  (DMA_XFER_TYPE_FRAME << DMA_CHCTRL_XFER_TYPE_SHIFT) | (src_addr_mode << DMA_CHCTRL_SRC_ADDR_MODE_SHIFT) |
                  (dest_addr_mode << DMA_CHCTRL_DEST_ADDR_MODE_SHIFT);
    pkt->EIOFF = 0U;
    pkt->FIOFF = 0U;
}

uhal_status_t dma_set_transfer_mem(const dma_peripheral_t peripheral, const dma_channel_t dma_channel, const void* src, void* dst, const size_t size,
                                   const dma_opt_t dma_options, const uint8_t do_software_trigger) {
    volatile DMA_Type* const ctrl = get_dma_ctrl_inst(peripheral);
    volatile DMARAM_Type* const ram = get_dma_ram_inst(peripheral);
    if ((ctrl == (volatile DMA_Type*)0) || (ram == (volatile DMARAM_Type*)0) || (src == (const void*)0) || (dst == (void*)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    dma_configure_channel(ram, dma_channel, (uint32_t)(uintptr_t)src, (uint32_t)(uintptr_t)dst, size, (uint32_t)dma_options, DMA_ADDR_MODE_POST_INCREMENT,
                           DMA_ADDR_MODE_POST_INCREMENT);
    if (do_software_trigger != 0U) {
        ctrl->SWCHENAS = (1UL << (uint32_t)dma_channel);
    }
    return UHAL_STATUS_OK;
}

uhal_status_t dma_set_transfer_peripheral_to_mem(const dma_peripheral_t dma_peripheral, const dma_channel_t dma_channel,
                                                 const dma_peripheral_location_t src, void* dst, const size_t size, const dma_opt_t dma_options) {
    volatile DMARAM_Type* const ram = get_dma_ram_inst(dma_peripheral);
    if ((ram == (volatile DMARAM_Type*)0) || (dst == (void*)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    dma_configure_channel(ram, dma_channel, (uint32_t)src, (uint32_t)(uintptr_t)dst, size, (uint32_t)dma_options, DMA_ADDR_MODE_CONSTANT,
                           DMA_ADDR_MODE_POST_INCREMENT);
    /* Not triggered here -- call dma_set_trigger(peripheral, channel,
     * DMA_TRIGGER_SOFTWARE) once ready to start (see dma_platform_specific.h). */
    return UHAL_STATUS_OK;
}

uhal_status_t dma_set_transfer_mem_to_peripheral(const dma_peripheral_t dma_peripheral, const dma_channel_t dma_channel, const void* src,
                                                 const dma_peripheral_location_t dst, const size_t size, const dma_opt_t dma_options) {
    volatile DMARAM_Type* const ram = get_dma_ram_inst(dma_peripheral);
    if ((ram == (volatile DMARAM_Type*)0) || (src == (const void*)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    dma_configure_channel(ram, dma_channel, (uint32_t)(uintptr_t)src, (uint32_t)dst, size, (uint32_t)dma_options, DMA_ADDR_MODE_POST_INCREMENT,
                           DMA_ADDR_MODE_CONSTANT);
    /* Not triggered here -- call dma_set_trigger(peripheral, channel,
     * DMA_TRIGGER_SOFTWARE) once ready to start (see dma_platform_specific.h). */
    return UHAL_STATUS_OK;
}

uhal_status_t dma_set_trigger(const dma_peripheral_t dma_peripheral, const dma_channel_t dma_channel, const dma_trigger_t trigger) {
    volatile DMA_Type* const ctrl = get_dma_ctrl_inst(dma_peripheral);
    if (ctrl == (volatile DMA_Type*)0) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    switch (trigger) {
        case DMA_TRIGGER_SOFTWARE:
            ctrl->SWCHENAS = (1UL << (uint32_t)dma_channel);
            return UHAL_STATUS_OK;
        default:
            return UHAL_STATUS_INVALID_PARAMETERS;
    }
}

uhal_status_t dma_reset_trigger(const dma_peripheral_t dma_peripheral, const dma_channel_t dma_channel, const dma_trigger_t trigger) {
    volatile DMA_Type* const ctrl = get_dma_ctrl_inst(dma_peripheral);
    if (ctrl == (volatile DMA_Type*)0) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    switch (trigger) {
        case DMA_TRIGGER_SOFTWARE:
            ctrl->SWCHENAR = (1UL << (uint32_t)dma_channel);
            return UHAL_STATUS_OK;
        default:
            return UHAL_STATUS_INVALID_PARAMETERS;
    }
}

uhal_status_t dma_deinit(const dma_peripheral_t dma_peripheral) {
    volatile DMA_Type* const ctrl = get_dma_ctrl_inst(dma_peripheral);
    if (ctrl == (volatile DMA_Type*)0) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    ctrl->GCTRL = DMA_GCTRL_DMARES;
    return UHAL_STATUS_OK;
}
