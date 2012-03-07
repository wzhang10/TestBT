
/*
 * File:   main.c
 * Author: mark
 *
 * Created on January 29, 2012, 5:07 PM
 *
 * The MCU will only transimit data when it gets a request
 *
 * Possibile incoming requests/data:
 * 1. Request for a key BT_REQ_KEY
 * 2. The other end is sending me a key BT_SEND_KEY
 *
 * Possible responses are:
 * 1. Send a key BT_SEND_KEY
 * 3. I have no more key BT_NO_KEY
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libconn/connection.h"
#include "libconn/bt_app.h"
#include "timer.h"
#include "HardwareProfile.h"
#include "uart.h"
#include "log.h"
#include "data.h"

/* ---------------------------helper----------------------- */
struct LinkedList{
    char* data;
    struct LinkedList *next;
};
#define HDR_SZ  10
#define HDR_SIG 0x75

struct Header{
    char signature;
    unsigned char size;
    short seq;
    short ack;
    char action;
    char unused;
    short extra;
};
static short currentSeq = 1;

typedef enum{
    BT_SYN = 0x01,
    BT_ACK = 0x02,
    BT_REQ_KEY = 0x04,
    BT_SEND_KEY = 0x08,
    BT_NO_KEY = 0x10,
    BT_NO_SUCH_REQ = 0x20,
    BT_SEND_G = 0x40,
    BT_LAST = 0x80
} HEADER_FLAG;

short getCurrentSeq(){
    if(currentSeq==-1)
        currentSeq = 1;
    return currentSeq++;
}

struct LinkedList* getListFirst(struct LinkedList** l){
    if((*l)==0)return 0;
    struct LinkedList *ret = (*l);
    (*l) = (*l)->next;
    return ret;
}

void linkedListadd(struct LinkedList* item, struct LinkedList **l){
    if(!(*l))
        (*l) = item;
    else{
        struct LinkedList *tmp = *l;
        while(tmp->next)
            tmp=tmp->next;
        tmp->next=item;
    }
}

int linkedListIsEmpty(struct LinkedList* l){
    return (l==0)?1:0;
}

void freeLinkedList(struct LinkedList* l){
    if(l){
        if(l->data)
            free(l->data);
        free(l);
    }
}

void printHeader(const char* data) {
    struct Header *hdr = (struct Header *)data;
    char *htmp = data;
    log_printf("sig=%x size=%x seq=%X ack=%X extra=%X action=%01X = ", hdr->signature, hdr->size, hdr->seq, hdr->extra, hdr->ack, hdr->action);
    if((hdr->action & BT_SYN))
        log_printf("BT_SYN");
    if((hdr->action & BT_ACK))
        log_printf("|BT_ACK");
    if((hdr->action & BT_REQ_KEY))
        log_printf("|BT_REQ_KEY");
    if((hdr->action & BT_SEND_KEY))
        log_printf("|BT_SEND_KEY");
    if((hdr->action & BT_NO_KEY))
        log_printf("|BT_NO_KEY");
    if((hdr->action & BT_NO_SUCH_REQ))
        log_printf("|BT_NO_SUCH_REQ");
    if((hdr->action & BT_SEND_G))
        log_printf("|BT_SEND_G");
    if((hdr->action & BT_LAST))
            log_printf("|BT_LAST");
    log_printf("\n");
}

int checkSignature(const void* data){
    char c;
    memcpy(&c, data, 1);
    return (c ^ HDR_SIG);
}
/* ---------------------------helper----------------------- */

struct LinkedList *receivedQ;
void printHeader(const char* data);

static CHANNEL_HANDLE handle;
unsigned char *keyWriteAddr;

typedef enum {
	MAIN_POWER_ON,
	MAIN_POWER_OFF,
	MAIN_CONNECTING,
	MAIN_WAIT_FOR_ACTION,
	MAIN_SEND_KEY,
	MAIN_RECEIVE_KEY
}MainState;
MainState mainState = MAIN_POWER_ON;
char *tmpBuffer;
int tmp_len;
struct LinkedList *receivedQ=0;
void btCallback(CHANNEL_HANDLE h, const void* data, UINT32 data_len);

void keyInit() {
	keyIndex = 0;    //only in ram

	keySize = 0;
	keyWriteAddr = keyStartAddress;
}

const unsigned char *keyReadData() {
    log_printf("----------keyReadData----------\n");
    if (keySize==0 || keyIndex*keySize >= KEY_MAX_SIZE){
        log_printf("keyReadData returning 0\n");
        return 0;
    }
    keyIndex++;
    return keyStartAddress+(keyIndex-1)*keySize;

}

int keyWriteData(char *data, unsigned int ks) {
    log_printf("----------keyWriteData----------\n");
    if(keyWriteAddr+ks > keyStartAddress+KEY_MAX_SIZE)return 0;
    keySize = ks;
    memcpy(keyWriteAddr, data, keySize);
    keyWriteAddr += keySize;
    //log_printf("data=%s ks=%d keySize=%d keyWriteAddress=%x keyStartAddress=%x\n\rKeys=%s\n\r", data, ks, keySize, keyWriteAddr, keyStartAddress, keyStartAddress);
    return keySize;
}

