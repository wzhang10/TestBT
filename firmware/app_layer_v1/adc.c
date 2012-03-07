/*
 * Copyright 2011 Ytai Ben-Tsvi. All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ARSHAN POURSOHI OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied.
 */

#include "adc.h"

#include "Compiler.h"
#include "logging.h"
#include "protocol.h"
#include "pins.h"

static unsigned int analog_scan_bitmask;
static int analog_scan_num_channels;

// we need to generate a priority 1 interrupt in order to send a message
// containing ADC-captured data.
// this is the reasononing:
// we need to protect the outgoing-message buffer from concurrent access. this
// is achieved by making sure it is only written to by priority 1 code.
// however, in the case of ADC, we must service the "done" interrupt quickly
// to stop the ADC before our buffer gets overwritten, so this would be a
// priority 6 interrupt. then, in order to write to the output buffer, it would
// trigger the priority 1 interrupt using GFX1, that will read the ADC data and
// write to the buffer.
// we've "abused" GFX1 interrupt that is never used in order to generate the
// interrupt - we just manually raise its IF flag whenever we need an interrupt
// and service this interrupt.
static inline void ScanDoneInterruptInit() {
  _GFX1IF = 0;
  _GFX1IP = 1;
  _GFX1IE = 1;
}

// call this function to generate the priority 1 interrupt.
static inline void ScanDoneInterruptTrigger() {
  _GFX1IF = 1;
}

// timer 3 is clocked @2MHz
// we set its period to 2000 so that a match occurs @1KHz
// used for ADC
static inline void Timer3Prepare() {
  _T3IE = 0;       // disable interrupt
  _T3IF = 0;       // clear interrupt
//  PR3   = 0x4E20;  // period is 20000 clocks
  PR3   = 0x07CF;  // period is 2000 clocks
  _T3IP = 1;       // interrupt priority 1 (this interrupt may write to outgoing channel)
}

static inline void Timer3Start() {
  TMR3  = 0x0000;  // reset counter
  _T3IE = 1;       // enable interrupt
}

static inline void Timer3Stop() {
  _T3IE = 0;       // disable interrupt
  _T3IF = 0;       // clear interrupt
}

void ADCInit() {
  _AD1IE = 0;        // disable interrupt
  _AD1IF = 0;        // clear interrupt
  AD1CON1 = 0x0000;  // ADC off
  AD1CON2 = 0x0400;  // Avdd Avss ref, scan inputs, single buffer, interrupt on every sample 
  AD1CON3 = 0x1F01;  // system clock, 31 Tad acquisition time, ADC clock @8MHz
  AD1CHS  = 0x0000;  // Sample AN0 against negative reference.
  AD1CSSL = 0x0000;  // reset scan mask.

  _AD1IF = 0;
  _AD1IP = 6;        // high priority to stop automatic sampling
  _AD1IE = 1;        // enable interrupt

  ScanDoneInterruptInit();  // when triggered, generates an immediate interrupt to read ADC buffer
  Timer3Prepare();  // runs when ADC is used to periodically trigger sampling

  analog_scan_bitmask = 0x0000;
  analog_scan_num_channels = 0;
}

static inline int CountOnes(unsigned int val) {
  int res = 0;
  while (val) {
    if (val & 1) ++res;
    val >>= 1;
  }
  return res;
}

static inline void ReportAnalogInStatus() {
  volatile unsigned int* buf = &ADC1BUF0;
  int num_channels = CountOnes(AD1CSSL);
  int i;
  BYTE var_arg[16];
  int var_arg_pos = 0;
  int group_header_pos;
  int pos_in_group;
  int value;
  OUTGOING_MESSAGE msg;
  msg.type = REPORT_ANALOG_IN_STATUS;
  for (i = 0; i < num_channels; i++) {
    pos_in_group = i & 3;
    if (pos_in_group == 0) {
      group_header_pos = var_arg_pos;
      var_arg[var_arg_pos++] = 0;  // reset header
    }
    value = buf[i];
    //log_printf("%d", value);
    var_arg[group_header_pos] |= (value & 3) << (pos_in_group * 2);  // two LSb to group header
    var_arg[var_arg_pos++] = value >> 2;  // eight MSb to channel byte
  }
  AppProtocolSendMessageWithVarArg(&msg, var_arg, var_arg_pos);
}

static inline void ReportAnalogInFormat() {
  unsigned int mask = analog_scan_bitmask;
  int channel = 0;
  BYTE var_arg[16 / 4 * 5];
  int var_arg_pos = 0;
  OUTGOING_MESSAGE msg;
  msg.type = REPORT_ANALOG_IN_FORMAT;
  msg.args.report_analog_in_format.num_pins = analog_scan_num_channels;
  while (mask) {
    if (mask & 1) {
      var_arg[var_arg_pos++] = PinFromAnalogChannel(channel);
    }
    mask >>= 1;
    ++channel;
  }
  AppProtocolSendMessageWithVarArg(&msg, var_arg, var_arg_pos);
}

static inline void ADCStart() {
  Timer3Start();
}

static inline void ADCStop() {
  Timer3Stop();
}

static inline void ADCTrigger() {
  if (AD1CSSL != analog_scan_bitmask) {
    ReportAnalogInFormat();
    AD1CSSL = analog_scan_bitmask;
    _SMPI = analog_scan_num_channels - 1;
  }
  if (analog_scan_num_channels) {
    AD1CON1 = 0x80E6;  // start ADC
  } else {
    ADCStop();
  }
}

void ADCSetScan(int pin, int enable) {
  int channel = PinToAnalogChannel(pin);
  int mask;
  if (channel == -1) return;
  mask = 1 << channel;
  if (!!(mask & analog_scan_bitmask) == enable) return;

  if (enable) {
    if (analog_scan_num_channels) {
      // already running, just add the new channel
      _T3IE = 0;
      ++analog_scan_num_channels;
      analog_scan_bitmask |= mask;
      _T3IE = 1;
    } else {
      // first channel, start running
      analog_scan_num_channels = 1;
      analog_scan_bitmask = mask;
      ADCStart();
    }
  } else {
    // if this was the last channel, the next T2 interrupt will stop the sampling
    _T3IE = 0;
    --analog_scan_num_channels;
    analog_scan_bitmask &= ~mask;
    _T3IE = 1;
  }
}

void __attribute__((__interrupt__, auto_psv)) _T3Interrupt() {
  // TODO: check that the previous sample is done?
  ADCTrigger();
  _T3IF = 0;  // clear
}

void __attribute__((__interrupt__, auto_psv)) _GFX1Interrupt() {
  ReportAnalogInStatus();
  _GFX1IF = 0;  // clear
}

void __attribute__((__interrupt__, auto_psv)) _ADC1Interrupt() {
  AD1CON1 = 0x0000;  // ADC off
  ScanDoneInterruptTrigger();
  _AD1IF = 0;  // clear
}
