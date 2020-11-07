

#include "uart.h"


void dma_restart()
{
    DMA1CTL &= ~DMAEN;
    uint8_t* currentBuffer = igppCurrentBufferPtr();
    DMACTL0 |= DMA1TSEL_20;  // USCIA1 receive as trigger
    DMA1CTL = DMADSTINCR_3 + DMADSTBYTE + DMASRCBYTE + DMAIFG + DMADT_4;
    DMA1SZ = PANEL_DATA_SIZE;
    __data16_write_addr((unsigned short) &DMA1SA,(unsigned long) &UCA1RXBUF);
    __data16_write_addr((unsigned short) &DMA1DA,(unsigned long) currentBuffer);
    DMA1CTL |= DMAEN;
}

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


    dma_restart();
}


void uart_putc(uint8_t c)
{
    while (UCA1STAT & UCBUSY)
    {
        ;
    }
    UCA1TXBUF = c;
}


//This timer does 16kHz f_scan
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A1_VECTOR
__interrupt void USACI_A1_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A1_VECTOR))) USACI_A1_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(UCA1IV,4))
      {
      case 0:break;                             // Vector 0 - no interrupt
      case 2:                                   // Vector 2 - RXIFG
        while (!(UCA1IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
        UCA1TXBUF = UCA1RXBUF;                  // TX -> RXed character
        break;
      case 4:break;                             // Vector 4 - TXIFG
      default: break;
      }
}
