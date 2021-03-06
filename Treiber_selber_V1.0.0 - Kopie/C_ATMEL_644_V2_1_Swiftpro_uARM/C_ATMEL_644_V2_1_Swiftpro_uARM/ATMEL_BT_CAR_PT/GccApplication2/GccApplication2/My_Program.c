/*
O.K.E
by Tendai und Jan
*/

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include "BT_CAR_V2_0.h"
#include <avr/pgmspace.h>							// Erm�glicht die Platzierung von "static const" im Code-Segment, statt im RAM.
													// Definition mit "PROGMEM", Lesen mit "pgm_read_byte, pgm_read_ptr"	
#define F_CPU 16000000UL
#include <util/delay.h>	
#include "light_ws2812.h"										
																			
#pragma GCC optimize 0								// Optimierung ausschalten, damit das Debugging m�glich ist

#define UART_MAXSTELLEN 50
#define LED_AMOUNT		10
#define LEFT			1
#define RIGHT			2
#define UP				3
#define DOWN			4
#define BACKWARD		5
#define FORWARD			6

u8 Receive_count=0,i=0;
char trennzeichen[] = " ";
char *ptr_Abschnitt;
int x_Koordinate,y_Koordinate,x_tmp,y_tmp,alt_x_tmp,alt_y_tmp,z_Koordinate,speed=10000;
u8 DIP_Switch=0, LCD_Taster=0, k=0,m=0;
u16 x_Wert_ADC,y_Wert_ADC;
u8 uart_str_complete = 0;     // 1 --> String komplett empfangen
u8 uart_str_complete1 = 0;     // 1 --> String komplett empfangen
unsigned char data_bytes_recieved=0;
u8 uart_str_count = 0;
u8 uart_str_count1 = 0;
char uart_string[UART_MAXSTELLEN + 1] = "";
char uart_string_send[UART_MAXSTELLEN + 1] = "";
char uart_string1[UART_MAXSTELLEN + 1] = "";
char uart_string_send1[UART_MAXSTELLEN + 1] = "";
unsigned char data [12];
unsigned char final_data[12];

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
	PORTB &= 0xF0;			//4xNull ausgeben um Eingangskapazit�t zu "leeren", diese Pins werden im n�chsten Befehl zu Eing�ngen umgeschaltet

	DDRB  =  0xF0;			// PB0...PB3 neu als Inputs (4 Taster)
	asm  ("nop");			//Zeit f�r Synchronisation
	asm  ("nop");			//Zeit f�r Synchronisation
	asm  ("nop");			//Zeit f�r Synchronisation
	asm  ("nop");			//Zeit f�r Synchronisation
	asm  ("nop");			//Zeit f�r Synchronisation
	asm  ("nop");			//Zeit f�r Synchronisation
	asm  ("nop");			//Zeit f�r Synchronisation
	asm  ("nop");			//Zeit f�r Synchronisation
	asm  ("nop");			//Zeit f�r Synchronisation
	asm  ("nop");			//Zeit f�r Synchronisation
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

	ADMUX = (ADMUX & 0xF8) | Kanal;	//Mux Bits l�schen, neuen Kanal setzen
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

void init_UART1(void)
{
	// Initialisierung UART0 (RxD0, TxD0)
	UBRR1 = 16;	//Bei 16MHZ und U2X0=1 --> 115k2 Baudrate
	UCSR1A |= (1 << U2X1);
	//UCSR0C muss nicht initialisiert werden, Standardwerte: Asynchron, 8Databits,Noneparity, ein Stopbit
	UCSR1B |= (1 << RXCIE1) | (1 << RXEN1) | (1 << TXEN1); //TxD, RxD Enable und RxD Interrupt Enable
}

void send_Byte_0(u8 val)
{
	//Wait for empty Buffer
	while( !(UCSR0A & (1<<UDRE0)) );
	UDR0=val;	//Transmit starts
}

void send_Byte_1(u8 val)
{
	//Wait for empty Buffer
	while( !(UCSR1A & (1<<UDRE1)) )
	;
	
	UDR1=val;	//Transmit starts
}

