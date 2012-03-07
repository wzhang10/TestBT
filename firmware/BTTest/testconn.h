#include "GenericTypeDefs.h"

typedef void (*ChannelCallback) (int ch, const void* data, UINT32 size);

int canOpenChannel();
void connInit();
int connTasks();
void ConnectionOpenChannelBtServer(ChannelCallback cb);
