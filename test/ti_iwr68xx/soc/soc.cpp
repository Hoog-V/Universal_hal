#include <cstring>
#include "AWR6843.h"
#include "AWR6843_DSSREG.h"
#include "AWR6843_TOPRCM.h"
/* mpu_config() makes 2 + 12*4 + 2 = 52 fake calls in one soc_init() --
 * above fff's default 50-entry call_history, which would silently drop
 * the last couple of calls (see REGISTER_CALL's bounds check in fff.h). */
#define FFF_CALL_HISTORY_LEN 64u
#include "fff.h"
#include "gmock/gmock.h"
#include "hal_soc.h"

DEFINE_FFF_GLOBALS;

TOPRCM_Type Mock_TOP_RCM = {};
DSSREG_Type Mock_DSSREG = {};
TOPRCM_Type* TOP_RCM = &Mock_TOP_RCM;
DSSREG_Type* DSSREG = &Mock_DSSREG;

/* soc_iwr68xx.c's CP15 c6 (MPU) access is hand-written assembly
 * (soc_mpu_iwr68xx.asm) -- not host-buildable, and not what this test is
 * about (the region table/sequencing is; the actual CP15 opcodes are
 * exercised by the existing hardware bring-up, see the AWR6xxx_Toolchain
 * memory notes). fff fakes stand in so mpu_config()'s calls into it are
 * observable instead. */
extern "C" {
FAKE_VOID_FUNC(uhal_mpu_enable);
FAKE_VOID_FUNC(uhal_mpu_disable);
FAKE_VOID_FUNC(uhal_mpu_enable_background_region);
FAKE_VOID_FUNC(uhal_mpu_disable_background_region);
FAKE_VOID_FUNC(uhal_mpu_set_region, uint32_t);
FAKE_VOID_FUNC(uhal_mpu_set_region_base_address, uint32_t);
FAKE_VOID_FUNC(uhal_mpu_set_region_type_and_permission, uint32_t, uint32_t);
FAKE_VOID_FUNC(uhal_mpu_set_region_size_register, uint32_t);
}

class ti_iwr68xx_soc : public ::testing::Test {
   protected:
    void SetUp() override {
        memset((void*)TOP_RCM, 0x00, sizeof(Mock_TOP_RCM));
        memset((void*)DSSREG, 0x00, sizeof(Mock_DSSREG));
        RESET_FAKE(uhal_mpu_enable);
        RESET_FAKE(uhal_mpu_disable);
        RESET_FAKE(uhal_mpu_enable_background_region);
        RESET_FAKE(uhal_mpu_disable_background_region);
        RESET_FAKE(uhal_mpu_set_region);
        RESET_FAKE(uhal_mpu_set_region_base_address);
        RESET_FAKE(uhal_mpu_set_region_type_and_permission);
        RESET_FAKE(uhal_mpu_set_region_size_register);
        FFF_RESET_HISTORY();
        /* APLL calibration "done" by default -- most tests aren't about
         * the clock wait loop, and an unsatisfied wait spins the full
         * ~1M-iteration timeout (still fast, but pointless noise). */
        TOP_RCM->SPARE0 = TOPRCM_SPARE0_APLL_CAL_DONE;
    }
};

TEST_F(ti_iwr68xx_soc, mpu_config_programs_all_12_regions_in_order) {
    EXPECT_EQ(UHAL_STATUS_OK, soc_init());

    ASSERT_EQ(12U, uhal_mpu_set_region_fake.call_count);
    for (uint32_t i = 0; i < 12U; i++) {
        EXPECT_EQ(i, uhal_mpu_set_region_fake.arg0_history[i]) << "region " << i;
    }
    EXPECT_EQ(12U, uhal_mpu_set_region_base_address_fake.call_count);
    EXPECT_EQ(12U, uhal_mpu_set_region_type_and_permission_fake.call_count);
    EXPECT_EQ(12U, uhal_mpu_set_region_size_register_fake.call_count);

    /* Region 0: background, whole 4GB, no access. */
    EXPECT_EQ(0x0U, uhal_mpu_set_region_base_address_fake.arg0_history[0]);

    /* Region 1: TCMA IRAM at 0x0. */
    EXPECT_EQ(0x0U, uhal_mpu_set_region_base_address_fake.arg0_history[1]);

    /* Region 2: TCMB DRAM at 0x08000000. */
    EXPECT_EQ(0x08000000U, uhal_mpu_set_region_base_address_fake.arg0_history[2]);

    /* Every region's size write ORs in MPU_REGION_ENABLE (bit 0). */
    for (uint32_t i = 0; i < 12U; i++) {
        EXPECT_EQ(1U, uhal_mpu_set_region_size_register_fake.arg0_history[i] & 1U) << "region " << i;
    }
}

