/* 
 * File:   pwm.h
 * Author: Abdullah Almosalami
 * Comments: This header file and the corresponding source file are meant to initialize
 *           and drive the PWM peripherals of the dsPIC33CK64MC105
 * Revision history: 0.1
 */


/* ----------------------------------------------------------------------------
 * Description of the PWM Peripheral
 * ----------------------------------------------------------------------------
 * OK WARNING: You may have thought that PWM would be simple to configure. You
 * were wrong. This is one juicy peripheral with a lot of information up ahead.
 * Prepare yourself young one.
 * 
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
 * Architecture Overview
 * ---------------------
 * As with the registers, the PWM module overall contains a common set of controls
 * and features as well as multiple instances of PWM Generators (PGx). Each PG can
 * be independently configured or multiple PGs can be used to achieve complex
 * multiphase systems. Each PG consists of a signal generator and an Output Control
 * block.
 * 
 * By looking at Figure 3-2 of the family reference document, while it appears
 * quite nasty, you can observe a few things if you follow one item at a time.
 * For example, you can see where some of the common controls and individual
 * controls interact. For example, (top left) MPHASE, MDC, and MPER are all set 
 * in the common control registers, where as PGxPHASE, PGxDC, and PGxPER are for 
 * a single PG's own control registers. The MPHSEL, MDCSEL, and MPERSEL control 
 * lines help choose between which controls we want to use for the PG. We see
 * something similar for the PWM clock input.
 * 
 * We also see going into the central PWM Generator block a TRIG input, which has
 * a bunch of logic tied to it. Without knowing anything else, this likely has
 * something to do with triggering the PG to start some action. And then the
 * output of the PG is referred to as raw_pwm, which first goes through some output
 * control blocks like Complementary Mode Override, Output Control & Dead-Time Generator,
 * etc., before finally going out onto the pins. We also see feedback from the final
 * output into the triggering system. Ok, so how are we to make sense of all that...
 * 
 * Each PG consists of a) a signal generator that generates its signal based on
 * trigger inputs and control registers, and b) an Output Control block.
 * 
 * PWM Generator operation is based on triggers. To generate a PWM cycle, an
 * SOC (Start-of-Cycle) trigger must be received; this can either be self-triggered
 * or from an external source. Once the SOC event occurs, the PWMxH output goes
 * active until the internal counter reaches the duty cycle value, after which
 * the pin transitions to inactive until the EOC is reached, which is when the
 * internal counter reaches the period value. This internal counter goes off of
 * the PGxCLK selected for that PG.
 * 
 * Ok that was all the general idea. Let's get into the specifics of operation.
 * 
 * Operation
 * ---------
 * Each PG can independently select one of several clock sources. The input clock
 * is selected from the MCLKSEL bits in PCLKCON (e.g., FVCO/3, FPLLO, FVCO/2, FOSC)
 * Then the CLKSEL bits in PGxCONL are used to select the clock for the actual PG
 * module. IT SHOULD BE NOTED that the PWM module may run at a clock speed equal
 * to or faster than the CPU clock, which means that writes and reads to registers may
 * present with delayed behavior. See datasheet for necessary calculations
 * to select the clock, set the period, phase, trigger offset, duty cycle, and 
 * dead time.
 * 
 * To see further how the PWM signal is generated, note:
 *      - PGxPHASE determines the position of the PWM signal rising edge from the
 *        start of the timer count cycle (i.e., SOC).
 *      - PGxDC determines the position of the PWM signal falling edge from the
 *        start of the timer count cycle.
 *      - PGxPER determines the end of the PWM timer count cycle.
 * 
 * Going off of that, you can infer that:
 *      - If PGxPHASE = PGxDC, the line just stays low, because you're asking for
 *        the rising and falling edge of the pulse to be at the same point...
 *      - If PGxDC >= PGxPER, the line just stays high because the period sets the EOC
 *        and the counter never gets to PGxDC to set the falling edge.
 * 
 * Now there are actual various modes for generating the raw PWM signal from a PG,
 * and the mode is selected using the MODSEL bits in PGxCONL:
 *      1- Independent Edge PWM mode (default)
 *      2- Variable Phase PWM mode
 *      3- Independent Edge PWM mode, Dual Output
 *      4- Center-Aligned PWM mode
 *      5- Double Update Center-Aligned PWM mode
 *      6- Dual Edge Center-Aligned PWM mode
 * 
 * I think the most straight forward mode is the Independent Edge PWM mode, where
 * all the PGs are independent from each other and there is no complex triggering
 * scheme involved. You just set the period, duty cycle, and phase, enable the
 * module, and off it goes. You can vary the duty cycle by writing to PGxDC, and
 * likewise with the phase.
 * 
 * For controlling the output onto the PWMxH and PWMxL pins, there are three modes,
 * which are selected through the PMOD bits in PGxIOCONH:
 *      1- Complementary Output mode (default)
 *      2- Independent Output mode
 *      3- Push-Pull Output mode
 * 
 * For my application, I am driving half H-bridges, so Complementary Output mode
 * makes sense. In this mode, the PWMxH and PWMxL signals are never active at the
 * same time. Also, a dead-time switching delay may be inserted between the two
 * signals and is controlled by the PGxDT register.
 * 
 * Ok, so once you set the signal-specific details up, how do you actually have
 * the dsPIC33 start outputting the signal on the pins?
 * 
 * Trigger Operation
 * A PWM cycle ONLY STARTS when it receives a SOC trigger. This SOC trigger can
 * come from MANY sources, which allows for some crazy shit. But, for my purposes,
 * I just want the cycle to start again as soon as it ends. This is called
 * self-triggering, where the EOC signal is also the SOC signal for the next
 * PWM cycle. This trigger is selected by having the SOCS bits be 0000b, which
 * is also the default state! The EOC signal is also generated when the ON bit in
 * PGxCONL[15] has been set, which is how you get the PWM output started!
 * 
 * But wait, we want three PWM signals, and we want them to sync up in period...
 * So how can we ensure they are in-sync? This can be done by having a PG host
 * trigger other PGs. For example, you could have phase A's PWM signal come from
 * the host PG, which you set as internally triggered as described in the
 * previous paragraph. And you can then set the phase B and C PWM signals to get
 * triggered by the EOC signal from the host PG! Only the first PWM cycle will
 * be missed, but afterwards, they will all be in sync. To set the trigger of the
 * client PGs to be a host's PG, you set the client PGs SOCS bits to have the host
 * be the trigger and for the host, you set the PGTRGSEL[2:0] bits in PGxEVT[2:0]
 * to 000b.
 * 
 * Finally, you will want to vary the duty cycle in order to actually produce
 * the SPWM signal you desire. In order to accomplish this, you will likely
 * want to keep track of the number of PWM cycles that have occurred. For example,
 * if you want to be able to split a 60Hz sine wave cycle into 100 sections, where
 * each section has a designated duty cycle, then you'll want to change the duty
 * cycle every ~166.667 us. With a PWM frequency of 120kHz, you'll change the duty
 * cycle every 2000 PWM periods. I'm not sure if there is a way to configure all
 * this within the PWM module, but for now, I'll use an EOC interrupt event to
 * increment a counter in an ISR and once the counter has reached 2000, I'll set
 * the PGxDC register to the next value in an array look-up table. To configure
 * PWM interrupts (section 4.2.10.2 in the reference manual), the IEVTSEL bits
 * within PGxEVTH[9:8] allow the user to select one of the following to trigger
 * act as an interrupt event: a) EOC, b) TRIGA compare event, c) ADC Trigger event,
 * d) none (disabled).
 * 
 * Ok, but if you are updating the duty cycle while the module is running, will there
 * be some issues with doing that? What happens when you change the duty cycle in
 * the middle of a PWM cycle? This is why the PWM module allows buffering certain SFRs, so
 * that the user application can modify the SFRs while the PWM module uses the previously
 * latched values, and upon the start of the next PWM cycle, the new values will be
 * latched and used. The PGxDC SFR is included in this. To enable this, the UPDMOD
 * bits in PGxCONH should be set to 000 (default). However, in order to actually
 * prepare this transfer operation, the UPDREQ bit within the PGxSTAT register
 * must be set... To have this be automatically set as soon as you write to the
 * register, you will want to set the UPDTRG bits in PGxEVTL to choose which
 * register write will automatically set the UPDREQ bit. A very common register to
 * choose is the PGxDC, as many applications vary duty cycle.
 * 
 * But wait, you are updating three PWM modules at once. Hmm... And you have set them
 * up as host-client. In this case, what you're gonna want to do is set the MSTEN
 * bit in PGxCONH of the host PG. Then, when the host PG's UPDREQ bit is set,
 * this is effectively broadcast to the client PGs as well. The client PGs need
 * to have their UPDMOD bits set to 010b instead of 000b for this operation!
 * 
 * Alright, that was a motherload of information, and is also one of the more basic
 * uses of the PWM peripheral. Zoinks! But let's get it!
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
#define PWM_SET_FREQ_SCALE(x)           (FSCL = (x))

/* FSMINPER - Frequency Scaling Minimum Period Register -  Register 11-3 in datasheet
 * Default/POR: 00000000
 *              00000000
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |...........................FSMINPER..............................................................|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |.................................................................................................|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 *      (R/W) FSMINPER : Frequency Scaling Minimum Period bits -- This holds the minimum clock period
 *                      (max clock frequency) that can be produced by the frequency scaling circuit.
 * 
 */
