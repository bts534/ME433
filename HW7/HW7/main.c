#include <proc/p32mx250f128b.h>
#include "PIC32Setup.h"
//#include "i2c_slave.h"
#include "ILI9163C.h"
#include <stdio.h>
#include "i2c_master_noint.h"

#define SLAVE_ADDR 0b1101010
#define CTRL1_XL 0x10
#define CTRL2_G 0x11
#define CTRL3_C 0x12
#define OUT_TEMP_L 0x20
#define OUTZ_H_XL 0x2D
#define WHO_AM_I 0x0F

short temperature;
short gyroX;
short gyroY;
short gyroZ;
short accelX;
short accelY;
short accelZ;

//char len = sprintf(msg, 'Hello World!');
void setupIMU() {
    i2c_master_start();
    i2c_master_send(SLAVE_ADDR << 1); // slave address + write bit
    i2c_master_send(CTRL1_XL);
    i2c_master_send(0b10000010); 
    i2c_master_stop();
    
    i2c_master_start();
    i2c_master_send(SLAVE_ADDR << 1);
    i2c_master_send(CTRL2_G);
    i2c_master_send(0b10001000);
    i2c_master_stop();
    
    i2c_master_start();
    i2c_master_send(SLAVE_ADDR << 1);
    i2c_master_send(CTRL3_C);
    i2c_master_send(0b00000100);
    i2c_master_stop();
    
    
    //i2c_master_start();
    //i2c_master_send(SLAVE_ADDR << 1); // indicate a write, send slave address
    //i2c_master_send(GPIO);
    //i2c_master_send(0b00000001); // turn on pin 0
    //i2c_master_stop();
}

char read_register(unsigned char register_addr) {
    i2c_master_start();
    i2c_master_send(SLAVE_ADDR << 1 | 0); // indicate a write
    i2c_master_send(register_addr);
    
    i2c_master_restart();
    i2c_master_send((SLAVE_ADDR << 1) | 1); // send slave address, left shifted by 1,
                                            // and then a 1 in lsb, indicating read
    char master_read = i2c_master_recv();       // receive a byte from the bus
    i2c_master_ack(1);                      // send NACK (1):  master needs no more bytes
    i2c_master_stop();   
    return master_read;
}

void I2C_read_multiple(unsigned char address, unsigned char register_addr, unsigned char* data, int length) {
    
    i2c_master_start();
    i2c_master_send(address << 1); // indicate write
    i2c_master_send(register_addr);
    
    i2c_master_restart();
    i2c_master_send((address << 1) | 1); // send slave address, left shifted by 1,
                                            // and then a 1 in lsb, indicating read
    
    int i =0;
    for (i=0; i < length; i ++) {
    
        data[i] = i2c_master_recv();       // receive a byte from the bus
        if (i == length-1) {
            i2c_master_ack(1); 
        } else {
            i2c_master_ack(0); // let the chip know we want to keep reading  
        }
            
    }
    
    //i2c_master_ack(1);   // send NACK (1):  master needs no more bytes
    i2c_master_stop();
}

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

//i2c
ANSELBbits.ANSB2 = 0;
ANSELBbits.ANSB3 = 0;  
i2c_master_setup();                       // init I2C2, which we use as a master


// do your TRIS and LAT commands here

__builtin_enable_interrupts();

// turn off analog outputs
SPI1_init();
LCD_init();
LCD_clearScreen(WHITE);

setupIMU();

char msg[100] ;
char data[14];
int count =-100;
    while(1) {
        _CP0_SET_COUNT(0);
        
        while (_CP0_GET_COUNT() < 480000000/2/5) {}
        // code for debugging, should print 105 to the screen if connection working
        I2C_read_multiple(SLAVE_ADDR, WHO_AM_I, data, 2);

        sprintf(msg, "T: %d", data[0]);
        draw_string(msg,40,30,BLUE, WHITE);

        I2C_read_multiple(SLAVE_ADDR, OUT_TEMP_L, data, 14);

        // convert data to shorts
        temperature = (data[1] << 8) | data[0];
        gyroX = (data[3] << 8) | data[2];
        gyroY = (data[5] << 8) | data[4];
        gyroZ = (data[7] << 8) | data[6];
        accelX = (data[9] << 8) | data[8];
        accelY = (data[11] << 8) | data[10];
        accelZ = (data[13] << 8) | data[12];

        sprintf(msg, "T: %3.3f", (float) temperature);
        draw_string(msg,40,50,BLUE, WHITE);
        
        sprintf(msg, "X: %d", gyroX);
        draw_string(msg,40,60,RED, WHITE);
        
        sprintf(msg, "Y: %d", gyroY);
        draw_string(msg,40,70,RED, WHITE);
        
        sprintf(msg, "Z: %d", gyroZ);
        draw_string(msg,40,80,RED, WHITE);
        
        sprintf(msg, "aX: %d", accelX);
        draw_string(msg,40,90,BLACK, WHITE);
        

        
        //draw_progress_bar(temperature,50,2,RED,WHITE);
    }
    return 0;
}
