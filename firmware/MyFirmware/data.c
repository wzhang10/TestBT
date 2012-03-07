#include "data.h"
unsigned char Data[KEY_MAX_SIZE+4];
unsigned char *keyStartAddress = Data+4;
unsigned int *keyIndexAddr = Data;
unsigned int *keySizeAddr = Data+2;
