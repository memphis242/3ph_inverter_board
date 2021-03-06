/*
 * File:   dspic33_setup_test.c
 * Author: Abdullah Almosalami
 *
 * Created on November 12, 2021, 8:59 PM
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
#define FCY 100000000u      // Fcy is 100 MHz once PLL is set and we have FPLLO = 400 MHz
                            // This is needed to use the __delay_ms function/macro
#include <libpic30.h>       // Contains the __delay_ms function/macro
#define ei()    (INTCON2bits.GIE = 1u)
#define di()    (INTCON2bits.GIE = 0u)




void __attribute__((__interrupt__,no_auto_psv)) _INT0Interrupt(void) {
    // ISR Code
    LATCbits.LATC0 ^= 1u;   // Toggle LED
    
    // Clear flag
    IFS0bits.INT0IF = 0u;
}


int main(void) {
    
    // <editor-fold defaultstate="collapsed" desc="Oscillator Switch to XTPLL with POSC">
    /* So first, I'd like to set up the system clock FOSC to be at 200 MHz (to give Fcy 100MHz)
     * off of the 10 MHz external crystal. The first steps were done above already
     * by setting the FOSCSEL and FOSC configuration registers to start up with
     * the internal fast RC oscillator FRC and then to allow switching to the
     * PLL, which will source from the Primary Oscillator and will operate in
     * XT (medium frequency) mode. Next will be to utilize the oscillator-related
     * SFRs (OSCCON, PLLFBD, and PLLDIV) to actually carry out the switch.
     * 
     * Since I'm going off of 10 MHz and would like FPLLO of 400 MHz (will be div by two for FOSC),
     * and we recall that FPLLO = FPLLI x ( M / (N1 x N2 x N3) ) (recall there are important restrictions),
     * so I'll have M = 40, and N1=N2=N3=1.
     */ 
    CLKDIVbits.PLLPRE = 1u;     // N1 = 1
    PLLFBDbits.PLLFBDIV = 40u;  // M = 40
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
    
    // Now set up an I/O pin for blinking an LED --> I'll choose RC0
    ANSELCbits.ANSELC0 = 0u;        // Configure as digital I/O
    TRISCbits.TRISC0 = 0u;          // Configure as output
    
    // Ok, now let's try to set up an INT0 interrupt
    // Firstly, INT0 is on RB2, so need to set that as input
    ANSELBbits.ANSELB2 = 0u;
    TRISBbits.TRISB2 = 1u;
    // Now set up the interrupt
    INTCON2bits.INT0EP = 0u;    // Rising-edge
    INTCON1bits.NSTDIS = 1u;    // Disable nested interrupts
    IPC0bits.INT0IP = 6u;       // Priority level 6
    IFS0bits.INT0IF = 0u;       // Clear flag
    IEC0bits.INT0IE = 1u;       // Enable the interrupt source
    
    ei();   // Enable all unmasked interrupt sources
    
    // Now LED blink!
    while(1){
        
//        LATCbits.LATC0 ^= 1u;   // Toggle LED
//        __delay_ms(250);        // Delay 250 ms
        
    }
    
    
    return;
}