#define PWM_SET_FREQ_SCALE_MIN_PERIOD(x)           (FSMINPER = (x))

/* MPHASE - Master Phase Register -  Register 11-4 in datasheet
 * Default/POR: 00000000
 *              00000000
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |...........................MPHASE................................................................|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |.................................................................................................|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 *      (R/W) MPHASE : Master Phase bits -- This holds the phase offset value that can be shared by
 *                      multiple PWM Generators
 * 
 */
#define PWM_SET_MASTER_PHASE(x)        (MPHASE = (x))

/* MDC - Master Duty Cycle Register -  Register 11-5 in datasheet
 * Default/POR: 00000000
 *              00000000
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |...........................MDC...................................................................|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |.................................................................................................|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 *      (R/W) MDC : Master Duty Cycle bits -- Holds the duty cycle value that can be shared by multiple
 *                  PWM generators --> SHOULD NOT BE LESS THAN 0x0008u
 * 
 */
#define PWM_SET_MASTER_DUTY_CYCLE(x)       (MDC = (x))

/* MPER - Master Period Register -  Register 11-6 in datasheet
 * Default/POR: 00000000
 *              00000000
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |...........................MPER..................................................................|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |.................................................................................................|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 *      (R/W) MPHASE : Master Period bits -- This holds the period offset value that can be shared by
 *                      multiple PWM Generators
 * 
 */
