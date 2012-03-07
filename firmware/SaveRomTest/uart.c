/* 
 * File:   uart.c
 * Author: mark
 *
 * Created on February 6, 2012, 11:11 PM
 */
#include "HardwareProfile.h"
#include <stdlib.h>
#include "uart.h"

struct ByteQueue{
    char c;
    struct ByteQueue *next;
};

struct ByteQueue *queue;

void UARTPutChar(char c){
    U1TXREG = c;
    while(U1STAbits.TRMT == 0);
}

void queueAdd(char c){
    if(!queue){
        queue = malloc(sizeof(struct ByteQueue));
        queue->c = c;
        queue->next = 0;
    }else{
        struct ByteQueue *t = queue;
        while(t->next)t=t->next;
        t->next = malloc(sizeof(struct ByteQueue));
        t->next->c = c;
        t->next->next = 0;
    }
}

void queueSend(){
    struct ByteQueue *t = queue;
    if(t){
        queue = t->next;
        UARTPutChar(t->c);
    }
}

void UARTTask(){
    queueSend();
}

void UARTSendStr(char *str){
    unsigned char c;
    while( (c = *str++) )
        queueAdd(c);
}

void UARTSendStrNow(char *str){
    unsigned char c;
    while( (c = *str++) )
        UARTPutChar(c);
}

//pin 6 rx
//pin 4 tx
void UARTInit(int baud){
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

void UARTSend(char *data, unsigned int sz){
    int i=0;
    while(i<sz){
        queueAdd(*(data+i));
        i++;
    }
}

//char UARTGetChar(){
//   char Temp;
//   //wait for buffer to fill up, wait for interrupt
//   while(IFS0bits.U1RXIF == 0);
//   Temp = U1RXREG;
//   //reset interrupt
//   IFS0bits.U1RXIF = 0;
//   //return my received byte
//   return Temp;
//}
