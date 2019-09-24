/*************************************************************************** 
    MSW-Winterthur Berufsfachschule, Zeughausstrasse 56, 8400 Winterthur     
    ---------------------------------------------------------------------    
	Bluetooth CAR V21
    Datum       : 7.7.2015
    Version		: 2.1
    Beschreibung: Demo Projekt für das Board BT_CAR_V21, Microprozessor TMEL644PA
    Autor  		: Peter Trüb, LCD Treiber Christian Riedel

****************************************************************************/

//Treiber ws_2812 --> https://github.com/cpldcpu/light_ws2812/tree/master/light_ws2812_AVR

#include <avr/io.h>
#include <avr/interrupt.h>
#include "BT_CAR_V2_0.h"
#include "light_ws2812.h"
#include <avr/pgmspace.h>							// Ermöglicht die Platzierung von "static const" im Code-Segment, statt im RAM.
													// Definition mit "PROGMEM", Lesen mit "pgm_read_byte, pgm_read_ptr"
																			
#pragma GCC optimize 0								// Optimierung ausschalten, damit das Debugging möglich ist

#define DIP_SWITCH 8

#define MAX_RGBs 79
#define COLORLENGTH 79
#define FADE 5

#define ALL_OFF			0x50
#define ROBO_ON_THE_RUN	0x30
#define WAITING_FOR_FIRST_CMD	0x10
//#define NULL
#define FINISHED		0x20
#define IDLE			0x40

struct cRGB led[MAX_RGBs];
struct cRGB colors[8];

u8 DIP_Switch, T=0;
u16 i=0,j=0,k=0,L=0;


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

