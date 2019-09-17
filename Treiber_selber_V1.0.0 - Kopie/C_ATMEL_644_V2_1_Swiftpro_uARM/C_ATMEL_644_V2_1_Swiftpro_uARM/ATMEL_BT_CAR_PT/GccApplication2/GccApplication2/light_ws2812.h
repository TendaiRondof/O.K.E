/*
 * light weight WS2812 lib include
 *
 * Version 2.0a3  - Jan 18th 2014
 * Author: Tim (cpldcpu@gmail.com) 
 *
 * Please do not change this file! All configuration is handled in "ws2812_config.h"
 *
 * License: GNU GPL v2 (see License.txt)
 +
 */ 

/****************************************************************************

1. Add "light_ws2812.c", "light_ws2812.h" and "ws2812_config.h" to your project.
2. Update "ws2812_config.h" according to your I/O pin.
3. Make sure F_CPU is correctly defined in your makefile or the project
   (For AtmelStudio: Project->Properties->Toolchain->AVR/GNU C Compiler->Symbols. Add symbol F_CPU=xxxxx)
4. Call "ws2812_setleds" with a pointer to the LED array and the number LEDs.
5. Alternatively you can use "ws2812_setleds_pin" to control up to 8 LED strips on the same Port.

****************************************************************************/

//Treiber ws_2812 --> https://github.com/cpldcpu/light_ws2812/tree/master/light_ws2812_AVR

#ifndef LIGHT_WS2812_H_
#define LIGHT_WS2812_H_

#include <avr/io.h>
#include <avr/interrupt.h>
//#include "ws2812_config.h"

//#ifndef WS2812_CONFIG_H_
//#define WS2812_CONFIG_H_

///////////////////////////////////////////////////////////////////////
// Define I/O pin
///////////////////////////////////////////////////////////////////////

#define ws2812_port A     // Data port 
#define ws2812_pin  7     // Data out pin

//#endif /* WS2812_CONFIG_H_ */


/*
 *  Structure of the LED array
 */

struct cRGB { uint8_t g; uint8_t r; uint8_t b; };

/* User Interface
 * 
 * Input:
 *         ledarray:           An array of GRB data describing the LED colors
 *         number_of_leds:     The number of LEDs to write
 *         pinmask (optional): Bitmask describing the output bin. e.g. _BV(PB0)
 *
 * The functions will perform the following actions:
 *         - Set the data-out pin as output
 *         - Send out the LED data 
 *         - Wait 50µs to reset the LEDs
 */

void ws2812_setleds    (struct cRGB *ledarray, uint16_t number_of_leds);
void ws2812_setleds_pin(struct cRGB *ledarray, uint16_t number_of_leds,uint8_t pinmask);

/* 
 * Old interface / Internal functions
 *
 * The functions take a byte-array and send to the data output as WS2812 bitstream.
 * The length is the number of bytes to send - three per LED.
 */

void ws2812_sendarray     (uint8_t *array,uint16_t length);
void ws2812_sendarray_mask(uint8_t *array,uint16_t length, uint8_t pinmask);


/*
 * Internal defines
 */

#define CONCAT(a, b)            a ## b
#define CONCAT_EXP(a, b)   CONCAT(a, b)

#define ws2812_PORTREG  CONCAT_EXP(PORT,ws2812_port)
#define ws2812_DDRREG   CONCAT_EXP(DDR,ws2812_port)

#endif /* LIGHT_WS2812_H_ */