#define FCY 4000000UL
#include "xc.h"
#include <stdint.h>
#include <libpic30.h>


#pragma config POSCMOD = NONE, FNOSC = FRC
#pragma config FCKSM   = CSDCMD, IESO = OFF, OSCIOFNC = OFF
#pragma config WDTPS   = PS32768, FWPSA = PR128, FWDTEN = OFF, WINDIS = ON
#pragma config ICS     = PGx2,   GWRP = OFF,  GCP = OFF,   JTAGEN = OFF

/* ------------ tablice animacji ---------------------------- */
static const uint8_t snakeTable[] = {0x07,0x0E,0x1C,0x38,0x70,0xE0,0x70,0x38,0x1C,0x0E,0x07};
static const uint8_t queueTable[] = {
  0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x81,0x82,0x84,0x88,0x90,0xA0,0xC0,0xC1,
  0xC2,0xC4,0xC8,0xD0,0xE0,0xE1,0xE2,0xE4,0xE8,0xF0,0xF1,0xF2,0xF4,0xF8,0xF9,0xFA,
  0xFC,0xFD,0xFF
};

/* ------------ zmienne globalne ---------------------------- */
static uint8_t portValue = 0, bcd_units = 0, bcd_tens = 0;
static uint8_t snakeIndex = 0, queueIndex = 0;

static uint8_t generator = 0b111001;      
static uint8_t mode = 1;                  

static uint8_t prevS6 = 1, prevS13 = 1;   

/* ------------ reset wszystkich liczników ------------------ */
static inline void resetStates(void)
{
    portValue = bcd_units = bcd_tens = 0;
    snakeIndex = queueIndex = 0;
    generator = 0b111001;                 
}

/* ------------ obs?uga przycisków -------------------------- */
static inline void readButtons(void)
{
    uint8_t nowS6  = PORTDbits.RD6;   
    uint8_t nowS13 = PORTDbits.RD13;  

    __delay32(15000);                 

    
    if (prevS6 && !nowS6) {
        mode = (mode == 9) ? 1 : mode + 1;
        resetStates();
    }
   
    if (prevS13 && !nowS13) {
        mode = (mode == 1) ? 9 : mode - 1;
        resetStates();
    }

    prevS6  = nowS6;
    prevS13 = nowS13;
}


void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)
{
    switch (mode)
    {
        case 1:  LATA = portValue++;                                  break; 
        case 2:  LATA = portValue--;                                  break; 
        case 3:  portValue++;  LATA = portValue ^ (portValue>>1);     break; 
        case 4:  portValue--;  LATA = portValue ^ (portValue>>1);     break; 
        case 5:  if (++bcd_units > 9) { bcd_units = 0; bcd_tens = (bcd_tens+1)%10; }
                 LATA = (bcd_tens<<4) | bcd_units;                    break; 
        case 6:  if (!bcd_units) { bcd_units = 9; bcd_tens = bcd_tens ? bcd_tens-1 : 9; }
                 else --bcd_units;
                 LATA = (bcd_tens<<4) | bcd_units;                    break; 
        case 7:  LATA = snakeTable[snakeIndex++];
                 if (snakeIndex >= sizeof(snakeTable)) snakeIndex = 0;break; 
        case 8:  LATA = queueTable[queueIndex++];
                 if (queueIndex >= sizeof(queueTable)) queueIndex = 0;break; 
        case 9:  
                 generator = ((generator>>1) |
                              (((generator>>5) ^ (generator>>4)) & 1) << 5) & 0x3F;
                 LATA = generator;                                    break; 
    }
    _T1IF = 0;         
}

/* ------------ main() ------------------------------------- */
int main(void)
{
    TRISA   = 0x0000;   
    AD1PCFG = 0xFFFF;   

    TRISD |= (1<<6) | (1<<13);  

  
    T1CON = 0x8030;
    TMR1  = 0;
    PR1   = 0x0FFF;
    _T1IF = 0;
    _T1IP = 1;
    _T1IE = 1;

    resetStates();
    while (1)
        readButtons();
}
