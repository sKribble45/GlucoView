
#ifndef __EPD_2in13_V4_H_
#define __EPD_2in13_V4_H_

#include "DEV_Config.h"


// Display resolution
#define EPD_2in13_V4_WIDTH       122
#define EPD_2in13_V4_HEIGHT      250

void EPD_2in13_V4_Init(void);
void EPD_2in13_V4_Init_Fast(void);
void EPD_2in13_V4_Init_GUI(void);
void EPD_2in13_V4_Clear(void);
void EPD_2in13_V4_Clear_Black(void);
void EPD_2in13_V4_Display(UBYTE *Image);
void EPD_2in13_V4_Display_Fast(UBYTE *Image);
void EPD_2in13_V4_Display_Base(UBYTE *Image);
void EPD_2in13_V4_Display_Mem(UBYTE *Image);
void EPD_2in13_V4_Display_Partial(UBYTE *Image);
void EPD_2in13_V4_Sleep(void);


#endif
