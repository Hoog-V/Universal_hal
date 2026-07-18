#include <cstring>
#include "AWR6843.h"
#include "AWR6843_ESM.h"
#include "fff.h"
#include "gmock/gmock.h"
#include "hal_esm.h"
#include "ti/sysbios/family/arm/v7r/vim/Hwi.h"

DEFINE_FFF_GLOBALS;

ESM_Type Mock_ESM = {};
ESM_Type* ESM = &Mock_ESM;

/* esm_iwr68xx.c is inherently RTOS-coupled -- the R4F VIM FIQ/IRQ lines are
 * dispatched by SYS/BIOS's Hwi_create(), there's no raw register
 * alternative (see the file's own header). fff fakes stand in for the
 * mocked ti/sysbios/family/arm/v7r/vim/Hwi.h API (mocks/ti/...) so
 * esm_init()/esm_register_notifier()/esm_deregister_notifier() are host-
 * testable without a real xdctools-generated SYS/BIOS package. */
extern "C" {
FAKE_VALUE_FUNC(Hwi_Handle, Hwi_create, int, Hwi_FuncPtr, Hwi_Params*, Error_Block*);
FAKE_VOID_FUNC(Hwi_Params_init, Hwi_Params*);
FAKE_VALUE_FUNC(xdc_UInt, Hwi_disable);
FAKE_VOID_FUNC(Hwi_restore, xdc_UInt);
FAKE_VOID_FUNC(Error_init, Error_Block*);
}

/* A distinct non-NULL value per Hwi_create() call -- esm_init() creates
 * two Hwi objects (high-priority FIQ, low-priority IRQ) and only checks
 * each return against NULL, so any non-NULL value works. */
static int fake_hwi_object;

class ti_iwr68xx_esm : public ::testing::Test {
   protected:
    void SetUp() override {
        memset((void*)ESM, 0x00, sizeof(Mock_ESM));
        RESET_FAKE(Hwi_create);
        RESET_FAKE(Hwi_Params_init);
        RESET_FAKE(Hwi_disable);
        RESET_FAKE(Hwi_restore);
        RESET_FAKE(Error_init);
        Hwi_create_fake.return_val = (Hwi_Handle)&fake_hwi_object;
    }
};

TEST_F(ti_iwr68xx_esm, init_creates_fiq_then_irq_handlers) {
    EXPECT_EQ(UHAL_STATUS_OK, esm_init(0U));

    ASSERT_EQ(2U, Hwi_create_fake.call_count);
    EXPECT_EQ(SOC_XWR68XX_MSS_ESM_HIGH_PRIORITY_INT, (uint32_t)Hwi_create_fake.arg0_history[0]);
    EXPECT_EQ(SOC_XWR68XX_MSS_ESM_LOW_PRIORITY_INT, (uint32_t)Hwi_create_fake.arg0_history[1]);
    ASSERT_EQ(2U, Hwi_Params_init_fake.call_count);
}

TEST_F(ti_iwr68xx_esm, init_sets_fiq_and_irq_hwi_types) {
    /* Hwi_Params_init() zeroes hwi_params, then esm_init() sets .type
     * itself -- capture what was actually passed to Hwi_create() via a
     * custom fake that snapshots *params before it goes out of scope. */
    static Hwi_Type captured_type[2];
    static int call_idx = 0;
    call_idx = 0;
    Hwi_create_fake.custom_fake = [](int, Hwi_FuncPtr, Hwi_Params* params, Error_Block*) -> Hwi_Handle {
        captured_type[call_idx++] = params->type;
        return (Hwi_Handle)&fake_hwi_object;
    };

    EXPECT_EQ(UHAL_STATUS_OK, esm_init(0U));
    EXPECT_EQ(Hwi_Type_FIQ, captured_type[0]);
    EXPECT_EQ(Hwi_Type_IRQ, captured_type[1]);
}

TEST_F(ti_iwr68xx_esm, init_fails_when_fiq_hwi_create_fails) {
    Hwi_create_fake.return_val = nullptr;
    EXPECT_EQ(UHAL_STATUS_ERROR, esm_init(0U));
    /* Only the FIQ Hwi was attempted -- IRQ creation never runs after the
     * first failure. */
    EXPECT_EQ(1U, Hwi_create_fake.call_count);
}

TEST_F(ti_iwr68xx_esm, init_fails_when_irq_hwi_create_fails) {
    Hwi_Handle results[] = {(Hwi_Handle)&fake_hwi_object, nullptr};
    SET_RETURN_SEQ(Hwi_create, results, 2);
    EXPECT_EQ(UHAL_STATUS_ERROR, esm_init(0U));
    EXPECT_EQ(2U, Hwi_create_fake.call_count);
}

TEST_F(ti_iwr68xx_esm, init_without_clear_leaves_status_registers_alone) {
    ESM->ESMSR1 = 0x1U;
    ESM->ESMSR2 = 0x2U;
    ESM->ESMSR3 = 0x4U;
    ESM->ESMSR4 = 0x8U;
    ESM->ESMSSR2 = 0x10U;
    EXPECT_EQ(UHAL_STATUS_OK, esm_init(0U));
    EXPECT_EQ(0x1U, ESM->ESMSR1);
    EXPECT_EQ(0x2U, ESM->ESMSR2);
    EXPECT_EQ(0x4U, ESM->ESMSR3);
    EXPECT_EQ(0x8U, ESM->ESMSR4);
    EXPECT_EQ(0x10U, ESM->ESMSSR2);
}

