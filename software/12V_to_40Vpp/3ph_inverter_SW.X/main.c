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
    IFS4bits.PWM1IF = 0u;       // Clear flag
}


int main(void) {
    
    // <editor-fold defaultstate="collapsed" desc="Oscillator Switch to XTPLL with POSC">
    /* So first, I'd like to set up the system clock FOSC to be at 240 MHz (to give Fcy 120MHz)
     * off of the 10 MHz external crystal. The first steps were done above already
     * by setting the FOSCSEL and FOSC configuration registers to start up with
     * the internal fast RC oscillator FRC and then to allow switching to the
     * PLL, which will source from the Primary Oscillator and will operate in
     * XT (medium frequency) mode. Next will be to utilize the oscillator-related
     * SFRs (OSCCON, PLLFBD, and PLLDIV) to actually carry out the switch.
     * 
     * Since I'm going off of 10 MHz and would like FPLLO of 480 MHz (will be div by two for FOSC),
     * and we recall that FPLLO = FPLLI x ( M / (N1 x N2 x N3) ) (recall there are important restrictions),
     * so I'll have M = 48, and N1=N2=N3=1.
     */ 
    CLKDIVbits.PLLPRE = 1u;     // N1 = 1
    PLLFBDbits.PLLFBDIV = 48u;  // M = 48
    PLLDIVbits.POST1DIV = 1u;   // N2 = 1
    PLLDIVbits.POST2DIV = 1u;   // N3 = 1
    
    // Now initiate switch over
    __builtin_write_OSCCONH(0x03u);     // This configures the NOSC bits to 0x3, which = POSCPLL
    __builtin_write_OSCCONL(OSCCON | 0x01u);    // This sets the OSWEN bit, which requests for a clock switch
    
    // Wait for clock switch to occur...
    while(OSCCONbits.OSWEN);
    
    // Wait for PLL to lock
    while(!OSCCONbits.LOCK);
    
    // TADA!
    // </editor-fold>
    
    // PWM
    pwm_init_default();
    
    // Now set up an I/O pin for debug LED --> I'll choose RC0
    ANSELCbits.ANSELC0 = 0u;        // Configure as digital I/O
    TRISCbits.TRISC0 = 0u;          // Configure as output
    
    ei();   // Enable all unmasked interrupt sources
    
    uint16_t dc1 = 500u;
    uint16_t dc2 = 250u;
    uint8_t i = 0x00u;
    // Event-handler
    while(1){
        LATCbits.LATC0 ^= 1u;
        i ^= 1u;
        
        __delay_ms(500);
    }

    
    return 1;
}
