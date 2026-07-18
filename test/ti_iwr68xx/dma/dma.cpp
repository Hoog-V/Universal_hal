#include <cstring>
#include "AWR6843.h"
#include "AWR6843_DMA.h"
#include "gmock/gmock.h"
#include "hal_dma.h"

DMA_Type    Mock_DMA_1 = {};
DMA_Type    Mock_DMA_2 = {};
DMARAM_Type Mock_DMA_1_RAM = {};
DMARAM_Type Mock_DMA_2_RAM = {};
DMA_Type*    DMA_1 = &Mock_DMA_1;
DMA_Type*    DMA_2 = &Mock_DMA_2;
DMARAM_Type* DMA_1_RAM = &Mock_DMA_1_RAM;
DMARAM_Type* DMA_2_RAM = &Mock_DMA_2_RAM;

class ti_iwr68xx_dma : public ::testing::Test {
   protected:
    void SetUp() override {
        memset((void*)DMA_1, 0x00, sizeof(Mock_DMA_1));
        memset((void*)DMA_2, 0x00, sizeof(Mock_DMA_2));
        memset((void*)DMA_1_RAM, 0x00, sizeof(Mock_DMA_1_RAM));
        memset((void*)DMA_2_RAM, 0x00, sizeof(Mock_DMA_2_RAM));
    }
};

TEST_F(ti_iwr68xx_dma, init_invalid_peripheral) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, dma_init((dma_peripheral_t)0xFF, DMA_INIT_OPT_USE_DEFAULT));
}

TEST_F(ti_iwr68xx_dma, init_resets_then_enables_and_assigns_default_port) {
    EXPECT_EQ(UHAL_STATUS_OK, dma_init(DMA_PERIPHERAL_1, DMA_INIT_OPT_USE_DEFAULT));
    /* Final GCTRL write is DMAEN (reset-then-enable, in that order --
     * only the last write is observable through a plain register). */
    EXPECT_EQ((uint32_t)DMA_GCTRL_DMAEN, DMA_1->GCTRL);
    for (uint32_t i = 0; i < 4U; i++) {
        EXPECT_EQ((uint32_t)DMA_PAR_DEFAULT_ALL_CHANNELS, DMA_1->PAR[i]);
    }
    /* DMA_2 untouched. */
    EXPECT_EQ(0U, DMA_2->GCTRL);
}

TEST_F(ti_iwr68xx_dma, set_transfer_mem_invalid_params) {
    uint8_t src[4], dst[4];
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, dma_set_transfer_mem((dma_peripheral_t)0xFF, DMA_CHANNEL_0, src, dst, 4U, DMA_OPT_ELEM_SIZE_8_BIT, 1U));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, dma_set_transfer_mem(DMA_PERIPHERAL_1, DMA_CHANNEL_0, nullptr, dst, 4U, DMA_OPT_ELEM_SIZE_8_BIT, 1U));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, dma_set_transfer_mem(DMA_PERIPHERAL_1, DMA_CHANNEL_0, src, nullptr, 4U, DMA_OPT_ELEM_SIZE_8_BIT, 1U));
}

TEST_F(ti_iwr68xx_dma, set_transfer_mem_configures_channel_and_triggers) {
    uint8_t src[16];
    uint8_t dst[16];
    EXPECT_EQ(UHAL_STATUS_OK,
              dma_set_transfer_mem(DMA_PERIPHERAL_1, DMA_CHANNEL_3, src, dst, 16U, DMA_OPT_ELEM_SIZE_32_BIT, /*do_software_trigger=*/1U));

    volatile DMA_ChannelPacket_Type* const pkt = &DMA_1_RAM->PRIMARYCONTROLPACKET[DMA_CHANNEL_3];
    EXPECT_EQ((uint32_t)(uintptr_t)src, pkt->ISADDR);
    EXPECT_EQ((uint32_t)(uintptr_t)dst, pkt->IDADDR);
    EXPECT_EQ((1U << 16) | 16U, pkt->ITCOUNT);

    const uint32_t expected_chctrl = (DMA_ELEM_SIZE_32BIT << DMA_CHCTRL_SRC_ELEM_SIZE_SHIFT) | (DMA_ELEM_SIZE_32BIT << DMA_CHCTRL_DEST_ELEM_SIZE_SHIFT) |
                                      (DMA_XFER_TYPE_FRAME << DMA_CHCTRL_XFER_TYPE_SHIFT) | (DMA_ADDR_MODE_POST_INCREMENT << DMA_CHCTRL_SRC_ADDR_MODE_SHIFT) |
                                      (DMA_ADDR_MODE_POST_INCREMENT << DMA_CHCTRL_DEST_ADDR_MODE_SHIFT);
    EXPECT_EQ(expected_chctrl, pkt->CHCTRL);

    /* Software trigger requested -- SWCHENAS bit for channel 3 set. */
    EXPECT_EQ((1UL << DMA_CHANNEL_3), DMA_1->SWCHENAS);
}