#define PWM_SET_MASTER_PERIOD(x)        (MPER = x)

/* CMBTRIGL - Combinational Trigger Register Low -  Register 11-7 in datasheet
 * Default/POR: xxxxxxxx
 *              xxxx 0 0 0 0
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |...........................undef...............................................................|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |.................................................|...CTA4EN..|...CTA3EN..|...CTA2EN..|...CTA1EN..|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 *      (R/W) CTAxEN : Enable Trigger Output from PWM Generator x as source for Combinational Trigger A bit
 *                      1 = Enables specified trigger signal to be OR'd into the Combinational Trigger A signal
 *                      0 = Disabled
 * 
 */
//TODO

/* CMBTRIGH - Combinational Trigger Register HIGH -  Register 11-8 in datasheet
 * Default/POR: xxxxxxxx
 *              xxxx 0 0 0 0
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |...........................undef...............................................................|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |.................................................|...CTB4EN..|...CTB3EN..|...CTB2EN..|...CTB1EN..|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 *      (R/W) CTBxEN : Enable Trigger Output from PWM Generator x as source for Combinational Trigger B bit
 *                      1 = Enables specified trigger signal to be OR'd into the Combinational Trigger B signal
 *                      0 = Disabled
 * 
 */

/* LOGCONy - Combinational PWM Logic Control -  Register 11-9 in datasheet
 * Note: Recall 'y' denotes common function instance (A to F).
 *      - y = A, C, E assigns logic function to the PWMxH pin
 *      - y = B, D, F assigns logic function to the PWMxL pin
 * Default/POR: 0000 0000
 *              0 0 00 x000
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |....................PWMS1y.......................|...................PWMS2y......................|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |...S1yPOL...|...S2yPOL...|..........PWMLFy.......|....undef..|................PWMLFyD............|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 *      (R/W) PWMS1y : Combinatorial PWM Logic Source #1 Selection bits
 *                      F -> 8 = Reserved
 *                      7 = PWM4L
 *                      6 = PWM4H
 *                      5 = PWM3L
 *                      4 = PWM3H
 *                      3 = PWM2L
 *                      2 = PWM2H
 *                      1 = PWM1L
 *                      0 = PWM1H
 * 
 *      (R/W) PWMS2y : Combinatorial PWM Logic Source #2 Selection bits
 *                      F -> 8 = Reserved
 *                      7 = PWM4L
 *                      6 = PWM4H
 *                      5 = PWM3L
 *                      4 = PWM3H
 *                      3 = PWM2L
 *                      2 = PWM2H
 *                      1 = PWM1L
 *                      0 = PWM1H
 * 
 *      (R/W) S1yPOL or S2yPOL : Combinatorial PWM Logic Source #1 or #2 Polarity bit
 *                      1 = Input is inverted, 0 = Input is positive logic
 * 
 *      (R/W) PWMLFy : Combinatorial PWM Logic Function Selection bits
 *                      11 = Reserved
 *                      10 = PWMS1y ^ PWMS2y (XOR)
 *                      01 = PWMS1y & PWMS2y (AND)
 *                      00 = PWMS1y | PWMS2y (OR)
 * 
 *      (R/W) PWMLFyD : Combinatorial PWM Logic Destination Selection bits
 *                      111 - 100 = Reserved
 *                      011 = Logic function is assigned to PWM4H or PWM4L pin
 *                      010 = Logic function is assigned to PWM3H or PWM3L pin
 *                      001 = Logic function is assigned to PWM2H or PWM2L pin
 *                      000 = Logic function is assigned to PWM1H or PWM1L pin
 * 
 */




// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="PWM GENERATOR-SPECIFIC REGISTERS & MACROS">
/* PGxCONL - PWM Generator x Control Register Low -  Register 11-12 in datasheet
 * Default/POR: 0 xxxx 000
 *              xxx 00 000
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |...ON.......|......................undef.....................|...............TRGCNT..............|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |.................undef...............|.........CLKSEL........|.............MODSEL................|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 * 
 *      (R/W) ON : PWMx Enable bit
 *      (R/W) TRGCNT : Trigger Count select bits
 *              111 = PWM Generator produces eight PWM cycles after getting triggered
 *              110 = "" seven
 *              101 = "" six
 *              100 = "" five
 *              011 = "" four
 *              010 = "" three
 *              001 = "" two
 *              000 = "" one
 *      (R/W) CLKSEL : Clock Selection bits
 *              11 = PG uses Master Clock scaled by frequency scaling circuit
 *              10 = PG uses Master clock divided by clock divider circuit
 *              01 = PG uses Master clock (selected by MCLKSEL in PCLKCON)
 *              00 = No clock, PG is in Low Power state (default)
 *      (R/W) MODSEL : Mode Selection bits
 *              111 = Dual Edge Center-Aligned PWM mode (interrupt/register update twice per cycle)
 *              110 = Dual Edge Center-Aligned PWM mode ("" once per cycle)
 *              101 = Double-Update Center-Aligned PWM mode
 *              100 = Center-Aligned PWM mode
 *              011 = Reserved
 *              010 = Independent Edge PWM mode, dual output
 *              001 = Variable Phase PWM mode
 *              000 = Independent Edge PWM mode
 * 
 */