// ==============================================================================================================
// Hier beginnt das Hauptprogramm "main"
// --------------------------------------------------------------------------------------------------------------
int main (void)
{
	unsigned char activated=0;
	unsigned char activated2=MAX_RGBs/2;
	unsigned char data;
	init_BT_CAR_V2_0();			// Das Board wird hier initialisiert
	write_text(0,4, PSTR("RGB Strip DEMO!"));	
	wait_1ms(1000);	
	clear_lcd();				// LCD clear
	write_text(0, 0 ,PSTR("RGB STRIP START"));
	write_text(3,11, PSTR("time: "));

    for(i=MAX_RGBs; i>0; i--)
    {
	    led[i-1].r=0;led[i-1].g=0;led[i-1].b=0;
    }

    //Rainbowcolors
    colors[0].r=150; colors[0].g=150; colors[0].b=150;
    colors[1].r=255; colors[1].g=000; colors[1].b=000;//red
    colors[2].r=255; colors[2].g=100; colors[2].b=000;//orange
    colors[3].r=100; colors[3].g=255; colors[3].b=000;//yellow
    colors[4].r=000; colors[4].g=255; colors[4].b=000;//green
    colors[5].r=000; colors[5].g=100; colors[5].b=255;//light blue (türkis)
    colors[6].r=000; colors[6].g=000; colors[6].b=255;//blue
    colors[7].r=100; colors[7].g=000; colors[7].b=255;//violet

	while(1)
	{    
		
		data=(PIND&0xF0);
		write_zahl(1,0,data,3,0,0);
		//DIP_Switch=get_DIP_Switch();
		switch(data)
		{
			case ALL_OFF: //alle led aus
			 for(i=MAX_RGBs; i>0; i--)
			 {
				 led[i-1].r=0;led[i-1].g=0;led[i-1].b=0;
			 }
			 ws2812_sendarray((uint8_t *)&led,MAX_RGBs*3); //Ausgabe des ganzen Arrays
			 break;
					break;
			
			case ROBO_ON_THE_RUN: //robo move		
					for(i=0;i<MAX_RGBs;i++)
					{
						if ((i==activated)||(i==activated2))
						{
							led[i].r=255;
							led[i].g=128;
							led[i].b=0;
						}
						else
						{
							led[i].r=led[i].r/1.1;
							led[i].g=led[i].g/1.1;
							led[i].b=led[i].b/1.1;
						}
					}
					if (activated>MAX_RGBs)
					{
						activated=0;
					}
					else
					{
						activated++;
					}
					if (activated2>MAX_RGBs)
					{
						activated2=0;
					}
					else
					{
						activated2++;
					}
					wait_1ms(50);
					ws2812_setleds(led,MAX_RGBs);
				break;
					
					
			case WAITING_FOR_FIRST_CMD: //tendai rechne
				for(i=0;i<MAX_RGBs;i++)
				{
					if ((i==activated)||(i==activated2))
					{
						led[i].r=255;
						led[i].g=0;
						led[i].b=0;
					}
					else
					{
						led[i].r=led[i].r/1.1;
						led[i].g=led[i].g/1.1;
						led[i].b=led[i].b/1.1;
						//led[i].r-=10;
						//led[i].g-=10;
						//led[i].b-=10;
					}
				}
				if (activated>MAX_RGBs)
				{
					activated=0;
				}
				else
				{
					activated++;
				}
				if (activated2>MAX_RGBs)
				{
					activated2=0;
				}
				else
				{
					activated2++;
				}
				wait_1ms(50);
				ws2812_setleds(led,MAX_RGBs);
			break;		
					
					
			case 4: //Alle RGB's gelb 
					for(i=0;i<MAX_RGBs;i++)
					{
						if ((i==activated)||(i==activated2))
						{
							led[i].r=255;
							led[i].g=128;
							led[i].b=0;
						}
						else
						{
							led[i].r=led[i].r/1.1;
							led[i].g=led[i].g/1.1;
							led[i].b=led[i].b/1.1;
						}
					}
					if (activated>MAX_RGBs)
					{
						activated=0;
					}
					else
					{
						activated++;
					}
					if (activated2>MAX_RGBs)
					{
						activated2=0;
					}
					else
					{
						activated2++;
					}
					wait_1ms(50);
					ws2812_setleds(led,MAX_RGBs);
					break;
					
			
					
			case FINISHED://Grünes Lauflicht für RGBs
					for(i=0;i<MAX_RGBs;i++)
					{
						if ((i==activated)||(i==activated2))
						{
							led[i].r=0;
							led[i].g=255;
							led[i].b=0;
						}
						else
						{
							led[i].r=led[i].r/1.1;
							led[i].g=led[i].g/1.1;
							led[i].b=led[i].b/1.1;
						}
					}
					if (activated>MAX_RGBs)
					{
						activated=0;
					}
					else
					{
						activated++;
					}
					if (activated2>MAX_RGBs)
					{
						activated2=0;
					}
					else
					{
						activated2++;
					}
					wait_1ms(50);
					ws2812_setleds(led,MAX_RGBs);
					break;
					
			case IDLE://Blaues Lauflicht für RGBs
					for(i=0;i<MAX_RGBs;i++)
					{
						if ((i==activated)||(i==activated2))
						{
							led[i].r=0;
							led[i].g=0;
							led[i].b=255;
						}
						else
						{
							led[i].r=led[i].r/1.1;
							led[i].g=led[i].g/1.1;
							led[i].b=led[i].b/1.1;
						}
					}
					if (activated>MAX_RGBs)
					{
						activated=0;
					}
					else
					{
						activated++;
					}
					if (activated2>MAX_RGBs)
					{
						activated2=0;
					}
					else
					{
						activated2++;
					}
					wait_1ms(50);
					ws2812_setleds(led,MAX_RGBs);
					break;
					
			//case 7:
			//break;
					
					
			//case 128://Blitz
					 ////Zuerst alle RGBs schwach gelb
					 //for(i=MAX_RGBs; i>0; i--)
					 //{
						 //led[i-1].r=55;led[i-1].g=55;led[i-1].b=0;
					 //}
					 //ws2812_sendarray((uint8_t *)&led,MAX_RGBs*3); //Ausgabe des ganzen Arrays
					 //wait_1ms(1000);					 
					 ////Blitz bei RGB's 10 bis 20
					 //for(T=10;T<20;T++)
					 //{
						 //led[T].r=255;led[T].g=255;led[T].b=255;
					 //}
					 ////Erlaubt einen Arrayauschnitt auszugeben
					 //ws2812_sendarray((uint8_t *)&led,MAX_RGBs*3); //Ausgabe des ganzen Arrays
					 //wait_1ms(50);					 
					 //break;
					 
			default: //Alle RGBs clear
					 for(i=MAX_RGBs; i>0; i--)
					 {
						 led[i-1].r=0;led[i-1].g=0;led[i-1].b=0;
					 }
					 ws2812_sendarray((uint8_t *)&led,MAX_RGBs*3); //Ausgabe des ganzen Arrays
					 break;
		}
		write_zahl(3, 16,tick_1s,4,0,0);
		write_zahl(0,0,DIP_Switch,3,0,0);
  }
}