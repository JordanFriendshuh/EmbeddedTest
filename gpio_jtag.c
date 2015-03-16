//*****************************************************************************
//
// gpio_jtag.c - Example to demonstrate recovering the JTAG interface.
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "inc/hw_nvic.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "drivers/buttons.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>GPIO JTAG Recovery (gpio_jtag)</h1>
//!
//! This example demonstrates changing the JTAG pins into GPIOs, a with a
//! mechanism to revert them to JTAG pins.  When first run, the pins remain in
//! JTAG mode.  Pressing the USR_SW1 button will toggle the pins between JTAG
//! mode and GPIO mode.  Because there is no debouncing of the push button
//! (either in hardware or software), a button press will occasionally result
//! in more than one mode change.
//!
//! In this example, four pins (PC0, PC1, PC2, and PC3) are switched.
//!
//! UART0, connected to the ICDI virtual COM port and running at 115,200,
//! 8-N-1, is used to display messages from this application.
//
//*****************************************************************************

//*****************************************************************************
//
// System clock rate in Hz.
//
//*****************************************************************************
#define PIN_0 1
#define PIN_1 1<<1
#define PIN_2 1<<2
#define PIN_3 1<<3
#define PIN_4 1<<4
#define PIN_5 1<<5
#define PIN_6 1<<6
#define PIN_7 1<<6


uint32_t g_ui32SysClock;
int testFlag;

//*****************************************************************************
//
// The current mode of pins PC0, PC1, PC2, and PC3.  When zero, the pins
// are in JTAG mode; when non-zero, the pins are in GPIO mode.
//
//*****************************************************************************
volatile uint32_t g_ui32Mode;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// The interrupt handler for the PB4 pin interrupt.  When triggered, this will
// toggle the JTAG pins between JTAG and GPIO mode.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    uint8_t ui8Buttons;
    uint8_t ui8ButtonsChanged;

    //
    // Grab the current, debounced state of the buttons.
    //
    ui8Buttons = ButtonsPoll(&ui8ButtonsChanged, 0);

    //
    // If the USR_SW1 button has been pressed, and was previously not pressed,
    // start the process of changing the behavior of the JTAG pins.
    //
    if(BUTTON_PRESSED(USR_SW1, ui8Buttons, ui8ButtonsChanged))
    {
        //
        // Toggle the pin mode.
        //
        g_ui32Mode ^= 1;

        //
        // See if the pins should be in JTAG or GPIO mode.
        //
        if(g_ui32Mode == 0)
        {
            //
            // Change PC0-3 into hardware (i.e. JTAG) pins.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x01;
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= 0x01;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x02;
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= 0x02;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x04;
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= 0x04;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x08;
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= 0x08;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x00;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = 0;

            //
            // Turn on the LED to indicate that the pins are in JTAG mode.
            //
            ROM_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                             GPIO_PIN_0);
        }
        else
        {
            //
            // Change PC0-3 into GPIO inputs.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x01;
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) &= 0xfe;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x02;
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) &= 0xfd;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x04;
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) &= 0xfb;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x08;
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) &= 0xf7;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x00;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = 0;
            ROM_GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, (GPIO_PIN_0 | GPIO_PIN_1 |
                                                       GPIO_PIN_2 |
                                                       GPIO_PIN_3));

            //
            // Turn off the LED to indicate that the pins are in GPIO mode.
            //
            ROM_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                             GPIO_PIN_1);
        }
    }
}

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, g_ui32SysClock);
}

void spin(){
	volatile int i;
	for(i = 0; i<100; i++);
}

void timerInit(int count){
	int volatile data;
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R0;
	data = HWREG(SYSCTL_RCGCGPIO);
	spin();
	HWREG(TIMER0_BASE + TIMER_O_CTL) &= ~TIMER_CTL_TAEN; //disable timer for setup
	HWREG(TIMER0_BASE + TIMER_O_CFG) = TIMER_CFG_32_BIT_TIMER;
	HWREG(TIMER0_BASE + TIMER_O_TAMR) = TIMER_TAMR_TAMR_PERIOD;
	HWREG(TIMER0_BASE + TIMER_O_TAILR) = count;
	HWREG(TIMER0_BASE + TIMER_O_IMR) = TIMER_IMR_TATOIM;
	HWREG(NVIC_EN0) |= 0x00080000;
	HWREG(TIMER0_BASE + TIMER_O_CTL) |= TIMER_CTL_TAEN;
}

void timerHandler(){
	testFlag = 1;
}

void testIntEnable(){
	int volatile data;
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
	data = HWREG(SYSCTL_RCGCGPIO);
	spin();
	HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) &= ~(PIN_5 | PIN_0);                //set to input
	HWREG(GPIO_PORTB_BASE + GPIO_O_DEN) |= PIN_5 | PIN_0;
	//need some reg tricks to prevent false inturrupts
	HWREG(GPIO_PORTB_BASE + GPIO_O_IM) &= ~0xff;
	HWREG(GPIO_PORTB_BASE + GPIO_O_IS) &= ~0xff;
	HWREG(GPIO_PORTB_BASE + GPIO_O_IBE) &= ~0xff;
	HWREG(GPIO_PORTB_BASE + GPIO_O_IEV) |= PIN_5 | PIN_0;
	//HWREG(GPIO_PORTB_BASE + GPIO_O_IS) |= PIN_5 | PIN_0;
	//HWREG(GPIO_PORTB_BASE + GPIO_O_IBE) |= PIN_5 | PIN_0;                 //interrupt on both edges
	HWREG(GPIO_PORTB_BASE + GPIO_O_IM) |= PIN_5 | PIN_0;                  //set bits in the interrupt mask register
	HWREG(NVIC_EN0) |= NVIC_EN0_INT_M;
	spin();
	HWREG(NVIC_ST_RELOAD) = 15999;
	spin();
	HWREG(NVIC_ST_CURRENT) = 0;
	spin();
	HWREG(NVIC_ST_CTRL) |=  NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;
}

void portBInterrupt(){
	testFlag = 1;
}

//*****************************************************************************
//
// Toggle the JTAG pins between JTAG and GPIO mode with a push button selecting
// between the two.
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32Mode;

    //
    // Set the clocking to run directly from the crystal at 80MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 80000000);
    //
    // Enable the peripherals used by this application.
    //
    spin();
    //ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    spin();

    //testIntEnable();
    timerInit(0x0fffffff);
    //
    // Set up a SysTick Interrupt to handle polling and debouncing for our
    // buttons.
    //

   // IntMasterEnable();

    //
    // Configure the LEDs as outputs and turn them on in the JTAG state.
    //

//    UARTprintf("Starting this shit\n");

    //
    // Loop forever.  This loop simply exists to display on the UART the
    // current state of PC0-3; the handling of changing the JTAG pins to and
    // from GPIO mode is done in GPIO Interrupt Handler.
    //
    testFlag = 0;
    while(1)
    {
    	if(testFlag){
  //  		UARTprintf("Shit got changed\n");
    		testFlag = 0;
    	}

    }
}