TEST_F(ti_iwr68xx_dma, set_transfer_mem_without_trigger_does_not_fire) {
    uint8_t src[4], dst[4];
    EXPECT_EQ(UHAL_STATUS_OK, dma_set_transfer_mem(DMA_PERIPHERAL_1, DMA_CHANNEL_0, src, dst, 4U, DMA_OPT_ELEM_SIZE_8_BIT, /*do_software_trigger=*/0U));
    EXPECT_EQ(0U, DMA_1->SWCHENAS);
}

TEST_F(ti_iwr68xx_dma, set_transfer_peripheral_to_mem_uses_constant_source_addr_mode) {
    uint8_t dst[8];
    const dma_peripheral_location_t src_loc = DMA_PERIPHERAL_LOCATION_SCIA_RX;
    EXPECT_EQ(UHAL_STATUS_OK, dma_set_transfer_peripheral_to_mem(DMA_PERIPHERAL_1, DMA_CHANNEL_1, src_loc, dst, 8U, DMA_OPT_ELEM_SIZE_8_BIT));

    volatile DMA_ChannelPacket_Type* const pkt = &DMA_1_RAM->PRIMARYCONTROLPACKET[DMA_CHANNEL_1];
    EXPECT_EQ((uint32_t)src_loc, pkt->ISADDR);
    EXPECT_EQ((uint32_t)(uintptr_t)dst, pkt->IDADDR);
    EXPECT_EQ((uint32_t)DMA_ADDR_MODE_CONSTANT, (pkt->CHCTRL >> DMA_CHCTRL_SRC_ADDR_MODE_SHIFT) & 0x1U);
    EXPECT_EQ((uint32_t)DMA_ADDR_MODE_POST_INCREMENT, (pkt->CHCTRL >> DMA_CHCTRL_DEST_ADDR_MODE_SHIFT) & 0x1U);
    /* Never auto-triggered -- caller must call dma_set_trigger() separately. */
    EXPECT_EQ(0U, DMA_1->SWCHENAS);
}

TEST_F(ti_iwr68xx_dma, set_transfer_mem_to_peripheral_uses_constant_dest_addr_mode) {
    uint8_t src[8];
    const dma_peripheral_location_t dst_loc = DMA_PERIPHERAL_LOCATION_SCIA_TX;
    EXPECT_EQ(UHAL_STATUS_OK, dma_set_transfer_mem_to_peripheral(DMA_PERIPHERAL_1, DMA_CHANNEL_2, src, dst_loc, 8U, DMA_OPT_ELEM_SIZE_8_BIT));

    volatile DMA_ChannelPacket_Type* const pkt = &DMA_1_RAM->PRIMARYCONTROLPACKET[DMA_CHANNEL_2];
    EXPECT_EQ((uint32_t)(uintptr_t)src, pkt->ISADDR);
    EXPECT_EQ((uint32_t)dst_loc, pkt->IDADDR);
    EXPECT_EQ((uint32_t)DMA_ADDR_MODE_POST_INCREMENT, (pkt->CHCTRL >> DMA_CHCTRL_SRC_ADDR_MODE_SHIFT) & 0x1U);
    EXPECT_EQ((uint32_t)DMA_ADDR_MODE_CONSTANT, (pkt->CHCTRL >> DMA_CHCTRL_DEST_ADDR_MODE_SHIFT) & 0x1U);
}

TEST_F(ti_iwr68xx_dma, set_trigger_invalid_params) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, dma_set_trigger((dma_peripheral_t)0xFF, DMA_CHANNEL_0, DMA_TRIGGER_SOFTWARE));
}

TEST_F(ti_iwr68xx_dma, set_trigger_sets_swchenas_bit) {
    EXPECT_EQ(UHAL_STATUS_OK, dma_set_trigger(DMA_PERIPHERAL_2, DMA_CHANNEL_5, DMA_TRIGGER_SOFTWARE));
    EXPECT_EQ((1UL << DMA_CHANNEL_5), DMA_2->SWCHENAS);
}

TEST_F(ti_iwr68xx_dma, reset_trigger_sets_swchenar_bit) {
    EXPECT_EQ(UHAL_STATUS_OK, dma_reset_trigger(DMA_PERIPHERAL_2, DMA_CHANNEL_5, DMA_TRIGGER_SOFTWARE));
    EXPECT_EQ((1UL << DMA_CHANNEL_5), DMA_2->SWCHENAR);
}

TEST_F(ti_iwr68xx_dma, deinit_invalid_peripheral) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, dma_deinit((dma_peripheral_t)0xFF));
}

TEST_F(ti_iwr68xx_dma, deinit_resets_controller) {
    DMA_1->GCTRL = DMA_GCTRL_DMAEN;
    EXPECT_EQ(UHAL_STATUS_OK, dma_deinit(DMA_PERIPHERAL_1));
    EXPECT_EQ((uint32_t)DMA_GCTRL_DMARES, DMA_1->GCTRL);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);

    if (RUN_ALL_TESTS()) {
    }
    return 0;
}
