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

#include "uart.h"

void uart_init()
{
    PMAPKEYID = 0x2D52;

    P4MAP4 = PM_UCA1TXD;

    P4MAP5 = PM_UCA1RXD;

    P4SEL |= BIT4+BIT5;                       // P4.4,5 = USCI_A1 TXD/RXD
    UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA1CTL1 |= UCSSEL_2;                     // SMCLK
    UCA1BR0 = 48;                              // 26MHz 500000 (see User's Guide)
    UCA1BR1 = 0;                              // 1MHz 115200
    UCA1MCTL |= UCBRS_0 + UCBRF_0;            // Modulation UCBRSx=0, UCBRFx=0
    UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    DMA1CTL &= ~DMAEN;
    uint8_t* currentBuffer = igppCurrentBufferPtr();
    DMACTL0 |= DMA1TSEL_20;  // USCIA1 receive as trigger
    DMA1CTL = DMADSTINCR_3 + DMADSTBYTE + DMASRCBYTE + DMAIFG + DMADT_4;
    DMA1SZ = PANEL_DATA_SIZE;
    __data16_write_addr((unsigned short) &DMA1SA,(unsigned long) &UCA1RXBUF);
    __data16_write_addr((unsigned short) &DMA1DA,(unsigned long) currentBuffer);
    DMA1CTL |= DMAEN;
}
