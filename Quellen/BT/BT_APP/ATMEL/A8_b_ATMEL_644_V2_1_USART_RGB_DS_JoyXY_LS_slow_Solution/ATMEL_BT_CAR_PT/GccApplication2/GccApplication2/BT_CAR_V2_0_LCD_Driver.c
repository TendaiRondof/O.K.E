/*************************************************************************** 
    MSW-Winterthur Berufsfachschule, Zeughausstrasse 56, 8400 Winterthur     
    ---------------------------------------------------------------------    
    File        : sirius_driver.c
    Datum       : 1.4.2015
    Version			:	1.00
    Beschreibung: Treiber für ATMEL Mocca Mini Board
    Hardware    : ATMEL ATMEGA644
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


#define LCD_Refresh_Time				200			// Zeitintervall für LCD-Ausgaben:  200 x 1ms = 200ms 

u32 volatile tick_1ms_cnt		= 0;
u32 tick_1ms					= 0;
u32 tick_1s						= 0;
u16 LCD_Refresh_Time_cnt		= LCD_Refresh_Time;
u8  write_RGB_LCD_activ = 0;

void init_Timer0_1ms_int(void)
{
	// Initialisierung Timer0 1ms Interrupt
	TCCR0A |= (1 <<   WGM01);	//CTC with OCRA (Timer Counter Compare: Counter zählt bis OCRA, dann Counter Neustart)
	TCCR0B |= (1 <<   CS01) | (1 >> CS00);	//16MHz / 64 = 4us
	OCR0A = 250-1;	//250 * 4us = 1ms
	TIMSK0 |= (1 << OCIE0A);
}

ISR (TIMER0_COMPA_vect) // Dieser Timer-Interrupt löst im 1ms-Takt aus.
{
	tick_1ms++;
	if(! (tick_1ms%1000) ) tick_1s++;
	if(tick_1ms_cnt > 0) tick_1ms_cnt--;										// Wird für 1ms-Tick benötigt
}

void wait_1ms(u32 delay)
{
	if (delay != 0) tick_1ms_cnt = delay; else tick_1ms_cnt = 1;
	while(tick_1ms_cnt != 0);
}

void delay_nop(u8 time)				// time = 1 ==> Delay = 2.08us
															// time > 1 ==> Delay = time * 0.62us
{ u8 i;
	for(i=0; i<time; i++) __asm__ __volatile__ ("nop");
}



#pragma GCC optimize 2						// Optimitation-Level nicht verstellen, weil sonst das Timing der LCD-Schnittstelle nicht mehr stimmen würde!!!

// Siganlnamen für Daten und Clock
#define LCD_SOD_0					(PORTC &= ~0x01)	//PC.0: LCD SOD = 0  (Serial Out Data)
#define LCD_SOD_1					(PORTC |=  0x01)	//PC.0: LCD SOD = 1
#define LCD_SCLK_0				(PORTC &= ~0x02)	//PC.1: LCD SCLK = 0 (Serial CLOCK)
#define LCD_SCLK_1				(PORTC |=  0x02)	//PC.1: LCD SCLK = 1

void write_lcd_f(u8 rs, u8 wert)
{	unsigned char i;
	
	write_RGB_LCD_activ = 1;
	// LCD: EDIP204	
	//Manche Befehle müssen mehrfach ausgeführt werden, damit die minimale Pulslänge von xxx ns eingehalten wird. (bei 8MHz)

	//Synchronisierung: Clock-Signal 5x toggeln
	LCD_SOD_1;																			// Während nachfolgend 5 Synch-Bits gesendet werden muss SOD = 1 sein.
	LCD_SCLK_0;LCD_SCLK_0; LCD_SCLK_1;LCD_SCLK_1;		// Pulsbreite je 500ns (Keine Schleife verwenden, da sonst Laufzeit unnötig grösser wird.)
	LCD_SCLK_0;LCD_SCLK_0; LCD_SCLK_1;LCD_SCLK_1;
	LCD_SCLK_0;LCD_SCLK_0; LCD_SCLK_1;LCD_SCLK_1;
	LCD_SCLK_0;LCD_SCLK_0; LCD_SCLK_1;LCD_SCLK_1;
	LCD_SCLK_0;LCD_SCLK_0; LCD_SCLK_1;
	
	//R/W: 1=Read, 0=Write
	LCD_SOD_0;																									// R/W = 0
	LCD_SCLK_0;LCD_SCLK_0; LCD_SCLK_1;													// R/W-Bit senden
	
	//RS Register Selection: 0=Command, 1=Data
	if (rs == 'C') LCD_SOD_0; else LCD_SOD_1; 
	LCD_SCLK_0;LCD_SCLK_0; LCD_SCLK_1;													// RS-Bit senden

	//End-Marke 0
	LCD_SOD_0;
	LCD_SCLK_0;LCD_SCLK_0; LCD_SCLK_1;													// END-Bit senden
	
	for (i = 0; i < 4; i++)																			// Daten-Bit 0-3 senden
	{ LCD_SCLK_0;
		if (wert & 0x01) LCD_SOD_1; else LCD_SOD_0;
		wert = wert >> 1;
		LCD_SCLK_0;	LCD_SCLK_1;
	}

	LCD_SOD_0;																									// 4x "0" senden
	for (i = 0; i < 4; i++) {LCD_SCLK_0;LCD_SCLK_0; LCD_SCLK_1;}
	
	for (i = 0; i < 4; i++)																			// Daten-Bit 4-7 senden
	{ LCD_SCLK_0;
		if (wert & 0x01) LCD_SOD_1; else LCD_SOD_0;
		wert = wert >> 1;
		LCD_SCLK_0;	LCD_SCLK_1;
	}

	LCD_SOD_0;																									// 4x "0" senden
	for (i = 0; i < 4; i++)	{LCD_SCLK_0;LCD_SCLK_0; LCD_SCLK_1;}

	// Write-Befehl auf 50us verlängern, damit minimale Execution-Time 39us/43us eingehalten ist.
	for (i = 0; i < 20; i++) LCD_SOD_1;
	write_RGB_LCD_activ = 0;
}

/**********************************************************************************
  write_text
 
  Text an xy-Position ausgeben (auf integriertem LCD-Display)
  y_pos:   Zeile-Nummer (0..3)
  x_pos:   Spalte-Nummer (0..19) (Zeichenposition auf Zeile)
  str_ptr: Adresse des zu schreibenden Textes
***********************************************************************************/
void write_text(u8 y_pos, u8 x_pos, const char* str_ptr)
{	u8 wert, str_p = 0;
	
	x_pos += y_pos * 0x20;																					// Position auf LCD berechnen 4x20 Zeichen
	write_lcd_f ('C',x_pos | 0x80);																	// LCD-Cursor auf gewünsschte Adresse setzen = Position auf LCD
	wert = pgm_read_byte(&str_ptr[str_p++]);
	while(wert != 0)
	{	write_lcd_f ('D',wert);																				// Daten in obige Adresse schreiben
		wert = pgm_read_byte(&str_ptr[str_p++]);
	}
}

