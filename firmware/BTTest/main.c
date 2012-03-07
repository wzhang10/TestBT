
/*
 * File:   main.c
 * Author: mark
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GenericTypeDefs.h"
#include "HardwareProfile.h"
#include "usb_config.h"
#include "usb_host_android.h"
#include "testbt.h"
#include "testconn.h"
#include "log.h"

void btCallback(int h, const void* data, unsigned long data_len) {
    mLED_0_On();
    if (data) {
        mLED_0_Toggle();
        //log_printf("%lx %lx %lx %lx %lx", data, data+2, data+4, data+6, data+8);
    } else {
        log_printf("btCallback data pointer invalid\n\r");
        ConnectionOpenChannelBtServer(&btCallback);
    }
}

int main() {
    mInitAllLEDs();
    
    log_init(9600);
    log_printf("----------btTestmain----------\n\r");
    connInit();
    while (1) {
        connTasks(); //does USB Host tasks and BT tasks
        if (canOpenChannel()) {
            ConnectionOpenChannelBtServer(&btCallback);
        }
    }
    return (0);
}