void XYZ_to_Display(u16 Delay_LCD)
{
	uart_str_complete = 1;	//String f�r �nderungen blockieren!
	clear_lcd();
	ptr_Abschnitt = strtok(uart_string, trennzeichen);
	while(ptr_Abschnitt != NULL)
	{
		write_text_ram(k, 0, ptr_Abschnitt); //Ausgabe Array
		k++;
		ptr_Abschnitt = strtok(NULL, trennzeichen);
		//Jeder Aufruf gibt das Token zur�ck. Das Trennzeichen wird mit '\0' �berschrieben.
		//Die Schleife l�uft durch bis strtok() den NULL-Zeiger zur�ckliefert.
	}
	k=0;
	uart_str_complete = 0;
	wait_1ms(Delay_LCD);
}

void to_pc (char *_str)
{
	//for(m=0;m<UART_MAXSTRLEN+1;m++) //Array l�schen nicht n�tig!
	//uart_string[m]='\0';
	while (*_str)
	{   
		send_Byte_0(*_str);
		_str++;
	}
	uart_str_complete = 0; //String freigeben um Antwort zu empfangen
}

void to_uARM(char *_str)
{
	//for(m=0;m<UART_MAXSTRLEN+1;m++) //Array l�schen nicht n�tig!
	//uart_string[m]='\0';
	while (*_str)
	{   
		send_Byte_1(*_str);
		_str++;
	}
	uart_str_complete1 = 0; //String freigeben um Antwort zu empfangen
	while (uart_str_complete1 == 0); //Warten bis Antwort erfolgt ist
	//XYZ_to_Display(2000); //Kann eingesetzt werden um die Kommunikation auf dem LCD anzuzeigen
	//setzt dann uart_str_complete = 0

}

void send_to_uArm(char *str)
{
	to_uARM(str);
	to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
	while(uart_string1[4] == 0x31) //ASCII '1' --> moving
	{
		to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
	}
	//to_uARM("P2220\n"); //Koordinaten abfragen
	//XYZ_to_Display(0);	//Kann eingesetzt werden um die Kommunikation auf dem LCD anzuzeigen
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
	wait_1ms(100); //optional f�r ruhigere Bewegungen und LCD Display
	send_to_uArm(uart_string_send);	
}

