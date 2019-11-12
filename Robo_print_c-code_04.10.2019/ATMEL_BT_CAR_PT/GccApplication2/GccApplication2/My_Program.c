/*
O.K.E
by Tendai und Jan
*/

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>							// Ermöglicht die Platzierung von "static const" im Code-Segment, statt im RAM.
													// Definition mit "PROGMEM", Lesen mit "pgm_read_byte, pgm_read_ptr"	
#define F_CPU 16000000UL
#include <util/delay.h>											
																			
#pragma GCC optimize 0								// Optimierung ausschalten, damit das Debugging möglich ist

#define UART_MAXSTELLEN 50
#define LEFT			1
#define RIGHT			2
#define UP				3
#define DOWN			4
#define BACKWARD		5
#define FORWARD			6
#define NOTEAMOUNT		20
#define DATA_LENGTH		4
#define DO_START_UP		1


#define ALL_OFF			0x50
#define ROBO_ON_THE_RUN	0x30
#define WAITING_FOR_FIRST_CMD	0x10
#define FINISHED		0x20
#define IDLE			0x40


unsigned int x_Wert_ADC,y_Wert_ADC;
unsigned char uart_str_complete,uart_str_complete1 = 0;     // 1 --> String komplett empfangen
unsigned char uart_str_count,uart_str_count1= 0;
char uart_string[UART_MAXSTELLEN + 1] = "";
char uart_string_send[UART_MAXSTELLEN + 1] = "";
char uart_string1[UART_MAXSTELLEN + 1] = "";
char uart_string_send1[UART_MAXSTELLEN + 1] = "";
unsigned char data [12];
unsigned char final_data[12];
unsigned char play_sound,routine_done,good,false_state,data_bytes_recieved,pc_ready,good,false_state=0;
unsigned int lol=0;
unsigned char play_beep=1;

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
void set_led_mode (unsigned char modus)
{
	clear_port();
	PORTD|=modus;
}
void send_Byte_0(unsigned char val)
{
	//Wait for empty Buffer
	while( !(UCSR0A & (1<<UDRE0)) );
	UDR0=val;	//Transmit starts
}
void send_Byte_1(unsigned char val)
{
	//Wait for empty Buffer
	while( !(UCSR1A & (1<<UDRE1)));
	UDR1=val;	//Transmit starts
}
void to_pc (char *_str)
{
	while (*_str)
	{   
		send_Byte_0(*_str);
		_str++;
	}
	uart_str_complete = 0; //String freigeben um Antwort zu empfangen
}
void to_uARM(char *_str)
{
	while (*_str)
	{   
		send_Byte_1(*_str);
		_str++;
	}
	uart_str_complete1 = 0; //String freigeben um Antwort zu empfangen
}
void send_to_uArm(char *str)	//cha mer alles lösche.. wird nöd brucht
{
	to_uARM(str);
	to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
	while(uart_string1[4] == 0x31) //ASCII '1' --> moving
	{
		to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
	}
}
void start_nowait()
{
	send_to_uArm("G0 X200 Y0 Z150 F6000\n");			//ausgansgpkt	(200 0 150)
}
ISR (USART0_RX_vect) // züg mit em räbi
{
	unsigned char nextChar;
	nextChar = UDR0;
	
	switch(nextChar)
	{
		case 'A':
			set_led_mode(ROBO_ON_THE_RUN);
			to_uARM("G0 X200 Y0 Z-20 F6000\n");
		break;
		
		case 'C':
			set_led_mode(WAITING_FOR_FIRST_CMD);
			play_sound=1;
		break;
		
		case 'S':
			set_led_mode(IDLE);
		break;
		
		case '\n':
			data_bytes_recieved = 0;
			uart_str_complete=1;
			break;
			
		case 'O':
			set_led_mode(FINISHED);
			start_nowait();
			routine_done=1;
		break;
		
		case 'G':
			good=1;
		break;
		
		case 'E':
			false_state=1;
		break;
		
		case 'D':
			pc_ready=1;
		break;
		
		case 'M':
			play_sound=1;
		break;
		
		case 'B':
			if (play_beep!=0)
			{
				play_beep=0;
			}
			else
			{
				play_beep=1;
			}			
		default:
			data[data_bytes_recieved]=nextChar;
			data_bytes_recieved++;
		break;
	}
	
	
  // Daten aus dem Puffer lesen
  
	//if( nextChar != '\n' && uart_str_count1 < UART_MAXSTELLEN)
	//{
	  //data[data_bytes_recieved]=nextChar;
	  //data_bytes_recieved++;	
	//} 
	//else
	//{
		//for (i=0;i<=data_bytes_recieved;i++)
		//{
			//final_data[i]=data[i];
		//}  
	 //uart_str_complete=1;
	//}
 }
