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
    PWM_CLK_FPLLO;  // FPLLO
    PWM_CLKDIV4;    // Divide ratio is 1:4
    
    // Set PWM Master period
    PWM_SET_MASTER_PERIOD(999);     // F_PG_CLK / F_PWM - 1 = (120MHz) / (120kHz) - 1
    // Set Master phase shift
    PWM_SET_MASTER_PHASE(0u);
    
    
    /* ----------------------------
     * Individual PG configuration
       ----------------------------*/
    // Select master clock input as source, without any changes
    
    
    PWM1_TRIGGER_COUNT_1;    // PG produces one PWM cycle after SOC trigger
    PWM2_TRIGGER_COUNT_1;
    PWM3_TRIGGER_COUNT_1;
    PWM1_CLKSEL_MASTER_CLK_DIV;    // Master clock is selected
    PWM2_CLKSEL_MASTER_CLK_DIV;
    PWM3_CLKSEL_MASTER_CLK_DIV;
    PWM1_INDEPENDENT_EDGE_MODE;    // Independent Edge PWM mode
    PWM2_INDEPENDENT_EDGE_MODE;
    PWM3_INDEPENDENT_EDGE_MODE;
    
    PWM1_USE_OWN_DUTY_CYCLE;    // Duty cycle from PGxDC
    PWM2_USE_OWN_DUTY_CYCLE;
    PWM3_USE_OWN_DUTY_CYCLE;
    PWM1_USE_MASTER_PERIOD;   // Master Period register used
    PWM2_USE_MASTER_PERIOD;
    PWM3_USE_MASTER_PERIOD;
    PWM1_USE_MASTER_PHASE;    // Master Phase register used
    PWM2_USE_MASTER_PHASE;
    PWM3_USE_MASTER_PHASE;
    
    PWM1_OUTPUT_COMPLEMENTARY;    // Complementary output mode
    PWM2_OUTPUT_COMPLEMENTARY;
    PWM3_OUTPUT_COMPLEMENTARY;
    
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
    PWM1H_PIN_ENABLE;    // PWMxH pin
    PWM2H_PIN_ENABLE;
    PWM3H_PIN_ENABLE;
    PWM1L_PIN_ENABLE;    // PWMxL pin
    PWM2L_PIN_ENABLE;
    PWM3L_PIN_ENABLE;
    
    PWM1H_PIN_POLARITY_ACTIVE_HIGH;    // Pin is active-high
    PWM2H_PIN_POLARITY_ACTIVE_HIGH;
    PWM3H_PIN_POLARITY_ACTIVE_HIGH;
    PWM1L_PIN_POLARITY_ACTIVE_HIGH;
    PWM2L_PIN_POLARITY_ACTIVE_HIGH;
    PWM3L_PIN_POLARITY_ACTIVE_HIGH;
    
    
    // Set up PG1 as host and PG2 and PG3 as clients for trigger setup
    PMW1_SELF_TRIGGER;  // Self-trigger for PG1
    PWM2_PG1_TRIGGER;  // PG1-triggered
    PWM3_PG1_TRIGGER;  // PG1-triggered
    
    
    // Configure SFR updates from PG1DC
    PWM1_MASTER_UPDATE_ENABLE;     // PG1 as Master update for UPDREQ
    PWM1_UPDATE_ON_WRITE_TO_DUTY_CYCLE;    // A write to the PG1DC register automatically sets UPDREQ in PG1STAT
    PWM1_UPDATE_ON_SOC;
    PWM2_UPDATE_ON_MASTER_REQ_SOC;
    PWM3_UPDATE_ON_MASTER_REQ_SOC;
    
    // Set up interrupts at PG1 EOC --> Note this is the PWM Generator 1 interrupt,
    // with the reserved XC16 ISR name _PWM1Interrupt, vector #75
    PWM1_EOC_INTERRUPT_SELECT;   // EOC will generate interrupt from PG1
    PWM1_INTERRUPT_PRIORITY(6u);      // Priority level 6
    PWM1_CLEAR_IF;       // Clear flag
    PWM1_ENABLE_INTERRUPT;       // Enable interrupt
    
    // Load up with initial duty cycle values...
    PWM2_UPDATE_DUTY_CYCLE(500);
    PWM3_UPDATE_DUTY_CYCLE(500);
    PWM1_UPDATE_DUTY_CYCLE(500);
    
    //-------------------------------------------------------------------------
    // Enable all the PGs!
    PWM2_ON;
    PWM3_ON;
    // PG1 is last to turn on since it acts as the host PG
    PWM1_ON;
}
