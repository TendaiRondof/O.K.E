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

#define MAX_RGBs 60
#define COLORLENGTH 60
#define FADE 5

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

		DIP_Switch=get_DIP_Switch();
		switch(DIP_Switch)
		{
			case 1: //Erste RGB Rot
					led[0].r=255;
					led[0].g=0;
					led[0].b=0;
					ws2812_setleds(led,1);
					break;
			
			case 2: //Erste RGB Grün, zweite Blau		
					led[0].r=0;
					led[0].g=255;
					led[0].b=0;
					led[1].r=0;
					led[1].g=0;
					led[1].b=255;
					ws2812_setleds(led,2);
					break;
					
			case 4: //Alle RGB's gelb 
					for(i=MAX_RGBs; i>0; i--)
					{
						led[i-1].r=55;led[i-1].g=55;led[i-1].b=0;
					}
					ws2812_setleds(led,MAX_RGBs); //Daten an alle RGB's!
					break;
					
			case 8: //Rotes Lauflicht für RGBs
					for(i=MAX_RGBs; i>0; i--)
					{
						led[MAX_RGBs-i].r=255;led[MAX_RGBs-i].g=0;led[MAX_RGBs-i].b=0;
						ws2812_setleds(led,MAX_RGBs);
						led[MAX_RGBs-i].r=0;led[MAX_RGBs-i].g=0;led[MAX_RGBs-i].b=0;
						wait_1ms(100);
					}
					break;
					
			case 16://Grünes Lauflicht für RGBs
					for(i=MAX_RGBs; i>0; i--)
					{
						led[MAX_RGBs-i].r=0;led[MAX_RGBs-i].g=255;led[MAX_RGBs-i].b=0;
						ws2812_setleds(led,MAX_RGBs);
						led[MAX_RGBs-i].r=0;led[MAX_RGBs-i].g=0;led[MAX_RGBs-i].b=0;
						wait_1ms(100);
					}
					break;
					
			case 32://Blaues Lauflicht für RGBs
					for(i=MAX_RGBs; i>0; i--)
					{
						led[MAX_RGBs-i].r=0;led[MAX_RGBs-i].g=0;led[MAX_RGBs-i].b=255;
						ws2812_setleds(led,MAX_RGBs);
						led[MAX_RGBs-i].r=0;led[MAX_RGBs-i].g=0;led[MAX_RGBs-i].b=0;
						wait_1ms(100);
					}
					break;
					
			case 64://Rainbow Colors
					//Rainbow Colors
					//Farben ausserhalb while(1) definiert
					//shift all values by one led
					i=0;
					for(i=(MAX_RGBs*3); i>1; i--)
						led[i-1]=led[i-2];
					//change colour when colourlength is reached
					if(k>COLORLENGTH)
					{
						j++;
						k=0;
					}
					k++;
					//loop colors
					if(j>8)
						j=0;
							
					//fade red
					if(led[0].r<colors[j].r)
						led[0].r+=FADE;
					if(led[0].r>255.r)
						led[0].r=255;
							
					if(led[0].r>colors[j].r)
						led[0].r-=FADE;
					//if(led[0].r<0)
					//led[0].r=0;
					//fade green
					if(led[0].g<colors[j].g)
						led[0].g+=FADE;
					if(led[0].g>255)
						led[0].g=255;
							
					if(led[0].g>colors[j].g)
						led[0].g-=FADE;
					//if(led[0].g<0)
					//led[0].g=0;
					//fade blue
					if(led[0].b<colors[j].b)
						led[0].b+=FADE;
					if(led[0].b>255)
						led[0].b=255;
							
					if(led[0].b>colors[j].b)
						led[0].b-=FADE;
					//if(led[0].b<0)
					//led[0].b=0;
							
					ws2812_sendarray((uint8_t *)&led, MAX_RGBs*3); //Daten an alle RGBs!			
					wait_1ms(1);
					break;
					
			case 128://Blitz
					 //Zuerst alle RGBs schwach gelb
					 for(i=MAX_RGBs; i>0; i--)
					 {
						 led[i-1].r=55;led[i-1].g=55;led[i-1].b=0;
					 }
					 ws2812_sendarray((uint8_t *)&led,MAX_RGBs*3); //Ausgabe des ganzen Arrays
					 wait_1ms(1000);					 
					 //Blitz bei RGB's 10 bis 20
					 for(T=10;T<20;T++)
					 {
						 led[T].r=255;led[T].g=255;led[T].b=255;
					 }
					 //Erlaubt einen Arrayauschnitt auszugeben
					 ws2812_sendarray((uint8_t *)&led,MAX_RGBs*3); //Ausgabe des ganzen Arrays
					 wait_1ms(50);					 
					 break;
					 
			default: //Alle RGBs clear
					 for(i=MAX_RGBs; i>0; i--)
					 {
						 led[i-1].r=0;led[i-1].g=0;led[i-1].b=0;
					 }
					 ws2812_sendarray((uint8_t *)&led,MAX_RGBs*3); //Ausgabe des ganzen Arrays
					 break;
		}
		write_zahl(3, 16,tick_1s,4,0,0);
  }
}