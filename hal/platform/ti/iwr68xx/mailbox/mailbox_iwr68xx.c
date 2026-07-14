/**
* \file            mailbox_iwr68xx.c
* \brief           Source file which implements the standard Mailbox API functions
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
* Protocol ported from the mmWave SDK's Mailbox_write()/Mailbox_read()/
* Mailbox_readFlush() and its box-full/box-empty ISR processing functions
* (ti/drivers/mailbox/src/mailbox.c) -- this driver replicates what those
* ISRs do by polling the same status bits directly instead of registering
* interrupt handlers (this peripheral's IRQ registration needs SYS/BIOS's
* Hwi_create(), the same reason esm_iwr68xx.c is optional).
*
* The MSS<->DSS link (only) uses the SDK's "multi-channel" framing: a
* 4-byte channel-ID header prepended to every message. This isn't optional
* -- it's required for wire-compatibility with the actual DSS binary this
* project flashes (example/Barebones_MSS/prebuilt/placeholder_dss.bin is
* byte-identical to a vital-signs-tracking demo's built DSS image, whose
* MSS and DSS sides both call Mailbox_open() with chType=MAILBOX_CHTYPE_MULTI,
* chId=MAILBOX_CH_ID_0 -- see mss_main.c/dss_main.c in that demo's firmware
* tree). Skipping the header would desync every message by 4 bytes against
* that DSS image. MSS<->BSS never uses this framing (matches the SDK: the
* multi-channel path is explicitly skipped whenever remoteEndpoint==BSS).
*
* Each of the four hardware register blocks (MSS_BSS, BSS_MSS, MSS_DSS,
* DSS_MSS -- see AWR6843.h) multiplexes two single-bit doorbell lines: bit
* 0 ("mailbox") signals a new message is ready; bit 1 ("mailbox ack")
* signals the previous message on that same block was consumed. For a
* given link (e.g. MSS<->BSS), MSS triggers bit 0 on the MSS_BSS block to
* send, and observes bit 1 on the MSS_BSS block for BSS's acknowledgement;
* MSS observes bit 0 on the BSS_MSS block to know BSS sent something, and
* triggers bit 1 on the BSS_MSS block to acknowledge having read it.
*/
#include <hal_mailbox.h>
#include <AWR6843.h>
#include <string.h>

/* Matches the mmWave SDK's mailbox_internal.h/mailbox.h -- see this file's
 * header for why the DSS link needs this. */
#define MAILBOX_MULTI_CH_HEADER_SIZE 4U
#define MAILBOX_CH_ID_0               0U

typedef struct {
    volatile MAILBOX_Type* out_reg; /* register block this core triggers to send */
    volatile MAILBOX_Type* in_reg;  /* register block the remote core triggers */
    uint8_t* out_mem;                /* shared memory this core writes messages into */
    uint8_t* in_mem;                  /* shared memory the remote core writes messages into */
    uint8_t header_size;               /* MAILBOX_MULTI_CH_HEADER_SIZE for the DSS link, 0 for BSS */
} mailbox_link_t;

static uhal_status_t get_link(const mailbox_peripheral_t mailbox_peripheral, mailbox_link_t* const link) {
    switch (mailbox_peripheral) {
        case MAILBOX_PERIPHERAL_MSS_BSS:
            link->out_reg = MBOX_MSS_BSS_REG;
            link->in_reg = MBOX_BSS_MSS_REG;
            link->out_mem = (uint8_t*)SOC_XWR68XX_MSS_MBOX_MSS_BSS_MEM_BASE_ADDRESS;
            link->in_mem = (uint8_t*)SOC_XWR68XX_MSS_MBOX_BSS_MSS_MEM_BASE_ADDRESS;
            link->header_size = 0U;
            return UHAL_STATUS_OK;
        case MAILBOX_PERIPHERAL_MSS_DSS:
            link->out_reg = MBOX_MSS_DSS_REG;
            link->in_reg = MBOX_DSS_MSS_REG;
            link->out_mem = (uint8_t*)SOC_XWR68XX_MSS_MBOX_MSS_DSS_MEM_BASE_ADDRESS;
            link->in_mem = (uint8_t*)SOC_XWR68XX_MSS_MBOX_DSS_MSS_MEM_BASE_ADDRESS;
            link->header_size = MAILBOX_MULTI_CH_HEADER_SIZE;
            return UHAL_STATUS_OK;
        default:
            return UHAL_STATUS_INVALID_PARAMETERS;
    }
}

