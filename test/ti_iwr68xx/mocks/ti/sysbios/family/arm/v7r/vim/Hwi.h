#ifndef MOCK_TI_SYSBIOS_FAMILY_ARM_V7R_VIM_HWI_H
#define MOCK_TI_SYSBIOS_FAMILY_ARM_V7R_VIM_HWI_H
/* Minimal stand-in for SYS/BIOS's R4F VIM Hwi module
 * (ti/sysbios/family/arm/v7r/vim/Hwi.h) -- esm_iwr68xx.c is inherently
 * RTOS-coupled (see its own file header: the R4F VIM FIQ/IRQ lines are
 * dispatched by SYS/BIOS's Hwi_create(), there's no raw register
 * alternative), so unit-testing it on a host build means faking this API
 * rather than linking the real xdctools-generated SYS/BIOS package. Only
 * the symbols esm_iwr68xx.c actually calls are modeled; Hwi_create()/
 * Hwi_disable()/Hwi_restore() are fff fakes (see esm.cpp) so tests can
 * control their return values and inspect how esm_iwr68xx.c called them. */
#include <stddef.h> /* NULL -- esm_iwr68xx.c compares Hwi_create()'s return against it */
#include <stdint.h>
#include "xdc/runtime/Error.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef uintptr_t xdc_UInt;
typedef uintptr_t xdc_UArg;

typedef enum {
    Hwi_Type_FIQ,
    Hwi_Type_IRQ
} Hwi_Type;

typedef struct {
    Hwi_Type type;
} Hwi_Params;

typedef void (*Hwi_FuncPtr)(xdc_UArg);
typedef void* Hwi_Handle;

void Hwi_Params_init(Hwi_Params* params);
Hwi_Handle Hwi_create(int interruptNum, Hwi_FuncPtr fxn, Hwi_Params* params, Error_Block* eb);
xdc_UInt Hwi_disable(void);
void Hwi_restore(xdc_UInt key);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* MOCK_TI_SYSBIOS_FAMILY_ARM_V7R_VIM_HWI_H */
