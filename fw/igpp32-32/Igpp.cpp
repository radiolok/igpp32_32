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

#include <Igpp.h>

//Double buffering mechanism
volatile uint8_t frameBuffer[2][(DISPLAY_HEIGTH >> 3) * DISPLAY_WIDTH] = {{0x00}, {0x00}};

volatile uint8_t currentFrame = 0;//or 1

/*
anode_latch P1.0
anode_mr P1.1
anode OE P1.2
anode srclk P4.0
anode DI P4.4

cathode_latch P1.3
cathode_mr P1.4
cathode OE P1.5
cathode srclk P4.3
cathode DI P4.1
*/

void igppInit()
{
    P1DIR |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5;
    P1OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5);

    P4DIR |= BIT1 | BIT3;
    P4OUT &= ~(BIT1 | BIT3);

    SpiInit();

    //Scan TA0: 40kHz

    TA0R = 0x00;
    TA0CCTL0 |= CCIE; //CCR0 interrupt
    TA0CCR0 = 150;// 7.5MHz/ 40kHz
    TA0CTL |= TASSEL_2 | MC_1 | ID_2; //SMCLK 26MHz div 4 = 7.5MHz

    //timeout TB):

    TB0R = 0x00;
    TB0CCTL0 |= CCIE; //CCR0 interrupt
    TB0CTL |= TBSSEL_2;        //SMCLK, 6MHz. Inerrupt Enabled , flag enabled

    P1OUT |= BIT1 | BIT4; //MR is up

    igppTick();
}

uint8_t anodesData[2][12] = {
                  {0xAA, 0xAA, 0xAA, 0xAA,
                   0xAA, 0xAA, 0xAA, 0xAA,
                   0xAA, 0xAA, 0xAA, 0xAA},
                  {0x55, 0x55, 0x55, 0x55,
                   0x55, 0x55, 0x55, 0x55,
                   0x55, 0x55, 0x55, 0x55}
                    };
uint8_t currentAnodesData = 0;

void igppAnodeClear()
{
    P1OUT  &= ~BIT1;
    P1OUT  |= BIT1;
    igppTick();
}

void igppLatch()
{
    static uint8_t cathodePos = 0;

    while (UCA0STAT & UCBUSY)
    {
        ;
    }
    if (cathodePos == 0)
    {
        igppCathodeClear();
        igppCathodeDataHigh();
        igppCathodeTick();
        igppCathodeDataLow();
    }
    else
    {
        igppCathodeTick();
    }
    if (++cathodePos >= DISPLAY_WIDTH)
    {
        cathodePos = 0;
    }
    P1OUT  |= BIT0 | BIT3;
    P1OUT  &= ~( BIT0 | BIT3);
    igppAnodeWait(20, igppAnodeClear);
}

void igppTick()
{

    currentAnodesData = (currentAnodesData + 1) & 0x01;
    SpiASend(anodesData[currentAnodesData], 12, igppLatch);
}

void igppSend(uint8_t column)
{

}

void igppSendAnode(uint8_t column)
{

}

void igppSendCathode(uint8_t column)
{

}

void (*m_callback)()  = NULL;


inline void igppAnodeWait(uint16_t us, void (*callback)())
{
//Time setup   TimerTB0:
    TB0CCR0 = us * 6 ; // 6MHz SMCL
    TB0R = 0;
    TB0CTL |= MC_1; // Count up to the TB0CCR0 value
    m_callback = callback;
}

//This timer does 50kHz f_scan
// Timer B0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMERA0_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) TIMERA0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    //igppTick();
}

// Timer B0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMERB0_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_B0_VECTOR))) TIMERB0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    TB0CTL &= ~MC_1;
    if (m_callback)
        (m_callback)();

}
