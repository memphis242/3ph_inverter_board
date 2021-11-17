/* 
 * File:   pwm.h
 * Author: Abdullah Almosalami
 * Comments: This header file and the corresponding source file are meant to initialize
 *           and drive the PWM peripherals of the dsPIC33CK64MC105
 * Revision history: 0.1
 */


/* ----------------------------------------------------------------------------
 * Brief Description of the PWM Peripheral
 * ----------------------------------------------------------------------------
 * The dsPIC33 and PIC24 devices feature High-Resolution PWM peripherals, meant
 * to be a part of motor drive schemes, inverters, DC/DC conversion, and so on.
 * 
 * There are two types of registers associated with the PWM peripherals:
 *      1- Common (i.e., shared by all PWM Generators)
 *      2- PWM Generator-specific
 * An 'x' in the register name denotes an instance of a PWM Generator, and a 'y'
 * in the register name denotes an instance of a common function. See below for
 * an outline of all the registers. Note that the DS70005320D document was used
 * as the main reference document for the information I used to write these files.
 * 
 * 
 */
  
#ifndef PWM_H
#define	PWM_H

#include <xc.h> // include processor files - each processor file is guarded.  


// <editor-fold defaultstate="collapsed" desc="COMMON REGISTERS & MACROS">
/* PCLKCON - PWM Clock Control Register -  Register 11-1 in datasheet
 * Default/POR: xxxxxxx 0
 *              xx 00 xx 00
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |........................................undef........................................|...LOCK....|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |..........undef..........|.........DIVSEL........|........undef..........|.......MCLKSEL.........|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 *      (R/W) LOCK : Lock bit -- 1 = Write-protected registers and bits are locked, 0 = unlocked
 *      (R/W) DIVSEL : PWM Clock Divider Selection bits
 *                  11 = Divide by 16
 *                  10 = Divide by 8
 *                  01 = Divide by 4
 *                  00 = Divide by 2
 *      (R/W) MCLKSEL : PWM Master Clock Selection bits
 *                  11 = FVCO / 3
 *                  10 = FPLLO
 *                  01 = FVCO / 2
 *                  00 = FOSC
 * 
 */
#define PCLKCON_LOCK_BIT            (PCLKCONbits.LOCK)
#define PWM_LOCK_REGISTERS          (PCLKCONbits.LOCK = 1u)
#define PWM_UNLOCK_REGISTERS        (PCLKCONbits.LOCK = 0u)
#define PCLKCON_DIVSEL_BITS         (PCLKCONbits.DIVSEL)
#define PWM_CLKDIV16                (PCLKCONbits.DIVSEL = 3u)
#define PWM_CLKDIV8                 (PCLKCONbits.DIVSEL = 2u)
#define PWM_CLKDIV4                 (PCLKCONbits.DIVSEL = 1u)
#define PWM_CLKDIV2                 (PCLKCONbits.DIVSEL = 0u)
#define PCLKCON_MCLK_BITS           (PCLKCONbits.MCLKSEL)
#define PWM_CLK_FVCO3               (PCLKCONbits.MCLKSEL = 3u)
#define PWM_CLK_FPLLO               (PCLKCONbits.MCLKSEL = 2u)
#define PWM_CLK_FVCO2               (PCLKCONbits.MCLKSEL = 1u)
#define PWM_CLK_FOSC                (PCLKCONbits.MCLKSEL = 0u)

/* FSCL - Frequency Scale Register -  Register 11-2 in datasheet
 * Default/POR: 00000000
 *              00000000
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |...........................FSCL..................................................................|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |.................................................................................................|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 *      (R/W) FSCL : Frequency Scale bits -- This value is added to the frequency scaling accumulator at
 *                                           each pwm_master_clk. When the accumulated value exceed the 
 *                                           value of FSMINPER, a clock pulse is produced.
 * 
 */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="PWM GENERATOR-SPECIFIC REGISTERS & MACROS">
/* CCPxCON - CCPx Control Register -  Register 15-1 in Datasheet
 * Default/POR: 00 00 0000
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |..........undef..........|.........DCxB..........|.....................CCPxM.....................|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 * 
 */


// </editor-fold>

 
// Enums


// Functions



#endif	/* PWM_H */

