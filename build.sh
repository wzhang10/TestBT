#!/bin/bash
IOIO_DIR=`pwd`
PROJ_DIR=$IOIO_DIR/firmware/$1
DIST_DIR=$PROJ_DIR/dist
BTSTACK_DIR=$IOIO_DIR/firmware/libbtstack
cd $BTSTACK_DIR
make  -f nbproject/Makefile-PIC24FJ256DA206.mk dist/PIC24FJ256DA206/libbtstack.a
cd $PROJ_DIR
#make clean
make
cd $IOIO_DIR
tools/make-ioio-bundle $DIST_DIR MyFirmware.ioioapp IOIO0023
adb push MyFirmware.ioioapp /sdcard/ioio/
