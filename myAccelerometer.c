#include <plib.h>
#include "myAccelerometer.h"

#define ACC_MASTER_SPI_CLK_DIVIDER 10

#define ID_REGISTER 0x00
#define DATA_FORMAT_REG 0x31

//4 mg per LSB
#define FULL_RES_FLAG (1<<3) 

// the number read out of XYZ bytes should be multiplied 
// by this number to get the "g" data
// if full resolution is not used, this number changes
#define RESOLUTION 4 

#define DECI 0.1

// +/- 8g
#define RANGE_8g 2 

#define POWER_CNTL_REG 0X2D
#define MEASURE_ON (1<<3)

#define OUTPUT_RATE_REG 0X2C
#define OUTPUT_RATE_3_13 0x6
   
#define READ_COMMAND 0X80
#define WRITE_COMMAND 0X0
#define SINGLE_BYTE 0X0
#define MULTI_BYTE 0X40
#define DATA_START_REG 0x32

#define DATA_REG_COUNT 6
#define BYTE_BITS 8

// LSB: 625 micro second
// 8 * 625 micro = 5 milli seconds
#define TAP_DURATION 0x8   
#define TAP_DURATION_REGISTER 0x21 

// LSB: 1.25 milli second
// The length of the window to look for a second tap
// 0xFF * 1.25 = 320 milli second
#define TAP_WINDOW 0xFF  
#define TAP_WINDOW_REGISTER 0x23

// LSB: 62.5 mg
// Our choice = 2 g 
#define TAP_THRESHOLD_REGISTER 0x1D 
#define TAP_THRESHOLD 0x20

#define TAP_AXES_REGISTER 0x2A
#define TAP_XYZ_EN_BIT_MASK 7


#define INT_ENABLE_REGISTER 0x2E 
#define INT_MAP_REGISTER 0x2F 




void setAccelReg(SpiChannel chn, unsigned int address, unsigned int data) //data corresponds to the sensitivity
{
    SpiChnPutC(chn, WRITE_COMMAND | address);
    SpiChnPutC(chn, data);
    SpiChnGetC(chn); //get rid of garbage
    SpiChnGetC(chn);
}

unsigned char getAccelReg(SpiChannel chn, unsigned int address)
{
    SpiChnPutC(chn, READ_COMMAND | address);
    SpiChnPutC(chn, 0); //sending to receive
    SpiChnGetC(chn); //getting rid of garbage
    char data = SpiChnGetC(chn); //grabbing what we actually want
    return data;
}

void getAccelData(SpiChannel chn, short *xyzArray) //gets the accelerometer data and stores in an array
{
    unsigned char dataArray[6];
    int i;
    
    SpiChnPutC(chn, READ_COMMAND | MULTI_BYTE | DATA_START_REG); 
    
    //sending 6 because need 6 values
    for (i=0; i < DATA_REG_COUNT; i++)
        SpiChnPutC(chn, 0);
    
    SpiChnGetC(chn); //getting rid of the garbage
    
    for(i = 0; i < DATA_REG_COUNT; i++) //storing all 6 of values for x, y, z; there are two for each
        dataArray[i] = SpiChnGetC(chn);
    
    xyzArray[0] = (dataArray[1] << BYTE_BITS | dataArray[0]); //shifting to put MSB in front
    xyzArray[1] = (dataArray[3] << BYTE_BITS | dataArray[2]);
    xyzArray[2] = (dataArray[5] << BYTE_BITS | dataArray[4]);
}

void displayAccelData(short *xyzArray) //used to display axis data for the accelerometer
{
    char buf[17];
    OledSetCursor(0,0);
    int x = xyzArray[0] * RESOLUTION * DECI;
    if (x)
        sprintf(buf, "X-Axis: %+2d    ", x); //display x-axis digital read from accelerometer
    else 
        sprintf(buf, "X-Axis: 0       "); //display x-axis digital read from accelerometer
    OledPutString(buf);
    
    
    OledSetCursor(0,1);
    int y = xyzArray[1] * RESOLUTION * DECI;
    if (y)
        sprintf(buf, "Y-Axis: %+2d    ", y); //display y-axis digital read from accelerometer
    else
        sprintf(buf, "Y-Axis: 0       "); //display y-axis digital read from accelerometer
    OledPutString(buf);
    
    
    OledSetCursor(0,2);
    int z = xyzArray[2] * RESOLUTION * DECI;
    if (z)
        sprintf(buf, "Z-Axis: %+2d    ", z); //display z-axis digital read from accelerometer
    else
        sprintf(buf, "Z-Axis: 0       "); //display z-axis digital read from accelerometer
    OledPutString(buf);
}   

void initAccelerometer(SpiChannel chn)
{
    setAccelReg(chn, POWER_CNTL_REG, MEASURE_ON);
    setAccelReg(chn, DATA_FORMAT_REG, FULL_RES_FLAG | RANGE_8g);
    setAccelReg(chn, OUTPUT_RATE_REG, OUTPUT_RATE_3_13);
    
    setAccelReg(chn, TAP_AXES_REGISTER, TAP_XYZ_EN_BIT_MASK);
    setAccelReg(chn, TAP_DURATION_REGISTER, TAP_DURATION);
    setAccelReg(chn, TAP_THRESHOLD_REGISTER, TAP_THRESHOLD);
    setAccelReg(chn, TAP_WINDOW_REGISTER, TAP_WINDOW);
    
    
    BYTE EnabledInterrupts = SINGLE_TAP_BIT_MASK;
    
    setAccelReg(chn, INT_MAP_REGISTER, EnabledInterrupts);
    setAccelReg(chn, INT_ENABLE_REGISTER, EnabledInterrupts);

}

void initAccMasterSPI(SpiChannel chn)
{
    SpiChnOpen(chn, SPI_OPEN_MSTEN 
                    | SPI_OPEN_MSSEN 
                    | SPI_OPEN_CKP_HIGH 
                    | SPI_OPEN_ENHBUF 
                    | SPI_OPEN_MODE8, ACC_MASTER_SPI_CLK_DIVIDER);
}

