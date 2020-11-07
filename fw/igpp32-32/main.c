/*BSD 2-Clause License

Copyright (c) 2020, Artem Kashkanov
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#include <string.h>

#include "define.h"
#include "driverlib.h"

#include "Igpp.h"
#include "uart.h"
#include "game_of_life.h"

/*
 * NOTE: Modify hal.h to select a specific evaluation board and customize for
 * your own board.
 */
#include "hal.h"


int main(void)
{
	P6DIR |= BIT0;
	P6OUT &= ~BIT0;

    /*
    XT1 = DCO, 24MHz

    MCLK = 24MHz

    SMCLK = 24MHz

    F_scan = 40kHz

    XT2 = 16MHz - for USB Clock

	 */
  // Hold the watchdog timer.
  WDT_A_hold(__MSP430_BASEADDRESS_WDT_A__);

  // MSP430 USB requires a Vcore setting of at least 2.
  PMM_setVCore(PMM_CORE_LEVEL_3);

  USBHAL_initPorts();           // Config GPIOS for low-power (output low)
  UCSCTL3 = SELREF_2;                       // Set DCO FLL reference = REFO

  __bis_SR_register(SCG0);                  // Disable the FLL control loop
  UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
  UCSCTL1 = DCORSEL_7;                      // Select DCO range 50MHz operation
  UCSCTL2 = FLLD_0 + 731;                   // Set DCO Multiplier for 24MHz
                                          // (N + 1) * FLLRef = Fdco
                                          // (731 + 1) * 32768 = 24MHz
  __bic_SR_register(SCG0);                  // Enable the FLL control loop

  __delay_cycles(782000);

  UCSCTL4 |= SELM_3;                       //MCLK = FLL = 24MHz
  UCSCTL5 |= DIVM_0;                       //f_dco/1

  UCSCTL4 |= SELS_3;                      // SMCLK = FLL = 24MHz
  UCSCTL5 |= DIVS_0;                      // f_dco / 1

  UCSCTL4 |= SELA_2;                        // Set ACLK = VCO

    igppInit();
    uart_init();

#ifdef GAME_OF_LIFE
    initEpoch();
#endif

    __enable_interrupt();  // Enable interrupts globally

   // uart_putc('A');

    while (1)
        {
        __bis_SR_register(LPM0_bits + GIE);
        //uart_putc('A');

#ifdef GAME_OF_LIFE
            checkEpoch();
#endif

        }  //while(1)
}
