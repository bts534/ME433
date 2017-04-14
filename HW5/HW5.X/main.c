
#include <proc/p32mx250f128b.h>
#include "PIC32Setup.h"
//#include "i2c_slave.h"
#include "i2c_master_noint.h"

#define SLAVE_ADDR 0b0100000
#define IODIR 0x00
#define GPIO 0x09

void setupExpander() {
    i2c_master_start();
    i2c_master_send(SLAVE_ADDR << 1); // indicate a write, send slave address
    i2c_master_send(IODIR);
    i2c_master_send(0b11110000); // set  first 4 as outputs and last 4 as inputs
    i2c_master_stop();
    
    //i2c_master_start();
    //i2c_master_send(SLAVE_ADDR << 1); // indicate a write, send slave address
    //i2c_master_send(GPIO);
    //i2c_master_send(0b00000001); // turn on pin 0
    //i2c_master_stop();
}

char readExpander() {
    i2c_master_start();
    i2c_master_send(SLAVE_ADDR << 1 | 0); // indicate a write
    i2c_master_send(GPIO);
    
    i2c_master_restart();
    i2c_master_send((SLAVE_ADDR << 1) | 1); // send slave address, left shifted by 1,
                                            // and then a 1 in lsb, indicating read
    char master_read = i2c_master_recv();       // receive a byte from the bus
    i2c_master_ack(1);                      // send NACK (1):  master needs no more bytes
    i2c_master_stop();   
    return master_read;
}

void setExpander(char pin, char level) {
    char io_current = readExpander();
    char io_new;
    if (level == 1) {
        io_new = io_current | (1 << pin); // set the pin to the new value
    } else if (level == 0) {
        io_new = io_current & ~(1 << pin); // set the pin to the new value        
    }
            
            
    i2c_master_start();
    i2c_master_send(SLAVE_ADDR << 1); // indicate a write, send slave address
    i2c_master_send(GPIO);
    i2c_master_send(io_new); // turn on the new pin
    i2c_master_stop();
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

//i2c_slave_setup(SLAVE_ADDR);              // init I2C5, which we use as a slave 
ANSELBbits.ANSB2 = 0;
ANSELBbits.ANSB3 = 0;                             
//  (comment out if slave is on another pic)
i2c_master_setup();                       // init I2C2, which we use as a master
  
__builtin_enable_interrupts();

// turn off analog outputs


unsigned char master_write0 = 0xCD;       // first byte that master writes
unsigned char master_write1 = 0x91;       // second byte that master writes
unsigned char master_read0  = 0x00;       // first received byte
unsigned char master_read1  = 0x00;       // second received byte


setupExpander();
  
// make waveform arrays
int count =0;
    while(1) {
        setExpander(0,readExpander()>>7);
        //_CP0_SET_COUNT(0);
        
        //while (_CP0_GET_COUNT() < 480000000/2) {
        //if (( != 1) {
         //   setExpander(0,1);
        //}
        //}
        
        //while (_CP0_GET_COUNT() < 480000000) {
        //    setExpander(0,0);
        //}
    }
    return 0;
}
