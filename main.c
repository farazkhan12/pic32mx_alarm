// Faraz Khan
// Lab 4
// shake up and down for ~5 seconds

#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include <stdio.h>
#include <stdarg.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include "myBoardConfigFall2016.h"
#include "myAccelerometer.h"

// Defines
#define BUTTON1 (1 << 6)
#define BUTTON1BIT 6
#define BUTTON2 (1 << 7)
#define BUTTON2BIT 7
#define BUTTON3 (1 << 0)
#define BUTTON3BIT 0
#define LEDS    (0xF << 12)
#define LED1 (1<<12)
#define LED2 (1<<13)
#define OLED_SIZE 64
#define DEBOUNCE_WINDOW 10
#define INT1_PORT_MASK_BIT 4  // put a real value

unsigned int Button1Debounce(); 
unsigned int Button2Debounce(); 
unsigned int Button3Debounce(); 
unsigned int Button1RisingEdge(); 
unsigned int Button2RisingEdge(); 
unsigned int Button3RisingEdge(); 

volatile unsigned int timer2_ms_value = 0;

void initAll();
void initExternalInterrupt();
void processAccInt();
void OledPutFormattedStringAt
            (int X, int Y, int n, char *buf, const char *format, ...);

// Global variables
const SpiChannel ch = SPI_CHANNEL3; //going to be reading from this channel, will never change so set to const
//volatile BYTE AccINTSources = 0;
volatile unsigned int TapCount = 0;

enum tState {STABLE0, TRANS0to1, STABLE1, TRANS1to0}; 
int milliseconds = 0;
int seconds = 0;



void initExternalInterrupt()
{
    
    // Configure the external interrupt pin as digital input  
    // This is the pin INT1 of PIC32 that is connected to INT2 of ADXL345
    TRISESET = INT1_PORT_MASK_BIT;
    
    INTSetVectorPriority(INT_EXTERNAL_1_VECTOR, EXT_INT_PRI_4);
    
    //configure interrupt as rising edge
    //mINT1SetEdgeMode(1); //this does not work for everyone
    INTCONCLR = (1 << _INTCON_INT1EP_POSITION); 
    INTCONSET = ((1) << _INTCON_INT1EP_POSITION);
    
    INTClearFlag(INT_INT1);
    INTEnable(INT_INT1, INT_ENABLED);
    
    //clear the possible pending interrupt on the ADXL345 side
    INTClearFlag(INT_INT1);
}
// The interrupt handler for timer2
// IPL4 medium interrupt priority
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) {
    timer2_ms_value++; // Increment the millisecond counter.
    INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
    milliseconds++;
    seconds = (milliseconds)/1000;
    
}

void initTimer2() {
    // Configure Timer 2 to request a real-time interrupt once per millisecond.
    // The period of Timer 2 is (16 * 625)/(10 MHz) = 1ms.
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624);
    
    // Setup Timer 2 interrupts
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);
}
//external interrupt for the accelerometer 
void __ISR(_EXTERNAL_1_VECTOR, IPL4AUTO) _EXTERNAL1HANDLER(void) 
{ 
    BYTE AccINTSources = getAccelReg(ch, INT_SOURCE_REGISTER);
    
    if (AccINTSources & SINGLE_TAP_BIT_MASK)
    {
        PORTGINV = LED1;
        TapCount++;
    }
    
    
    INTClearFlag(INT_INT1);
}

void initAll() //initialize timers, interrupts, accelerometer, SPI and configures interrupt
{
    TRISGCLR  = LEDS;
    ODCGCLR = LEDS;
    PORTGCLR = LEDS;
    //TRISGSET = BUTTON1 | BUTTON2; // Inputs
    DDPCONbits.JTAGEN = 0; //used to disable jtag controller that is enable by default
    TRISASET = BUTTON3;
    
    // Initialize Timer1 and OLED for display
    DelayInit();
    OledInit();
    OledClearBuffer();

    initINTController();
    
    //initialize stuff for the accelerometer
    initAccMasterSPI(ch);
    initAccelerometer(ch);
    initExternalInterrupt();    
    
    initTimer2();
}

