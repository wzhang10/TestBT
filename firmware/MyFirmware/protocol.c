///*
// * File:   protocol.c
// * Author: mark
// *
// * Created on February 16, 2012, 10:49 PM
// */
//
//#include <stdio.h>
//#include <stdlib.h>
//#include "protocol.h"
//#include "uart.h"
//#include "log.h"
//#include "../libconn/bt_app.h"
//#include <time.h>
//#include "data.h"
//
//extern char *keyReadData();
//extern int keyWriteData(char *data, int ks);
////extern const char *GetG();
////extern int WriteG(char *data, int n);
////extern int GSize;
//
//static short currentSeq = 1;
//unsigned short rtt = 0;
//
//int bufferLocation = 0;
//int numOfByteLeft = 0;
//int firstRecv = 1;
//
//struct LinkedList *sendQ=0;
//struct LinkedList *waitAckQ=0;
//struct LinkedList *receivedQ=0;
//
//struct WaitAckPkg {
//    char * data;
//    long timeout;
//};
//
//short getCurrentSeq(){
//    if(currentSeq==-1)
//        currentSeq = 1;
//    return currentSeq++;
//}
//
//int checkSignature(const void* data){
//    char c;
//    memcpy(&c, data, 1);
//    return (c ^ HDR_SIG);
//}
//
//struct LinkedList* getListFirst(struct LinkedList** l){
//    if((*l)==0)return 0;
//    struct LinkedList *ret = (*l);
//    (*l) = (*l)->next;
//    return ret;
//}
//
//struct LinkedList* detachFromLinkedList(struct LinkedList *item, struct LinkedList **l){
//    log_printf("----------detachFromLinkedList----------\n");
//    if(item==(*l))return getListFirst(l);
//    struct LinkedList *tmp = (*l);
//    while(tmp->next!=item && tmp->next!=0){
//        tmp = tmp->next;
//    }
//    if(!tmp->next)return 0;
//    struct LinkedList* ret = tmp->next;
//    tmp->next = ret->next;
//    return ret;
//}
//
//void linkedListadd(struct LinkedList* item, struct LinkedList **l){
//    if(l==&sendQ)log_printf("adding to sendQ\n");
//    if(l==&receivedQ)log_printf("adding to rQ\n");
//    if(l==&waitAckQ)log_printf("adding to waitAckQ\n");
//
//    if(!(*l))
//        (*l) = item;
//    else{
//        struct LinkedList *tmp = *l;
//        while(tmp->next)
//            tmp=tmp->next;
//        tmp->next=item;
//    }
//
//    log_printf("sQ@%x sQ=%x ", &sendQ, sendQ);
//    log_printf("rQ@%x rQ=%x ", &receivedQ, receivedQ);
//    log_printf("wQ@%x wQ=%x\n", &waitAckQ, waitAckQ);
//}
//
//int linkedListIsEmpty(struct LinkedList* l){
//    return (l==0)?1:0;
//}
//
//void freeLinkedList(struct LinkedList* l){
//    if(l){
//        if(l->data)
//            free(l->data);
//        free(l);
//    }
//}
//
//void printHeader(const char* data) {
//    struct Header *hdr = (struct Header *)data;
//    log_printf("sig=%x size=%x seq=%x ack=%x extra=%x action=", hdr->signature, hdr->size, hdr->seq, hdr->extra, hdr->ack);
//    if((hdr->action & BT_SYN))
//        log_printf("BT_SYN");
//    if((hdr->action & BT_ACK))
//        log_printf("|BT_ACK");
//    if((hdr->action & BT_REQ_KEY))
//        log_printf("|BT_REQ_KEY");
//    if((hdr->action & BT_SEND_KEY))
//        log_printf("|BT_SEND_KEY");
//    if((hdr->action & BT_NO_KEY))
//        log_printf("|BT_NO_KEY");
//    if((hdr->action & BT_NO_SUCH_REQ))
//        log_printf("|BT_NO_SUCH_REQ");
//    if((hdr->action & BT_SEND_G))
//        log_printf("|BT_SEND_G");
//    log_printf("\n");
//}
//
////void EncryptWithG(char *data, int n){
////    char *G = GetG();
////    int i=0;
////    int j=0;
////    while(i<n){
////        data[i]^=G[j];
////        i++;
////        j++;
////        if(j==GSize)j=0;
////    }
////}
//
////void calculateRTT(short ack){
////    long t1 = time(0);
////    struct LinkedList *rttReq = malloc(sizeof(struct LinkedList));
////    struct Header *h = malloc(HDR_SZ);
////    h->signature = HDR_SIG;
////    h->size = 0;
////    h->seq = getCurrentSeq();
////    h->ack = ack;
////    h->action = BT_ACK | BT_SYN | BT_NO_SUCH_REQ;
////    h->extra = t1 & 0xffff;
////    rttReq->data = (char *)h;
////    rttReq->next = 0;
////    linkedListadd(rttReq, &sendQ);
////}
//
////void checkForRetransmission(){
////    return;// TODO: fix this, doesn't really work right now
////    struct LinkedList* headOfList = waitAckQ;
////    if(!headOfList)return;
////    if(!rtt)return;
////    struct WaitAckPkg * pkg = headOfList->data;
////    long now = time(0);
////    if(now > pkg->timeout){
////        headOfList = getListFirst(&waitAckQ);
////        struct LinkedList *retransmit = malloc(sizeof(struct LinkedList));
////        retransmit->data = pkg->data;
////        retransmit->next = 0;
////        pkg->data = 0;
////        linkedListadd(retransmit, &sendQ);
////        freeLinkedList(headOfList);
////    }
////}
//
//void sendQTask(){
//    //checkForRetransmission();
//    struct LinkedList* headOfList = getListFirst(&sendQ);
//    if(!headOfList)return;
//    struct Header *h = (struct Header *)headOfList->data;
//    h->unused = 0xff;
//    log_printf("sending header: ");
//    printHeader(headOfList->data);
//    BTWrite(headOfList->data, h->size+HDR_SZ);
//    if((h->action & BT_SYN)){
//        struct WaitAckPkg * pkg = malloc(sizeof(struct WaitAckPkg));
//        pkg->data = headOfList->data;
//        pkg->timeout = time(0)+rtt;
//        headOfList->data = pkg;
//        headOfList->next = 0;
//        linkedListadd(headOfList, &waitAckQ);
//    }else
//        freeLinkedList(headOfList);
//}
//
//// process the received messages
//void receivedQTask(){
//    //log_printf("----------receivedQTask----------\n");
//    struct LinkedList* headOfList = getListFirst(&receivedQ);
//    if(!headOfList)return;
//    struct Header *h = (struct Header *)headOfList->data;
//
//    if(firstRecv && !(h->action ^ BT_SYN)){
//        firstRecv = 0;
//        calculateRTT(h->seq);
//    }
//
//    if(h->action & BT_ACK){
//        // Got an acknowledgment, remove from list if finds match
//        log_printf("Got an acknowledgement\n");
//        int answered = 0;
//        if(!linkedListIsEmpty(waitAckQ)){
//            struct LinkedList *tmp = waitAckQ;
//            while(tmp){
//                struct WaitAckPkg *wap = (struct WaitAckPkg *)tmp->data;
//                struct Header *ih = (struct Header *)wap->data;
//                log_printf("header in waitAckQ:");
//                printHeader(ih);
//                log_printf("\n");
//                if(h->ack == ih->seq){
//                    // remove tmp
//                    log_printf("found maching ack to seq\n");
//                    tmp = detachFromLinkedList(tmp, &waitAckQ);
//                    if(!(ih->action ^ (BT_ACK | BT_SYN | BT_NO_SUCH_REQ))){
//                        log_printf("found rtt packet\n");
//                        //rtt calculation packet
//                        long now = time(0);
//                        rtt = (now & 0xffff) - ih->extra;
//                        log_printf("now=%x, ih->extra=%x, RTT=%x\n", now, ih->extra, rtt);
//                    }
//                    freeLinkedList(tmp);
//                    answered = 1;
//                    break;
//                }
//                tmp = tmp->next;
//            }
//        }
//        if(!answered && (h->action & BT_SYN)){
//            struct LinkedList *noReqAck = malloc(sizeof(struct LinkedList));
//
//            struct Header *hdrData = malloc(HDR_SZ);
//            hdrData->signature = HDR_SIG;
//            hdrData->size = 0;
//            hdrData->seq = getCurrentSeq();
//            hdrData->ack = h->seq;
//            hdrData->action = BT_NO_SUCH_REQ|BT_ACK;
//            hdrData->extra = 0xffff;
//
//            noReqAck->next = 0;
//            noReqAck->data = (char *)hdrData;
//            linkedListadd(noReqAck, &sendQ);
//        }
//    }
//
//    if(h->action & BT_SEND_KEY){
//        // Got keys
//        if(!keyWriteData((char *)(headOfList->data+HDR_SZ), h->size)){
//            //TODO: NO MORE ROOM IN MEMORY TO SAVE
//        }
//
//        struct Header *hdrData = malloc(HDR_SZ);
//        hdrData->signature = HDR_SIG;
//        hdrData->size = 0;
//        hdrData->seq = getCurrentSeq();
//        hdrData->ack = h->seq;
//        hdrData->action = BT_ACK;
//        hdrData->extra = 0xffff;
//
//        struct LinkedList *sendItem = malloc(sizeof(struct LinkedList));
//        sendItem->next = 0;
//        sendItem->data = (char *)hdrData;
//        linkedListadd(sendItem, &sendQ);
//
//    }else if(h->action & BT_REQ_KEY){
//        //unsigned int keySz = (long)(*(headOfList->data+HDR_SZ)) & 0xffff;
//
//        char *data;
//        char *key = keyReadData();
//        log_printf("keyReadData returned =%s @%x size=%d\n", key, (UINT)key, keySize);
//        if(key)
//        	data = malloc(keySize+HDR_SZ);
//        else
//        	data = malloc(HDR_SZ);
//
//        struct Header *hdrData = (struct Header *)data;
//        hdrData->signature = HDR_SIG;
//        hdrData->size = keySize;
//        hdrData->seq = getCurrentSeq();
//        hdrData->ack = h->seq;
//        hdrData->action = BT_SYN | BT_SEND_KEY | BT_ACK;
//
//        if(!key){
//            hdrData->action = BT_NO_KEY | BT_ACK;
//            hdrData->size = 0;
//            hdrData->extra = 0;
//        }else{
//            memcpy(data+HDR_SZ, key, keySize);
//            // using the extra field to specify key index
//            hdrData->extra = keyIndex-1;
//            //EncryptWithG(data+HDR_SZ, keySz);
//            //log_printf("encrypted keys=%s\n", data+HDR_SZ);
//        }
//
//        struct LinkedList *sendItem = malloc(sizeof(struct LinkedList));
//        sendItem->next = 0;
//        sendItem->data = data;
//        linkedListadd(sendItem, &sendQ);
//
//    } /* else if(h->action & BT_SEND_G){
//        if(!WriteG((char *)(headOfList->data+HDR_SZ), h->size)){
//            //TODO: NO MORE ROOM IN MEMORY TO SAVE
//        }
//
//        struct Header *hdrData = malloc(HDR_SZ);
//        hdrData->signature = HDR_SIG;
//        hdrData->size = 0;
//        hdrData->seq = getCurrentSeq();
//        hdrData->ack = h->seq;
//        hdrData->action = BT_ACK;
//        hdrData->extra = 0xffff;
//        struct LinkedList *sendItem = malloc(sizeof(struct LinkedList));
//        sendItem->next = 0;
//        sendItem->data = (char *)hdrData;
//        linkedListadd(sendItem, &sendQ);
//    } */
//    freeLinkedList(headOfList);
//}
//
//
//void sendHexToUART(char *data, int n){
//    int i=0;
//    while(i<n){
//        unsigned char c1;
//        unsigned char c = data[i++];
//        if(c>9){
//            c1 = c>>4 &0x0f;
//            c = c & 0x0f;
//        }else{
//            c1 = 0;
//        }
//        if(c1>9){
//            c1 += 7;
//        }
//        if(c>9){
//            c += 7;
//        }
//        c1+=0x30;
//        c+=0x30;
//        UARTPutChar(c1);
//        UARTPutChar(c);
//        UARTPutChar(0xA0);
//    }
//}
//
