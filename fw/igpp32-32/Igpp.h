/*MIT License

Copyright (c) 2020 Artem Kashkanov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#ifndef IGPP_H_
#define IGPP_H_

#include "define.h"

#include "Spi.h"

void igppInit();

inline void igppClear()
{
    P1OUT  &= ~BIT1;
    P1OUT  |= BIT1;
}

inline void igppLatch()
{
    P1OUT |= BIT0 | BIT3;
    //250ns
    P1OUT &= ~(BIT0 | BIT3);
}

inline void igppCathodeOn() {P1OUT |= BIT5;}
inline void igppCathodeOff() {P1OUT &= ~BIT5;}

void igppAnodeOn();

void igppAnodeOff();

void igppAnodeWait(uint16_t us, void (*callback)());

void igppSend(uint8_t column);

void igppSendAnode(uint8_t column);

void igppSendCathode(uint8_t column);

void igppAnodeWait(uint16_t us, void (*callback)());

#endif /* IGPP_H_ */
