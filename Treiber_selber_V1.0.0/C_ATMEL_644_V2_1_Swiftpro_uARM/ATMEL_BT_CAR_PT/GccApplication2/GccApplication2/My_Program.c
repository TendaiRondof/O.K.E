/*************************************************************************** 
    MSW-Winterthur Berufsfachschule, Zeughausstrasse 56, 8400 Winterthur     
    ---------------------------------------------------------------------    
	Bluetooth CAR V21
    Datum       : 7.7.2015
    Version		: 2.1
    Beschreibung: Demo Projekt f�r das Board BT_CAR_V21, Microprozessor TMEL644PA
    Autor  		: Peter Tr�b, LCD Treiber Christian Riedel

****************************************************************************/

//snprintf(test,100,"G2204 X%d Y10 Z10 F1000\n",45);

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include "BT_CAR_V2_0.h"
#include <avr/pgmspace.h>							// Erm�glicht die Platzierung von "static const" im Code-Segment, statt im RAM.
													// Definition mit "PROGMEM", Lesen mit "pgm_read_byte, pgm_read_ptr"
																			
#pragma GCC optimize 0								// Optimierung ausschalten, damit das Debugging m�glich ist
u8 Receive_count=0,i=0;

char trennzeichen[] = " ";
char *ptr_Abschnitt;
int x_Koordinate,y_Koordinate,x_tmp,y_tmp,alt_x_tmp,alt_y_tmp,z_Koordinate,speed=10000;
u8 DIP_Switch=0, LCD_Taster=0, k=0,m=0;
u16 x_Wert_ADC,y_Wert_ADC;

#define UART_MAXSTELEN 50
#define LEFT			1
#define RIGHT			2
#define UP				3
#define DOWN			4
#define BACKWARD		5
#define FORWARD			6




typedef enum
{
	gagunggi,
	CASE1,
	CASE2,
	CASE3,
	CASE4,
	MOVE_PROTOTYPE_5,
} color;


u8 uart_str_complete = 0;     // 1 --> String komplett empfangen
unsigned char data_bytes_recieved=0;
u8 uart_str_count = 0;
char uart_string[UART_MAXSTELEN + 1] = "";
char uart_string_send[UART_MAXSTELEN + 1] = "";
unsigned char data [6];
unsigned char final_data[6];


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

void to_uARM(char *_str)
{
	//for(m=0;m<UART_MAXSTRLEN+1;m++) //Array l�schen nicht n�tig!
	//uart_string[m]='\0';
	while (*_str)
	{   
		send_Byte_1(*_str);
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
  unsigned char nextChar;
  // Daten aus dem Puffer lesen
  nextChar = UDR0;
  if (data_bytes_recieved<=4)
  {
	  data[data_bytes_recieved]=nextChar;
	  data_bytes_recieved++;
	  if (data_bytes_recieved>4)
	  {
		data_bytes_recieved=0;
		//copy data to different array
		final_data[0]=data[0];
		final_data[1]=data[1];
		final_data[2]=data[2];
		final_data[3]=data[3];
		final_data[4]=data[4];
		uart_str_complete=1; 
	  }
  }   
}

void get_tecnical_data (unsigned char data_request)
{
	switch (data_request)
	{
		case 0x01:	//get_angles of joints
			send_to_uArm("P2200\n");
		break;
		
		case 0x02:	//get_devicename
			send_to_uArm("P2201\n");		
		break;
		
		case 0x03:	//get hardware version
			send_to_uArm("P2202\n");
		break;
		
		case 0x04:	//get softwareversion
			send_to_uArm("P2203\n");
		break;
		
		case 0x05:	//get API version
			send_to_uArm("P2204\n");
		break;
		
		case 0x06:	//get UID
			send_to_uArm("P2205\n");
		break;
		
		case 0x07:	//get current coordinates
			send_to_uArm("P2220\n");
		break;
		
		case 0x08:	//check status
		send_to_uArm("P2400\n");
		break;
		
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

void setup_bt ()
{
	to_uARM("M2234 V1\n");
	to_uARM("M2245 GUGUSELI\n");
	
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


int main (void)
{
	init_BT_CAR_V2_0();			// Das Board wird hier initialisiert	
	wait_1ms(1000);	
	clear_lcd();				// LCD clear
	init_ADC();
	init_UART0();
	init_UART1();
	unsigned char taster;
	int i;
	unsigned char direction;
	unsigned char data_1;
	unsigned char data_2;
	unsigned char data_3;
	unsigned char x1;
	unsigned char y1;
	unsigned char z1;
	unsigned char alt=0;
	unsigned char neu=0;
	unsigned int recieved_X;
	unsigned int recieved_Y;
	unsigned int recieved_Z;
	unsigned char buffer [50];
	
	
	clear_lcd();
	while(1)
	{
		direction=get_direction();
		taster = get_LCD_Taster();
		neu=taster;
		DIP_Switch=get_DIP_Switch();
		move(direction,1);
		
		
		write_zahl(0,8,x1,6,2,2);
		write_zahl(1,8,y1,6,2,2);
		write_zahl(2,8,z1,6,2,2);
		write_zahl(0,1,direction,4,0,0);
		write_zahl(1,1,taster,4,0,0);	
		
		
		switch (DIP_Switch)
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
			case MOVE_PROTOTYPE_5:
			if (uart_str_complete!=0)
			{
				uart_str_complete=0;
				send_Byte_0('A');
			}
			switch (final_data[0])//final data decoding
			{
				case 'X':
					recieved_X=final_data[1]*1000+final_data[2]*100+final_data[3]*10+final_data[4];
				break;
				
				case 'Y':
					recieved_Y=final_data[1]*1000+final_data[2]*100+final_data[3]*10+final_data[4];
				break;
				
				case 'Z':
					recieved_Z=final_data[1]*1000+final_data[2]*100+final_data[3]*10+final_data[4];
				break;
			}
			snprintf(buffer,50,"G0 X%d Y%d Z150 F1000\n",recieved_X,recieved_Y);
			send_to_uArm(buffer);
			
			
			
			
			
			break;
		}
		alt=neu;
	} //end while(1)
} //end main