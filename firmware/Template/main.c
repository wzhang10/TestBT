
/*
 * File:   main.c
 * Author: mark
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "HardwareProfile.h"
#include "log.h"

int main() {
    mInitAllLEDs();
    
    log_init(9600);
    log_printf("----------main----------\n");

    while (1) {
    }
    return (0);
}
