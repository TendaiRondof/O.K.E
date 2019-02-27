/*************************************************************************** 
    MSW-Winterthur Berufsfachschule, Zeughausstrasse 56, 8400 Winterthur     
    ---------------------------------------------------------------------    
    File        : sirius_driver.c
    Datum       : 1.4.2015
    Version			:	1.00
    Beschreibung: HW-Treiber für BT_CAR_V2_0
    Hardware    : ATMEGA644
    Autor  			:	Ch. Riedel

****************************************************************************
    History
  		5.4.2015	: 1. Version erstellt	(ri)

****************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "BT_CAR_V2_0.h"
#include <avr/pgmspace.h>							// Ermöglicht die Platzierung von "static const" im Code-Segment, statt im RAM.
													// Definition mit "PROGMEM", Lesen mit "pgm_read_byte, pgm_read_ptr"
																		
#pragma GCC optimize 0								// Optimierung ausschalten, damit das Debugging möglich ist

void LED_Show(void)
{
	u8 i;
	
	PORTB=0x01;
	for(i=0;i<8;i++)
	{
		wait_1ms(50);
		PORTB=0x01<<i;
	}
	PORTB=0x80;
	for(i=0;i<8;i++)
	{
		wait_1ms(50);
		PORTB=0x80>>i;
	}
	wait_1ms(100);
	PORTB=0x00;	
}

void init_BT_CAR_V2_0(void)
{
	// Initialisierung PortA
	DDRA  = 0x60;			// Eingang PA0..PA4 as Input (analog Signals), PA5 and PA6 as Output and PA7 as Input (SRG Ansteuerung)

	//Initialisierung Port B
	DDRB  =  0xFF;			// LED Port, alles Outputs, ACHTUNG PB.0...PB.3 sind auch Taster am LCD Display

	//Initialisierung Port C
	//PC0, PC1 im LCD Driver definiert
	//PC2...PC5 für JTAG Schnittstelle
	//PC6, PC7 32.678 KHz Quarz 

	//Initialisierung PortD
	DDRD  =  0xFC;			// PD7,PD6,PD5,PD4 als Outputs (PWM), PD3 Output (Enable HB), PD2 Output (Summer)
							// PD1 und PD0 sind TxD und RxD von UART0 werden über RXEN und TXEN aktiviert
							
	sei(); 					// Global Interrupts aktivieren	
	lcd_init();				// Das LCD muss ebenfalls initialisiert werden, um später genutz werden zu können
	clear_lcd();
	//write_text(0, 0,PSTR("LCD init ok"));
	LED_Show();	
}

