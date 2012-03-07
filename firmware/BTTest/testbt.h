#include "GenericTypeDefs.h"

typedef enum {
  STATE_BT_DISCONNECTED,
  STATE_BT_INITIALIZING,
  STATE_BT_INITIALIZED,
} BT_STATE;


typedef void (*BTCallback) (int h, const void *data, UINT32 size);

void BTInit();
void BTTasks();
void BTShutdown();
int  BTAccepting();
void BTSetCallback(BTCallback cb);
void BTWrite(const void *data, int size);
int  BTCanWrite();
void BTClose();
