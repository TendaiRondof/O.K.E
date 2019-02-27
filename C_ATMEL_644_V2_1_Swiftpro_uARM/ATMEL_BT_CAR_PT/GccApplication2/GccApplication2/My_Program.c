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
{	u16 AD_Result;

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
    {   /* so lange *_str != '\0' also ungleich dem "String-Endezeichen(Terminator)" */
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
	write_text(0,4, PSTR("READY TO GO!"));	
	wait_1ms(1000);	
	clear_lcd();				// LCD clear
	init_ADC();
	init_UART0();

	while(1)
	{
		DIP_Switch=get_DIP_Switch();
		switch(DIP_Switch)
		{
			case 0x01:	//Sauger montieren: Einfaches Pick and Place mit Sauger
						send_to_uArm("G0 X250 Y0 Z100 F20000\n");
						send_to_uArm("G0 X250 Y0 Z35 F20000\n");
						send_to_uArm("G0 X250 Y0 Z27 F1000\n");
						send_to_uArm("M2231 V0\n");	//Pumpe ausschalten
						wait_1ms(1500);						
						send_to_uArm("M2231 V1\n");	//ansaugen, Pumpe einschalten
						wait_1ms(200);
						send_to_uArm("G0 X250 Y0 Z100 F20000\n");
						send_to_uArm("G0 X250 Y-150 Z100 F20000\n");						
						send_to_uArm("G0 X250 Y-150 Z35 F20000\n");
						send_to_uArm("G0 X250 Y-150 Z27 F1000\n");						
						send_to_uArm("M2231 V0\n");	//Pumpe ausschalten
						wait_1ms(1500);
						send_to_uArm("M2231 V1\n");	//ansaugen, Pumpe einschalten
						wait_1ms(200);													
						send_to_uArm("G0 X250 Y-150 Z100 F20000\n");						
						break;

			case 0x02: 	//Fahren bis X Koordinate  > 200mm, Relativkoordinaten verwenden.
						send_to_uArm("G0 X150 Y0 Z100 F10000\n"); //Max. Speed 30000
						x_Koordinate=Get_uArm_Koordinate('X'); //Abholen von X Koordinate
						y_Koordinate=Get_uArm_Koordinate('Y'); //Abholen von Y Koordinate
						z_Koordinate=Get_uArm_Koordinate('Z'); //Abholen von Z Koordinate
						while (Get_uArm_Koordinate('X') < 200) //Warten bis X >= 200
							send_to_uArm("G2204 X1 Y0 Z0 F1000\n"); //Relative Bewegung	
						send_to_uArm("G2203\n"); //Stop Befehl
						//wait_1ms(100);							
						break;
			
			case 0x04:	//Schwenkbewegungen, ACHTUNG schaltet Pumpe automatisch aus!!!
						//send_to_uArm("G0 X250 Y0 Z100 F40000\n"); setzt Winkelauf Null			
						send_to_uArm("G2202 N0 V30\n");// Basemotor schwenkt Winkel 30° Basemotor Range 0..180°
						send_to_uArm("G2202 N0 V70\n");// Basemotor schwenkt Winkel 70° Basemotor Range 0..180°						
						send_to_uArm("G2202 N1 V60\n");// Motor left schwenkt Winkel 60° Motor left Range 0..130°
						send_to_uArm("G2202 N1 V95\n");// Motor left schwenkt Winkel 95° Motor left Range 0..130°	
						send_to_uArm("G2202 N2 V22\n");// Motor right schwenkt Winkel 22° Motor right Range 0..106°
						send_to_uArm("G2202 N2 V53\n");// Motor right schwenkt Winkel 53° Motor right Range 0..106°
						send_to_uArm("G2202 N3 V60\n");// End-effektor-motor schwenkt Winkel 60° Motor right Range 0..180°	
						wait_1ms(500); //End-effektor-motor hat keine Rückmeldung mit send_to_uARM("M2200\n") uARM is moving? 1 Yes / 0 N0
									   //Benötigt deshalb wait_1ms()
						send_to_uArm("G2202 N3 V150\n");// End-effektor-motor schwenkt Winkel 150° Motor right Range 0..180°
						break;
						
			
			case 0x08:	//Relativ Bewegungen
						send_to_uArm("G0 X150 Y100 Z100 F20000\n");
						for(i=0;i<5;i++)
						{
							send_to_uArm("G2204 X0 Y-50 Z0 F5000\n"); //Relative Bewegung Y-50
							wait_1ms(500); //zur besseren Veranschaulichung
						}
						break;
								
			case 0x10:	//Joysticksteuerung mit Relativkoordinaten mehrere Geschwindigkeitsstufen
						//Grosser Ausschlag mit Joystick --> Höhere Geschwindigkeit
						//zittert leicht ADC Wert müsste gefiltert weden
						send_to_uArm("G0 X150 Y0 Z100 F20000\n");
						DIP_Switch=get_DIP_Switch();
						while(DIP_Switch==0x10)
						{
							DIP_Switch=get_DIP_Switch();							
							x_Wert_ADC=get_ADC_Channel(0); //Joystick X
							y_Wert_ADC=get_ADC_Channel(1); //Joystick Y
							//X-Achse
							if (x_Wert_ADC <400)
							{
								if (x_Wert_ADC <100)
									send_to_uArm("G2204 X-3 Y0 Z0 F10000\n"); //Relative Bewegung
								else if (x_Wert_ADC <250)
										send_to_uArm("G2204 X-2 Y0 Z0 F1000\n"); //Relative Bewegung
									 else	
										send_to_uArm("G2204 X-1 Y0 Z0 F50\n"); //Relative Bewegung									
							}
							if (x_Wert_ADC > 600)
							{
								if (x_Wert_ADC >900)
									send_to_uArm("G2204 X3 Y0 Z0 F10000\n"); //Relative Bewegung
								else if (x_Wert_ADC >750)
										send_to_uArm("G2204 X2 Y0 Z0 F1000\n"); //Relative Bewegung
									 else	
										send_to_uArm("G2204 X1 Y0 Z0 F50\n"); //Relative Bewegung										
							}							
							
							//Y-Achse
							if (y_Wert_ADC <400)
							{
								if (y_Wert_ADC <100)
									send_to_uArm("G2204 X0 Y3 Z0 F10000\n"); //Relative Bewegung
								else if (y_Wert_ADC <250)
										send_to_uArm("G2204 X0 Y2 Z0 F1000\n"); //Relative Bewegung
									 else	
										send_to_uArm("G2204 X0 Y1 Z0 F50\n"); //Relative Bewegung									
							}
							if (y_Wert_ADC > 600)
							{
								if (y_Wert_ADC >900)
									send_to_uArm("G2204 X0 Y-3 Z0 F10000\n"); //Relative Bewegung
								else if (y_Wert_ADC >750)
										send_to_uArm("G2204 X0 Y-2 Z0 F1000\n"); //Relative Bewegung
									 else	
										send_to_uArm("G2204 X0 Y-1 Z0 F50\n"); //Relative Bewegung										
							}	
						LCD_Taster=get_LCD_Taster();
						if(LCD_Taster==0x01)
							send_to_uArm("G2204 X0 Y0 Z1 F500\n"); //Relative Bewegung
						if(LCD_Taster==0x02)
							send_to_uArm("G2204 X0 Y0 Z-1 F500\n"); //Relative Bewegung							
						}
						break;
						
			case 0x20:	//Gripper montieren! Gripper Pick and Place
						send_to_uArm("G0 X150 Y100 Z100 F10000\n");
						send_to_uArm("G0 X150 Y100 Z20 F10000\n");						
						send_to_uArm("G0 X150 Y100 Z-15 F2000\n");
						
						send_to_uArm("M2232 V0\n");	//Gripper open: 1 close / 0 open
						wait_1ms(2000); //Gripper open: Benötigt wait_1ms(2000) oder Abfrage über P2232
						send_to_uArm("P2232\n"); // get status of Gripper 0 stop, 1 working, 2 grabbing things												
						send_to_uArm("M2232 V1\n");	//Gripper close: 1 close / 0 open
						wait_1ms(2000); //Gripper open: Benötigt wait_1ms(2000)  oder Abfrage über P2232						
						send_to_uArm("P2232\n"); // get status of Gripper 0 stop, 1 working, 2 grabbing things	
						send_to_uArm("G0 X150 Y100 Z100 F10000\n");
						
						//End effektor motor Start
						send_to_uArm("G2202 N3 V90\n");// End-effektor-motor schwenkt Winkel 60° Motor right Range 0..180°	
						wait_1ms(2000); //End-effektor-motor hat keine Rückmeldung mit send_to_uARM("M2200\n") uARM is moving? 1 Yes / 0 N0
									   //Benötigt deshalb wait_1ms()
						send_to_uArm("G2202 N3 V180\n");// End-effektor-motor schwenkt Winkel 150° Motor right Range 0..180°
						wait_1ms(1000); //End-effektor-motor hat keine Rückmeldung mit send_to_uARM("M2200\n") uARM is moving? 1 Yes / 0 N0
									   //Benötigt deshalb wait_1ms()						
						send_to_uArm("G2202 N3 V90\n");// End-effektor-motor schwenkt Winkel 60° Motor right Range 0..180°							
						//End effektor motor End
												
						send_to_uArm("G0 X150 Y-100 Z100 F10000\n");												
						send_to_uArm("G0 X150 Y-100 Z20 F10000\n");						
						send_to_uArm("G0 X150 Y-100 Z-15 F2000\n");
						send_to_uArm("M2232 V0\n");	//Gripper open: 1 close / 0 open
						wait_1ms(2000); //Gripper open: Benötigt wait_1ms(2000)  oder Abfrage über P2232						
						send_to_uArm("P2232\n"); // get status of Gripper 0 stop, 1 working, 2 grabbing things	
						send_to_uArm("M2232 V1\n");	//Gripper close: 1 close / 0 open
						wait_1ms(2000); //Gripper open: Benötigt wait_1ms(2000)  oder Abfrage über P2232						
						send_to_uArm("P2232\n"); // get status of Gripper 0 stop, 1 working, 2 grabbing things						
						send_to_uArm("G0 X150 Y-100 Z100 F10000\n");						
						break;
													
			case 0x40:	//Joysticksteuerung mit Variablen und absolut Koordinaten
						send_to_uArm("G0 X200 Y0 Z100 F20000\n");
						DIP_Switch=get_DIP_Switch();
						while(DIP_Switch==0x40)
						{						
							DIP_Switch=get_DIP_Switch();
							alt_x_tmp=x_tmp; //Neu = alt
							alt_y_tmp=y_tmp;
							x_tmp=150+get_ADC_Channel(0)/8; //Joystick X ca. 150...270
							y_tmp=170-get_ADC_Channel(1)/3; //Joystick Y ca. -170...170
							
							//kleiner Filter Mindeständerung 2 ADC Einheiten, sonst "zittert" der Robi
							if ( abs(x_tmp-alt_x_tmp)>2) //abs --> Betrag
								x_Koordinate=x_tmp;
							if ( abs(y_tmp-alt_y_tmp)>2)
								y_Koordinate=y_tmp;								

							z_Koordinate=100;
							speed=10000;								
							uArm_goto_xyz(x_Koordinate,y_Koordinate,z_Koordinate,speed); //G0 Weg-Befehl, es können direkt Variablen übergeben werden
						}
						break;
						
			case 0x80:	//Verschiedene Beschleunigungen einstellen
						send_to_uArm("M204 P50 T50 R50\n"); //Beschleunigung setzen P,T,R Werte 0..300 (Standardwerte P,T,R=200 )
														    //P = Printing moves
															//R = Retract only (no X, Y, Z) moves
															//T = Travel (non printing) moves

						send_to_uArm("G0 X200 Y100 Z100 F20000\n");
						wait_1ms(2000); //Für LCD Anzeige
						send_to_uArm("M204 P300 T300 R300\n");						
						send_to_uArm("G0 X200 Y-100 Z100 F20000\n");
						wait_1ms(2000); //Für LCD Anzeige						
						break;	
											
			case 0x81:	//Einfaches Pick and Place mit Gripper, für schnelle Bewegungen werden Schwenkbewegungen mit den Motoren ausgeführt
						//Die Beschleunigung wird erhöht auf T=300
						send_to_uArm("M204 P300 T300 R300\n");	//Beschleunigung setzen P,T,R Werte 0..300 (Standardwerte P,T,R=200 )
																//P = Printing moves
																//R = Retract only (no X, Y, Z) moves
																//T = Travel (non printing) moves
						send_to_uArm("M2232 V0\n");	//Gripper open: 1 close / 0 open
						send_to_uArm("G0 X200 Y0 Z100 F10000\n");
						send_to_uArm("G2202 N0 V90\n");//ist Mitte Basemotor schwenkt							
						DIP_Switch=get_DIP_Switch();
						while(DIP_Switch==0x81)
						{
							DIP_Switch=get_DIP_Switch();
							send_to_uArm("G0 X200 Y0 Z0 F10000\n"); //90° ist Mitte, also jetzt 60° nach links von hinten gesehen						
							send_to_uArm("G0 X200 Y0 Z-15 F2000\n");
							send_to_uArm("M2232 V1\n");	//Gripper close: 1 close / 0 open
							wait_1ms(2000);						
							send_to_uArm("G0 X200 Y0 Z100 F10000\n");
							send_to_uArm("G2202 N0 V30\n");// Basemotor schwenkt Winkel 30° Basemotor Range 0..180°																				
							x_Koordinate=Get_uArm_Koordinate('X'); //Abholen von X Koordinate
							y_Koordinate=Get_uArm_Koordinate('Y'); //Abholen von Y Koordinate
							z_Koordinate=Get_uArm_Koordinate('Z'); //Abholen von Z Koordinate
							speed=10000;
							uArm_goto_xyz(x_Koordinate,y_Koordinate,(z_Koordinate-115),speed);
							send_to_uArm("M2232 V0\n");	//Gripper open: 1 close / 0 open							
							wait_1ms(2000);
							send_to_uArm("M2232 V1\n");	//Gripper close: 1 close / 0 open
							wait_1ms(2000);														
							uArm_goto_xyz(x_Koordinate,y_Koordinate,z_Koordinate,speed);	
							send_to_uArm("G2202 N0 V90\n");//ist Mitte Basemotor schwenkt													
						}
						break;	
						
			case 0x82:	//Einfaches Pick and Place mit Gripper, für schnelle Bewegungen nur Schwenkbewegungen
						//Die Beschleunigung wird erhöht auf T=300
						send_to_uArm("M204 P300 T300 R300\n");	//Beschleunigung setzen P,T,R Werte 0..300 (Standardwerte P,T,R=200 )
																//P = Printing moves
																//R = Retract only (no X, Y, Z) moves
																//T = Travel (non printing) moves
						send_to_uArm("G0 X200 Y0 Z20 F10000\n");						

						DIP_Switch=get_DIP_Switch();
						while(DIP_Switch==0x82)
						{
							DIP_Switch=get_DIP_Switch();
							send_to_uArm("G2202 N1 V40\n");//Motor2, gripper nahe am Boden
							wait_1ms(1000);
							send_to_uArm("M2232 V0\n");	//Gripper open: 1 close / 0 open
							wait_1ms(1000);															
							send_to_uArm("M2232 V1\n");	//Gripper close: 1 close / 0 open
							wait_1ms(1000);									
							send_to_uArm("G2202 N1 V70\n");//Motor3 40° Gripper abheben
							send_to_uArm("G2202 N0 V60\n");//ist Mitte Basemotor schwenkt
							send_to_uArm("G2202 N1 V40\n");//Motor2, gripper nahe am Boden
							wait_1ms(1000);															
							send_to_uArm("M2232 V0\n");	//Gripper open: 1 close / 0 open							
							wait_1ms(1000);								
							send_to_uArm("M2232 V1\n");	//Gripper close: 1 close / 0 open
							wait_1ms(1000);
							send_to_uArm("G2202 N1 V70\n");//Motor2 40° Gripper abheben
							send_to_uArm("G2202 N0 V90\n");//ist Mitte Basemotor schwenkt
							send_to_uArm("G2202 N1 V40\n");//Motor2, gripper nahe am Boden							
						}
						break;
			default:	send_to_uArm("G0 X200 Y0 Z100 F10000\n");	//Wichtig als Startposition!
						send_to_uArm("M2231 V0\n");	//Pumpe ausschalten																									
						break;
		} //end Switch
	} //end while(1)
} // end main