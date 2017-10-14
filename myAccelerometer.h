// ECE 2534:        accelerometer.h
// Purpose:         User-generated accelerometer helper function prototypes.
// Written by:      JST/HZ/LN
// Last modified:   31 March 2017 (by LN)

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#define INT_SOURCE_REGISTER 0x30
#define INACT_BIT_MASK (1<<3)
#define ACT_BIT_MASK (1<<4)
#define DOUBLE_TAP_BIT_MASK (1 << 5)
#define SINGLE_TAP_BIT_MASK (1<<6)

//////////////////////////////////////////////////////////////////////
//Description: writes data to register at provided address for accelerometer
//Pre-conditions: none
//Inputs: SpiChannel, address, data
//Outputs: none
//Post-conditions: writes data to register
void setAccelReg(SpiChannel chn, unsigned int address, unsigned int data);

//////////////////////////////////////////////////////////////////////
//Description: pull information from accelerometer register
//Pre-conditions: None
//Inputs: SpiChannel, address
//Outputs: none
//Post-conditions: none
unsigned char getAccelReg(SpiChannel chn, unsigned int address);

//////////////////////////////////////////////////////////////////////
//Description: pulls data from the registers in the accelerometer that correspond 
//             to the x, y, and z axis values when moving the accelerometer
//             It combines high byte and low byte of each axis to create one short for each axis
//Pre-conditions: None
//Inputs: SpiChannel, xyzArray[]
//Outputs: none
//Post-conditions: data for x, y, z are stored in an array
void getAccelData(SpiChannel chn, short *xyzArray);

//////////////////////////////////////////////////////////////////////
//Description: displays the x, y, z digital values from the orientation of the accelerometer on the Oled
//Pre-conditions: None
//Inputs: Array containing x,y and z readings (this is the same array filled in getAccelData)
//Outputs: None
//Post-conditions: x, y, z value of accelerometer on the Oled
void displayAccelData(short *xyzArray);

//////////////////////////////////////////////////////////////////////
//Description: configures accelerometer by writing to configuration registers
//Pre-conditions: None
//Inputs: SpiChannel
//Outputs: none
//Post-conditions: accelerometer is setup to see x, y, z orientation and be able to use double tap
void initAccelerometer(SpiChannel chn);

//////////////////////////////////////////////////////////////////////
//Description: initializes SPI to be used as a master for PMOD ACL
//Pre-conditions: None
//Inputs: None
//Outputs: none
//Post-conditions: SPI can now be used with the accelerometer
void initAccMasterSPI(SpiChannel chn);

#endif
