/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************


#include "app.h"
#include "ILI9163C.h"
#include <stdio.h>
#include "i2c_master_noint.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

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

char msg[100] ;
char data[14];

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


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

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;

    
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
    
//    // make A4 an output pin
//    TRISAbits.TRISA4 = 0;
//    // set A4 to high
//    LATAbits.LATA4 = 1;
    
    //i2c
    ANSELBbits.ANSB2 = 0;
    ANSELBbits.ANSB3 = 0;  
    i2c_master_setup();                       // init I2C2, which we use as a master

    
    SPI1_init();
    LCD_init();
    LCD_clearScreen(WHITE);
    setupIMU();
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
       
        
            if (appInitialized)
            {
                I2C_read_multiple(SLAVE_ADDR, WHO_AM_I, data, 2);

                sprintf(msg, "WHOAMI: %d", data[0]);
                draw_string(msg,40,30,BLUE, WHITE);
                
                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
//            // blink code
//            _CP0_SET_COUNT(0);
//            
//            LATAbits.LATA4 = 1;
//            
//            while (_CP0_GET_COUNT() < 12000) {
//                
//            }
//            
//            LATAbits.LATA4 = 0;
//            
//            while (_CP0_GET_COUNT() < 24000) {
//                
//            }
            
            _CP0_SET_COUNT(0);
        
            while (_CP0_GET_COUNT() < 480000000/2/5/20) {}
            // code for debugging, should print 105 to the screen if connection working
            I2C_read_multiple(SLAVE_ADDR, WHO_AM_I, data, 2);

            sprintf(msg, "WHOAMI: %d", data[0]);
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

            gyroX = gyroX * 35 ; // 1000 dps sensitivity; mdps/LSB
            gyroY = gyroY * 35 ;
            gyroZ = gyroZ * 35 ;

            accelX = accelX * 0.061  ;// 2g sensitivity ; units are mg
            accelY = accelY * 0.061 ;
            accelZ = accelZ * 0.061 ;

            //sprintf(msg, "T: %3.3f", (float) temperature);
            //draw_string(msg,40,50,BLUE, WHITE);

    //        sprintf(msg, "X: %3.3f", (float)gyroX);
    //        draw_string(msg,40,60,RED, WHITE);
    //        
    //        sprintf(msg, "Y: %3.3f", (float)gyroY);
    //        draw_string(msg,40,70,RED, WHITE);
    //        
    //        sprintf(msg, "Z: %3.3f", (float)gyroZ);
    //        draw_string(msg,40,80,RED, WHITE);
    //        
    //        sprintf(msg, "aX: %3.3f", (float)accelX);
    //        draw_string(msg,40,90,BLACK, WHITE);
    //        
    //        sprintf(msg, "aY: %3.3f", (float)accelY);
    //        draw_string(msg,40,100,BLACK, WHITE);
    //        
    //        sprintf(msg, "aZ: %3.3f", (float)accelZ);
    //        draw_string(msg,40,110,BLACK, WHITE);



            draw_progress_bar(accelX/2.0,50,2,RED,WHITE);

            draw_progress_bar(accelY/2.0,70,2,BLUE,WHITE);
            break;
        }

        /* TODO: implement your application state machine.*/
        

        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

 

/*******************************************************************************
 End of File
 */