int main()
{    
	initAll();
    short xyzArray[3];
    char buf[OLED_SIZE];
    
    
    //state machine
    enum {BASE, ALARM, CURRENT} state = BASE;
    
                //day of week index clock
                int index = 0; 
                
                //day of week index alarm
                int indexa = 0;
                const char *adw[7];
                adw[0] = "SU";
                adw[1] = "MO";
                adw[2] = "TU";
                adw[3] = "WE";
                adw[4] = "TH";
                adw[5] = "FR";
                adw[6] = "SA";
                int alarm_day = 1;
                int alarm_month = 1;
                int alarm_hour = 0;
                int alarm_min = 0;
                int alarm_sec = 0;

                const char *cdw[7];
                cdw[0] = "SU";
                cdw[1] = "MO";
                cdw[2] = "TU";
                cdw[3] = "WE";
                cdw[4] = "TH";
                cdw[5] = "FR";
                cdw[6] = "SA";

                int clock_day = 1;
                int clock_month = 1;
                int clock_hour = 0;
                int clock_min = 0;
                int clock_sec = 0;
                char message[20];
                timer2_ms_value = 0;
                int timesPress = 0;
                seconds = 0;
                int alarmState = 0;
                int activate = 0;
                int alarmy = 0;
   
    while(1)
	{
        
        
        switch(state){
            
            case BASE: 
                

                //variables for alarm and clock hour/min/sec & day/month/year

                OledSetCursor(0, 0);
                sprintf(message, "ALARM  %s %02d/%02d", adw[indexa], alarm_month, alarm_day);
                OledPutString(message);
                OledSetCursor(0, 1);
                if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                }
                if (alarmState == 1)
                {
                    sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                } 
                OledSetCursor(0, 2);
                sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                OledPutString(message);
                sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                OledSetCursor(0, 3);
                OledPutString(message);
                


                while (timer2_ms_value < 1000){
                    
                }
                clock_sec++;
                timer2_ms_value = 0;

                if (clock_sec > 59) {
                    clock_min++;

                    clock_sec = 0;
                    seconds = 0;
                }
                if (clock_min > 59 && clock_sec > 59) {
                    clock_hour++;
                    clock_min = 0;
                    clock_sec = 0;
                    seconds = 0;
                }
                if (clock_hour > 23 && clock_min > 59 && clock_sec > 59){
                    clock_hour = 0;
                    clock_min = 0;
                    clock_sec = 0;
                    seconds = 0;
                }
                
                
                
                
                //if buttons are pressed change states..
                if (Button1RisingEdge()){
                    state = CURRENT;
                    
                }
                if (Button2RisingEdge()){
                    state = ALARM;
                    OledClearBuffer();
                    seconds = clock_sec;
                    milliseconds = clock_sec*1000;
                }
                if (PORTA & BUTTON3){
                    if (alarmState == 1){
                        alarmy++;
                        if ( alarmy%2 == 1 ){
                            PORTGSET = LED1;
                            alarmState = 0;
                            OledClearBuffer();
                            
                        }
                        
                    }
                    else if (alarmState == 0){
                        alarmy++;
                        if( alarmy%2 == 0){
                            PORTGINV = LED1;
                            alarmState = 1;
                            OledClearBuffer();
                        }
                        
                    }
                    
                }
                if ( (adw[indexa] == cdw[index]) && (alarm_month == clock_month) && (alarm_day == clock_day) && (alarm_hour == clock_hour) && (alarm_min == clock_min) && (alarm_sec == clock_sec)){
                    activate = 1;
                    milliseconds = 0;
                    if (alarmState == 1){                       
                        PORTGSET = LEDS;
                        while (milliseconds < 6200){
                            
                            }
                        if (activate){
                                PORTGINV = LEDS;
                                state = BASE;
                                alarmState =0;
                                break;
                        }
                    }
                    
                }
                
                break;
            case ALARM:
                
                while(timesPress == 0){
                                    OledSetCursor(0, 0);
                sprintf(message, "ALARM  %s %02d/%02d", adw[indexa], alarm_month, alarm_day);
                OledPutString(message);
                OledSetCursor(0, 1);
                if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                }
                if (alarmState == 1)
                {
                    sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                } 
                    OledSetCursor(0, 2);
                    sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                    OledPutString(message);
                    sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                    OledSetCursor(0, 3);
                    OledPutString(message);

                    clock_sec = seconds;
                    if (clock_sec > 59) {
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                        clock_min++;
                    }
                    if (clock_min > 59 && clock_sec > 59) {
                        clock_hour++;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    if (clock_hour > 23 && clock_min > 59 && clock_sec > 59) {
                        clock_hour = 0;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    
                    //increment
                    if (Button1RisingEdge()) {
                        
                        indexa++;
                        if (indexa == 7){
                            indexa = 0;
                        }
                        OledSetCursor(0, 0);
                        sprintf(message, "ALARM  %s %02d/%02d", adw[index], alarm_month, alarm_day);
                        OledPutString(message);

                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        
                        indexa--;
                        if (indexa == -1){
                            indexa = 6;
                        }
                        OledSetCursor(0, 0);
                        sprintf(message, "ALARM  %s %02d/%02d", adw[index], alarm_month, alarm_day);
                        OledPutString(message);
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress++;
                        break;
                    }
                }
                while (timesPress == 1){
                OledSetCursor(0, 0);
                sprintf(message, "ALARM  %s %02d/%02d", adw[indexa], alarm_month, alarm_day);
                OledPutString(message);
                OledSetCursor(0, 1);
                if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                }
                if (alarmState == 1)
                {
                    sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                } 
                    OledSetCursor(0, 2);
                    sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                    OledPutString(message);
                    sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                    OledSetCursor(0, 3);
                    OledPutString(message);

                    clock_sec = seconds;
                    if (clock_sec > 59) {
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                        clock_min++;
                    }
                    if (clock_min > 59 && clock_sec > 59) {
                        clock_hour++;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    if (clock_hour > 23 && clock_min > 59 && clock_sec > 59) {
                        clock_hour = 0;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    //increment
                    if (Button1RisingEdge()) {
                        alarm_month++;
                        if (alarm_month == 13){
                            alarm_month = 1;
                        }
                        OledSetCursor(0, 0);
                    sprintf(message, "ALARM  %s %02d/%02d", adw[index], alarm_month, alarm_day);
                    OledPutString(message);
                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        alarm_month--;
                        if (alarm_month == 0){
                            alarm_month = 12;                           
                        }
                        OledSetCursor(0, 0);
                    sprintf(message, "ALARM  %s %02d/%02d", adw[index], alarm_month, alarm_day);
                    OledPutString(message);
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress++;
                        break;
                    }
                    
                }
                while (timesPress == 2){
                                    OledSetCursor(0, 0);
                sprintf(message, "ALARM  %s %02d/%02d", adw[indexa], alarm_month, alarm_day);
                OledPutString(message);
                OledSetCursor(0, 1);
                if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                }
                if (alarmState == 1)
                {
                    sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                } 
                    OledSetCursor(0, 2);
                    sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                    OledPutString(message);
                    sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                    OledSetCursor(0, 3);
                    OledPutString(message);

                    clock_sec = seconds;
                    if (clock_sec > 59) {
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                        clock_min++;
                    }
                    if (clock_min > 59 && clock_sec > 59) {
                        clock_hour++;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    if (clock_hour > 23 && clock_min > 59 && clock_sec > 59) {
                        clock_hour = 0;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    if (Button1RisingEdge()) {
                        alarm_day++;
                        if ( alarm_month == 4 || alarm_month == 6 || alarm_month ==9 || alarm_month ==11 )
                        {
                            if (alarm_day >= 31){
                                alarm_day = 0;
                            }
                        }
                        else if ( alarm_month == 2)
                        {
                            if (alarm_day >= 29){
                                alarm_day = 0;
                            }
                        }
                        else
                        {
                            if (alarm_day >= 32){
                                alarm_day = 0;
                            } 
                        }
                        OledSetCursor(0, 0);
                    sprintf(message, "ALARM  %s %02d/%02d", adw[index], alarm_month, alarm_day);
                    OledPutString(message);
                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        alarm_day--;
                        if (alarm_day <= 0){
                            if ( alarm_month == 4 || alarm_month == 6 || alarm_month ==9 || alarm_month ==11){
                                alarm_day = 30;
                            }
                            else if ( alarm_month == 2){
                                alarm_day = 28;
                            }
                            else{
                                alarm_day = 31;
                            }
                            
                        }
                        OledSetCursor(0, 0);
                    sprintf(message, "ALARM  %s %02d/%02d", adw[index], alarm_month, alarm_day);
                    OledPutString(message);
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress++;
                        break;
                    }
                }
                while (timesPress == 3){
                                    OledSetCursor(0, 0);
                sprintf(message, "ALARM  %s %02d/%02d", adw[indexa], alarm_month, alarm_day);
                OledPutString(message);
                OledSetCursor(0, 1);
                if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                }
                if (alarmState == 1)
                {
                    sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                } 
                    OledSetCursor(0, 2);
                    sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                    OledPutString(message);
                    sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                    OledSetCursor(0, 3);
                    OledPutString(message);

                    clock_sec = seconds;
                    if (clock_sec > 59) {
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                        clock_min++;
                    }
                    if (clock_min > 59 && clock_sec > 59) {
                        clock_hour++;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    if (clock_hour > 23 && clock_min > 59 && clock_sec > 59) {
                        clock_hour = 0;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    if (Button1RisingEdge()) {
                        alarm_hour++;
                        if (alarm_hour == 24 ){
                            alarm_hour = 0;
                        }
                        OledSetCursor(0, 1);
                    if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                    }
                    if (alarmState == 1)
                    {
                        sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                        OledPutString(message);
                    } 
                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        alarm_hour--;
                        if (alarm_hour == -1){
                            alarm_hour = 23;
                        }
                        OledSetCursor(0, 1);
                    if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                    }
                    if (alarmState == 1)
                    {
                        sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                        OledPutString(message);
                    } 
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress++;
                        break;
                    }
                }
                while (timesPress == 4){
                                    OledSetCursor(0, 0);
                sprintf(message, "ALARM  %s %02d/%02d", adw[indexa], alarm_month, alarm_day);
                OledPutString(message);
                OledSetCursor(0, 1);
                if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                }
                if (alarmState == 1)
                {
                    sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                } 
                    OledSetCursor(0, 2);
                    sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                    OledPutString(message);
                    sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                    OledSetCursor(0, 3);
                    OledPutString(message);

                    clock_sec = seconds;
                    if (clock_sec > 59) {
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                        clock_min++;
                    }
                    if (clock_min > 59 && clock_sec > 59) {
                        clock_hour++;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    if (clock_hour > 23 && clock_min > 59 && clock_sec > 59) {
                        clock_hour = 0;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    if (Button1RisingEdge()) {
                        alarm_min++;
                        if (alarm_min > 59){
                            alarm_min = 0;
                        }
                        OledSetCursor(0, 1);
                    if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                    }
                    if (alarmState == 1)
                    {
                        sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                        OledPutString(message);
                    } 
                        

                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        alarm_min--;
                        if (alarm_min < 0){
                            alarm_min = 59;
                        }
                        OledSetCursor(0, 1);
                    if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                    }
                    if (alarmState == 1)
                    {
                        sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                        OledPutString(message);
                    } 
                        
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress++;
                        break;
                    }
                }
                while (timesPress == 5){
                                    OledSetCursor(0, 0);
                sprintf(message, "ALARM  %s %02d/%02d", adw[indexa], alarm_month, alarm_day);
                OledPutString(message);
                OledSetCursor(0, 1);
                if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                }
                if (alarmState == 1)
                {
                    sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                } 
                    OledSetCursor(0, 2);
                    sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                    OledPutString(message);
                    sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                    OledSetCursor(0, 3);
                    OledPutString(message);

                    clock_sec = seconds;
                    if (clock_sec > 59) {
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                        clock_min++;
                    }
                    if (clock_min > 59 && clock_sec > 59) {
                        clock_hour++;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    if (clock_hour > 23 && clock_min > 59 && clock_sec > 59) {
                        clock_hour = 0;
                        clock_min = 0;
                        clock_sec = 0;
                        seconds = 0;
                        milliseconds = 0;
                    }
                    if (Button1RisingEdge()) {
                        alarm_sec++;
                        if (alarm_sec >59){
                            alarm_sec = 0;
                        }
                        OledSetCursor(0, 1);
                    if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                    }
                    if (alarmState == 1)
                    {
                        sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                        OledPutString(message);
                    } 

                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        alarm_sec--;
                        if (alarm_sec == -1){
                            alarm_sec = 59;
                        }
                        OledSetCursor(0, 1);
                    if (alarmState == 0){
                    sprintf(message, "OFF  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                    OledPutString(message);
                    }
                    if (alarmState == 1)
                    {
                        sprintf(message, "ON  %02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);
                        OledPutString(message);
                    } 
                        
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress = 0;
                        state = BASE;
                        OledClearBuffer();
                        break;
                    }
                }
                
                

                break;
            case CURRENT:
                
                 while(timesPress == 0){
                    
                    //increment
                    if (Button1RisingEdge()) {
                        index++;
                        if (index == 7){
                            index = 0;
                        }
                        OledSetCursor(0, 2);
                        sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                        OledPutString(message);

                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        index--;
                        if (index == -1){
                            index = 6;
                        }
                        OledSetCursor(0, 2);
                        sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                        OledPutString(message);
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress++;
                        break;
                    }
                }
                while (timesPress == 1){
                    //increment
                    if (Button1RisingEdge()) {
                        clock_month++;
                        if (clock_month == 13){
                            clock_month = 1;
                        }
                        OledSetCursor(0, 2);
                    sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                    OledPutString(message);
                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        clock_month--;
                        if (clock_month == 0){
                            clock_month = 12;                           
                        }
                        OledSetCursor(0, 2);
                    sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                    OledPutString(message);
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress++;
                        break;
                    }
                    
                }
                while (timesPress == 2){
                    if (Button1RisingEdge()) {
                        clock_day++;
                        if ( clock_month == 4 || clock_month == 6 || clock_month ==9 || clock_month ==11 )
                        {
                            if (clock_day >= 31){
                                clock_day = 0;
                            }
                        }
                        else if ( clock_month == 2)
                        {
                            if (clock_day >= 29){
                                clock_day = 0;
                            }
                        }
                        else
                        {
                            if (clock_day >= 32){
                                clock_day = 0;
                            } 
                        }
                        OledSetCursor(0, 2);
                    sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                    OledPutString(message);
                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        clock_day--;
                        if (clock_day <= 0){
                            if ( clock_month == 4 || clock_month == 6 || clock_month ==9 || clock_month ==11){
                                clock_day = 30;
                            }
                            else if ( clock_month == 2){
                                clock_day = 28;
                            }
                            else{
                                clock_day = 31;
                            }
                            
                        }
                        OledSetCursor(0, 2);
                    sprintf(message, "CLOCK  %s %02d/%02d", cdw[index], clock_month, clock_day);
                    OledPutString(message);
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress++;
                        break;
                    }
                }
                while (timesPress == 3){
                    if (Button1RisingEdge()) {
                        clock_hour++;
                        if (clock_hour == 24 ){
                            clock_hour = 0;
                        }
                        OledSetCursor(0, 3);
                        sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                        OledPutString(message);
                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        clock_hour--;
                        if (clock_hour == -1){
                            clock_hour = 23;
                        }
                        OledSetCursor(0, 3);
                        sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                        OledPutString(message);
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress++;
                        break;
                    }
                }
                while (timesPress == 4){
                    if (Button1RisingEdge()) {
                        clock_min++;
                        if (clock_min > 59){
                            clock_min = 0;
                        }
                        OledSetCursor(0, 3);
                        sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                        OledPutString(message);
                        

                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        clock_min--;
                        if (clock_min < 0){
                            clock_min = 59;
                        }
                        OledSetCursor(0, 3);
                        sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                        OledPutString(message);
                        
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress++;
                        break;
                    }
                }
                while (timesPress == 5){
                    if (Button1RisingEdge()) {
                        clock_sec++;
                        if (clock_sec >59){
                            clock_sec = 0;
                        }
                        OledSetCursor(0, 3);
                        sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                        OledPutString(message);

                    }
                    //decrement
                    if (Button2RisingEdge()) {
                        clock_sec--;
                        if (clock_sec == -1){
                            clock_sec = 59;
                        }
                        OledSetCursor(0, 3);
                        sprintf(message, "ON   %02d:%02d:%02d", clock_hour, clock_min, clock_sec);
                        OledPutString(message);
                        
                    }
                    if (Button3RisingEdge()){
                        
                        timesPress = 0;
                        state = BASE;
                        break;
                    }
                }
                
                break;
                
        }
// for copying
        //increment
                    
        
		//getAccelData(ch, xyzArray);
        //displayAccelData(xyzArray);
        //OledPutFormattedStringAt(0, 3, OLED_SIZE, buf, "Taps: %d", TapCount);
       
    }
    return 0;
}


void initINTController()
{
    // Configure the system for vectored interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

    // Enable the interrupt controller
    INTEnableInterrupts();
}





/*******************************************************************************
 * Function: 
 *          OledPutFormattedStringAt(int X, int Y, int n, char *buf, const char *format, ...)
 * 
 * Summary: 
 *          Puts a formatted string in Oled
 * Description:
 *          This routine creates a formatted string similar to what sprintf does 
 *          and puts it in X, Y cooridiates of oled
 * Precondition:
 *          buf should have space for n characters 
 * Parameters:
 *          int X, int Y : The Oled cooridinates
 *          int n: the maximum size of the string that goes on Oled
 *          char *buf: The pointer to the string that gets created
 *          const char *format, ... : The string inside " " along with all extra parameters needed to complete it
 * Returns:
 *          Nothing
 * Example:
    char buf[10];
    OledPutFormattedStringAt(0, 0, 10, buf, "%8d", Cnt);
 * Remarks:
 *          None
 *****************************************************************************/
void OledPutFormattedStringAt(int X, int Y, int n, char *buf, const char *format, ...)
{
    va_list args;
    va_start (args, format);
    vsnprintf (buf, n, format, args);
    va_end (args);    
    OledSetCursor(X,Y);
    OledPutString(buf);
}
          


//Button DebouncinG!!
unsigned int Button1Debounce() 
{     
    // state of button1. It is static to keep track it in between calls
    static enum tState state = STABLE0; 
    
    // the beginning of the debounce Time
    static int debounceStartTime = 0;
    
    // the momentary status of button1
    // It is 1, if button1 is down and 0, if not
    unsigned char button1Down = ((PORTG & BUTTON1) == BUTTON1);
    
    switch (state) {
        case STABLE0:  
            if (button1Down) 
            {
                debounceStartTime = milliseconds;
                state = TRANS0to1;
            }
            break;
            
        case TRANS0to1:
            if (button1Down && 
                ((milliseconds - debounceStartTime) >= DEBOUNCE_WINDOW))  
                state = STABLE1;
            
            if (!button1Down)
                state = STABLE0;
            break;
            
        case STABLE1:
            if (!button1Down) 
            {
                debounceStartTime = milliseconds;
                state = TRANS1to0;
            }             
            break;
            
        case TRANS1to0:
            if (!button1Down && 
                ((milliseconds - debounceStartTime) >= DEBOUNCE_WINDOW))  
                    state = STABLE0;
            
            if (button1Down)
                state = STABLE1;
            break;
    }
    
        if ((state == STABLE1) || (state == TRANS1to0))
        return 1;

    else
        return 0;
}

// similar to Button1Debounce, except for button2
unsigned int Button2Debounce() 
{     

    // state of button2. It is static to keep track it in between calls
    static enum tState state = STABLE0; 
    
    // the beginning of the debounce Time
    static int debounceStartTime = 0;
    
    // the momentary status of button1
    // It is 1, if button1 is down and 0, if not
    unsigned char button2Down = ((PORTG & BUTTON2) == BUTTON2);
    
    switch (state) {
        case STABLE0:  
            if (button2Down) 
            {
                debounceStartTime = milliseconds;
                state = TRANS0to1;
            }
            break;
            
        case TRANS0to1:
            if (button2Down && 
                ((milliseconds - debounceStartTime) >= DEBOUNCE_WINDOW))  
                state = STABLE1;
            
            if (!button2Down)
                state = STABLE0;
            break;
            
        case STABLE1:
            if (!button2Down) 
            {
                debounceStartTime = milliseconds;
                state = TRANS1to0;
            }             
            break;
            
        case TRANS1to0:
            if (!button2Down && 
                ((milliseconds - debounceStartTime) >= DEBOUNCE_WINDOW))  
                    state = STABLE0;
            
            if (button2Down)
                state = STABLE1;
            break;
    }
    
    if ((state == STABLE1) || (state == TRANS1to0))
        return 1;

    else
        return 0;
}

// similar to Button3Debounce, except for button3
unsigned int Button3Debounce() 
{     

    // state of button3. It is static to keep track it in between calls
    static enum tState state = STABLE0; 
    
    // the beginning of the debounce Time
    static int debounceStartTime = 0;
    
    // the momentary status of button1
    // It is 1, if button1 is down and 0, if not
    unsigned char button3Down = ((PORTA & BUTTON3) == BUTTON3);
    
    switch (state) {
        case STABLE0:  
            if (button3Down) 
            {
                debounceStartTime = milliseconds;
                state = TRANS0to1;
            }
            break;
            
        case TRANS0to1:
            if (button3Down && 
                ((milliseconds - debounceStartTime) >= DEBOUNCE_WINDOW))  
                state = STABLE1;
            
            if (!button3Down)
                state = STABLE0;
            break;
            
        case STABLE1:
            if (!button3Down) 
            {
                debounceStartTime = milliseconds;
                state = TRANS1to0;
            }             
            break;
            
        case TRANS1to0:
            if (!button3Down && 
                ((milliseconds - debounceStartTime) >= DEBOUNCE_WINDOW))  
                    state = STABLE0;
            
            if (button3Down)
                state = STABLE1;
            break;
    }
    
    if ((state == STABLE1) || (state == TRANS1to0))
        return 1;

    else
        return 0;
}
unsigned int Button1RisingEdge() 
{     
    static int lastDebouncedStatus;
    int currentDebouncedStatus = Button1Debounce();
    int edge = ((currentDebouncedStatus == 1)
            && (lastDebouncedStatus == 0));     
    lastDebouncedStatus = currentDebouncedStatus;     
    return edge; 
}

// Similar to Button1RisingEdge(), except it is for button 2
unsigned int Button2RisingEdge() 
{     
    static int lastDebouncedStatus;
    int currentDebouncedStatus = Button2Debounce();
    int edge = ((currentDebouncedStatus == 1)
            && (lastDebouncedStatus == 0));     
    lastDebouncedStatus = currentDebouncedStatus;     
    return edge; 
}

// Similar to Button1RisingEdge(), except it is for button 3
unsigned int Button3RisingEdge() 
{     
    static int lastDebouncedStatus;
    int currentDebouncedStatus = Button3Debounce();
    int edge = ((currentDebouncedStatus == 1)
            && (lastDebouncedStatus == 0));     
    lastDebouncedStatus = currentDebouncedStatus;     
    return edge; 
}

