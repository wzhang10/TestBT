//struct LinkedList{
//    char* data;
//    struct LinkedList *next;
//};
//
//void linkedListadd(struct LinkedList* item, struct LinkedList **l);
//
//#define HDR_SZ  10
//#define HDR_SIG 0x75
//
//struct Header{
//    //ensure packets delivery
//    char signature;
//    unsigned char size;
//    short seq;
//    short ack;
//
//    //higher protocol
//    char action;
//    char unused;
//    short extra;
//};
//
//int checkSignature(const void* data);
//
//typedef enum{
//    BT_SYN = 0x01,
//    BT_ACK = 0x02,
//    BT_REQ_KEY = 0x04,
//    BT_SEND_KEY = 0x08,
//    BT_LAST = 0x0c,
//    BT_NO_KEY = 0x10,
//    BT_NO_SUCH_REQ = 0x20,
//    BT_SEND_G = 0x40
//} HEADER_FLAG;
//
//void sendQTask();
//void receivedQTask();
