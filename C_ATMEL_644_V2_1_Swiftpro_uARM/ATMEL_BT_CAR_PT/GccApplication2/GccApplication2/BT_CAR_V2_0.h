typedef unsigned char			 u8;
typedef unsigned int			u16;
typedef unsigned long int u32;

extern u32 tick_1ms;
extern u32 tick_1s;

// Das sind die Funktionen, die genutzt werden können.
extern void init_BT_CAR_V2_0(void);
extern void LED_Show(void);
extern int abs();

extern void lcd_init(void);
extern void clear_lcd(void);
extern void write_zahl(u8 x_pos, u8 y_pos, u32 zahl_v, u8 s_vk, u8 s_nk, u8 komma);
extern void write_text(u8 y_pos, u8 x_pos, const char* str_ptr);
extern void write_text_ram(u8 y_pos, u8 x_pos, const char* str_ptr);
extern void wait_1ms(u32 delay);
extern void delay_nop(u8 time);

// Port-Definitionen
//#define LED_1_ON						(PORTB |=  0x01)	//PBA.0: LED_1 = EIN
