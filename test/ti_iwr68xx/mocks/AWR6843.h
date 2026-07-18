#ifndef AWR6843_H
#define AWR6843_H

/* Extern c for compiling with c++*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "AWR6843_types.h"
#include "AWR6843_IOMUX.h"
#include "AWR6843_GIO.h"
#include "AWR6843_SCI.h"
#include "AWR6843_TOPRCM.h"
#include "AWR6843_ESM.h"
#include "AWR6843_MIBSPI.h"
#include "AWR6843_DMA.h"
#include "AWR6843_MAILBOX.h"
#include "AWR6843_DSSREG.h"

/* dma_platform_specific.h's DMA_PERIPHERAL_LOCATION_* macros build plain
 * numeric constants (stored into DMA_ChannelPacket_Type's ISADDR/IDADDR
 * fields, never dereferenced as pointers by the DMA driver itself) from
 * these two base addresses -- the real hardware values are safe to reuse
 * verbatim for that purpose. */
#define SOC_XWR68XX_MSS_SCI_A_BASE_ADDRESS   0xFFF7E500U
#define SOC_XWR68XX_MSS_MIBSPIA_BASE_ADDRESS 0xFFF7F400U

/* VIM line numbers esm_iwr68xx.c wires its two Hwi_create() calls to --
 * not memory addresses, so safe to reuse verbatim (matches AWR6843.h). */
#define SOC_XWR68XX_MSS_ESM_HIGH_PRIORITY_INT 0U
#define SOC_XWR68XX_MSS_ESM_LOW_PRIORITY_INT  20U

extern GIO_Type* GIO;
extern IOMUX_Type* IOMUX;
extern SCI_Type* SCI_A;
extern SCI_Type* SCI_B;
extern TOPRCM_Type* TOP_RCM;
extern ESM_Type* ESM;
extern MIBSPI_Type* MIBSPI_A;
extern MIBSPI_Type* MIBSPI_B;
extern DMA_Type* DMA_1;
extern DMA_Type* DMA_2;
extern DMARAM_Type* DMA_1_RAM;
extern DMARAM_Type* DMA_2_RAM;
extern MAILBOX_Type* MBOX_MSS_BSS_REG;
extern MAILBOX_Type* MBOX_BSS_MSS_REG;
extern MAILBOX_Type* MBOX_MSS_DSS_REG;
extern MAILBOX_Type* MBOX_DSS_MSS_REG;
extern DSSREG_Type* DSSREG;

/* mailbox_iwr68xx.c casts SOC_XWR68XX_MSS_MBOX_*_MEM_BASE_ADDRESS directly
 * to uint8_t* for its shared-memory message buffers -- on real hardware
 * those are fixed physical addresses, but the same bare integer literals
 * cast to a pointer on a host build are either truncated (64-bit) or an
 * unmapped address a memcpy would fault on. Point them at ordinary host
 * arrays instead; each test .cpp that exercises mailbox_iwr68xx.c provides
 * the actual storage (extern here, defined there -- same split as
 * Mock_GIO/GIO above). */
extern uint8_t mock_mbox_mss_bss_mem[2048];
extern uint8_t mock_mbox_bss_mss_mem[2048];
extern uint8_t mock_mbox_mss_dss_mem[2048];
extern uint8_t mock_mbox_dss_mss_mem[2048];
#define SOC_XWR68XX_MSS_MBOX_MSS_BSS_MEM_BASE_ADDRESS mock_mbox_mss_bss_mem
#define SOC_XWR68XX_MSS_MBOX_BSS_MSS_MEM_BASE_ADDRESS mock_mbox_bss_mss_mem
#define SOC_XWR68XX_MSS_MBOX_MSS_DSS_MEM_BASE_ADDRESS mock_mbox_mss_dss_mem
#define SOC_XWR68XX_MSS_MBOX_DSS_MSS_MEM_BASE_ADDRESS mock_mbox_dss_mss_mem

#ifdef __cplusplus
}
#endif /* __cplusplus */






#endif /* AWR6843_H */
