/*************************************************************************** 
    MSW-Winterthur Berufsfachschule, Zeughausstrasse 56, 8400 Winterthur     
    ---------------------------------------------------------------------    
	Bluetooth CAR V21
    Datum       : 7.7.2015
    Version		: 2.1
    Beschreibung: Demo Projekt für das Board BT_CAR_V21, Microprozessor TMEL644PA
    Autor  		: Peter Trüb, LCD Treiber Christian Riedel

****************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "BT_CAR_V2_0.h"
#include <avr/pgmspace.h>							// Ermöglicht die Platzierung von "static const" im Code-Segment, statt im RAM.
													// Definition mit "PROGMEM", Lesen mit "pgm_read_byte, pgm_read_ptr"
																			
#pragma GCC optimize 0								// Optimierung ausschalten, damit das Debugging möglich ist

#define JOYSTICK_X 0
#define JOYSTICK_Y 1
#define LICHTSENSOR 2
#define DIP_SWITCH 8

u8 DIP_Switch, LCD_Taster,Receive_count=0,i=0;
u8 Data_Array[4];

void init_ADC(void)
{
	// Initialisierung des ADC's
	ADCSRA |=  (1 << ADEN);						// A/D-Wandler Enable
	ADMUX  |= (1 << REFS0);						// Vcc als Referenz
	ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);		// Clock = CPU / 128 ==> ADC_Clock = 125kHz ==> Conversen-Time = 104us
}

u8 get_DIP_Switch(void)
{
	u8 DIP_Result=0;
	int j;
	
	PORTA |= 0x20;	//PA5=High Load
	PORTA &= 0xDF;	//PA5=Low Load
	PORTA |= 0x20;	//PA5=High --> Load an SRG
	
	for (j=7;j>=0;j--)
	{
		DIP_Result = DIP_Result | ((PINA & 0x80) >> (7-j)); //Bit an PA7 einlesen, MSB first
		PORTA &= 0xBF;	//PA6=Low Clk
		PORTA |= 0x40;	//PA6=High --> steigende Flanke an Clk von SRG
	}
	return(DIP_Result);
}

u8 get_LCD_Taster(void)
{
	u8 LCD_Taster;
	
	//Initialisierung Port B
	PORTB &= 0xF0;			//4xNull ausgeben um Eingangskapazität zu "leeren", diese Pins werden im nächsten Befehl zu Eingängen umgeschaltet

	DDRB  =  0xF0;			// PB0...PB3 neu als Inputs (4 Taster)
	asm  ("nop");			//Zeit für Synchronisation
	asm  ("nop");			//Zeit für Synchronisation
	asm  ("nop");			//Zeit für Synchronisation
	asm  ("nop");			//Zeit für Synchronisation
	asm  ("nop");			//Zeit für Synchronisation
	asm  ("nop");			//Zeit für Synchronisation
	asm  ("nop");			//Zeit für Synchronisation
	asm  ("nop");			//Zeit für Synchronisation
	asm  ("nop");			//Zeit für Synchronisation
	asm  ("nop");			//Zeit für Synchronisation
	LCD_Taster = PINB & 0x0F;
	//Initialisierung Port B
	DDRB  =  0xFF;			// jetzt wieder LED Port, alles Outputs
	return(LCD_Taster);
}

u16 get_ADC_Channel(u8 Kanal)
{	u16 AD_Result;

	ADMUX = (ADMUX & 0xF8) | Kanal;	// Die drei Mux Bits löschen und neuen Kanal setzen
	ADCSRA |=  (1 << ADSC);			// Start, Single Mode
	while(ADCSRA & (1 << ADSC))		// Warten, bis Messung fertig ist.
	;
	AD_Result = ADCL + (ADCH << 8);	//	ADCL zuerst lesen und dann erst ADCH !!!
	return(AD_Result);
}

void init_RGB(void)
{
	TCCR1A = 0x00;
	TCCR1B = 0x00;
	TCCR2A = 0x00;
	TCCR2B = 0x00;
	
	//Init TIMER1 16 Bit
	TCCR1A |= (1 << WGM10);
	TCCR1B |= (1 << WGM12); // Fast PWM 8Bit, Top=0xFF
	TCCR1A |= (1 << COM1A1) | (1 << COM1B1);	//Clear OCnA/OCnB Pin when Compare Match
	TCCR1B |= (1 << CS11);	//16MHz/8	--> 1/2MHzx255 --> 7.843kHz PWM Frequenz
	OCR1A = 0;	//Comparewert 1A
	OCR1B = 0;	//Comparewert 1B

	//Init TIMER2 8 Bit
	TCCR2A |= (1 << WGM21) | (1 << WGM20); // Fast PWM 8Bit, Top=0xFF
	TCCR2A |= (1 << COM2B1);	//Clear OCnB Pin when Compare Match
	TCCR2B |= (1 << CS21);	//16MHz/8	--> 1/2MHzx255 --> 7.843kHz PWM Frequenz
	OCR2B = 0;	//Comparewert 2B
}

void init_UART0(void)
{
	// Initialisierung UART0 (RxD0, TxD0)
	UBRR0 = 16;	//Bei 16MHZ und U2X0=1 --> 115k2 Baudrate
	UCSR0A |= (1 << U2X0);
	//UCSR0C muss nicht initialisiert werden, Standardwerte: Asynchron, 8Databits,Noneparity, ein Stopbit
	UCSR0B |= (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0); //TxD, RxD Enable und RxD Interrupt Enable
}

void send_Byte(u8 val)
{
	//Wait for empty Buffer
	while( !(UCSR0A & (1<<UDRE0)) )
	;
	
	UDR0=val;	//Transmit starts
}

void Send_USART(u8 Code, u8 value)
{
	u8 Hunderter,Zehner,Einer;
	
	Hunderter=value/100;
	Zehner=(value-Hunderter*100)/10;
	Einer=value-Hunderter*100-Zehner*10;
	
	switch (Code)
	{
		case DIP_SWITCH:	send_Byte('D');
							break;
		case JOYSTICK_X:	send_Byte('X');
							break;						
		case JOYSTICK_Y:	send_Byte('Y');
							break;						
		case LICHTSENSOR:	send_Byte('S');
							break;							
	}
	send_Byte(Hunderter+0x30);
	send_Byte(Zehner+0x30);
	send_Byte(Einer+0x30);
}

ISR (USART0_RX_vect) // UART0 Empfangsinterrupt
{

	Receive_count++;
	Data_Array[i] = UDR0;	//Empfangspuffer lesen
	i++;

	if (i==sizeof(Data_Array))
	{
		i=0;
		switch(Data_Array[0])
		{
			case 'R': OCR1B=((Data_Array[1]-0x30)*100+(Data_Array[2]-0x30)*10+(Data_Array[3]-0x30)); //Rot
					break;
			case 'G': OCR1A=((Data_Array[1]-0x30)*100+(Data_Array[2]-0x30)*10+(Data_Array[3]-0x30)); //Green
					break;
			case 'B': OCR2B=((Data_Array[1]-0x30)*100+(Data_Array[2]-0x30)*10+(Data_Array[3]-0x30)); //Blau
					break;
			case 'L': PORTB=((Data_Array[1]-0x30)*100+(Data_Array[2]-0x30)*10+(Data_Array[3]-0x30)); //LED Port
					break;
			case 'D': Send_USART(DIP_SWITCH,get_DIP_Switch()); //DIP_Switch
					break;
			case 'X': Send_USART(JOYSTICK_X,get_ADC_Channel(JOYSTICK_X)>>2); //Joystick_X 8-Bit
					break;	
			case 'Y': Send_USART(JOYSTICK_Y,get_ADC_Channel(JOYSTICK_Y)>>2); //Joystick_Y 8-Bit
					break;	
			case 'S': Send_USART(LICHTSENSOR,get_ADC_Channel(LICHTSENSOR)>>2); //Lichtsensor 8-Bit
					break;																								
		}
	}
}

// ==============================================================================================================
// Hier beginnt das Hauptprogramm "main"
// --------------------------------------------------------------------------------------------------------------

int main (void)
{
	init_BT_CAR_V2_0();			// Das Board wird hier initialisiert
	write_text(0,4, PSTR("READY TO GO!"));	
	wait_1ms(1000);	
	clear_lcd();				// LCD clear
	init_ADC();
	init_RGB();
	init_UART0();
	
	write_text(0, 0 ,PSTR("JoyX: "));
	write_text(1, 0 ,PSTR("JoyY: "));
	write_text(2, 0 ,PSTR("Lich: "));
	write_text(3, 0, PSTR("DipS: "));
	write_text(0, 11,PSTR("RxD : "));	
	write_text(1, 11,PSTR("LED : "));
	write_text(3, 11,PSTR("time: "));
	while(1)
	{
		wait_1ms(100);
		write_zahl(0, 5 ,get_ADC_Channel(JOYSTICK_X)>>2,4,0,0);
		write_zahl(1, 5 ,get_ADC_Channel(JOYSTICK_Y)>>2,4,0,0);
		write_zahl(2, 5 ,get_ADC_Channel(LICHTSENSOR)>>2,4,0,0);
		write_zahl(3, 5 ,get_DIP_Switch(),4,0,0);
		write_zahl(0, 16,Receive_count,4,0,0);
		write_zahl(1, 16,PINB,4,0,0);
		write_zahl(3, 16,tick_1s,4,0,0);		
	}
}