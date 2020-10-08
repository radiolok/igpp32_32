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

#include <Spi.h>



void SpiInit()
{
    P4SEL |= BIT0 | BIT1 | BIT3 | BIT4;

    PMAPKEYID = 0x2D52;

    P4MAP0 = PM_UCA0CLK;
    P4MAP1 = PM_UCB0SIMO;

    P4MAP3 = PM_UCB0CLK;
    P4MAP4 = PM_UCA0SIMO;

    UCA0CTL0 = UCMST | UCSYNC;//MSB, syncronous
    UCA0CTL1 = UCSSEL_2; //SMCLK

    UCA0BR0 = 4;// 26MHz/4 = 7.5MHz

    //UCA0IE = UCTXIE; //enable interrupt

}

void SpiATest()
{
    UCA0TXBUF = 0xAA;
}


uint8_t SpiABuffer[SPI_BUFFER_SIZE] = {0};
volatile uint16_t SpiABufferTxHead = 0;
volatile uint16_t SpiABufferTxLength = 0;

/**Anodes Spi*/
void SpiASend(uint8_t* data, uint16_t size)
{
    DMACTL0 |= DMA1TSEL_17;  // UCB1TXIFG as trigger
    DMA1CTL = DMASRCINCR_3 + DMADSTBYTE + DMASRCBYTE + DMAIE + DMAEN;
    __data16_write_addr((unsigned short) &DMA1SA,(unsigned long) *data);
    __data16_write_addr((unsigned short) &DMA1DA,(unsigned long) &UCA0TXBUF);
    DMA1SZ = size;
    if (!(DMA1CTL & DMAEN)) {
        DMA1SZ = size;
        DMA1CTL |= DMAEN;
    }
    UCA0IFG &= ~UCTXIFG;
    UCA0IFG |=  UCTXIFG;
}

/**Cathodes Spi*/
void SpiBSend(uint8_t* data, uint16_t size)
{

}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt void SPI_A_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A0_VECTOR))) SPI_A_ISR (void)
#else
#error Compiler not supported!
#endif
{
    if (UCA0IV & 0x04)
    {
        if ( SpiABufferTxHead < SpiABufferTxLength)
        {
            UCA0TXBUF = SpiABuffer[SpiABufferTxHead++];
        }
    }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_B0_VECTOR
__interrupt void SPI_B_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B0_VECTOR))) SPI_B_ISR (void)
#else
#error Compiler not supported!
#endif
{

}
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=DMA_VECTOR
__interrupt void DmaIsr (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(DMA_VECTOR))) DmaIsr (void)
#else
#error Compiler not supported!
#endif
{
    if (DMAIV & DMAIV_DMA0IFG) {
        return;
    }
    if (DMAIV & DMAIV_DMA1IFG) {
        return;
    }
    if (DMAIV & DMAIV_DMA2IFG) {
        return;
    }

}
