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


#include <string.h>

#include "define.h"
#include "driverlib.h"


#include "Igpp.h"

#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/usb.h"                 // USB-specific functions
#include "USB_API/USB_HID_API/UsbHid.h"
#include "USB_app/usbConstructs.h"

// Global flags set by events
volatile uint8_t bHIDDataReceived_event = FALSE;
volatile uint8_t bDataReceiveCompleted_event = FALSE;

// Application globals
volatile uint16_t usbEvents = USB_VBUSON_EVENTMASK + USB_VBUSOFF_EVENTMASK +
    USB_DATARECEIVED_EVENTMASK + USB_USBSUSPEND_EVENTMASK +
    USB_USBRESUME_EVENTMASK +
    USB_USBRESET_EVENTMASK;

char outString[40] = "";
uint16_t i;
uint16_t x,y;
uint8_t ret;

int main(void)
{
	/*
    XT1 = DCO, 24MHz

    MCLK = 24MHz

    SMCLK = 6MHz

    F_scan = 40kHz

    XT2 = 16MHz - for USB Clock

	 */
  // Hold the watchdog timer.
  WDT_A_hold(__MSP430_BASEADDRESS_WDT_A__);

  // MSP430 USB requires a Vcore setting of at least 2.
  PMM_setVCore(PMM_CORE_LEVEL_3);

    UCSCTL3 = SELREF_2;                       // Set DCO FLL reference = REFO
    UCSCTL4 |= SELA_2;                        // Set ACLK = REFO

    __bis_SR_register(SCG0);                  // Disable the FLL control loop
    UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
    UCSCTL1 = DCORSEL_7;                      // Select DCO range 50MHz operation
    UCSCTL2 = FLLD_0 + 731;                   // Set DCO Multiplier for 24MHz
                                            // (N + 1) * FLLRef = Fdco
                                            // (731 + 1) * 32768 = 24MHz
    __bic_SR_register(SCG0);                  // Enable the FLL control loop

    __delay_cycles(782000);

    UCSCTL4 |= SELM_5;                       //MCLK = XT2 = 26MHz
    UCSCTL5 |= DIVM_0;                       //f_dco/1

    UCSCTL4 |= SELS_5;                      // SMCLK = XT2 = 26MHz
    UCSCTL5 |= DIVS_0;                      // f_dco / 1

   igppInit();

    __enable_interrupt();  // Enable interrupts globally

    while (1)
       {
           // Check the USB state and directly main loop accordingly
           switch (USB_getConnectionState())
           {
               // This case is executed while your device is enumerated on the
               // USB host
               case ST_ENUM_ACTIVE:
                   // Enter LPM0 until an event occurs.
                   __bis_SR_register(LPM0_bits + GIE);

                   // This flag is set by the handleDataReceived event; this
                   // event is only enabled when waiting for 'press any key'
                   if (bHIDDataReceived_event){
                       bHIDDataReceived_event = FALSE;

                       // Change the event flags, in preparation for receiving 1K
                       // data. No more data-received.  We only used this for
                       // 'press any key'
                       usbEvents &= ~USB_DATARECEIVED_EVENTMASK;

                       // But enable receive-completed; we want to be prompted when
                       // 1K data has been received
                       usbEvents |= USB_RECEIVECOMPLETED_EVENTMASK;

                       USB_setEnabledEvents(usbEvents);
                       // We don't care what char the key-press was so we reject it
                       USBHID_rejectData(HID0_INTFNUM);

                       strcpy(outString,"I'm ready to receive 1K of data.\r\n");

                       // Send it over USB. If it failed for some reason; abort and
                       // leave the main loop
                       if ( USBHID_sendDataAndWaitTillDone((uint8_t*)outString,
                               strlen(outString),HID0_INTFNUM,0)){
                           USBHID_abortSend(&x,HID0_INTFNUM);
                           break;
                       }

                       uint8_t* currentBuffer = igppLoadBufferPtr();
                       // If USBHID_receiveData fails because of surprise removal
                       // or suspended by host abort and leave main loop
                       if (USBHID_receiveData(currentBuffer,PANEL_DATA_SIZE, HID0_INTFNUM) ==
                           USBHID_BUS_NOT_AVAILABLE){
                           USBHID_abortReceive(&x,HID0_INTFNUM);
                           break;
                       }
                   }

                   // This flag would have been set by the handleReceiveCompleted
                   // event; this event is only enabled while receiving 1K data,
                   // and signals that all 1K has been received
                   if (bDataReceiveCompleted_event){
                       bDataReceiveCompleted_event = FALSE;
                       // Prepare the outgoing string
                       igppChangeBuffer();
                       strcpy(outString,"Thanks for the data.\r\n");
                       // Send the response over USB.  If it failed for some reason
                       // abort and leave the main loop
                       if (USBHID_sendDataInBackground((uint8_t*)outString,
                               strlen(outString),HID0_INTFNUM,0)){
                           USBHID_abortSend(&x,HID0_INTFNUM);
                           break;
                       }

                       // Change the event flags, in preparation for 'press any key'
                       // No more receive-completed.
                       usbEvents &= ~USB_RECEIVECOMPLETED_EVENTMASK;

                       // This will tell us that data -- any key -- has arrived
                       usbEvents |= USB_DATARECEIVED_EVENTMASK;
                       USB_setEnabledEvents(usbEvents);
                   }

                   break;


               // These cases are executed while your device is disconnected from
               // the host (meaning, not enumerated); enumerated but suspended
               // by the host, or connected to a powered hub without a USB host
               // present.
               case ST_PHYS_DISCONNECTED:
               case ST_ENUM_SUSPENDED:
               case ST_PHYS_CONNECTED_NOENUM_SUSP:
                   __bis_SR_register(LPM3_bits + GIE);
                   _NOP();
                   break;

               // The default is executed for the momentary state
               // ST_ENUM_IN_PROGRESS.  Usually, this state only last a few
               // seconds.  Be sure not to enter LPM3 in this state; USB
               // communication is taking place here, and therefore the mode must
               // be LPM0 or active-CPU.
               case ST_ENUM_IN_PROGRESS:
               default:;
           }

       }  //while(1)
}

/*
 * ======== UNMI_ISR ========
 */
#if defined(__TI_COMPILER_VERSION__) || (__IAR_SYSTEMS_ICC__)
#pragma vector = UNMI_VECTOR
__interrupt void UNMI_ISR (void)
#elif defined(__GNUC__) && (__MSP430__)
void __attribute__ ((interrupt(UNMI_VECTOR))) UNMI_ISR (void)
#else
#error Compiler not found!
#endif
{
    switch (__even_in_range(SYSUNIV, SYSUNIV_BUSIFG ))
    {
        case SYSUNIV_NONE:
            __no_operation();
            break;
        case SYSUNIV_NMIIFG:
            __no_operation();
            break;
        case SYSUNIV_OFIFG:
            UCS_clearFaultFlag(UCS_XT2OFFG);
            UCS_clearFaultFlag(UCS_DCOFFG);
            SFR_clearInterrupt(SFR_OSCILLATOR_FAULT_INTERRUPT);
            break;
        case SYSUNIV_ACCVIFG:
            __no_operation();
            break;
        case SYSUNIV_BUSIFG:
            // If the CPU accesses USB memory while the USB module is
            // suspended, a "bus error" can occur.  This generates an NMI.  If
            // USB is automatically disconnecting in your software, set a
            // breakpoint here and see if execution hits it.  See the
            // Programmer's Guide for more information.
            SYSBERRIV = 0; //clear bus error flag
            USB_disable(); //Disable
    }
}