/**********************************************************************************\
* write_text_RAM
*
* Text an xy-Position ausgeben (auf integriertem LCD-Display)
* y_pos:   Zeile-Nummer (0..3)
* x_pos:   Spalte-Nummer (0..19) (Zeichenposition auf Zeile)
* str_ptr: Adresse des zu schreibenden Textes
\**********************************************************************************/
void write_text_ram(u8 y_pos, u8 x_pos, const char* str_ptr)
{	u8 str_p = 0;
	
	x_pos += y_pos * 0x20;																					// Position auf LCD berechnen 4x20 Zeichen
	write_lcd_f ('C',x_pos | 0x80);																	// LCD-Cursor auf gewünsschte Adresse setzen = Position auf LCD
	while (str_ptr[str_p]) write_lcd_f ('D',str_ptr[str_p++]);			// Daten in obige Adresse schreiben
}


/**********************************************************************************\
* write_zahl
*
* Zahl an xy-Position ausgeben (auf integriertem LCD-Display)
* y_pos:   Zeile-Nummer (0..3)
* x_pos:   Spalte-Nummer (0..19) (Zeichenposition auf Zeile)
* zahl_v:  Zahl (0...4'294'967'296)
* s_vk:
* s_nk:
* komma:  0 = kein Komma
*         1 = vor der      letzten Ziffer das Komma setzen (Zahl=Zahl/10)
*         2 = vor der zweitletzten Ziffer das Komma setzen (Zahl=Zahl/100)
*         3 = vor der drittletzten Ziffer das Komma setzen (Zahl=Zahl/1'000)
*         9 = vor der viertletzten Ziffer das Komma setzen (Zahl=Zahl/10'000)
\**********************************************************************************/
void write_zahl(u8 x_pos, u8 y_pos, u32 zahl_v, u8 s_vk, u8 s_nk, u8 komma)
{
	u32		zehner;
	char	send_buffer[12];
	u8		i, pos, pos_t, nullen_loeschen = 1;

	//Umwandlung in die einzelnen Stellen-Zahlen 1er, 10er, 100er, ... 1'000'000'000er
	//zahl_v = 1234567890;

	if(s_vk >= 100) 
	{ s_vk -= 100;
		nullen_loeschen = 0;
	}
	if(s_vk > 10) s_vk = 10;
	
	if (s_nk > komma) s_nk = komma;									// unmöglicher Fall: mehr Nachkommastellen als Komma überhaupt geschoben werden 
	if (s_vk + s_nk > 10) s_nk = 10 - s_vk;					// unsinniger  Fall: zu viele Stellen
	
	zehner		= 10;
	send_buffer[11] = (zahl_v % 10) + 48;
	i = 10;
	do
	{ send_buffer[i] = ( (zahl_v / zehner) % 10) + 48;
		zehner *= 10;
	} while(i--);

	//Vor-Kommastellen kopieren
	pos = 0;
	pos_t = 12-komma-s_vk;
	//if(s_vk == 0) pos_t
	for (i = 0; i < s_vk; i++)
	{ send_buffer[pos++] = send_buffer[pos_t++];
	}
	if (s_nk > 0)
	{ send_buffer[pos++] = '.';

		//Nach-Kommastellen kopieren
		pos_t = 12-komma;
		for (i = 0; i < s_nk; i++) send_buffer[pos++] = send_buffer[pos_t++];
	}
	send_buffer[pos] = 0;    //Endmarke des Strings setzen

	if(nullen_loeschen)
	{ //Vorangehende Nullen löschen		(xyz)
	  i = 0;
	  while ((send_buffer[i] == 48) && (i < s_vk-1)) send_buffer[i++] = 32;
	}
	
	write_text_ram(x_pos, y_pos, send_buffer);
}


