/****************************************************************************
CToolsLib - TI-ETB EDMA3/INTCx Support Header File

Copyright (c) 2009-2012 Texas Instruments Inc. (www.ti.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
****************************************************************************/
#ifndef __EDMA_DEV_C66XX_H
#define __EDMA_DEV_C66XX_H

#ifdef __cplusplus
extern "C" {
#endif

/*! \file edma_dev-c66xx.h
    \version 0.1

    This file contains the addresses, definitions, and macros to support
     interfacing with the C66xx EDMA3 and external chip-level interrupt
     controller to drain the TI-ETB to a specified memory buffer.
 */

/******************************************************************************
 *
 *   Device EDMA configuration struct
 *
 ******************************************************************************/
typedef struct {
    uint32_t cc_id;
    uint32_t etbhalfChannel;
    uint32_t etbfullChannel;
} k2_edma_config_table_t;


#ifndef VREG
    #define VREG volatile uint32_t
#endif
 
/******************************************************************************
 *
 *   EDMA3 ADDRESSES, DEFINITIONS, AND MACROS
 * 
 ******************************************************************************/

/*  TI ETB addresses */
#define TETB_CORE0_BASE_ADDR  0x027d0000
#define TETB_CORE1_BASE_ADDR  0x027e0000
#define TETB_CORE2_BASE_ADDR  0x027f0000
#define TETB_CORE3_BASE_ADDR  0x02800000
#define TETB_SYSTEM_BASE_ADDR 0x02850000

/* EDMA Channel Controller addresses */
#define EDMA_TPCC_OFFSET      0x00020000
#define EDMA_TPCC0_BASE_ADDR  0x02700000
#define EDMA_TPCC1_BASE_ADDR  0x02720000
#define EDMA_TPCC2_BASE_ADDR  0x02740000
#define EDMA_TPCC3_BASE_ADDR  0x02728000 //EDMA TPCC3 is supported in keystone2 devices
#define EDMA_TPCC4_BASE_ADDR  0x02708000 //EDMA TPCC4 is supported in keystone2 devices
#if defined(C6670) || defined(C6678)
/*Fix for compiler warning/remark */
#define EDMA_TPCC_BASE_ADDR(cc) (EDMA_TPCC0_BASE_ADDR+((cc)*EDMA_TPCC_OFFSET))
#else
#define EDMA_TPCC_BASE_ADDR(cc) ((cc > 2)?(EDMA_TPCC3_BASE_ADDR-((cc-3)*EDMA_TPCC_OFFSET)):(EDMA_TPCC0_BASE_ADDR+((cc)*EDMA_TPCC_OFFSET)))
#endif
#define EDMA_TPCC_PARAM_BASE_ADDR(cc) (EDMA_TPCC_BASE_ADDR(cc) + 0x4000)

/* EDMA Transfer Controller addresses */
#define EDMA_TPCC0_TC0_BASE_ADDR  0x02760000
#define EDMA_TPCC0_TC1_BASE_ADDR  0x02768000
#define EDMA_TPCC1_TC0_BASE_ADDR  0x02770000
#define EDMA_TPCC1_TC1_BASE_ADDR  0x02778000
#define EDMA_TPCC1_TC2_BASE_ADDR  0x02780000
#define EDMA_TPCC1_TC3_BASE_ADDR  0x02788000
#define EDMA_TPCC2_TC0_BASE_ADDR  0x02790000
#define EDMA_TPCC2_TC1_BASE_ADDR  0x02798000
#define EDMA_TPCC2_TC2_BASE_ADDR  0x027a0000
#define EDMA_TPCC2_TC3_BASE_ADDR  0x027a8000

/******************************************************************************/
/*! Parameter RAM (PaRAM) structure and definitions
    _______________________  Byte address offset:
   |          OPT          |  +0h
   |-----------------------|
   |          SRC          |  +4h
   |-----------------------|
   |   BCNT    |   ACNT    |  +8h
   |-----------------------|
   |          DST          |  +Ch
   |-----------------------|
   |  DSTBIDX  |  SRCBIDX  |  +10h
   |-----------|-----------|
   |  BCNTRLD  |   LINK    |  +14h
   |-----------|-----------|
   |  DSTCIDX  |  SRCCIDX  |  +18h
   |-----------|-----------|
   |   RSVD    |   CCNT    |  +1Ch
    ----------------------- 

 ******************************************************************************/
#define PARAM_OPT_REG(ba) *(volatile uint32_t *)(ba)

#define PARAM_OPT_PRIV_SUPER (1 << 31) /* 0:User level, 1:Supervisor privilege*/
#define PARAM_OPT_PRIVID(id) ((id) << 24)
#define PARAM_OPT_ITCCHEN   (1 << 23) /* Intermediate xfer complete chain EN */
#define PARAM_OPT_TCCHEN    (1 << 22) /* Xfer complete chaining enabled */
#define PARAM_OPT_ITCINTEN  (1 << 21) /* Intermediate xfer complete INT EN */
#define PARAM_OPT_TCINTEN   (1 << 20) /* Xfer complete INT EN */
#define PARAM_OPT_TCC(bit)  ((bit) << 12) /* Xfer complete 6-bit code */
#define PARAM_OPT_TCC_EARLY (1 << 11)     /* Early completion interrupt */
#define PARAM_OPT_FWID(fw)  ((fw) << 8) /* FIFO Width */
#define PARAM_OPT_STATIC    (1 << 3)    /* Static PaRAM set */
#define PARAM_OPT_AB_SYNC   (1 << 2)    /* 0:A-sync, 1:AB-sync */
#define PARAM_OPT_DAM_CONST (1 << 1)    /* Destination address mode */
#define PARAM_OPT_SAM_CONST (1 << 0)    /* Source address mode */
                                        /* 0:Incrementing, 1:Constant DAM/SAM */
/* FIFO width options. Applies if either SAM or DAM is set to constant
 *  addressing mode.
 */
#define PARAM_OPT_FWID_8BIT    0
#define PARAM_OPT_FWID_16BIT   1
#define PARAM_OPT_FWID_32BIT   2
#define PARAM_OPT_FWID_64BIT   3
#define PARAM_OPT_FWID_128BIT  4
#define PARAM_OPT_FWID_256BIT  5

/* Parameter RAM (PaRAM) register offset macros
 */
#define PARAM_SRC_REG(ba)     *(volatile uint32_t *)((ba)+0x4)
#define PARAM_AB_CNT_REG(ba)    *(volatile uint32_t *)((ba)+0x8)
#define PARAM_DST_REG(ba)     *(volatile uint32_t *)((ba)+0xc)
#define PARAM_SRCBIDX_REG(ba) *(volatile uint32_t *)((ba)+0x10)
#define PARAM_LINK_REG(ba)    *(volatile uint32_t *)((ba)+0x14)
#define PARAM_SRCCIDX_REG(ba) *(volatile uint32_t *)((ba)+0x18)
#define PARAM_CCNT_REG(ba)    *(volatile uint32_t *)((ba)+0x1c)

#define PARAM_ACNT(val)  (val)
#define PARAM_ACNT_MASK  0xffff
#define PARAM_BCNT(val)  ((val) << 16)
#define PARAM_BCNT_MASK  0xffff0000
#define PARAM_BCNT_VALUE(ba) (((PARAM_AB_CNT_REG(ba)) & PARAM_BCNT_MASK) >> 16)
#define PARAM_SRC_BIDX(val) (val)
#define PARAM_DST_BIDX(val) ((val) << 16)
#define PARAM_LINK(val)     (val)
#define PARAM_LINK_MASK  0xffff
#define PARAM_BCNT_RLD(val) ((val) << 16)
#define PARAM_SRC_CIDX(val) (val)
#define PARAM_DST_CIDX(val) ((val) << 16)

/******************************************************************************/
/*! \par edma3_param
    \brief PaRAM structure 
*/ 
struct edma3_param {
    uint32_t options;     /* Transfer configuration options */
    uint32_t src_addr;    /* Byte address from which data is transferred */
    uint32_t ab_cnt;
    uint32_t dst_addr;    /* Byte address to which data is transferred */
    uint32_t srcdst_bidx;
    uint32_t link_bcnt;
    uint32_t srcdst_cidx;
    uint32_t ccnt;
};

/******************************************************************************/
/*! EDMA3 DMA Channel 0-63 Mapping Registers  (Offset 0x100-0x1FC)
    - DCHMAPn Field Descriptions
    - [13:5] The PaRAM set number (0-1FFh) for DMA channel
 */
#define EDMA3_DCHMAP_REG(cc,ch) *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x100+((ch)<<2))

/******************************************************************************/
/*! EDMA3 Event Registers -- READ ONLY -- (ER/ERH - Offset 0x1000/0x1004)
    - Event Status Registers (ER/ERH)
    - [31:0]  ER,  Events 0-31, offset 1000h
    - [63:32] ERH, Events 32-64, offset 1004h
 */
#define EDMA3_ER_REG(cc)  *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1000)
#define EDMA3_ERH_REG(cc) *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1004)

/******************************************************************************/
/*! EDMA3 Event Set Registers (ESR/ESRH - Offset 0x1010/0x1014)
    - [31:0]  ESR,  Events 0-31, offset 1010h
    - [63:32] ESRH, Events 32-64, offset 1014h
 */
#define EDMA3_ESR_REG(cc)  *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1010)
#define EDMA3_ESRH_REG(cc) *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1014)

/******************************************************************************/
/*! EDMA3 Event Enable Clear Registers (EECR/EECRH - Offset 0x1028/0x102C)
    - Used to disable events, Event Enable Registers (EER/EERH) are read only
    - [31:0]  EECR, Events 0-31, offset 1028h
    - [63:32] EECRH, Events 32-64, offset 102Ch
 */
#define EDMA3_EECR_REG(cc)  *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1028)
#define EDMA3_EECRH_REG(cc) *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x102C)

/******************************************************************************/
/*! EDMA3 Event Enable Set Registers (EESR/EESRH - Offset 0x1030/0x1034)
    - Used to enable events, Event Enable Registers (EER/EERH) are read only
    - [31:0]  EESR, Events 0-31, offset 1030h
    - [63:32] EESRH, Events 32-64, offset 1034h
 */
#define EDMA3_EESR_REG(cc)  *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1030)
#define EDMA3_EESRH_REG(cc) *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1034)

/******************************************************************************/
/*! EDMA3 Interrupt Enable Set Registers (IESR/IESRH - Offset 0x1060/0x1064)
    - Used to enable interrupts, Interrupt Registers (IER/IERH) are read only
    - [31:0]  IESR, Interrupts 0-31, offset 1060h
    - [63:32] EESRH, Interrupts 32-64, offset 1064h
 */
#define EDMA3_IESR_REG(cc)   *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1060)
#define EDMA3_IESRH_REG(cc)  *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1064)

/******************************************************************************/
/*! EDMA3 Interrupt Pending Registers (IPR/IPRH - Offset 0x1068/0x106C)
    - If the TCINTEN and/or ITCINTEN bit in the channel option parameter (OPT)
      is set in the PaRAM entry associated with the DMA channel, then the
      EDMA3TC(for normal completion) or the EDMA3CC(for early completion)
      returns a completion code on transfer or intermediate transfer completion.
      The value of the returned completion code is equal to the TCC bit in OPT
      for the PaRAM entry associated with the channel.
      When an interrupt transfer completion code with TCC=n is detected by the
      EDMA3CC, then the corresponding bit is set in the IPR/IPRH.
      Once set, remains set, must be cleared using ICR/ICRH.
    - [31:0]  IPR, Interrupts 0-31, offset 1068h
    - [63:32] IPRH, Interrupts 32-64, offset 106Ch
 */
#define EDMA3_IPR_REG(cc)   *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1068)
#define EDMA3_IPRH_REG(cc)  *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x106C)

/******************************************************************************/
/*! EDMA3 Interrupt Clear Registers (ICR/ICRH - Offset 0x1070/0x1074)
    - Used to clear Interrupt Pending Register (IPR/IPRH) bits
    - [31:0]  ICR, Interrupts 0-31, offset 1070h
    - [63:32] ICRH, Interrupts 32-64, offset 1074h
 */
#define EDMA3_ICR_REG(cc)  *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1070)
#define EDMA3_ICRH_REG(cc) *(VREG *)(EDMA_TPCC_BASE_ADDR(cc)+0x1074)


/****************************************************************************** 
 * *** SHOULD NOT BE NEEDED, JUST HERE FOR DEBUGGING PURPOSES *** 
#define EDMA3_QCHMAP0_REG *(VREG *)(EDMA_TPCC1_BASE_ADDR+0x200)
 * QDMA Event Enable Set Register * 
#define EDMA3_QEESR_REG *(VREG *)(EDMA_TPCC1_BASE_ADDR+0x108C)

#define EDMA3_CCSTAT_REG  *(VREG *)(EDMA_TPCC1_BASE_ADDR+0x640)
#define EDMA3_CCSTAT_ACTV (1 << 4)

#define EDMA_TPCC1_TC0_TCSTAT *(VREG *)(EDMA_TPCC1_TC0_BASE_ADDR+0x100)
#define EDMA_TPCC1_TC0_DFDST0 *(VREG *)(EDMA_TPCC1_TC0_BASE_ADDR+0x30C)
#define EDMA_TPCC1_TC0_DFDST1 *(VREG *)(EDMA_TPCC1_TC0_BASE_ADDR+0x34C)
#define EDMA_TPCC1_TC0_DFDST2 *(VREG *)(EDMA_TPCC1_TC0_BASE_ADDR+0x38C)
#define EDMA_TPCC1_TC0_DFDST3 *(VREG *)(EDMA_TPCC1_TC0_BASE_ADDR+0x3CC)


#define EDMA3_ECR_REG   *(VREG *)(EDMA_TPCC1_BASE_ADDR+0x1008)
#define EDMA3_ECRH_REG  *(VREG *)(EDMA_TPCC1_BASE_ADDR+0x100C)

#define EDMA3_ESR_REG   *(VREG *)(EDMA_TPCC1_BASE_ADDR+0x1010)
#define EDMA3_ESRH_REG  *(VREG *)(EDMA_TPCC1_BASE_ADDR+0x1014)


 ******************************************************************************/


/******************************************************************************
 *
 * INTERRUPT CONTROLLER (INTC0,INTC1,INTC2) ADDRESSES, DEFINITIONS, AND MACROS
 * 
 ******************************************************************************/

/*  External chip level interrupt controller base address values */
#define CIC_OFFSET      0x00004000
#define CIC0_BASE_ADDR  0x02600000
#define CIC1_BASE_ADDR  0x02604000
#define CIC2_BASE_ADDR  0x02608000
#define CIC3_BASE_ADDR  0x0260C000
#define CIC_BASE_ADDR(cx) (CIC0_BASE_ADDR+((cx)*CIC_OFFSET))

#define CIC1_STATUS_CLR_INDEX_REG (CIC1_BASE_ADDR+0x24)
#define CIC2_STATUS_CLR_INDEX_REG (CIC2_BASE_ADDR+0x24)
#define CIC3_STATUS_CLR_INDEX_REG (CIC3_BASE_ADDR+0x24)

/* System Input Event Values for external chip-level interrupt controller */
#if defined(C6670)

    #define CIC1_EVT_TETBHFULLINT  8
    #define CIC1_EVT_TETBFULLINT   9
    #define CIC1_EVT_TETBHFULLINT0 11
    #define CIC1_EVT_TETBFULLINT0  12
    #define CIC1_EVT_TETBHFULLINT1 14
    #define CIC1_EVT_TETBFULLINT1  15
    #define CIC1_EVT_TETBHFULLINT2 17
    #define CIC1_EVT_TETBFULLINT2  18
    #define CIC1_EVT_TETBHFULLINT3 20
    #define CIC1_EVT_TETBFULLINT3  21

    #define CIC2_EVT_TETBHFULLINT  16
    #define CIC2_EVT_TETBFULLINT   17
    #define CIC2_EVT_TETBHFULLINT0 19
    #define CIC2_EVT_TETBFULLINT0  20
    #define CIC2_EVT_TETBHFULLINT1 22
    #define CIC2_EVT_TETBFULLINT1  23
    #define CIC2_EVT_TETBHFULLINT2 25
    #define CIC2_EVT_TETBFULLINT2  26
    #define CIC2_EVT_TETBHFULLINT3 28
    #define CIC2_EVT_TETBFULLINT3  29

#elif defined(C6678)

    #define CIC2_EVT_TETBHFULLINT  8
    #define CIC2_EVT_TETBFULLINT   9
    #define CIC2_EVT_TETBHFULLINT0 11
    #define CIC2_EVT_TETBFULLINT0  12
    #define CIC2_EVT_TETBHFULLINT1 14
    #define CIC2_EVT_TETBFULLINT1  15
    #define CIC2_EVT_TETBHFULLINT2 17
    #define CIC2_EVT_TETBFULLINT2  18
    #define CIC2_EVT_TETBHFULLINT3 20
    #define CIC2_EVT_TETBFULLINT3  21
    #define CIC2_EVT_TETBHFULLINT4 118
    #define CIC2_EVT_TETBFULLINT4  119
    #define CIC2_EVT_TETBHFULLINT5 121
    #define CIC2_EVT_TETBFULLINT5  122
    #define CIC2_EVT_TETBHFULLINT6 124
    #define CIC2_EVT_TETBFULLINT6  125
    #define CIC2_EVT_TETBHFULLINT7 127
    #define CIC2_EVT_TETBFULLINT7  128

    #define CIC3_EVT_TETBHFULLINT  16
    #define CIC3_EVT_TETBFULLINT   17
    #define CIC3_EVT_TETBHFULLINT0 19
    #define CIC3_EVT_TETBFULLINT0  20
    #define CIC3_EVT_TETBHFULLINT1 22
    #define CIC3_EVT_TETBFULLINT1  23
    #define CIC3_EVT_TETBHFULLINT2 25
    #define CIC3_EVT_TETBFULLINT2  26
    #define CIC3_EVT_TETBHFULLINT3 28
    #define CIC3_EVT_TETBFULLINT3  29
    #define CIC3_EVT_TETBHFULLINT4 45
    #define CIC3_EVT_TETBFULLINT4  46
    #define CIC3_EVT_TETBHFULLINT5 48
    #define CIC3_EVT_TETBFULLINT5  49
    #define CIC3_EVT_TETBHFULLINT6 51
    #define CIC3_EVT_TETBFULLINT6  52
    #define CIC3_EVT_TETBHFULLINT7 54
    #define CIC3_EVT_TETBFULLINT7  55
	
#endif

/* **** CIC1 REGISTER ADDRESS VALUE DEFINITIONS **** */
#define CIC_ENABLE_GHINT_REG(cic)     (*(VREG *)((CIC_BASE_ADDR(cic))+0x10))
#define CIC_STATUS_REG(cic,os)        (*(VREG *)((CIC_BASE_ADDR(cic))+(0x200+((os)*4))))
#define CIC_CLEAR_REG(cic,os)         (*(VREG *)((CIC_BASE_ADDR(cic))+(0x280+((os)*4))))
#define CIC_CLEAR_REG_ADDR(cic,os)     ((CIC_BASE_ADDR(cic))+(0x280+((os)*4)))
#define CIC_ENABLE_REG(cic,os)        (*(VREG *)((CIC_BASE_ADDR(cic))+(0x300+((os)*4))))
#define CIC_ENABLE_HINT_REG(cic,os)   (*(VREG *)((CIC_BASE_ADDR(cic))+(0x1500+((os)*4))))

#define CIC_CHMAP_REG(cic,os) (*(volatile uint32_t *)((CIC_BASE_ADDR(cic))+(0x400+((os)*4))))
#define CIC_CHMAP_REG2(cic) (*(volatile uint32_t *)((CIC_BASE_ADDR(cic))+0x408))
#define CIC_CHMAP_REG3(cic) (*(volatile uint32_t *)((CIC_BASE_ADDR(cic))+0x40C))

#define CIC_STATUS_CLR_INDEX_REG(cic)     (*(VREG *)((CIC_BASE_ADDR(cic))+0x24))

#if defined(C6657)

#define EDMACC_TETBHFULLINT 40
#define EDMACC_TETBHFULLINT0 41
#define EDMACC_TETBHFULLINT1 42
#define EDMACC_TETBFULLINT 61
#define EDMACC_TETBFULLINT0 62
#define EDMACC_TETBFULLINT1 63

#endif

#if defined(C66AK2Hxx)

#define EDMACC3_TETBFULLINT0 32
#define EDMACC3_TETBHFULLINT0 33
#define EDMACC3_TETBFULLINT1 34
#define EDMACC3_TETBHFULLINT1 35
#define EDMACC3_TETBFULLINT2 36
#define EDMACC3_TETBHFULLINT2 37
#define EDMACC3_TETBFULLINT3 38
#define EDMACC3_TETBHFULLINT3 39

#define EDMACC2_TETBFULLINT4 6
#define EDMACC2_TETBHFULLINT4 7
#define EDMACC2_TETBFULLINT5 8
#define EDMACC2_TETBHFULLINT5 9
#define EDMACC2_TETBFULLINT6 10
#define EDMACC2_TETBHFULLINT6 11
#define EDMACC2_TETBFULLINT7 12
#define EDMACC2_TETBHFULLINT7 13

/* For 66AK2Hxx, TCI6638K2K, TCI6636K2H and 66AK2Exx */
#define EDMACC4_DBGTBR_DMAINT 30
#define EDMACC4_TETRISTBR_DMAINT 31

/* For TCI6630K2L */
#define EDMACC1_DBGTBR_DMAINT 4
#define EDMACC1_TETRISTBR_DMAINT 5

/* For 66AK2Exx - TIETB Events for DSP 0 are mapped to CC3*/
#define C66AK2E_EDMACC3_TETBFULLINT0 8
#define C66AK2E_EDMACC3_TETBHFULLINT0 9


#endif

#if defined(_66AK2Gxx)

#define EDMACC0_TETRISTBR_DMAINT 12
#define EDMACC0_DBGTBR_DMAINT 13
#define EDMACC0_TETBHFULLINT0 14
#define EDMACC0_TETBFULLINT0 15

#endif

#ifdef __cplusplus
}
#endif

#endif //__EDMA_DEV_C66XX_H
