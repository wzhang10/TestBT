/* 
 * File:   main.c
 * Author: mark
 *
 * Created on January 29, 2012, 5:07 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "HardwareProfile.h"

void blinkForEver(int i){
  while(1){
    mLED_0_On();
    DelayMs(i);
    mLED_0_Off();
    DelayMs(i);
  }
}

void blinkZero(){
    mLED_0_On();
    DelayMs(100);
    mLED_0_Off();
    DelayMs(1000);
}

void blinkOne(){
    mLED_0_On();
    DelayMs(300);
    mLED_0_Off();
    DelayMs(1000);
}

//pin 6 rx
//pin 4 tx
void initUart(int baud){
    // Unlock Registers
    asm volatile(
        "MOV    #OSCCON, w1 \n"
        "MOV    #0x46, w2   \n"
        "MOV    #0x57, w3   \n"
        "MOV.b  w2, [w1]    \n"
        "MOV.b  w3, [w1]    \n"
        "BCLR OSCCON, #6");

    RPINR18bits.U1RXR = 12;         //pin 6  RP12     u1rx
    RPOR2bits.RP4R = 3;             //pin 4     u1tx

    asm volatile(
        "MOV    #OSCCON, w1 \n"
        "MOV    #0x46, w2   \n"
        "MOV    #0x57, w3   \n"
        "MOV.b  w2, [w1]    \n"
        "MOV.b  w3, [w1]    \n"
        "BSET   OSCCON, #6");

    U1BRG = (4000000/(4*baud)-1) & 0xff;

    U1MODE =    ((1 & 1) << 15) |   //enable
                ((0 & 1) << 13) |   //no stop in idle
                ((0 & 1) << 12) |   //no IrDA
                ((0 & 1) << 11) |   //RTS in Simplex mode
                ((0 & 3) << 8)  |   //UxTX and UxRX pins are enabled and used;
                                    //UxCTS and UxRTS/BCLKx pins are controlled by port latches
                ((0 & 1) << 7)  |   //no wake up
                ((0 & 1) << 6)  |   //no loopback
                ((0 & 1) << 5)  |   //no auto baud
                ((0 & 1) << 4)  |   //no polarity inversion
                ((0 & 1) << 3)  |   //16 brg clock cycles per bit
                ((0 & 3) << 1)  |   //8 bit data, no parity
                (0 & 1);            //one stop bit

    U1STA = 0x8400;

    //reset RX interrupt flag
    IFS0bits.U1RXIF = 0;
}

void UARTPutChar(char c){
    while(U1STAbits.UTXBF == 1);
    U1TXREG = c;
}

char UARTGetChar()
{
   char Temp;
   //wait for buffer to fill up, wait for interrupt
   while(IFS0bits.U1RXIF == 0);
   Temp = U1RXREG;
   mLED_0_Toggle();
   //reset interrupt
   IFS0bits.U1RXIF = 0;
   //return my received byte
   return Temp;
}

int main() {

    mInitAllLEDs();
    initUart(9600);

    while(1){
        char a = UARTGetChar();
        UARTPutChar(a);
    }
    return (0);
}