int Get_uArm_Koordinate(char Achse)
{
	double Wert_tmp;
	int Wert;
	
	to_uARM("P2220\n"); //Koordinaten abfragen	
	uart_str_complete = 1;	//String f�r �nderungen blockieren!
	//clear_lcd();
	ptr_Abschnitt = strtok(uart_string, trennzeichen);
	k=0;
	while(ptr_Abschnitt != NULL)
	{
		//write_text_ram(k, 0, ptr_Abschnitt); //Ausgabe Array
		switch(Achse)
		{
			case 1:	if (k==1) //Abschnitt mit X
						{
							Wert_tmp=atof(++ptr_Abschnitt);//;atof(ptr_Abschnitt);
							Wert=(int)(Wert_tmp);
							return(Wert);
						}
						break;
			case 2:	if (k==2) //Abschnitt mit Y
						{
							Wert_tmp=atof(++ptr_Abschnitt);//;atof(ptr_Abschnitt);
							Wert=(int)(Wert_tmp);
							return(Wert);
						}
						break;	
			case 3:	if (k==3) //Abschnitt mit Z
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
		//Jeder Aufruf gibt das Token zur�ck. Das Trennzeichen wird mit '\0' �berschrieben.
		//Die Schleife l�uft durch bis strtok() den NULL-Zeiger zur�ckliefert.
	}
	k=0;
	uart_str_complete = 0;
	//wait_1ms(Delay_LCD);	
}

ISR (USART0_RX_vect) // UART0 Empfangsinterrupt
{
	unsigned char i;
	unsigned char j=0;
  unsigned char nextChar;
  // Daten aus dem Puffer lesen
  nextChar = UDR0;
  
  if( nextChar != '\n' && uart_str_count1 < UART_MAXSTELLEN)
  {
	  data[data_bytes_recieved]=nextChar;
	  data_bytes_recieved++;
		//copy data to different array	88 48 49 55  	
	} 
	else
	{
		for (i=0;i<=data_bytes_recieved;i++)
	{
		final_data[i]=data[i];
	}  
	 uart_str_complete=1;
	}
 }

ISR (USART1_RX_vect)
{
	unsigned char nextChar;

	// Daten aus dem Puffer lesen
	nextChar = UDR1;
	if( uart_str_complete1 == 0 )
	{	// Daten werden erst in uart_string geschrieben, wenn nicht String-Ende/max Zeichenl�nge erreicht ist/string gerade verarbeitet wird
		if( nextChar != '\n' && nextChar != '\r' && uart_str_count1 < UART_MAXSTELLEN)
		{
			uart_string1[uart_str_count1] = nextChar;
			uart_str_count1++;
		}
		else
		{
			uart_string1[uart_str_count1] = '\0';
			uart_str_count1 = 0;
			uart_str_complete1 = 1;
		}
	}
}

void move (unsigned char direction, int amount)
{
	
	
	switch (direction)
	{
		case BACKWARD:
		amount=amount*-1;
		snprintf(data,50,"G2204 X%d Y00 Z00 F1000\n",amount);
		to_uARM(data);
		
		break;
		
		case FORWARD:
		snprintf(data,50,"G2204 X%d Y00 Z00 F1000\n",amount);
		to_uARM(data);
		break;
		
		case UP:
		snprintf(data,50,"G2204 X00 Y00 Z%d F1000\n",amount);
		to_uARM(data);
		break;
		
		case DOWN:
		amount=amount*-1;
		snprintf(data,50,"G2204 X00 Y00 Z%d F1000\n",amount);
		to_uARM(data);
		break;
		
		case RIGHT:
		amount=amount*-1;
		snprintf(data,50,"G2204 X00 Y%d Z00 F1000\n",amount);
		to_uARM(data);
		break;
		
		case LEFT:
		snprintf(data,50,"G2204 X00 Y%d Z00 F1000\n",amount);
		to_uARM(data);
		break;
		
	}
}

unsigned char get_direction()
{
	unsigned int x;
	unsigned int y;
	x=get_ADC_Channel(1);
	y=get_ADC_Channel(0);
	if (x>700)
	{
		return LEFT;
	}
	if (x<300)
	{
		return RIGHT;
	}
	
	if (y>700)
	{
		return BACKWARD;
	}
	if (y<300)
	{
		return FORWARD;
	}
	if ((x>300)&&(x<700)&&(y<700)&&(y>300))
	{
		return 0;
	}
}

void start_up_routine ()
{
	init_BT_CAR_V2_0();			// Das Board wird hier initialisiert
	wait_1ms(1000);
	init_ADC();
	init_UART0();
	init_UART1();
	wait_1ms(1000);
	//while (final_data[0]!='D')
	//{
	//	to_pc('R');
	//}
	
	to_uARM("M2210 F500 T200\n");
	_delay_ms(200);
	to_uARM("M2210 F1000 T500\n");
	_delay_ms(500);
	to_uARM("M2210 F2000 T500\n");
	clear_lcd();
}

int main (void)
{
	start_up_routine();
	unsigned char taster;
	//unsigned char direction;
	int recieved_X; 
	int recieved_Y;
	unsigned char buffer [30];
	unsigned char check=0;
	unsigned char counter=0;
	unsigned char routine_done=0;
	unsigned int sound[14]={262,277,294,311,330,349,370,392,415,440,466,494,523,0};
	unsigned char music[50]={0,0,2,2,0,0,5,5,4,4,4,4,0,0,2,2,0,0,7,7,5,5,5,5,0,0,12,12,9,9,5,5,4,4,2,2,10,10,9,9,5,5,7,7,5,5,5,5,5,5};
	unsigned char sound_buffer[20];
	unsigned int tone;
	struct cRGB array[LED_AMOUNT];
	ws2812_setleds(array,LED_AMOUNT);
	
	while(1)
	{
	//	direction=get_direction();
		taster = get_LCD_Taster();
		DIP_Switch=get_DIP_Switch();
		if ((DIP_Switch&0x01)&&(taster&0x01))
		{
			to_uARM("M2210 F660 T100\n");
			_delay_ms(150);
			to_uARM("M2210 F660 T100\n");
			_delay_ms(300);
			to_uARM("M2210 F660 T100\n");
			_delay_ms(300);
			to_uARM("M2210 F510 T100\n");
			_delay_ms(100);
			to_uARM("M2210 F660 T100\n");
			_delay_ms(300);
			to_uARM("M2210 F770 T100\n");
			_delay_ms(550);
			to_uARM("M2210 F380 T100\n");
			_delay_ms(575);
			
			
			to_uARM("M2210 F510 T100\n");
			_delay_ms(450);
			to_uARM("M2210 F380 T100\n");
			_delay_ms(400);
			to_uARM("M2210 F320 T100\n");
			_delay_ms(500);
			to_uARM("M2210 F440 T100\n");
			_delay_ms(300);
			to_uARM("M2210 F480 T80\n");
			_delay_ms(330);
			to_uARM("M2210 F450 T100\n");
			_delay_ms(150);
			to_uARM("M2210 F430 T100\n");
			_delay_ms(300);
			to_uARM("M2210 F380 T100\n");
			_delay_ms(200);
			to_uARM("M2210 F660 T80\n");
			_delay_ms(200);
			to_uARM("M2210 F760 T50\n");
			_delay_ms(150);
			to_uARM("M2210 F860 T100\n");
			_delay_ms(300);
			to_uARM("M2210 F700 T80\n");
			_delay_ms(150);
			to_uARM("M2210 F760 T50\n");
			_delay_ms(350);
			to_uARM("M2210 F660 T80\n");
			_delay_ms(300);
			to_uARM("M2210 F520 T80\n");
			_delay_ms(150);
			to_uARM("M2210 F580 T80\n");
			_delay_ms(150);
			to_uARM("M2210 F480 T80\n");
			_delay_ms(500);
			
			to_uARM("M2210 F510 T100\n");
			_delay_ms(450);
			to_uARM("M2210 F380 T100\n");
			_delay_ms(400);
			to_uARM("M2210 F320 T100\n");
			_delay_ms(500);
			to_uARM("M2210 F440 T100\n");
			_delay_ms(300);
			to_uARM("M2210 F480 T80\n");
			_delay_ms(330);
			to_uARM("M2210 F450 T100\n");
			_delay_ms(150);
			to_uARM("M2210 F430 T100\n");
			_delay_ms(300);
			to_uARM("M2210 F380 T100\n");
			_delay_ms(200);
			to_uARM("M2210 F660 T80\n");
			_delay_ms(200);
			to_uARM("M2210 F760 T50\n");
			_delay_ms(150);
			to_uARM("M2210 F860 T100\n");
			_delay_ms(300);
			to_uARM("M2210 700 T80\n");
			_delay_ms(150);
			to_uARM("M2210 F760 T50\n");
			_delay_ms(350);
			to_uARM("M2210 F660 T80\n");
			_delay_ms(300);
			to_uARM("M2210 F520 T80\n");
			_delay_ms(150);
			to_uARM("M2210 F580 T80\n");
			_delay_ms(150);
			to_uARM("M2210 F480 T80\n");
			_delay_ms(500);

			to_uARM("M2210 F500 T100\n");
			_delay_ms(300);

			to_uARM("M2210 F760 T100\n");
			_delay_ms(100);
			to_uARM("M2210 F720 T100\n");
			_delay_ms(150);
			to_uARM("M2210 F680 T100\n");
			_delay_ms(150);
			to_uARM("M2210 F620 T150\n");
			_delay_ms(300);

			//to_uARM("M2210 F650 T150\n");
			//_delay_ms(300);
			//to_uARM("M2210 F380 T100\n");
			//_delay_ms(150);
			//to_uARM("M2210 F430 T100\n");
			//_delay_ms(150);
		}
		
		if (taster&0x08)
		{
			send_to_uArm("G0 X200 Y0 Z150 F1000\n");			//ausgansgpkt	(200 0 150)
			while(uart_string1[4] == 0x31) //ASCII '1' --> moving
			{
				to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
			}
			//to_uARM("M2210 F2000 T200\n");
			if (DIP_Switch&0x01)
			{
				to_uARM("M2210 F500 T200\n");
				_delay_ms(200);
				to_uARM("M2210 F1000 T500\n");
				_delay_ms(500);
				to_uARM("M2210 F2000 T500\n");
			}
		}
		if (uart_str_complete!=0)
		{
			routine_done=0;
			uart_str_complete=0;
			send_Byte_0('1');	
			for (counter=0;counter<=data_bytes_recieved;counter++)
			{
				switch (final_data[counter])//final data decoding
				{
					case 'X':
						recieved_Y=(final_data[counter+1]-48)*1000+(final_data[counter+2]-48)*100+(final_data[counter+3]-48)*10+final_data[counter+4]-48;
						send_Byte_0('1');
						_delay_ms(2);
						check++;
					break;
				
					case 'Y':
						recieved_X=(final_data[counter+1]-48)*1000+(final_data[counter+2]-48)*100+(final_data[counter+3]-48)*10+final_data[counter+4]-48;
						send_Byte_0('1');
						_delay_ms(2);
						check++;
					break;
						
					case 'O':
					routine_done=1;
					check++;	
				}
			}
			data_bytes_recieved=0;
			if ((check==0)||(check>2))
			{
				send_Byte_0('0');
			}
			else
			{
				check=0;
			}
			if (routine_done>=1)
			{
				send_to_uArm("G0 X200 Y0 Z150 F1000\n");
				routine_done=0;
				while(uart_string1[4] == 0x31) //ASCII '1' --> moving
				{
					to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
				}
				if (DIP_Switch&0x01)
				{
					to_uARM("M2210 F500 T200\n");
					_delay_ms(200);
					to_uARM("M2210 F1000 T500\n");
					_delay_ms(500);
					to_uARM("M2210 F2000 T500\n");
				}
			}
			else
			{
				if (DIP_Switch&0x80)
				{
					write_zahl(2,10,recieved_X,4,0,0);
					write_zahl(3,10,recieved_Y,4,0,0);
				}	
				//Grid anpassung
				recieved_X-=384;
				recieved_Y-=512;				
				recieved_X=((recieved_X/5)*-1)+200;
				recieved_Y=(recieved_Y/5)*-1;	
				////////////////
							
				snprintf(buffer,30,"G0 X%d Y%d Z-20 F6000\n",recieved_X,recieved_Y);
				if (DIP_Switch&0x80)
				{
					write_zahl(2,0,recieved_X,4,0,0);
					write_zahl(3,0,recieved_Y,4,0,0);
				}
				send_to_uArm(buffer);
				while(uart_string1[4] == 0x31) //ASCII '1' --> moving
				{
					to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
					ws2812_sendarray(array,LED_AMOUNT);
					
				}
				if (DIP_Switch&0x01)
				{
					to_uARM("M2210 F2000 T200\n");
				}
				send_Byte_0('1');
			}	
		}
	} //end while(1)
} //end main












//	neu=taster;
	//	DIP_Switch=get_DIP_Switch();
		//move(direction,1);
		
		
	//	write_zahl(0,8,x1,6,2,2);
		//write_zahl(1,8,y1,6,2,2);
	//	write_zahl(2,8,z1,6,2,2);
	//	write_zahl(0,1,direction,4,0,0);
	//	write_zahl(1,1,taster,4,0,0);	
		
		
	/*	switch (DIP_Switch)
		{
			case CASE1:
				if (taster&0x01)
				{
					move(UP,1);
				}
				if (taster&0x02)
				{
					move(DOWN,1);
				}
				if (taster&0x04)
				{
					//send_to_uArm("P2220\n");
					x1=Get_uArm_Koordinate(1);
					y1=Get_uArm_Koordinate(2);
					z1=Get_uArm_Koordinate(3);
					//XYZ_to_Display(50);
				
				}
			break;
			
			case CASE2:
				if (neu>alt)
				{
					if (taster==0x01)
					{
						move(LEFT,1);
					}	
					if (taster==0x02)
					{
						move(BACKWARD,1);
					}

					if (taster==0x04)
					{
						move(RIGHT,1);
					}
					if (taster==0x08)
					{
						move(FORWARD,1);
					}				
					//case default:
					//send_to_uArm("G0 X200 Y0 Z150 F1000\n");
					//break;
				}
			break;
			
			case CASE4:
			if (taster&0x08)
			{
				send_to_uArm("G0 X170 Y0 Z160 F1000\n");			//ausgansgpkt
				clear_lcd();
			}
			break;
			case 5:
			
			
			
			
			
			
			break;
			*/
			