TEST_F(ti_iwr68xx_esm, init_with_clear_acks_all_status_registers) {
    ESM->ESMSR1 = 0x1U;
    ESM->ESMSR2 = 0x2U;
    ESM->ESMSR3 = 0x4U;
    ESM->ESMSR4 = 0x8U;
    ESM->ESMSSR2 = 0x10U;
    EXPECT_EQ(UHAL_STATUS_OK, esm_init(1U));
    /* Write-1-to-clear against itself clears every set bit. */
    EXPECT_EQ(0U, ESM->ESMSR1);
    EXPECT_EQ(0U, ESM->ESMSR2);
    EXPECT_EQ(0U, ESM->ESMSR3);
    EXPECT_EQ(0U, ESM->ESMSR4);
    EXPECT_EQ(0U, ESM->ESMSSR2);
}

TEST_F(ti_iwr68xx_esm, register_notifier_invalid_params) {
    esm_notify_params_t params = {};
    int32_t index;
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, esm_register_notifier(nullptr, &index));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, esm_register_notifier(&params, nullptr));
}

TEST_F(ti_iwr68xx_esm, register_notifier_group1_unmasks_low_half) {
    esm_notify_params_t params = {};
    params.group_number = 1U;
    params.error_number = 5U;
    params.notify = [](void*) {};
    int32_t index = -1;

    EXPECT_EQ(UHAL_STATUS_OK, esm_register_notifier(&params, &index));
    EXPECT_EQ(0, index);
    EXPECT_EQ((1UL << 5U), ESM->ESMIESR1);
    EXPECT_EQ(0U, ESM->ESMIESR4);
    EXPECT_EQ(1U, Hwi_disable_fake.call_count);
    EXPECT_EQ(1U, Hwi_restore_fake.call_count);
}

TEST_F(ti_iwr68xx_esm, register_notifier_group1_unmasks_upper_half) {
    esm_notify_params_t params = {};
    params.group_number = 1U;
    params.error_number = 40U; /* >= 32 -- ESMIESR4, bit (40-32)=8 */
    params.notify = [](void*) {};
    int32_t index = -1;

    EXPECT_EQ(UHAL_STATUS_OK, esm_register_notifier(&params, &index));
    EXPECT_EQ((1UL << 8U), ESM->ESMIESR4);
    EXPECT_EQ(0U, ESM->ESMIESR1);
}

TEST_F(ti_iwr68xx_esm, register_notifier_group2_does_not_touch_ies_registers) {
    /* Group 2 errors are unmasked by default -- see esm_iwr68xx.c. */
    esm_notify_params_t params = {};
    params.group_number = 2U;
    params.error_number = 3U;
    params.notify = [](void*) {};
    int32_t index = -1;

    EXPECT_EQ(UHAL_STATUS_OK, esm_register_notifier(&params, &index));
    EXPECT_EQ(0U, ESM->ESMIESR1);
    EXPECT_EQ(0U, ESM->ESMIESR4);
}

TEST_F(ti_iwr68xx_esm, register_notifier_fails_once_table_is_full) {
    esm_notify_params_t params = {};
    params.notify = [](void*) {};
    int32_t index;
    for (uint32_t i = 0; i < ESM_MAX_NOTIFIERS; i++) {
        params.error_number = i;
        ASSERT_EQ(UHAL_STATUS_OK, esm_register_notifier(&params, &index));
    }
    EXPECT_EQ(UHAL_STATUS_ERROR, esm_register_notifier(&params, &index));
}

TEST_F(ti_iwr68xx_esm, deregister_notifier_invalid_index) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, esm_deregister_notifier(-1));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, esm_deregister_notifier((int32_t)ESM_MAX_NOTIFIERS));
}

TEST_F(ti_iwr68xx_esm, deregister_notifier_group1_masks_via_ecr) {
    esm_notify_params_t params = {};
    params.group_number = 1U;
    params.error_number = 5U;
    params.notify = [](void*) {};
    int32_t index = -1;
    ASSERT_EQ(UHAL_STATUS_OK, esm_register_notifier(&params, &index));

    EXPECT_EQ(UHAL_STATUS_OK, esm_deregister_notifier(index));
    /* Deliberately write-1-to-clear (ESMIECR1), not the SDK's write-0 --
     * see esm_iwr68xx.c's own comment for why. */
    EXPECT_EQ((1UL << 5U), ESM->ESMIECR1);
}

TEST_F(ti_iwr68xx_esm, deregister_notifier_frees_the_slot_for_reuse) {
    esm_notify_params_t params = {};
    params.notify = [](void*) {};
    int32_t index;
    for (uint32_t i = 0; i < ESM_MAX_NOTIFIERS; i++) {
        params.error_number = i;
        ASSERT_EQ(UHAL_STATUS_OK, esm_register_notifier(&params, &index));
    }
    ASSERT_EQ(UHAL_STATUS_OK, esm_deregister_notifier(0));
    /* One slot freed -- registering again should now succeed. */
    params.error_number = 99U;
    EXPECT_EQ(UHAL_STATUS_OK, esm_register_notifier(&params, &index));
    EXPECT_EQ(0, index);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);

    if (RUN_ALL_TESTS()) {
    }
    return 0;
}