ISR (USART1_RX_vect)
{
	unsigned char nextChar;

	// Daten aus dem Puffer lesen
	nextChar = UDR1;
	if( uart_str_complete1 == 0 )
	{	// Daten werden erst in uart_string geschrieben, wenn nicht String-Ende/max Zeichenlänge erreicht ist/string gerade verarbeitet wird
		
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
void clear_port()
{
	PORTD&=~0x70;
}
void make_sound()
{
	if (play_beep)
	{
		to_uARM("M2210 F500 T200\n");
		_delay_ms(200);
		to_uARM("M2210 F1000 T500\n");
		_delay_ms(500);
		to_uARM("M2210 F2000 T500\n");
	}
}
void goto_start()
{
	send_to_uArm("G0 X200 Y0 Z150 F6000\n");			//ausgansgpkt	(200 0 150)
	while(uart_string1[4] == 0x31) //ASCII '1' --> moving
	{
		to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
	}
}
void beep()
{
	if (play_beep)
	{
		to_uARM("M2210 F2000 T200\n");
	}
}
unsigned char read_buttons ()
{
	return PINB&0x03;
}
void buttons(unsigned char taster)
{
	if (taster&0x01)
	{
		goto_start();
		beep_short();
	}
	if (taster&0x02)
	{
		play_somthin_booy();
	}
}
void play_somthin_booy()
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
	//_delay_ms(500);
}
void play_short()
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
}
void beep_short()
{
	to_uARM("M2210 F500 T20\n");
}
void error_beep()
{
	if (play_beep)
	{
		to_uARM("M2210 F800 T250\n");
		_delay_ms(300);
		to_uARM("M2210 F400 T250\n");
		_delay_ms(300);
		to_uARM("M2210 F200 T250\n");
		_delay_ms(300);
	}
	else
	{
		play_short();
	}
}
void error_state()
{
	set_led_mode(WAITING_FOR_FIRST_CMD);
	error_beep();
	goto_start();
}

void start_up_routine ()
{
	DDRA=0x0f;
	DDRB=0x00;
	DDRC=0x00;
	DDRD=0x00;
	sei(); 					// Global Interrupts aktivieren
	init_UART0();
	init_UART1();
	set_led_mode(IDLE);
	_delay_ms(10);
	goto_start();
	make_sound();
	//if (DO_START_UP)
	//{
		//while(pc_ready==0)
		//{
			//send_Byte_0('D');
			//_delay_ms(500);
		//}
	//}
	beep_short();
	goto_start();
	set_led_mode(FINISHED);
	_delay_ms(5000);
	set_led_mode(IDLE);
	goto_start();
}

