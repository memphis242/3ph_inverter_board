/*
 * File:   pwm.c
 * Author: Abdullah Almosalami
 *
 * Created on November 17, 2021 4:37 PM
 */


#include <xc.h>
#include "pwm.h"
#include "duty_cycle.h"


// Global and static variables relevant to pwm operation
uint16_t pwm_cycle_counter = 0x0000u;


// Functions
/**
 * <h3>Function: pwm_init_default</h3>
 * 
 * <p>Initializes the PWM peripherals to their default state. I defined default as:
 * Utilize a master clock of 120MHz to set a PWM frequency of 120kHz. I will
 * want three PWM half H-bridge signals, so three PWM signals with complementary
 * outputs. This will require setting a host PG with two client PGs, for
 * triggering as well as for duty cycle updates. I will also have all the
 * PG's generate their PWM signal in Independent Edge PWM mode. So, based off of
 * that:</p>
 * <ul>
 *      <li>Use a master period of 999 (120MHz / 120kHz - 1)</li>
 *      <li>Use a master phase shift of 0, since I'm not interested in any phase
 *        shift between individual PWM signals in a given PWM cycle; the 120deg
 *        phase shift will be of the sine waves I'm attempting to trace out, and
 *        the lookup table array will take care of that.</li>
 *      <li>Use individual duty cycles for each PG that will change every 2000 PWM
 *        cycles. For this, I will use phase A's EOC event as an interrupt, for whose
 *        ISR will involve incrementing a counter and if the counter reaches 2000,
 *        initiate an update to duty cycle values of the PGs. Maybe to account for
 *        the time it takes to perform this updating, I'll lower the counter top
 *        to something like 1990 or so. Some experimentation will be needed.</li>
 *      <li>Set up Independent Edge PWM mode for all the PGs</li>
 *      <li>Set up Complementary Output mode for all the PGs --> will need to insert
 *          a dead time of about >30ns according to my calculations. I'll choose
 *          60ns. That amounts fo about 7.2 F_PGxCLK cycles. I'll go with 8.</li>
 *      <li>Set up PGA as the host-PG whose SOC triggers PGB and PGC</li>
 *      <li>Set up PGA as the host for whom setting the UPDREQ bit will also
 *          cause updates to the client PG registers.</li>
 *      <li>Load with initial duty cycle values!</li>
 *      <li>Start 'er up!</li>
 * 
 * @param none
 * @return non
 * 
 */
void pwm_init_default(void) {
    
    /* ---------------------
     * Master configuration
       --------------------- */
    /* Set the master clock input to be 120 MHz
     * I'm assuming I've set up FPLLO to be 480 MHz, so then I can just use
     * FPLLO and divide by 4.
     */
    PCLKCONbits.MCLKSEL = 0x2u;  // FPLLO
    PCLKCONbits.DIVSEL = 0x1u;   // Divide ratio is 1:4
    
    // Set PWM Master period
    MPER = 999;     // F_PG_CLK / F_PWM - 1 = (120MHz) / (120kHz) - 1
    
    // Set Master phase shift
    MPHASE = 0u;
    
    
    /* ----------------------------
     * Individual PG configuration
       ----------------------------*/
    // Select master clock input as source, without any changes
    
    
    PG1CONLbits.TRGCNT = 0u;    // PG produces one PWM cycle after SOC trigger
    PG2CONLbits.TRGCNT = 0u;
    PG3CONLbits.TRGCNT = 0u;
    PG1CONLbits.CLKSEL = 2u;    // Master clock is selected
    PG2CONLbits.CLKSEL = 2u;
    PG3CONLbits.CLKSEL = 2u;
    PG1CONLbits.MODSEL = 0u;    // Independent Edge PWM mode
    PG2CONLbits.MODSEL = 0u;
    PG3CONLbits.MODSEL = 0u;
    
    PG1CONHbits.MDCSEL = 0u;    // Duty cycle from PGxDC
    PG2CONHbits.MDCSEL = 0u;
    PG3CONHbits.MDCSEL = 0u;
    PG1CONHbits.MPERSEL = 1u;   // Master Period register used
    PG2CONHbits.MPERSEL = 1u;
    PG3CONHbits.MPERSEL = 1u;
    PG1CONHbits.MPHSEL = 1u;    // Master Phase register used
    PG2CONHbits.MPHSEL = 1u;
    PG3CONHbits.MPHSEL = 1u;
    
    PG1IOCONHbits.PMOD = 0u;    // Complementary output mode
    PG2IOCONHbits.PMOD = 0u;
    PG3IOCONHbits.PMOD = 0u;
    
    // Set dead-time to about >60ns since we're using complementary outputs
    // I'll note that the DRV8300 actually automatically inserts a 200ns dead-time
    // if it detects both PWMH and PWML pins are both HIGH at the same time.
    PG1DTL = 10u;   // For PWMxL
    PG2DTL = 10u;
    PG3DTL = 10u;
    PG1DTH = 10u;   // For PWMxH
    PG2DTH = 10u;
    PG3DTH = 10u;
    
    // Enable the PWM modules to control their respective PWMxH and PWMxL pins
    PG1IOCONHbits.PENH = 1u;    // PWMxH pin
    PG2IOCONHbits.PENH = 1u;
    PG3IOCONHbits.PENH = 1u;
    PG1IOCONHbits.PENL = 1u;    // PWMxL pin
    PG2IOCONHbits.PENL = 1u;
    PG3IOCONHbits.PENL = 1u;
    
    PG1IOCONHbits.POLH = 0u;    // Pin is active-high
    PG2IOCONHbits.POLH = 0u;
    PG3IOCONHbits.POLH = 0u;
    PG1IOCONHbits.POLL = 0u;
    PG2IOCONHbits.POLL = 0u;
    PG3IOCONHbits.POLL = 0u;
    
    
    // Set up PG1 as host and PG2 and PG3 as clients for trigger setup
    PG1CONHbits.SOCS = 0u;  // Self-trigger for PG1
    PG2CONHbits.SOCS = 1u;  // PG1-triggered
    PG3CONHbits.SOCS = 1u;  // PG1-triggered
    
    
    // Configure SFR updates from PG1DC
    PG1CONHbits.MSTEN = 1u;     // PG1 as Master update for UPDREQ
    PG1EVTLbits.UPDTRG = 1u;    // A write to the PG1DC register automatically sets UPDREQ in PG1STAT
    PG1CONHbits.UPDMOD = 0u;
    PG2CONHbits.UPDMOD = 2u;
    PG3CONHbits.UPDMOD = 2u;
    
    // Set up interrupts at PG1 EOC --> Note this is the PWM Generator 1 interrupt,
    // with the reserved XC16 ISR name _PWM1Interrupt, vector #75
    PG1EVTHbits.IEVTSEL = 0u;   // EOC will generate interrupt from PG1
    IPC16bits.PWM1IP = 6u;      // Priority level 6
    IFS4bits.PWM1IF = 0u;       // Clear flag
    IEC4bits.PWM1IE = 1u;       // Enable interrupt
    
    // Load up with initial duty cycle values...
    PWM2_UPDATE_DUTY_CYCLE(500);
    PWM3_UPDATE_DUTY_CYCLE(500);
    PWM1_UPDATE_DUTY_CYCLE(500);
    
    //-------------------------------------------------------------------------
    // Enable all the PGs!
    PG2CONLbits.ON = 1u;
    PG2CONLbits.ON = 1u;
    // PG1 is last to turn on since it acts as the host PG
    PG1CONLbits.ON = 1u;
}
