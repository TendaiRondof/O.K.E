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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include "BT_CAR_V2_0.h"
#include <avr/pgmspace.h>							// Ermöglicht die Platzierung von "static const" im Code-Segment, statt im RAM.
													// Definition mit "PROGMEM", Lesen mit "pgm_read_byte, pgm_read_ptr"
																			
#pragma GCC optimize 0								// Optimierung ausschalten, damit das Debugging möglich ist
u8 Receive_count=0,i=0;

char trennzeichen[] = " ";
char *ptr_Abschnitt;
int x_Koordinate,y_Koordinate,x_tmp,y_tmp,alt_x_tmp,alt_y_tmp,z_Koordinate,speed=10000;
u8 DIP_Switch=0, LCD_Taster=0, k=0,m=0;
u16 x_Wert_ADC,y_Wert_ADC;

#define UART_MAXSTRLEN 50
u8 uart_str_complete = 0;     // 1 --> String komplett empfangen
u8 uart_str_count = 0;
char uart_string[UART_MAXSTRLEN + 1] = "";
char uart_string_send[UART_MAXSTRLEN + 1] = "";

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

void init_ADC(void)
{
	// Initialisierung des ADC's
	ADCSRA |=  (1 << ADEN);									//A/D-Wandler Enable
	ADMUX  |= (1 << REFS0);									//Vcc als Referenz
	ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);	//Clock=CPU/128
															//ADC_Clk=125kHz(8us)
															//13 Clks->13x8=104us
}

