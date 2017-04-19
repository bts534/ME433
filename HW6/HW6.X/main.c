#include <proc/p32mx250f128b.h>
#include "PIC32Setup.h"
//#include "i2c_slave.h"
#include "ILI9163C.h"
#include <stdio.h>

#define SLAVE_ADDR 0b0100000
#define IODIR 0x00
#define GPIO 0x09

//char len = sprintf(msg, 'Hello World!');

void draw_ascii(char c, unsigned short x, unsigned short y, unsigned short fgColor, unsigned short bgColor) {
    char d = c - 0x20;
    int col = 0;
    int row = 0;
    for (col = 0; col < 5; col++) {
        if (x + col < 128) { // bounds check
            char c = 0x01;
            for (row = 0; row < 8; row++) {
                if (y+row < 128) {
                    //            LCD_drawPixel(x+col,y+row,0xffff);
                    if ((ASCII[d][col] >> row) & 1) {
                        LCD_drawPixel(x + col, y + row, fgColor);
                    } else {
                        LCD_drawPixel(x + col, y + row, bgColor);
                    }
                }
            }
        } 
    }
}

void draw_string(char* msg, unsigned short x, unsigned short y, unsigned short fgColor, unsigned short bgColor)
{
     // can put some black spaces after hello to erase stuff

    // loop through every element of msg
    int i = 0;
    while (msg[i] != 0) {// sprintf leaves a null character
        draw_ascii(msg[i], x+5*i+1, y, fgColor, bgColor);
        i++;
    }
}

void draw_progress_bar(signed short percentage, unsigned short y, unsigned short height, unsigned short fgColor, unsigned short bgColor)
{
    float startPos = percentage * 128.0/100.0/2.0 +64;
    float endPos;
    endPos = (percentage <0) ? 64 : startPos;
    startPos = (percentage<0) ? startPos : 64;
    
    
    int col = 64;
    int row = y;
    for (col = 0; col < 128; col ++) {
        
       for (row = y; row < y + height; row ++) {
            if (col >= startPos && col <= endPos) {
                LCD_drawPixel(col, row, fgColor);
            } else {
                LCD_drawPixel(col, row, bgColor);
            }
        }
    }
}

int main() {

__builtin_disable_interrupts();

// set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
__builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

// 0 data RAM access wait states
BMXCONbits.BMXWSDRM = 0x0;

// enable multi vector interrupts
INTCONbits.MVEC = 0x1;

// disable JTAG to get pins back
DDPCONbits.JTAGEN = 0;

// do your TRIS and LAT commands here

__builtin_enable_interrupts();

// turn off analog outputs
SPI1_init();
LCD_init();
LCD_clearScreen(WHITE);

char msg[100] ;

int count =-100;
    while(1) {
        sprintf(msg, "Hello world!");
        draw_string(msg, 32,64,BLACK,WHITE);
        
        
        draw_progress_bar(count, 42, 8, RED, WHITE);
        
        count ++;
        
        if (count > 100) {
            count = -100;
        }
    }
    return 0;
}
