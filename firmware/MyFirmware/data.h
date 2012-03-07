#define KEY_MAX_SIZE    1600
extern unsigned char *keyStartAddress;
extern unsigned int *keyIndexAddr;
extern unsigned int *keySizeAddr;
#define keyIndex ((*keyIndexAddr))
#define keySize ((*keySizeAddr))
