/*
 * File:   main.c
 * Author: Abdullah Almosalami
 *
 * Created on November 17, 2021, 3:57 PM
 */


// <editor-fold defaultstate="collapsed" desc="dsPIC33CK64MC105 CONFIGURATION">

// FSEC
#pragma config BWRP = OFF               // Boot Segment Write-Protect bit (Boot Segment may be written)
#pragma config BSS = DISABLED           // Boot Segment Code-Protect Level bits (No Protection (other than BWRP))
#pragma config BSEN = OFF               // Boot Segment Control bit (No Boot Segment)
#pragma config GWRP = OFF               // General Segment Write-Protect bit (General Segment may be written)
#pragma config GSS = DISABLED           // General Segment Code-Protect Level bits (No Protection (other than GWRP))
#pragma config CWRP = OFF               // Configuration Segment Write-Protect bit (Configuration Segment may be written)
#pragma config CSS = DISABLED           // Configuration Segment Code-Protect Level bits (No Protection (other than CWRP))
#pragma config AIVTDIS = OFF            // Alternate Interrupt Vector Table bit (Disabled AIVT)

// FBSLIM
#pragma config BSLIM = 0x1FFF           // Boot Segment Flash Page Address Limit bits (Enter Hexadecimal value)

// FOSCSEL
#pragma config FNOSC = FRC              // Oscillator Source Selection (Internal Fast RC (FRC))
#pragma config IESO = OFF               // Two-speed Oscillator Start-up Enable bit (Start up with user-selected oscillator source)

// FOSC
#pragma config POSCMD = XT              // Primary Oscillator Mode Select bits (XT Crystal Oscillator Mode)
#pragma config OSCIOFNC = OFF           // OSC2 Pin Function bit (OSC2 is clock output)
#pragma config FCKSM = CSECMD           // Clock Switching Mode bits (Clock switching is enabled,Fail-safe Clock Monitor is disabled)
#pragma config PLLKEN = ON              // PLL Lock Enable (PLL clock output will be disabled if LOCK is lost)
#pragma config XTCFG = G1               // XT Config (8-16 MHz crystals)
#pragma config XTBST = ENABLE           // XT Boost (Boost the kick-start)

// FWDT
// RWDTPS = No Setting
#pragma config RCLKSEL = LPRC           // Watchdog Timer Clock Select bits (Always use BFRC/256)
#pragma config WINDIS = ON              // Watchdog Timer Window Enable bit (Watchdog Timer operates in Non-Window mode)
#pragma config WDTWIN = WIN25           // Watchdog Timer Window Select bits (WDT Window is 25% of WDT period)
// SWDTPS = No Setting
#pragma config FWDTEN = ON_SW           // Watchdog Timer Enable bit (WDT controlled via SW, use WDTCON.ON bit)

// FPOR
#pragma config BISTDIS = DISABLED       // Memory BIST Feature Disable (mBIST on reset feature disabled)

// FICD
#pragma config ICS = PGD1               // ICD Communication Channel Select bits (Communicate on PGC1 and PGD1)
#pragma config JTAGEN = OFF             // JTAG Enable bit (JTAG is disabled)

// FDMTIVTL
#pragma config DMTIVTL = 0xFFFF         // Dead Man Timer Interval low word (Enter Hexadecimal value)

// FDMTIVTH
#pragma config DMTIVTH = 0xFFFF         // Dead Man Timer Interval high word (Enter Hexadecimal value)

// FDMTCNTL
#pragma config DMTCNTL = 0xFFFF         // Lower 16 bits of 32 bit DMT instruction count time-out value (0-0xFFFF) (Enter Hexadecimal value)

// FDMTCNTH
#pragma config DMTCNTH = 0xFFFF         // Upper 16 bits of 32 bit DMT instruction count time-out value (0-0xFFFF) (Enter Hexadecimal value)

// FDMT
#pragma config DMTDIS = OFF             // Dead Man Timer Disable bit (Dead Man Timer is Disabled and can be enabled by software)

// FDEVOPT
#pragma config ALTI2C1 = OFF            // Alternate I2C1 Pin bit (I2C1 mapped to SDA1/SCL1 pins)
#pragma config SMB3EN = SMBUS3          // SM Bus Enable (SMBus 3.0 input levels)
#pragma config SPI2PIN = PPS            // SPI2 Pin Select bit (SPI2 uses I/O remap (PPS) pins)

// FALTREG
#pragma config CTXT1 = OFF              // Specifies Interrupt Priority Level (IPL) Associated to Alternate Working Register 1 bits (Not Assigned)
#pragma config CTXT2 = OFF              // Specifies Interrupt Priority Level (IPL) Associated to Alternate Working Register 2 bits (Not Assigned)
#pragma config CTXT3 = OFF              // Specifies Interrupt Priority Level (IPL) Associated to Alternate Working Register 3 bits (Not Assigned)
#pragma config CTXT4 = OFF              // Specifies Interrupt Priority Level (IPL) Associated to Alternate Working Register 4 bits (Not Assigned)

// </editor-fold>


#include <xc.h>
#include "pwm.h"
#include "osc.h"
//#include "duty_cycle.h"


#define FCY 120000000u      // Fcy is 120 MHz once PLL is set and we have FPLLO = 480 MHz
                            // This is needed to use the __delay_ms function/macro
#include <libpic30.h>       // Contains the __delay_ms function/macro
#define ei()    (INTCON2bits.GIE = 1u)
#define di()    (INTCON2bits.GIE = 0u)


extern uint16_t pwm_cycle_counter;
static uint8_t pwm_duty_cycle_index = 0x00u;
extern const uint16_t pwm1_duty_cycle_vals[100];
extern const uint16_t pwm2_duty_cycle_vals[100];
extern const uint16_t pwm3_duty_cycle_vals[100];


void __attribute__((__interrupt__,no_auto_psv)) _PWM1Interrupt(void) {
    // ISR Code
    pwm_cycle_counter++;
    
    // If we've hit the 2000 periods count mark, update duty cycle values
    if(pwm_cycle_counter >= 20u) {
        pwm_cycle_counter = 0u;     // Reset the counter
        
        pwm_duty_cycle_index++;
        
        if(pwm_duty_cycle_index >= 100u) pwm_duty_cycle_index = 0u; // Reset index whenever it's at end
        
        // Recall that write to PG1DC will automatically update all the other PGs,
        // so do that last
        PWM2_UPDATE_DUTY_CYCLE(pwm2_duty_cycle_vals[pwm_duty_cycle_index]);
        PWM3_UPDATE_DUTY_CYCLE(pwm3_duty_cycle_vals[pwm_duty_cycle_index]);
        PWM1_UPDATE_DUTY_CYCLE(pwm1_duty_cycle_vals[pwm_duty_cycle_index]);
    }
    
    // Clear flag
    PWM1_CLEAR_IF;       // Clear flag
}


int main(void) {
    
    // Set up oscillator
    osc_init_default();
    
    // PWM
    pwm_init_default();
    
    // Now set up an I/O pin for debug LED --> I'll choose RC0
    ANSELCbits.ANSELC0 = 0u;        // Configure as digital I/O
    TRISCbits.TRISC0 = 0u;          // Configure as output
    
    ei();   // Enable all unmasked interrupt sources
    
    
    // Event-handler
    while(1);

    
    return 1;
}
