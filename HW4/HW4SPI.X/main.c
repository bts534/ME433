
#include <proc/p32mx250f128b.h>
#include <math.h>
#include "PIC32Setup.h"
#define CS LATAbits.LATA4

float sineWaveform[200];
float rampWaveform[200];

unsigned char spi_io(unsigned short o) {
    SPI1BUF = o; // write 
    while (!SPI1STATbits.SPIRBF) {
        // wait until byte is received
    }
    return SPI1BUF; // read to prevent buffer overflow - read does not equal write
}


// Notes from the datasheet
// bit 15: 1/0 write to DACB / DACA
//bit 14 BUF: VREF Input Buffer Control bit
//1 = Buffered
//0 = Unbuffered
//bit 13 GA: Output Gain Selection bit
//1 = 1x (VOUT = VREF * D/4096)
//0 = 2x (VOUT = 2 * VREF * D/4096)
//bit 12 SHDN: Output Shutdown Control bit
//1 = Active mode operation. VOUT is available.
//0 = Shutdown the selected DAC channel. Analog output is not available at the channel that was shut down.
// VOUT pin is connected to 500 k???typical)?
// bits 4 through 11 contain the 8 bits of information
// bits 0 through 3 are not used for 4902
void setVoltage(char channel, float voltage) {
    CS = 0; // set the chip select pin, RA0, to low
    int voltage2 = voltage;
    unsigned short out = (channel << 15) | 0x7000 | (voltage2 << 4);
    
    spi_io(out);
    
    CS = 1; // stop the write
}
// Data Sheet Notes:
// All writes to the MCP4902/4912/4922 are 16-bit words.
//Any clocks past the 16th clock will be ignored. The
//Most Significant 4 bits are Configuration bits. The
//remaining 12 bits are data bits. 
void spi_setup() {
    // make A4 an output pin for chip select
    TRISAbits.TRISA4 = 0;       // acts as ss1
    CS = 1;                     // we arent writing right now
    // select the pins for spi communication
    SS1Rbits.SS1R = 0b0011;     //  not used, just for practice
    SDI1Rbits.SDI1R = 0b0111;   // not used, just for practice
    RPA1Rbits.RPA1R = 0b0011;   // set output rpa1 (pin3) to sdo1
    SPI1CON = 0;
    SPI1BUF;                    // clear the SPI buffer by reading from it
    SPI1BRG = 0x1000;           // baud rate. set to 0x1 for fastests
    SPI1STATbits.SPIROV = 0;    // clear the overflow bit
    SPI1CONbits.MODE32 = 0;
    SPI1CONbits.MODE16 = 1;     // we'll need to send 16 bit words which contain 4 bits of config, and 8 bits of data, 4 bits unused
    SPI1CONbits.CKE = 1;   
    SPI1CONbits.MSTEN = 1;      // master operation
    SPI1CONbits.ON = 1;         // turn on SPI1
    
}

void makeWaveforms() {
    int i=0;
    for (i=0; i < 200; i ++) {
        sineWaveform[i] = 255.0/2.0*sin(2*3.14159*i/200) +255.0/2.0;
        rampWaveform[i] = 255.0/200.0*i;
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

// set up SPI
spi_setup();

// make waveform arrays
makeWaveforms();

int count =0;
    while(1) {
        _CP0_SET_COUNT(0);
        
        // 1 ms delay, sets the frequency of the sine wave
        while(_CP0_GET_COUNT() < 24000) {
            
        }
        
        setVoltage(0,sineWaveform[count]);
        setVoltage(1,rampWaveform[count]);
        
        count ++;
        
        if (count==199){
            count = 0;
        }

    }
    return 0;
}
