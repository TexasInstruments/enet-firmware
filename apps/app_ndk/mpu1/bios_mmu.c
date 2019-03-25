/*
 *  Copyright (c) Texas Instruments Incorporated 2018
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*!
 * \file     bios_mmu.c
 *
 * \brief    This file has the common MMU setting function for A53/A72.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdbool.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/family/arm/v8a/Mmu.h>

/* CSL Header files */
#include <ti/csl/soc.h>



/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

volatile uint32_t enableDebug = 0U;

void StartupEmulatorWaitFxn(void)
{
    do
    {
    }
    while (enableDebug != 0U);
}

void appMmuInit(void)
{
    Mmu_MapAttrs attrs;
    uint32_t mapIdx = 0U;
    bool retVal;

    /* This is for debug purpose - see the description of function header */
    StartupEmulatorWaitFxn();

    Mmu_initMapAttrs(&attrs);

    /* Main MMR0 cfg */
    attrs.attrIndx = Mmu_AttrIndx_MAIR0;
    retVal = Mmu_map(CSL_CTRL_MMR0_CFG0_BASE,
                     CSL_CTRL_MMR0_CFG0_BASE,
                     0x00900000,
                     &attrs);

    /* PSC0 */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_PSC0_BASE,
                         CSL_PSC0_BASE,
                         CSL_PSC0_SIZE,
                         &attrs);
    }

    /* gicv3 */
    if (retVal == TRUE)
    {
        uint32_t addr = 0U;
        uint32_t size = 0U;

#ifdef SOC_J721E
        addr     = CSL_COMPUTE_CLUSTER0_GIC_DISTRIBUTOR_BASE;
        size     = CSL_COMPUTE_CLUSTER0_GIC_DISTRIBUTOR_SIZE;
#elif SOC_AM65XX
        addr     = CSL_GIC0_DISTRIBUTOR_BASE;
        size     = CSL_GIC0_DISTRIBUTOR_SIZE;
#else
#error "Unsupported SOC"
#endif

        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(addr,
                         addr,
                         size,
                         &attrs);
    }

    /* Timers */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_TIMER0_CFG_BASE,
                         CSL_TIMER0_CFG_BASE,
                         0x000c0000,
                         &attrs);
    }

    /* UART */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_UART0_BASE,
                         CSL_UART0_BASE,
                         0x00040000,
                         &attrs);
    }

    /* I2C */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_I2C0_CFG_BASE,
                         CSL_I2C0_CFG_BASE,
                         0x00100000,
                         &attrs);
    }

    /* McSPI */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_MCSPI0_CFG_BASE,
                         CSL_MCSPI0_CFG_BASE,
                         0x00080000,
                         &attrs);
    }

    /* MCU MMR0 CFG */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_MCU_CTRL_MMR0_CFG0_BASE,
                         CSL_MCU_CTRL_MMR0_CFG0_BASE,
                         CSL_MCU_CTRL_MMR0_CFG0_SIZE,
                         &attrs);
    }

    /* PLL0 CFG */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_MCU_CTRL_MMR0_CFG0_BASE,
                         CSL_MCU_CTRL_MMR0_CFG0_BASE,
                         CSL_MCU_PLL0_CFG_SIZE,
                         &attrs);
    }

    /* WKUP MMR0 cfg */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_WKUP_CTRL_MMR0_CFG0_BASE,
                         CSL_WKUP_CTRL_MMR0_CFG0_BASE,
                         CSL_WKUP_CTRL_MMR0_CFG0_SIZE,
                         &attrs);
    }

    /* pinmux ctrl */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(0x02C40000, 0x02C40000, 0x00100000, &attrs);
    }

    /* pinmux ctrl */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(0x02A430000, 0x02A430000, 0x00001000, &attrs);
    }

    /* Main NAVSS */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(0x30800000, 0x30800000, 0x0C000000, &attrs);
    }

    /* PSC WKUP */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_WKUP_PSC0_BASE,
                         CSL_WKUP_PSC0_BASE,
                         CSL_WKUP_PSC0_SIZE,
                         &attrs);
    }

    /* MCU NAVSS */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(0x28380000, 0x28380000, 0x03880000, &attrs);
    }

    /* ctcontro10 */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(0x30000000ul, 0x30000000, 0x0F000000, &attrs);
    }

    /* CPSW */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_MCU_CPSW0_NUSS_BASE,
                         CSL_MCU_CPSW0_NUSS_BASE,
                         CSL_MCU_CPSW0_NUSS_SIZE,
                         &attrs);
    }

#if defined(SOC_J721E)
    /* CPSW */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_CPSW0_NUSS_BASE,
                         CSL_CPSW0_NUSS_BASE,
                         CSL_CPSW0_NUSS_SIZE,
                         &attrs);
    }
#endif

    /* ICSS-G 0 */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_PRU_ICSSG0_DRAM0_SLV_RAM_BASE,
                         CSL_PRU_ICSSG0_DRAM0_SLV_RAM_BASE,
                         0x0100000,
                         &attrs);
    }

    /* ICSS-G 1 */
    if (retVal == TRUE)
    {
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_PRU_ICSSG1_DRAM0_SLV_RAM_BASE,
                         CSL_PRU_ICSSG1_DRAM0_SLV_RAM_BASE,
                         0x0100000,
                         &attrs);
    }

     /* ICSS-G 2 */
    if (retVal == TRUE)
    {
#ifdef SOC_AM65XX
        mapIdx++;
        attrs.attrIndx = Mmu_AttrIndx_MAIR0;
        retVal = Mmu_map(CSL_PRU_ICSSG2_DRAM0_SLV_RAM_BASE,
                         CSL_PRU_ICSSG2_DRAM0_SLV_RAM_BASE,
                         0x0100000,
                         &attrs);
#endif
    }

    /* ddr_0 */
    if (retVal == TRUE)
    {
        attrs.attrIndx = Mmu_AttrIndx_MAIR7;
        mapIdx++;
        retVal = Mmu_map(0x80000000, 0x80000000, 0x20000000, &attrs);
    }

    /* MSMC */
    if (retVal == TRUE)
    {
        attrs.attrIndx = Mmu_AttrIndx_MAIR7;
        mapIdx++;
        retVal = Mmu_map(CSL_COMPUTE_CLUSTER0_MSMC_SRAM_BASE,
                         CSL_COMPUTE_CLUSTER0_MSMC_SRAM_BASE,
                         CSL_COMPUTE_CLUSTER0_MSMC_SRAM_SIZE,
                         &attrs);
    }

    /* OCMC */
    if (retVal == TRUE)
    {
        uint32_t ramBaseAddr = 0U;
        uint32_t ramSize     = 0U;

#ifdef SOC_J721E
        ramBaseAddr = CSL_MCU_MSRAM_1MB0_RAM_BASE;
        ramSize     = CSL_MCU_MSRAM_1MB0_RAM_SIZE;
#elif SOC_AM65XX
        ramBaseAddr = CSL_MCU_MSRAM0_RAM_BASE;
        ramSize     = CSL_MCU_MSRAM0_RAM_SIZE;
#else
#error "Unsupported SOC"
#endif

        attrs.attrIndx = Mmu_AttrIndx_MAIR7;
        mapIdx++;
        retVal = Mmu_map(ramBaseAddr,
                         ramBaseAddr,
                         ramSize,
                         &attrs);
    }

    if (retVal == FALSE)
    {
        while (1);
    }
}
