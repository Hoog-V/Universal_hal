/*-----------------------------------------------------------------------------
 * soc_mpu_iwr68xx_gnu.S
 *
 * Cortex-R4F MPU (CP15 c6) access helpers for soc_init()'s region table
 * setup -- GNU assembler port of soc_mpu_iwr68xx.asm (TI armcl syntax),
 * for builds using arm-none-eabi-gcc instead of TI's compiler. Same
 * function names/signatures (called from soc_iwr68xx.c exactly the same
 * way regardless of which one gets built) and the exact same CP15
 * coprocessor instructions/encodings -- only the assembler directive
 * syntax differs (.thumb_func vs .thumbfunc, no .asmfunc/.endasmfunc
 * wrapper, GNU's implicit-global-until-.type convention instead of .def).
 * Universal_hal/CMakeLists.txt selects between this file and the TI one
 * based on CMAKE_C_COMPILER_ID.
 *---------------------------------------------------------------------------*/
    .syntax unified
    .text
    .thumb

    .global uhal_mpu_enable
    .thumb_func
    .type uhal_mpu_enable, %function
uhal_mpu_enable:
    push  {r0}
    mrc   p15, #0, r0, c1, c0, #0
    orr   r0,  r0, #1
    dsb
    mcr   p15, #0, r0, c1, c0, #0
    isb
    pop   {r0}
    bx    lr
    .size uhal_mpu_enable, . - uhal_mpu_enable

    .global uhal_mpu_disable
    .thumb_func
    .type uhal_mpu_disable, %function
uhal_mpu_disable:
    push  {r0}
    mrc   p15, #0, r0, c1, c0, #0
    bic   r0,  r0, #1
    dsb
    mcr   p15, #0, r0, c1, c0, #0
    isb
    pop   {r0}
    bx    lr
    .size uhal_mpu_disable, . - uhal_mpu_disable

    .global uhal_mpu_enable_background_region
    .thumb_func
    .type uhal_mpu_enable_background_region, %function
uhal_mpu_enable_background_region:
    push  {r0}
    mrc   p15, #0, r0, c1, c0, #0
    orr   r0,  r0, #0x20000
    mcr   p15, #0, r0, c1, c0, #0
    pop   {r0}
    bx    lr
    .size uhal_mpu_enable_background_region, . - uhal_mpu_enable_background_region

    .global uhal_mpu_disable_background_region
    .thumb_func
    .type uhal_mpu_disable_background_region, %function
uhal_mpu_disable_background_region:
    push  {r0}
    mrc   p15, #0, r0, c1, c0, #0
    bic   r0,  r0, #0x20000
    mcr   p15, #0, r0, c1, c0, #0
    pop   {r0}
    bx    lr
    .size uhal_mpu_disable_background_region, . - uhal_mpu_disable_background_region

    .global uhal_mpu_set_region
    .thumb_func
    .type uhal_mpu_set_region, %function
uhal_mpu_set_region:
    mcr   p15, #0, r0, c6, c2, #0
    bx    lr
    .size uhal_mpu_set_region, . - uhal_mpu_set_region

    .global uhal_mpu_set_region_base_address
    .thumb_func
    .type uhal_mpu_set_region_base_address, %function
uhal_mpu_set_region_base_address:
    mcr   p15, #0, r0, c6, c1, #0
    bx    lr
    .size uhal_mpu_set_region_base_address, . - uhal_mpu_set_region_base_address

    .global uhal_mpu_set_region_type_and_permission
    .thumb_func
    .type uhal_mpu_set_region_type_and_permission, %function
uhal_mpu_set_region_type_and_permission:
    orr   r0,  r0, r1
    mcr   p15, #0, r0, c6, c1, #4
    bx    lr
    .size uhal_mpu_set_region_type_and_permission, . - uhal_mpu_set_region_type_and_permission

    .global uhal_mpu_set_region_size_register
    .thumb_func
    .type uhal_mpu_set_region_size_register, %function
uhal_mpu_set_region_size_register:
    mcr   p15, #0, r0, c6, c1, #2
    bx    lr
    .size uhal_mpu_set_region_size_register, . - uhal_mpu_set_region_size_register
