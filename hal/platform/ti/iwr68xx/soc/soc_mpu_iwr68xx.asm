;-------------------------------------------------------------------------------
; soc_mpu_iwr68xx.asm
;
; Cortex-R4F MPU (CP15 c6) access helpers for soc_init()'s region table setup.
; Ported 1:1 (same coprocessor instructions) from the mmWave SDK's
; ti/drivers/soc/src/soc_mpu.asm (SOC_MPU*), renamed uhal_mpu_* to avoid
; colliding with that library now that this project no longer links it.
;-------------------------------------------------------------------------------

    .text
    .arm

;-------------------------------------------------------------------------------
; Enable Mpu -- SCTLR.M (bit 0)

    .thumb
    .thumbfunc uhal_mpu_enable
    .def     uhal_mpu_enable
    .asmfunc

uhal_mpu_enable

        stmfd sp!, {r0}
        mrc   p15, #0, r0, c1, c0, #0
        orr   r0,  r0, #1
        dsb
        mcr   p15, #0, r0, c1, c0, #0
        isb
        ldmfd sp!, {r0}
        bx    lr

    .endasmfunc


;-------------------------------------------------------------------------------
; Disable Mpu -- SCTLR.M (bit 0)

    .thumb
    .thumbfunc uhal_mpu_disable
    .def     uhal_mpu_disable
    .asmfunc

uhal_mpu_disable

        stmfd sp!, {r0}
        mrc   p15, #0, r0, c1, c0, #0
        bic   r0,  r0, #1
        dsb
        mcr   p15, #0, r0, c1, c0, #0
        isb
        ldmfd sp!, {r0}
        bx    lr

    .endasmfunc


;-------------------------------------------------------------------------------
; Enable Mpu background region -- SCTLR.BR (bit 17)

    .thumb
    .thumbfunc uhal_mpu_enable_background_region
    .def     uhal_mpu_enable_background_region
    .asmfunc

uhal_mpu_enable_background_region

        stmfd sp!, {r0}
        mrc   p15, #0, r0,      c1, c0, #0
        orr   r0,  r0, #0x20000
        mcr   p15, #0, r0,      c1, c0, #0
        ldmfd sp!, {r0}
        bx    lr

    .endasmfunc


;-------------------------------------------------------------------------------
; Disable Mpu background region -- SCTLR.BR (bit 17)

    .thumb
    .thumbfunc uhal_mpu_disable_background_region
    .def     uhal_mpu_disable_background_region
    .asmfunc

uhal_mpu_disable_background_region

        stmfd sp!, {r0}
        mrc   p15, #0, r0,      c1, c0, #0
        bic   r0,  r0, #0x20000
        mcr   p15, #0, r0,      c1, c0, #0
        ldmfd sp!, {r0}
        bx    lr

    .endasmfunc


;-------------------------------------------------------------------------------
; Select Mpu region number -- RGNR (c6, c2, 0)

    .thumb
    .thumbfunc uhal_mpu_set_region
    .def     uhal_mpu_set_region
    .asmfunc

uhal_mpu_set_region

        mcr   p15, #0, r0, c6, c2, #0
        bx    lr

    .endasmfunc


;-------------------------------------------------------------------------------
; Set base address of the currently selected region -- DRBAR (c6, c1, 0)

    .thumb
    .thumbfunc uhal_mpu_set_region_base_address
    .def     uhal_mpu_set_region_base_address
    .asmfunc

uhal_mpu_set_region_base_address

        mcr   p15, #0, r0, c6, c1, #0
        bx    lr

    .endasmfunc


;-------------------------------------------------------------------------------
; Set type|permission of the currently selected region -- DRACR (c6, c1, 4)

    .thumb
    .thumbfunc uhal_mpu_set_region_type_and_permission
    .def     uhal_mpu_set_region_type_and_permission
    .asmfunc

uhal_mpu_set_region_type_and_permission

        orr   r0,  r0, r1
        mcr   p15, #0, r0, c6, c1, #4
        bx    lr

    .endasmfunc


;-------------------------------------------------------------------------------
; Set size/enable of the currently selected region -- DRSR (c6, c1, 2)

    .thumb
    .thumbfunc uhal_mpu_set_region_size_register
    .def     uhal_mpu_set_region_size_register
    .asmfunc

uhal_mpu_set_region_size_register

        mcr   p15, #0, r0, c6, c1, #2
        bx    lr

    .endasmfunc

;-------------------------------------------------------------------------------
