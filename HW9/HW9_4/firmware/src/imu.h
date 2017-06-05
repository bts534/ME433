/* 
 * File:   imu.h
 * Author: briansoetikno
 *
 * Created on June 5, 2017, 11:20 AM
 */

#ifndef IMU_H
#define	IMU_H

#ifdef	__cplusplus
extern "C" {
#endif

    
extern short gyroX;
extern short gyroY;
extern short gyroZ;
extern short accelX;
extern short accelY;
extern short accelZ;

void setupIMU();
char read_register(unsigned char register_addr) ;
void I2C_read_multiple(unsigned char address, unsigned char register_addr, unsigned char* data, int length) ;
void readIMUdata();



#ifdef	__cplusplus
}
#endif

#endif	/* IMU_H */

