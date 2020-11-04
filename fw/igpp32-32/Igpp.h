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

#ifndef IGPP_H_
#define IGPP_H_

#include <string.h>
#include "define.h"

enum igppFlagTypes
{
  igppFlagNone = 0,
  igppAnodesErase = 1,
  igppChangeBufferPending = 2
};

void spiInit();

void igppInit();

void igppAnodeClear();

inline void igppCathodeTick()
{
    P4OUT  |= BIT3;
    __no_operation();
    P4OUT  &= ~BIT3;
}

inline void igppCathodeDataHigh()
{
    P4OUT  |= BIT1;
}

inline void igppCathodeDataLow()
{
    P4OUT  &= ~BIT1;
}

inline void igppLatchAll()
{
    while (UCA0STAT & UCBUSY)
    {
        ;
    }
    P1OUT  |= BIT0 | BIT3;
    P1OUT  &= ~( BIT0 | BIT3);
}

inline void igppCathodeClear()
{
    P1OUT  &= ~BIT4;
    P1OUT  |= BIT4;
}

inline void igppCathodeLatch()
{
    P1OUT  |= BIT3;
    P1OUT  &= ~BIT3;
}

inline void igppAnodeLatch()
{
    P1OUT  |= BIT0;
    P1OUT  &= ~( BIT0);
}

inline void igppAnodeClear()
{
    P1OUT  &= ~(BIT1);
    P1OUT  |= (BIT1);
}

inline void igppClearAll()
{
    P1OUT  &= ~(BIT1 | BIT4);
    P1OUT  |= (BIT1 | BIT4);
}

void igppNextFrame();

uint8_t* igppLoadBufferPtr();
uint8_t* igppCurrentBufferPtr();

void igppChangeBuffer();

#endif /* IGPP_H_ */