u16 get_ADC_Channel(u8 Kanal)
{	
	u16 AD_Result;

	ADMUX = (ADMUX & 0xF8) | Kanal;	//Mux Bits löschen, neuen Kanal setzen
	ADCSRA |=  (1 << ADSC);			//Start, Single Mode
	while(ADCSRA & (1 << ADSC))		//Warten, bis Messung fertig ist.
	;
	AD_Result = ADCL + (ADCH << 8);	//ADCL zuerst lesen, dann erst ADCH!
	return(AD_Result);
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

void XYZ_to_Display(u16 Delay_LCD)
{
	uart_str_complete = 1;	//String für Änderungen blockieren!
	clear_lcd();
	ptr_Abschnitt = strtok(uart_string, trennzeichen);
	while(ptr_Abschnitt != NULL)
	{
		write_text_ram(k, 0, ptr_Abschnitt); //Ausgabe Array
		k++;
		ptr_Abschnitt = strtok(NULL, trennzeichen);
		//Jeder Aufruf gibt das Token zurück. Das Trennzeichen wird mit '\0' überschrieben.
		//Die Schleife läuft durch bis strtok() den NULL-Zeiger zurückliefert.
	}
	k=0;
	uart_str_complete = 0;
	wait_1ms(Delay_LCD);
}

void to_uARM(char *_str)
{
	//for(m=0;m<UART_MAXSTRLEN+1;m++) //Array löschen nicht nötig!
	//uart_string[m]='\0';
	while (*_str)
	{   
		send_Byte(*_str);
		_str++;
	}
	uart_str_complete = 0; //String freigeben um Antwort zu empfangen
	while (uart_str_complete == 0); //Warten bis Antwort erfolgt ist
	//XYZ_to_Display(2000); //Kann eingesetzt werden um die Kommunikation auf dem LCD anzuzeigen
	//setzt dann uart_str_complete = 0

}

void send_to_uArm(char *str)
{
	to_uARM(str);
	to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
	while(uart_string[4] == 0x31) //ASCII '1' --> moving
	{
		to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
	}
	to_uARM("P2220\n"); //Koordinaten abfragen
	XYZ_to_Display(0);	//Kann eingesetzt werden um die Kommunikation auf dem LCD anzuzeigen
						//setzt dann uart_str_complete = 0
}

void uArm_goto_xyz(int x, int y, int z, unsigned int speed)
{
	char x_string[6];
	char y_string[6];
	char z_string[6];
	char speed_string[6];
	
	uart_string_send[0] = '\0';//WICHTIG!!!!!
	itoa(x,x_string,10); //integer in String wandeln 10er System
	itoa(y,y_string,10);
	itoa(z,z_string,10);
	itoa(speed,speed_string,10);	
	strcat(uart_string_send,"G0 X"); //String verketten
	strcat(uart_string_send,x_string);
	strcat(uart_string_send," Y");
	strcat(uart_string_send,y_string);
	strcat(uart_string_send," Z");
	strcat(uart_string_send,z_string);
	strcat(uart_string_send," F");		
	strcat(uart_string_send,speed_string);
	strcat(uart_string_send,"\n");		
	wait_1ms(100); //optional für ruhigere Bewegungen und LCD Display
	send_to_uArm(uart_string_send);	
}

int Get_uArm_Koordinate(char Achse)
{
	double Wert_tmp;
	int Wert;
	
	to_uARM("P2220\n"); //Koordinaten abfragen	
	uart_str_complete = 1;	//String für Änderungen blockieren!
	//clear_lcd();
	ptr_Abschnitt = strtok(uart_string, trennzeichen);
	k=0;
	while(ptr_Abschnitt != NULL)
	{
		//write_text_ram(k, 0, ptr_Abschnitt); //Ausgabe Array
		switch(Achse)
		{
			case 'X':	if (k==1) //Abschnitt mit X
						{
							Wert_tmp=atof(++ptr_Abschnitt);//;atof(ptr_Abschnitt);
							Wert=(int)(Wert_tmp);
							return(Wert);
						}
						break;
			case 'Y':	if (k==2) //Abschnitt mit Y
						{
							Wert_tmp=atof(++ptr_Abschnitt);//;atof(ptr_Abschnitt);
							Wert=(int)(Wert_tmp);
							return(Wert);
						}
						break;	
			case 'Z':	if (k==3) //Abschnitt mit Z
						{
							Wert_tmp=atof(++ptr_Abschnitt);//;atof(ptr_Abschnitt);
							Wert=(int)(Wert_tmp);
							return(Wert);
						}
						break;
			default:	break;																	
		}
		k++;
		ptr_Abschnitt = strtok(NULL, trennzeichen);
		//Jeder Aufruf gibt das Token zurück. Das Trennzeichen wird mit '\0' überschrieben.
		//Die Schleife läuft durch bis strtok() den NULL-Zeiger zurückliefert.
	}
	k=0;
	uart_str_complete = 0;
	//wait_1ms(Delay_LCD);	
}

ISR (USART0_RX_vect) // UART0 Empfangsinterrupt
{
  unsigned char nextChar;

  // Daten aus dem Puffer lesen
  nextChar = UDR0;
  if( uart_str_complete == 0 )
  {	// Daten werden erst in uart_string geschrieben, wenn nicht String-Ende/max Zeichenlänge erreicht ist/string gerade verarbeitet wird
    if( nextChar != '\n' && nextChar != '\r' && uart_str_count < UART_MAXSTRLEN )
	{
      uart_string[uart_str_count] = nextChar;
      uart_str_count++;
    }
    else 
	{
      uart_string[uart_str_count] = '\0';
      uart_str_count = 0;
      uart_str_complete = 1;
    }
  }
}

// ==============================================================================================================
// Hier beginnt das Hauptprogramm "main"
// --------------------------------------------------------------------------------------------------------------

int main (void)
{
	init_BT_CAR_V2_0();			// Das Board wird hier initialisiert
	//write_text(0,4, PSTR("READY TO GO!"));	
	wait_1ms(1000);	
	clear_lcd();				// LCD clear
	init_ADC();
	init_UART0();
	send_to_uArm("G0 M202 N0\n");
	unsigned char version;
unsigned char taster;

clear_lcd();
send_to_uArm("P2201\n");
send_to_uArm("G2202 N0 V45\n");
send_to_uArm("G2202 N1 V45\n");
write_zahl(0,7,version,2,2,2);
	while(1)
	{
		taster = get_LCD_Taster();
		DIP_Switch=get_DIP_Switch();
		
		send_to_uArm("G2202 N0 V90\n");7
		
		if (DIP_Switch>0)
		{
		}
		
		
	} //end while(1)
} // end main