uhal_status_t mailbox_init(const mailbox_peripheral_t mailbox_peripheral) {
    mailbox_link_t link;
    if (get_link(mailbox_peripheral, &link) != UHAL_STATUS_OK) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    /* Clear any stale pending status left over from before this core booted. */
    link.out_reg->INT_STS_CLR = MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT;
    link.in_reg->INT_ACK = MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT;
    return UHAL_STATUS_OK;
}

uhal_status_t mailbox_write(const mailbox_peripheral_t mailbox_peripheral, const uint8_t* write_buff, const size_t size) {
    mailbox_link_t link;
    if ((get_link(mailbox_peripheral, &link) != UHAL_STATUS_OK) || (write_buff == (const uint8_t*)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    if ((size == 0U) || (size > MAILBOX_MAX_MESSAGE_SIZE)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }

    if (link.header_size != 0U) {
        const uint32_t ch_id = MAILBOX_CH_ID_0;
        (void)memcpy((void*)link.out_mem, (const void*)&ch_id, sizeof(ch_id));
    }
    (void)memcpy((void*)(link.out_mem + link.header_size), (const void*)write_buff, size);

    /* Trigger the "mailbox" line -- tells the remote core a message is ready. */
    link.out_reg->INT_TRIG = MAILBOX_INT_MAILBOX_BIT;

    /* Block until the remote core acknowledges (its own readFlush) -- see
     * this file's header for which bit means what on which block. */
    while ((link.out_reg->INT_STS_MASKED & MAILBOX_INT_MAILBOX_ACK_BIT) == 0U) {
    }
    link.out_reg->INT_ACK = MAILBOX_INT_MAILBOX_ACK_BIT;

    return UHAL_STATUS_OK;
}

uhal_status_t mailbox_message_pending(const mailbox_peripheral_t mailbox_peripheral) {
    mailbox_link_t link;
    if (get_link(mailbox_peripheral, &link) != UHAL_STATUS_OK) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    if ((link.in_reg->INT_STS_MASKED & MAILBOX_INT_MAILBOX_BIT) != 0U) {
        return UHAL_STATUS_OK;
    }
    return UHAL_STATUS_PERIPHERAL_IN_USE_WARNING;
}

uhal_status_t mailbox_read(const mailbox_peripheral_t mailbox_peripheral, uint8_t* read_buff, const size_t size) {
    mailbox_link_t link;
    if ((get_link(mailbox_peripheral, &link) != UHAL_STATUS_OK) || (read_buff == (uint8_t*)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    if ((size == 0U) || (size > MAILBOX_MAX_MESSAGE_SIZE)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }

    /* Block until a message arrives -- call mailbox_message_pending() first
     * to avoid blocking, see hal_mailbox.h. */
    while ((link.in_reg->INT_STS_MASKED & MAILBOX_INT_MAILBOX_BIT) == 0U) {
    }

    (void)memcpy((void*)read_buff, (const void*)(link.in_mem + link.header_size), size);

    /* Clear our own pending status for this message... */
    link.in_reg->INT_ACK = MAILBOX_INT_MAILBOX_BIT;
    /* ...then tell the remote core it can send another one. */
    link.out_reg->INT_TRIG = MAILBOX_INT_MAILBOX_ACK_BIT;

    return UHAL_STATUS_OK;
}

uhal_status_t mailbox_deinit(const mailbox_peripheral_t mailbox_peripheral) {
    mailbox_link_t link;
    if (get_link(mailbox_peripheral, &link) != UHAL_STATUS_OK) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    link.out_reg->INT_STS_CLR = MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT;
    link.in_reg->INT_ACK = MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT;
    return UHAL_STATUS_OK;
}