void sendKey(){
	char *key = keyReadData();
	int pkgSz;
	//log_printf("keyReadData returned=%s @%x size=%d\n", key, (UINT)key, keySize);
	if(key){
		pkgSz = keySize+HDR_SZ;
	}else{
		pkgSz = HDR_SZ;
	}

	char *data = malloc(pkgSz);
	if(!data){log_printf("malloc failed\n\r");exit(-1);}
	((struct Header *)data)->signature = HDR_SIG;
	((struct Header *)data)->size = keySize;
	((struct Header *)data)->seq = getCurrentSeq();
	//hdrData->ack = h->seq;
	((struct Header *)data)->action = BT_SYN | BT_SEND_KEY | BT_ACK;

	if(!key){
		((struct Header *)data)->action = BT_NO_KEY | BT_ACK;
		((struct Header *)data)->size = 0;
		((struct Header *)data)->extra = 0;
	}else{
		memcpy(data+HDR_SZ, key, keySize);
		// using the extra field to specify key index
		((struct Header *)data)->extra = keyIndex-1;
	}
	BTWrite(data, pkgSz);
	free(data);
}

void MainStateMachine(const char *data, int data_len){
	struct Header *h = 0;
	switch(mainState){
	case MAIN_POWER_ON:
		ConnectionInit();//Set states to disconnect
		keyInit();
		mainState = MAIN_CONNECTING;
		break;
	case MAIN_CONNECTING:
		if(ConnectionCanOpenChannel(CHANNEL_TYPE_BT)){
			handle = ConnectionOpenChannelBtServer(&btCallback);
			mainState = MAIN_WAIT_FOR_ACTION;
		}
		break;
	case MAIN_WAIT_FOR_ACTION:
		if(data){
			h = data;
			if(h->action & BT_SEND_KEY){
				mainState = MAIN_RECEIVE_KEY;
			}else if(h->action & BT_REQ_KEY){
				mainState = MAIN_SEND_KEY;
			}
		}
		break;
	case MAIN_SEND_KEY:
		//log_printf("MAIN_SEND_KEY\n\r");
		sendKey();
		mainState = MAIN_WAIT_FOR_ACTION;
		break;
	case MAIN_RECEIVE_KEY:
		//log_printf("MAIN_RECEIVE_KEY\n\r");
		if(data){
			h = data;
			char *key = data+HDR_SZ;
			keyWriteData(key, h->size);
			if(h->action & BT_LAST){
				char *data = malloc(HDR_SZ);
				if(!data){log_printf("malloc failed\n\r");exit(-1);}
				((struct Header *)data)->signature = HDR_SIG;
				((struct Header *)data)->size = keySize;
				((struct Header *)data)->seq = getCurrentSeq();
				//hdrData->ack = h->seq;
				((struct Header *)data)->action = BT_ACK;
				((struct Header *)data)->size = 0;
				((struct Header *)data)->extra = 0;
				BTWrite(data, HDR_SZ);
				free(data);
				mainState = MAIN_WAIT_FOR_ACTION;
			}
		}
		break;
	case MAIN_POWER_OFF:
		log_printf("power off..\n\r");
		while(1);
		break;
	}
}

void btCallback(CHANNEL_HANDLE h, const void* data, UINT32 data_len) {
    log_printf("----------btCallback----------\n");
    if (data) {
        mLED_0_Toggle();
        if (checkSignature(data)) {
            log_printf("signature check failed\n");
            return;
        }
        tmpBuffer = malloc(data_len);
        tmp_len = data_len;
        memcpy(tmpBuffer, data, tmp_len);
    } else {
        log_printf("btCallback data pointer invalid\n");
        handle = ConnectionOpenChannelBtServer(&btCallback);
    }
}

void _ISR _INT1Interrupt(){
	mLED_0_Toggle();
	log_printf("INTERRRUPT!!!\n\r");
	IFS1bits.INT1IF = 0;
}

void InterruptInit(){
	TRISDbits.TRISD8 = 1;
	RPINR0 = 0x0A00;

	INTCON2 = 0x0000;
	IFS1bits.INT1IF = 0;    /*Reset INT1 interrupt flag */
	IEC1bits.INT1IE = 1;    /*Enable INT1 Interrupt Service Routine */
	IPC5bits.INT1IP = 1;	/*set low priority*/
}

//rd8/rp2  pin3
int testMain(){
	//	CLKDIV = 0x0000;
	TRISB = 0xFFFF;
	TRISC = 0xFFFF;
	TRISD = 0xFFFF;
	TRISE = 0xFFFF;
	TRISF = 0xFFFF;
	TRISG = 0xFFFF;
    InterruptInit();

	UINT32 wait=0;
    while(1){
//    	LATDbits.LATD8 = ~LATDbits.LATD8;
    	if(wait > 3000000){
    		log_printf("f4=%d IFS1=%d    ", PORTFbits.RF4, IFS1bits.INT1IF);
    		wait = 0;
    	}
    	wait++;
//    	IFS1bits.INT1IF = PORTFbits.RF4;
    }
}

void SleepCall(){
	asm volatile("PWRSAV #0\n");
}

void ResetCall(){
	//RFCOMM_DISCONNECT
    char *a = 0x804;
    log_printf("0x804=%X", *a);
    asm volatile("RESET");
}

int main() {
    mInitAllLEDs();
    log_init(9600);
    log_printf("----------main-----------\n\r");

	TRISG = 0xFFFF;
    tmpBuffer=0;
    for(;;){
    	if(mainState!=MAIN_POWER_OFF && mainState!=MAIN_POWER_ON)
    		ConnectionTasks();
    	MainStateMachine(tmpBuffer, tmp_len);
    	if(tmpBuffer){
    		free(tmpBuffer);
    		tmpBuffer = 0;
    	}
    	if(PORTFbits.RF4)ResetCall();
    }
    return (0);
}