#define PWM1_ON                 (PG1CONLbits.ON = 1u)
#define PWM2_ON                 (PG2CONLbits.ON = 1u)
#define PWM3_ON                 (PG3CONLbits.ON = 1u)

#define PWM1_TRIGGER_COUNT_1    (PG1CONLbits.TRGCNT = 0u)
#define PWM2_TRIGGER_COUNT_1    (PG2CONLbits.TRGCNT = 0u)
#define PWM3_TRIGGER_COUNT_1    (PG3CONLbits.TRGCNT = 0u)
// More TODO

#define PWM1_CLKSEL_MASTER_CLK_DIV      (PG1CONLbits.CLKSEL = 2u)
#define PWM2_CLKSEL_MASTER_CLK_DIV      (PG2CONLbits.CLKSEL = 2u)
#define PWM3_CLKSEL_MASTER_CLK_DIV      (PG3CONLbits.CLKSEL = 2u)
// More TODO

#define PWM1_INDEPENDENT_EDGE_MODE      (PG1CONLbits.MODSEL = 0u)
#define PWM2_INDEPENDENT_EDGE_MODE      (PG2CONLbits.MODSEL = 0u)
#define PWM3_INDEPENDENT_EDGE_MODE      (PG3CONLbits.MODSEL = 0u)
// More TODO

/* PGxCONH - PWM Generator x Control Register High -  Register 11-13 in datasheet
 * Default/POR: 0 0 0 x 0 000
 *              x 0 xx 0000
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |...MDCSEL...|...MPERSEL..|...MPHSEL..|...undef...|...MSTEN...|..............UPMODE...............|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |...undef....|...TRGMOD...|.........undef.........|.....................SOCS......................|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 * 
 *      (R/W) MDCSEL : Master Duty Cycle register select --> 1 = PG uses MDC register, 0 = PG uses PGxDC
 *      (R/W) MDPERSEL : Master Period register select ""
 *      (R/W) MPHSEL : Master Phase register select ""
 *      (R/W) MSTEN : Master Update Enable bit --> 1 = PG broadcasts software set/clear of the UPDREQ bit and
 *                                                     EOC signal to other PGs - i.e., PG acts as Master Update.
 *                                                     Update is broadcast when MSTEN = 1 and UPDREQ = 1.
 *                                                 0 = No broadcast performed
 *      (R/W) UPMODE : PWM Buffer Update Mode Selection bits
 *                  011 = Client immediate update --> Updates data registers immediately, or as soon as possible,
 *                                                    when Master update request is received.
 *                  010 = Client SOC update --> Updates data registers at the start of the next cycle if a Master
 *                                              update request is received.
 *                  001 = Immediate update --> Updates data registers immediately, or asap, if UPDREQ = 1 for this PG
 *                  000 = SOC Update --> Updates data registers at start of next PWM cycle if UPDREQ = 1 for this PG
 *      (R/W) TRGMOD : PG Trigger Mode Selection bit --> 1 = PG operates in Retriggerable mode, 0 = Single Trigger mode
 *      (R/W) SOCS : Start-of-Cycle Selection bits
 *                  F = TRIG bit or PCI Sync function only (no hardware trigger source is selected)
 *                  E -> 5 = Reserved
 *                  4 = Trigger output selected by PG4 PGTRGSEL[2:0] bits in PGxEVTL[2:0]
 *                  3 = "" PG3
 *                  2 = "" PG2
 *                  1 = "" PG1
 *                  0 = Local EOC --> PG is self-triggered
 *
 */
