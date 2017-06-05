#include "imu.h"
#include "i2c_master_noint.h"

// IMU stuff
#define SLAVE_ADDR 0b1101010
#define CTRL1_XL 0x10
#define CTRL2_G 0x11
#define CTRL3_C 0x12
#define OUT_TEMP_L 0x20
#define OUTZ_H_XL 0x2D
#define WHO_AM_I 0x0F

short temperature;
short gyroX=0;
short gyroY=0;
short gyroZ=0;
short accelX=0;
short accelY=0;
short accelZ=0;

char msg[100] ;
char data[14];

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

void readIMUdata()
{
    I2C_read_multiple(SLAVE_ADDR, WHO_AM_I, data, 2);

    //sprintf(msg, "WHOAMI: %d", data[0]);
    //draw_string(msg,40,30,BLUE, WHITE);

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
}