TEST_F(ti_iwr68xx_soc, mpu_config_brackets_regions_with_disable_then_enable) {
    EXPECT_EQ(UHAL_STATUS_OK, soc_init());

    ASSERT_EQ(1U, uhal_mpu_disable_fake.call_count);
    ASSERT_EQ(1U, uhal_mpu_disable_background_region_fake.call_count);
    ASSERT_EQ(1U, uhal_mpu_enable_background_region_fake.call_count);
    ASSERT_EQ(1U, uhal_mpu_enable_fake.call_count);

    /* fff's global call_history records every fake invocation, across all
     * fakes, in call order -- disable()/disable_background_region() must
     * be the first two calls (before any region is programmed), and
     * enable_background_region()/enable() the last two (after all 12). */
    ASSERT_GE(fff.call_history_idx, 4U);
    EXPECT_EQ((fff_function_t)uhal_mpu_disable, fff.call_history[0]);
    EXPECT_EQ((fff_function_t)uhal_mpu_disable_background_region, fff.call_history[1]);
    EXPECT_EQ((fff_function_t)uhal_mpu_enable_background_region, fff.call_history[fff.call_history_idx - 2]);
    EXPECT_EQ((fff_function_t)uhal_mpu_enable, fff.call_history[fff.call_history_idx - 1]);
}

TEST_F(ti_iwr68xx_soc, init_brings_up_bss_clock_domain) {
    TOP_RCM->BSSCTL = 0xFFFFFFFFU;
    EXPECT_EQ(UHAL_STATUS_OK, soc_init());
    EXPECT_EQ(0U, TOP_RCM->BSSCTL);
}

TEST_F(ti_iwr68xx_soc, init_returns_clock_error_when_apll_never_calibrates) {
    TOP_RCM->SPARE0 = 0U; /* never reaches TOPRCM_SPARE0_APLL_CAL_DONE */
    EXPECT_EQ(UHAL_STATUS_PERIPHERAL_CLOCK_ERROR, soc_init());
}

TEST_F(ti_iwr68xx_soc, init_leaves_secure_firewalls_alone_on_gp_parts) {
    TOP_RCM->EFUSEREGROW10 = 0U; /* part number 0 != TOPRCM_SECURE_PART_NUMBER */
    EXPECT_EQ(UHAL_STATUS_OK, soc_init());
    EXPECT_EQ(0U, TOP_RCM->SECURECFGREG1);
}

TEST_F(ti_iwr68xx_soc, init_disables_jtag_and_logger_firewalls_on_secure_parts) {
    TOP_RCM->EFUSEREGROW10 = (uint32_t)TOPRCM_SECURE_PART_NUMBER << TOPRCM_EFUSEREGROW10_PART_NUMBER_SHIFT;
    EXPECT_EQ(UHAL_STATUS_OK, soc_init());
    EXPECT_EQ((uint32_t)(TOPRCM_SECURECFGREG1_JTAGFIREWALLEN_MASK | TOPRCM_SECURECFGREG1_LOGGERFIREWALLEN_MASK), TOP_RCM->SECURECFGREG1);
}

TEST_F(ti_iwr68xx_soc, unhalt_dss_clears_halt_bit_and_waits_for_power_on) {
    DSSREG->GEMPWRSMCFG4 = DSSREG_GEMPWRSMCFG4_PWRSMLRSTHALT_BIT | DSSREG_GEMPWRSMCFG4_GEMEVENTMASK_BIT;
    DSSREG->GEMPWRSMCFG3 = (DSSREG_PWRSMMODESTATUS_ON << DSSREG_GEMPWRSMCFG3_PWRSMMODESTATUS_SHIFT);

    EXPECT_EQ(UHAL_STATUS_OK, soc_unhalt_dss());

    EXPECT_EQ(0U, DSSREG->GEMPWRSMCFG4 & DSSREG_GEMPWRSMCFG4_PWRSMLRSTHALT_BIT);
    EXPECT_EQ(0U, DSSREG->GEMPWRSMCFG4 & DSSREG_GEMPWRSMCFG4_GEMEVENTMASK_BIT);
}

TEST_F(ti_iwr68xx_soc, unhalt_dss_returns_error_when_power_domain_never_reports_on) {
    DSSREG->GEMPWRSMCFG3 = 0U; /* PWRSMMODESTATUS stays OFF */
    EXPECT_EQ(UHAL_STATUS_ERROR, soc_unhalt_dss());
    /* The halt bit is still released -- only the power-on wait failed. */
    EXPECT_EQ(0U, DSSREG->GEMPWRSMCFG4 & DSSREG_GEMPWRSMCFG4_PWRSMLRSTHALT_BIT);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);

    if (RUN_ALL_TESTS()) {
    }
    return 0;
}
