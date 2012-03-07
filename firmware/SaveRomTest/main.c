#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "HardwareProfile.h"
#include "log.h"



int main() {
    mInitAllLEDs();
    
    log_init(9600);
    log_printf("----------main----------\n");

    log_printf("going into loop...\n");
    while (1) {
        
    }
    return (0);
}