int main (void)
{
	unsigned char taster,direction,counter,go_through;
	int recieved_X,recieved_Y;
	unsigned char buffer [30];
	unsigned char return_buffer[30];
	start_up_routine();
	while(1)
	{
		taster=read_buttons();
		buttons(taster);
		if (play_sound!=0)
		{
			play_somthin_booy();
			play_sound=0;
		}
		if (routine_done>=1) //if routine done
		{
			make_sound();
			routine_done=0;
		}
		if (uart_str_complete!=0)
		{
			uart_str_complete=0;
			send_Byte_0('1');	
			recieved_Y=(data[1]-48)*1000+(data[2]-48)*100+(data[3]-48)*10+data[4]-48;
			recieved_X=(data[6]-48)*1000+(data[7]-48)*100+(data[8]-48)*10+data[9]-48;
			//da hets en 2 ms delay drin gha... wer weiss
			snprintf(return_buffer,30,"%d %d",recieved_X,recieved_Y);
			to_pc(return_buffer);
			while(((false_state==0)&&(good==0))||((taster&0x01)!=0))
			{
				taster = read_buttons();
			}
			if (good!=0)
			{
				good=0;
				//Grid anpassung
				recieved_X-=384;
				recieved_Y-=512;
				recieved_X=((recieved_X/5)*-1)+200;
				recieved_Y=(recieved_Y/5)*-1;
				snprintf(buffer,30,"G0 X%d Y%d Z-20 F6000\n",recieved_X,recieved_Y);//////////////////form new string
				//to_uArm(buffer);		//send new string
				to_uARM(buffer);
				_delay_ms(2);				//kleines wait bevor abfrage 
				while(uart_string1[4] == 0x31) //ASCII '1' --> moving
				{
					to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
				}
				beep();
				send_Byte_0('1');		//return that action done
			}
			else
			{
				false_state=0;
				error_state();
				_delay_ms(500);
				set_led_mode(IDLE);
			}
		}		
	} //end while(1)
} //end main






		//C#2/Db2  	69.30 	497.87
		//D#2/Eb2  	77.78 	443.55
		//F#2/Gb2  	92.50 	372.98
		//G#2/Ab2  	103.83 	332.29
		//A#2/Bb2  	116.54 	296.03
		//C#3/Db3  	138.59 	248.93
		//D#3/Eb3  	155.56 	221.77
		//C0=16,
		//C_0=17.32,
		//D0=18.35,
		//D_0=19.45,
		//E0=20.60,
		//F0=21.83,
		//F_0=23.12,
		//G0=24.50,
		//G_0=25.96,
		//A0=27.50,
		//A_0=29.14,
		//B0=30.87,
		//C1=32.70,
		//C_1=34.65,
		//D1=	36.71,
		//D_1=38.89,
		//E1=	41.20,
		//F1=43.65,
		//F_1=46.25,
		//G1=	49.00,
		//G_1=51.91,
		//A1=55.00,
		//A_1=58.27,



	//unsigned int notes[NOTEAMOUNT]{62,65,73,82,87,98,119,123,130,147,165,175}
	//typedef enum
	//{
			//
			//B1=62,
			//C2=65,
			//C_2=69,
			//D2=73,
			//D_2=78,
			//E2=82,
			//F2=87,
			//F_2=93, 	
			//G2=98,
			//G_2=104, 	
			//A2=110,
			//A_2=117, 	
			//B2=	123,
			//C3=131,
			//C_3=139, 	
			//D3=147,
			//D_3=156,
			//E3=165,
			//F3=175,
			//F_3=185, 	
			//G3=196,
			//G_3=208, 	
			//A3=220,
			//A_3=233, 	
			//B3=247,
			//C4=262,
			//C_4=277, 	
			//D4=294,
			//D_4=311, 	
			//E4=330,
			//F4=349,
			//F_4=370, 
			//G4=392,
			//G_4=415, 
			//A4=440,
			//A_4=466,
			//B4=	494,
			//C5=523,
			//C_5=554,
			//D5=587,
			//D_5=622, 	
			//E5=659,
			//F5=698,	
			//F_5=740, 
			//G5=784,
			//G_5=831, 	
			//A5=880,
			//A_5=932,	
			//B5=988,
			//C6=	1047,
			//C_6=1108, 
			//D6=1175,
			//D_6=1245,
			//E6=1319,
			//F6=1397,
			//F_6=1480, 
			//G6=1568,
			//G_6=1661,
			//A6=1760,
			//A_6=1865,
			//B6=1976,
			//C7=2093,
			//C_7=2217,
			//D7=2349,
			//D_7=2489,
			//E7=2637,
			//F7=2794,
			//F_7=2960,
			//G7=3136,
			//G_7=3322,
			//A7=	3520,
			//A_7=3729,
			//B7=3951,
			//C8=4186,
			//C_8=4435,
			//D8=	4699,
			//D_8=4978,
			//E8=5274,
			//F8=	5588,
			//F_8=5920,
			//G8=6272,
			//G_8=6645,
			//A8=7040,
			//A_8=7459,
			//B8=7902,
			//
	//} letter;