/**********************************************************************************\
* lcd_init
*
* Initialisierung des integrierten LCD-Displays
\**********************************************************************************/

void lcd_init(void)
{	u8 i;
	
	DDRC |= 0x01;													// Port PC0 auf Output setzen LCD-Outputs SOD
	DDRC |= 0x02;													// Port PC1 auf Output setzen LCD-Outputs SCLK

	for(i=0; i<50; i++) delay_nop(120);   // 10ms (50x 0.2us) warten, bis LCD gestartet ist (Power-Up)
	
	write_lcd_f('C',0x34);								// set 8-Bit-Interface RE = 1
	write_lcd_f('C',0x34);								// Nochmals, denn einige LCD starten sonst nicht korrekt.
	write_lcd_f('C',0x09);								// 4-Zeilen-Modus, 5-Dot Font-Breite
	write_lcd_f('C',0x30);								// set 8-Bit-Interface RE = 0
	write_lcd_f('C',0x0C);								// Display ON, Cursor OFF
	write_lcd_f('C',0x01);								// Clear Display
	for(i=0; i<8; i++) delay_nop(120);		// 1.6ms (8x 0.2us) warten, bis LCD gelöscht ist 
	
	write_lcd_f('C',0x07);								// Entry Mode
	init_Timer0_1ms_int(); //Timer0 initialisierung 1ms interrupt
}


/**********************************************************************************\
* clear_lcd
*
* Löscht die Anzeige auf dem LCD-Display
\**********************************************************************************/

void clear_lcd(void)
{ u8 i;
	write_lcd_f('C',0x01);      //Clear Display
	for(i=0; i<8; i++) delay_nop(120);		// 1.6ms (8x 0.2us) warten, bis LCD gelöscht ist 
}

#pragma GCC optimize 0								// Optimierung ausschalten, damit das Debugging möglich ist

