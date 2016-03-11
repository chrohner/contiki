/*
 * Copyright (C) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Sportident SRR test
// code adapted from the rime/all-timers.c code example


#include <stdio.h>

#include "contiki.h"
#include "sys/etimer.h"
#include "sys/stimer.h"
#include "sys/timer.h"
#include "sys/rtimer.h"
#include "leds.h"

#include "ti-lib.h"

#include "board-spi.h"
#include "srr.h"
#include "srr-const.h"


PROCESS(process1, "ETimer x Timer x STimer Process");
AUTOSTART_PROCESSES(&process1);

static int counter_etimer;
static int counter_timer;
static int counter_stimer;
static struct timer timer_timer;
static struct stimer timer_stimer;



void
do_timeout1()
{
    /* CC2500 example: set and read back channel and compare */
    /* alternate between blue and red channel, test for blue ->> red LED blinking */

    uint8_t ch[2] = {SRR_CHANNEL_BLUE, SRR_CHANNEL_RED};
    uint8_t buf = 0x00;
    bool ret;
    static uint8_t cnt = 0;
    
    leds_on(LEDS_RED);
    leds_on(LEDS_GREEN);
    clock_delay_usec(1000);  // LEDS be on at least for 1ms
    
    srr_open();

    srr_start();

    
    // set configuration
    srr_write(CC2500_CHANNR, ch[cnt%2]);
    
    // read register
    ret = srr_read(CC2500_READ | CC2500_CHANNR, &buf);
    
    if (ret) {
        leds_off(LEDS_GREEN);
    }
    if (buf == SRR_CHANNEL_BLUE) {
        leds_off(LEDS_RED);
    }
    
    cnt++;

    srr_close();
    
    
    
  counter_etimer++;
  if(timer_expired(&timer_timer)) {
    counter_timer++;
  }

  if(stimer_expired(&timer_stimer)) {
    counter_stimer++;
  }

  printf("\nProcess 1: %s", counter_timer == counter_etimer
         && counter_timer == counter_stimer ? "SUCCESS" : "FAIL");

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(process1, ev, data)
{
  static struct etimer timer_etimer;

  PROCESS_BEGIN();
  
    srr_reset();
    srr_config();
    
  while(1) {
    timer_set(&timer_timer, 2 * CLOCK_SECOND);
    stimer_set(&timer_stimer, 2);
    etimer_set(&timer_etimer, 2 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
    do_timeout1();
  }
    
  PROCESS_END();
}
