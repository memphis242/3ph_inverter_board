/*
 * File:   osc.c
 * Author: Abdullah Almosalami
 *
 * Created on November 18, 2021 10:54 PM
 */


#include <xc.h>
#include "osc.h"


// Global and static variables relevant to oscillator operation/configuration



// Functions
/**
 * <p>So first, I'd like to set up the system clock FOSC to be at 240 MHz (to give Fcy 120MHz)
 * off of the 10 MHz external crystal. The first steps were done above already
 * by setting the FOSCSEL and FOSC configuration registers to start up with
 * the internal fast RC oscillator FRC and then to allow switching to the
 * PLL, which will source from the Primary Oscillator and will operate in
 * XT (medium frequency) mode. Next will be to utilize the oscillator-related
 * SFRs (OSCCON, PLLFBD, and PLLDIV) to actually carry out the switch.</p>
 * 
 * <p>Since I'm going off of 10 MHz and would like FPLLO of 480 MHz (will be div by two for FOSC),
 * and we recall that FPLLO = FPLLI x ( M / (N1 x N2 x N3) ) (recall there are important restrictions),
 * so I'll have M = 48, and N1=N2=N3=1.</p>
 * 
 * @param none
 * @return none
 * 
 */
void osc_init_default(void) {
    
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
}

