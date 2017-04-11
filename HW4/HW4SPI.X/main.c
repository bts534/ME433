
#include "PIC32Setup.h"

unsigned char spi_io(unsigned char o) {
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
    LATBbits.LATB8 = 0; // set the chip select pin, RA0, to low
    int voltage2 = voltage;
    short out = (channel << 15) | 0x7000 | (voltage2 << 4);
    
    spi_io(out);
    
    LATBbits.LATB8 = 1; // stop the write
}
// Data Sheet Notes:
// All writes to the MCP4902/4912/4922 are 16-bit words.
//Any clocks past the 16th clock will be ignored. The
//Most Significant 4 bits are Configuration bits. The
//remaining 12 bits are data bits. 
void spi_setup() {
    // make A0 an output pin for chip select
    TRISBbits.TRISB8 = 0; // acts as ss1
    LATBbits.LATB8 = 1;     // we arent writing right now
    // select the pins for spi communication
    SS1Rbits.SS1R = 0b0011;  //  not used
    SDI1Rbits.SDI1R = 0b0111; // not used
    RPA1Rbits.RPA1R = 0b0011; // set output rpa1 (pin3) to sdo1
    SPI1CON = 0;
    SPI1BUF;                // clear the SPI buffer by reading from it
    SPI1BRG = 0x1;          // baud rate to 12 MHz 
    SPI1STATbits.SPIROV = 0; // clear the overflow bit
    SPI1CONbits.MODE32 = 0;
    SPI1CONbits.MODE16 = 1; // we'll need to send 16 bit words which contain 4 bits of config, and 8 bits of data, 4 bits unused
    SPI1CONbits.CKE = 1;   
    SPI1CONbits.MSTEN = 1;  // master operation
    SPI1CONbits.ON = 1;     // turn on SPI1
    
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

    while(1) {
        if (_CP0_GET_COUNT() >= 24000) {
            _CP0_SET_COUNT(0);
            setVoltage(0,1);
            setVoltage(1,2);
        }
    // use _CP0_SET_COUNT(0) and _CP0_GET_COUNT() to test the PIC timing

    // remember the core timer runs at half the sysclk

    }
    return 0;
}