// </editor-fold>
#define PWM1_USE_MASTER_DUTY_CYCLE           (PG1CONHbits.MDCSEL = 1u)
#define PWM2_USE_MASTER_DUTY_CYCLE           (PG2CONHbits.MDCSEL = 1u)
#define PWM3_USE_MASTER_DUTY_CYCLE           (PG3CONHbits.MDCSEL = 1u)
#define PWM1_USE_OWN_DUTY_CYCLE              (PG1CONHbits.MDCSEL = 0u)
#define PWM2_USE_OWN_DUTY_CYCLE              (PG2CONHbits.MDCSEL = 0u)
#define PWM3_USE_OWN_DUTY_CYCLE              (PG3CONHbits.MDCSEL = 0u)

#define PWM1_USE_MASTER_PERIOD               (PG1CONHbits.MPERSEL = 1u)
#define PWM2_USE_MASTER_PERIOD               (PG2CONHbits.MPERSEL = 1u)
#define PWM3_USE_MASTER_PERIOD               (PG3CONHbits.MPERSEL = 1u)
// More TODO

#define PWM1_USE_MASTER_PHASE                (PG1CONHbits.MPHSEL = 1u)
#define PWM2_USE_MASTER_PHASE                (PG2CONHbits.MPHSEL = 1u)
#define PWM3_USE_MASTER_PHASE                (PG3CONHbits.MPHSEL = 1u)
// More TODO

#define PWM1_MASTER_UPDATE_ENABLE            (PG1CONHbits.MSTEN = 1u)
// More TODO
#define PWM1_UPDATE_ON_SOC                   (PG1CONHbits.UPDMOD = 0u)
// More TODO
#define PWM2_UPDATE_ON_MASTER_REQ_SOC        (PG2CONHbits.UPDMOD = 2u)
#define PWM3_UPDATE_ON_MASTER_REQ_SOC        (PG3CONHbits.UPDMOD = 2u)
// More TODO
// I'll keep the below macros here for now since they are related to the above macros
#define PWM1_MANUAL_UPDATE_REQUEST           (PG1STATbits.UPDREQ = 1u)
#define PWM2_MANUAL_UPDATE_REQUEST           (PG2STATbits.UPDREQ = 1u)
#define PWM3_MANUAL_UPDATE_REQUEST           (PG3STATbits.UPDREQ = 1u)

#define PMW1_SELF_TRIGGER                    (PG1CONHbits.SOCS = 0u)
//More TODO
#define PWM2_PG1_TRIGGER                     (PG2CONHbits.SOCS = 1u)
#define PWM3_PG1_TRIGGER                     (PG3CONHbits.SOCS = 1u)
// More TODO
// I'll just put the below macros here for now since they are related to the above macros
#define PWM1_MANUAL_TRIGGER                  (PG1STATbits.TRSET = 1u)
#define PWM2_MANUAL_TRIGGER                  (PG2STATbits.TRSET = 1u)
#define PWM3_MANUAL_TRIGGER                  (PG3STATbits.TRSET = 1u)

/* PGxIOCONH - PWM Generator x I/O Control Register High -  Register 11-16 in datasheet
 * Default/POR: x 000 xxx 0
 *              xx 00 0 0 0 0
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 15  |    bit 14  |   bit 13  |   bit 12  |   bit 11  |   bit 10  |   bit 9   |   bit 8   |
 * +-------------------------------------------------------------------------------------------------+
 * |...undef....|...............CAPSRC...............|...............undef...............|..DTCMPSEL.|
 * +-------------------------------------------------------------------------------------------------+
 * +-------------------------------------------------------------------------------------------------+
 * |    bit 7   |    bit 6   |   bit 5   |   bit 4   |   bit 3   |   bit 2   |   bit 1   |   bit 0   |
 * +-------------------------------------------------------------------------------------------------+
 * |..........undef..........|........PMOD...........|...PENH....|...PENL....|...POLH....|...POLL....|
 * +-------------------------------------------------------------------------------------------------+
 * This register contains:
 * 
 *      (R/W) CAPSRC : Time Base Capture Source Selection bits
 *          ...
 *      (R/W) DTCMPSEL : Dead-Time Compensation bit
 *      (R/W) PMOD[1:0] : PG Output Mode selection
 *                  11 = Reserved
 *                  10 = Push-Pull mode
 *                  01 = Independent mode
 *                  00 = Complementary mode
 *      (R/W) PENH : PWMxH Output Port Enable
 *      (R/W) PENL : PWMxL Output Port Enable
 *      (R/W) POLH : PWMxH Output Polarity --> 1 = active-low, 0 = active-high
 *      (R/W) POLL : PWMxL Output Polarity --> ""
 * 
 */
