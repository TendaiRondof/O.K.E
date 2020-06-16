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


#define ALL_OFF			0x50
#define ROBO_ON_THE_RUN	0x30
#define WAITING_FOR_FIRST_CMD	0x10
#define FINISHED		0x20
#define IDLE			0x40



char trennzeichen[] = " ";
char *ptr_Abschnitt;
unsigned char uart_str_count,uart_str_count1,uart_str_complete,uart_str_complete1=0;
char uart_string[UART_MAXSTELLEN + 1] = "";
char uart_string_send[UART_MAXSTELLEN + 1] = "";
char uart_string1[UART_MAXSTELLEN + 1] = "";
char uart_string_send1[UART_MAXSTELLEN + 1] = "";
unsigned char data [12];
unsigned char final_data[12];
unsigned char play_sound,routine_done,good,false_state,data_bytes_recieved,pc_ready,play_beep, LCD_Taster, k ,m, Recieve_count;
unsigned int lol=0;
unsigned char good,false_state=0;





unsigned char get_taster(void)
{
	return PINB&0x03;
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
	//for(m=0;m<UART_MAXSTRLEN+1;m++) //Array löschen nicht nötig!
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
	//for(m=0;m<UART_MAXSTRLEN+1;m++) //Array löschen nicht nötig!
	//uart_string[m]='\0';
	while (*_str)
	{   
		send_Byte_1(*_str);
		_str++;
	}
	uart_str_complete1 = 0; //String freigeben um Antwort zu empfangen
//	while (uart_str_complete1 == 0); //Warten bis Antwort erfolgt ist
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
ISR (USART0_RX_vect) // raspberry
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
			
		default:
			data[data_bytes_recieved]=nextChar;
			data_bytes_recieved++;
		break;
	}
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
void set_led_mode (unsigned char modus)
{
	clear_port();
	PORTD|=modus;
}
void make_sound()
{
	to_uARM("M2210 F500 T200\n");
	_delay_ms(200);
	to_uARM("M2210 F1000 T500\n");
	_delay_ms(500);
	to_uARM("M2210 F2000 T500\n");
}
void goto_start()
{
	send_to_uArm("G0 X200 Y0 Z150 F6000\n");			//ausgansgpkt	(200 0 150)
	while(uart_string1[4] == 0x31) //ASCII '1' --> moving
	{
		to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
	}
}
void beep(unsigned char dipswitch)
{
	if (dipswitch)
	{
		to_uARM("M2210 F2000 T200\n");
	}
}
void start_nowait()
{
	send_to_uArm("G0 X200 Y0 Z150 F6000\n");			//ausgansgpkt	(200 0 150)
}
void start_up_routine ()
{
	DDRB  =  0x00;
	DDRA  = 0x0F;
	DDRD  =  0xFA;
	sei();
	init_UART0();
	init_UART1();
	_delay_ms(10);
	set_led_mode(IDLE);
	goto_start();
	make_sound();
	_delay_ms(2000);	
	//while(pc_ready==0)
	//{
		//send_Byte_0('D');
		//wait_1ms(500);
	//}
	goto_start();
	set_led_mode(FINISHED);
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
void error_beep()
{
	to_uARM("M2210 F800 T250\n");
	_delay_ms(250);
	to_uARM("M2210 F400 T250\n");
	_delay_ms(250);
	to_uARM("M2210 F200 T400\n");
}



int main (void)
{
	unsigned char taster,direction,counter,go_through,neu;
	int recieved_X,recieved_Y;
	unsigned char buffer [30];
	unsigned char return_buffer[30];
	start_up_routine();	
	goto_start();
	to_uARM("M2210 F500 T20\n");
	//set_led_mode(IDLE);
	goto_start();
	
	while(1)
	{	
		taster=get_taster();
		if (PINB&0x01)
		{
			goto_start();
			make_sound();
		}		
		if ((PINB&0x02)>(neu&0x02))
		{
			beep(1);
			if (play_beep!=0)	play_beep=0;
			else play_beep=1;

		}
		if (play_sound!=0)
		{
			play_somthin_booy();
			play_sound=0;
		}
		if (routine_done>=1) //if routine done
		{
			make_sound();
			_delay_ms(50);
			routine_done=0;
		}
		if (uart_str_complete!=0)
		{
			good=0;
			uart_str_complete=0;
			send_Byte_0('1');
			recieved_Y=(data[1]-48)*1000+(data[2]-48)*100+(data[3]-48)*10+data[4]-48;
			recieved_X=(data[6]-48)*1000+(data[7]-48)*100+(data[8]-48)*10+data[9]-48;
			_delay_ms(2);
			snprintf(return_buffer,30,"%d %d",recieved_X,recieved_Y);
			to_pc(return_buffer);
			while(((false_state==0)&&(good==0))||((taster&0x01)!=0))
			taster = get_taster();
			
			if (good!=0)
			{
				//Grid anpassung
				recieved_X-=384;
				recieved_Y-=512;
				recieved_X=((recieved_X/5)*-1)+200;
				recieved_Y=(recieved_Y/5)*-1;
				snprintf(buffer,30,"G0 X%d Y%d Z-20 F6000\n",recieved_X,recieved_Y);//////////////////form new string
				send_to_uArm(buffer);		//send new string
				_delay_ms(2);
				while(uart_string1[4] == 0x31) //ASCII '1' --> moving
				{
					to_uARM("M2200\n"); //uARM in moving? 1 Yes / 0 N0
				}
				beep(play_beep);
				send_Byte_0('1');		//return that action done
			}
			else
			{
				false_state=0;
				set_led_mode(WAITING_FOR_FIRST_CMD);
				error_beep();
				_delay_ms(1000);
				set_led_mode(IDLE);
			}
			neu=taster;
		}		
	} //end while(1)
} //end main













	//	DIP_Switch=get_DIP_Switch();
		//move(direction,1);
		
		
	//	write_zahl(0,8,x1,6,2,2);
		//write_zahl(1,8,y1,6,2,2);
	//	write_zahl(2,8,z1,6,2,2);
	//	write_zahl(0,1,direction,4,0,0);
	//	write_zahl(1,1,taster,4,0,0);	
		
		
	
			
			
			
			
			
			
			
			//to_uARM("M2210 F510 T100\n");
			//_delay_ms(450);
			//to_uARM("M2210 F380 T100\n");
			//_delay_ms(400);
			//to_uARM("M2210 F320 T100\n");
			//_delay_ms(500);
			//to_uARM("M2210 F440 T100\n");
			//_delay_ms(300);
			//to_uARM("M2210 F480 T80\n");
			//_delay_ms(330);
			//to_uARM("M2210 F450 T100\n");
			//_delay_ms(150);
			//to_uARM("M2210 F430 T100\n");
			//_delay_ms(300);
			//to_uARM("M2210 F380 T100\n");
			//_delay_ms(200);
			//to_uARM("M2210 F660 T80\n");
			//_delay_ms(200);
			//to_uARM("M2210 F760 T50\n");
			//_delay_ms(150);
			//to_uARM("M2210 F860 T100\n");
			//_delay_ms(300);
			//to_uARM("M2210 700 T80\n");
			//_delay_ms(150);
			//to_uARM("M2210 F760 T50\n");
			//_delay_ms(350);
			//to_uARM("M2210 F660 T80\n");
			//_delay_ms(300);
			//to_uARM("M2210 F520 T80\n");
			//_delay_ms(150);
			//to_uARM("M2210 F580 T80\n");
			//_delay_ms(150);
			//to_uARM("M2210 F480 T80\n");
			//_delay_ms(500);
			//
			//to_uARM("M2210 F500 T100\n");
			//_delay_ms(3//if (play_sound==1)
			//{
			//to_uARM("M2210 F660 T100\n");
			//_delay_ms(150);
			//to_uARM("M2210 F660 T100\n");
			//_delay_ms(300);
			//to_uARM("M2210 F660 T100\n");
			//_delay_ms(300);
			//to_uARM("M2210 F510 T100\n");
			//_delay_ms(100);
			//to_uARM("M2210 F660 T100\n");
			//_delay_ms(300);
			//to_uARM("M2210 F770 T100\n");
			//_delay_ms(550);
			//to_uARM("M2210 F380 T100\n");
			//_delay_ms(575);
			//
			//
			//to_uARM("M2210 F510 T100\n");
			//_delay_ms(450);
			//to_uARM("M2210 F380 T100\n");
			//_delay_ms(400);
			//to_uARM("M2210 F320 T100\n");
			//_delay_ms(500);
			//to_uARM("M2210 F440 T100\n");
			//_delay_ms(300);
			//to_uARM("M2210 F480 T80\n");
			//_delay_ms(330);
			//to_uARM("M2210 F450 T100\n");
			//_delay_ms(150);
			//to_uARM("M2210 F430 T100\n");
			//_delay_ms(300);
			//to_uARM("M2210 F380 T100\n");
			//_delay_ms(200);
			//to_uARM("M2210 F660 T80\n");
			//_delay_ms(200);
			//to_uARM("M2210 F760 T50\n");
			//_delay_ms(150);
			//to_uARM("M2210 F860 T100\n");
			//_delay_ms(300);
			//to_uARM("M2210 F700 T80\n");
			//_delay_ms(150);
			//to_uARM("M2210 F760 T50\n");
			//_delay_ms(350);
			//to_uARM("M2210 F660 T80\n");
			//_delay_ms(300);
			//to_uARM("M2210 F520 T80\n");
			//_delay_ms(150);
			//to_uARM("M2210 F580 T80\n");
			//_delay_ms(150);
			//to_uARM("M2210 F480 T80\n");
			//_delay_ms(500);
			//play_sound=0;
			//}00);