#include "testconn.h"
#include "testbt.h"
#include "usb_host_bluetooth.h"
#include "log.h"

static BT_STATE bt_state;

BOOL USB_ApplicationEventHandler(BYTE address, USB_EVENT event, void *data, DWORD size) {
    switch (event) {
   case EVENT_VBUS_REQUEST_POWER:
    // We'll let anything attach.
    return TRUE;

   case EVENT_VBUS_RELEASE_POWER:
    // We aren't keeping track of power.
    return TRUE;

   case EVENT_HUB_ATTACH:
    log_printf("***** USB Error - hubs are not supported *****");
    return TRUE;

   case EVENT_UNSUPPORTED_DEVICE:
    log_printf("***** USB Error - device is not supported *****");
    return TRUE;

   case EVENT_CANNOT_ENUMERATE:
    log_printf("***** USB Error - cannot enumerate device *****");
    return TRUE;

   case EVENT_CLIENT_INIT_ERROR:
    log_printf("***** USB Error - client driver initialization error *****");
    return TRUE;

   case EVENT_OUT_OF_MEMORY:
    log_printf("***** USB Error - out of heap memory *****");
    return TRUE;

   case EVENT_UNSPECIFIED_ERROR:   // This should never be generated.
    log_printf("***** USB Error - unspecified *****");
    return TRUE;

   default:
    return FALSE;
    }
}

int canOpenChannel(){
    return (bt_state==STATE_BT_INITIALIZED)?1:0;
}

static void ConnBTTasks() {
  switch (bt_state) {
    case STATE_BT_DISCONNECTED:
      if (USBHostBluetoothIsDeviceAttached()) {
        BTInit();
        bt_state = STATE_BT_INITIALIZING;
      }
      break;

    case STATE_BT_INITIALIZING:
    case STATE_BT_INITIALIZED:
      if (!USBHostBluetoothIsDeviceAttached()) {
        // disconnected
        BTShutdown();
        bt_state = STATE_BT_DISCONNECTED;
      } else {
#ifndef USB_ENABLE_TRANSFER_EVENT
        USBHostBluetoothTasks();
#endif
        BTTasks();
        bt_state = BTAccepting() ? STATE_BT_INITIALIZED : STATE_BT_INITIALIZING;
        break;
      }
  }
}

void connInit(){
    int res = USBHostInit(0);
    assert(res);
    bt_state = STATE_BT_DISCONNECTED;
}

int connTasks() {
  USBHostTasks();
  ConnBTTasks();

  return bt_state != STATE_BT_DISCONNECTED;
}

void ConnectionOpenChannelBtServer(ChannelCallback cb) {
  assert(BTAccepting());
  BTSetCallback(cb);
  // only one BT channel currently
}