#define PWM1_OUTPUT_COMPLEMENTARY       (PG1IOCONHbits.PMOD = 0u)
#define PWM2_OUTPUT_COMPLEMENTARY       (PG2IOCONHbits.PMOD = 0u)
#define PWM3_OUTPUT_COMPLEMENTARY       (PG3IOCONHbits.PMOD = 0u)
// more TODO

#define PWM1H_PIN_ENABLE                (PG1IOCONHbits.PENH = 1u)
#define PWM2H_PIN_ENABLE                (PG2IOCONHbits.PENH = 1u)
#define PWM3H_PIN_ENABLE                (PG3IOCONHbits.PENH = 1u)
#define PWM1L_PIN_ENABLE                (PG1IOCONHbits.PENL = 1u)
#define PWM2L_PIN_ENABLE                (PG2IOCONHbits.PENL = 1u)
#define PWM3L_PIN_ENABLE                (PG3IOCONHbits.PENL = 1u)

#define PWM1H_PIN_POLARITY_ACTIVE_HIGH   (PG1IOCONHbits.POLH = 0u)
#define PWM2H_PIN_POLARITY_ACTIVE_HIGH   (PG2IOCONHbits.POLH = 0u)
#define PWM3H_PIN_POLARITY_ACTIVE_HIGH   (PG3IOCONHbits.POLH = 0u)
#define PWM1L_PIN_POLARITY_ACTIVE_HIGH   (PG1IOCONHbits.POLL = 0u)
#define PWM2L_PIN_POLARITY_ACTIVE_HIGH   (PG2IOCONHbits.POLL = 0u)
#define PWM3L_PIN_POLARITY_ACTIVE_HIGH   (PG3IOCONHbits.POLL = 0u)
#define PWM1L_PIN_POLARITY_ACTIVE_LOW    (PG1IOCONHbits.POLL = 1u)
#define PWM2L_PIN_POLARITY_ACTIVE_LOW    (PG2IOCONHbits.POLL = 1u)
#define PWM3L_PIN_POLARITY_ACTIVE_LOW    (PG3IOCONHbits.POLL = 1u)
// more TODO

 
// General useful macros
#define PWM1_UPDATE_ON_WRITE_TO_DUTY_CYCLE      (PG1EVTLbits.UPDTRG = 1u)
#define PWM1_UPDATE_DUTY_CYCLE(X)               (PG1DC = (X))
#define PWM2_UPDATE_DUTY_CYCLE(X)               (PG2DC = (X))
#define PWM3_UPDATE_DUTY_CYCLE(X)               (PG3DC = (X))

// Interrupt-related
#define PWM1_EOC_INTERRUPT_SELECT       (PG1EVTHbits.IEVTSEL = 0u)
#define PWM1_INTERRUPT_PRIORITY(X)      (IPC16bits.PWM1IP = (X))
#define PWM1_CLEAR_IF                   (IFS4bits.PWM1IF = 0u)
#define PWM1_ENABLE_INTERRUPT           (IEC4bits.PWM1IE = 1u)



// Enums


// Functions
void pwm_init_default(void);


#endif	/* PWM_H */

