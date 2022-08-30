/* 
 * File Name: Lab07_main.c
 * Author: Shayan Bathaee
 * Date Created: May 14th 2021
 * 
 * Description: This file contains the main code which allows the user to operate
 * a toaster oven on the Uno32 board. The user can do things such as select the 
 * cooking temperature, bake, broil, and more using the dial and buttons on the board.
 * 
 */


// **** Include libraries here ****
// Standard libraries
#include <stdio.h>
#include <string.h>

//CSE13E Support Library
#include "BOARD.h"
#include "Oled.h"
#include "Leds.h"
#include "Adc.h"
#include "Buttons.h"

// Microchip libraries
#include <xc.h>
#include <sys/attribs.h>



// **** Set any macros or preprocessor directives here ****



// **** Set any local typedefs here ****

typedef enum {
    SETUP, SELECTOR_CHANGE_PENDING, COOKING, RESET_PENDING, ALERT
} OvenState;

typedef enum {
    BAKE, TOAST, BROIL
} Mode;

typedef enum {
    TIME_ARROW, TEMP_ARROW, NO_ARROW
} Arrow;

typedef struct {
    OvenState state;
    int timeLeft;
    int temp;
    Mode mode; // contains either Bake, broil, or toast
    Arrow arrow; // contains information on the position of the arrow
} OvenData;

// **** Declare any datatypes here ****

// **** Define any module-level, global, or external variables here ****
// NOTE: ALL TIME VALUES ARE IN MILLISECONDS
#define LINE_1_OFF "|\x02\x02\x02\x02\x02|"
#define LINE_1_COOK "|\x01\x01\x01\x01\x01|"
#define LINE_2 "|     |"
#define LINE_3 "|-----|"
#define LINE_4_OFF "|\x04\x04\x04\x04\x04|"
#define LINE_4_COOK "|\x03\x03\x03\x03\x03|"
#define LONG_PRESS 1
#define DEFAULT_TEMP 350
#define BASE_TEMP 300
#define DEFAULT_TIME 1000
#define ONE_SECOND 1000
#define HALF_SECOND 500
#define LED_COUNT 8
#define BROIL_TEMP 500
#define LEDS_OFF 0b00000000
#define LEDS_ALL 0b11111111
#define MODE_SWITCH 1
#define TIME_INTERVAL 200
#define ADC_SHIFT 2
#define ALERT_TIME 4000
#define ALERT_SWITCH 500

static char OledMsg[100];
static char modeString[10];
static int runtime = 0;
static int timeOne = 0;
static int timeTwo = 0;
static int CCTimeOne = 0; // CCTimeOne and CCTimeTwo are used to calculate the elapsed time
static int CCTimeTwo = 0; // from button 4 down to button 4 up when trying to cancel cooking
static int CCElapsedTime = 0;
static int AlertTimeOne = 0;
static int AlertTimeTwo = 0;
static int AlertElapsedTime = 0;
static int elapsedTime = 0;
static int ratio = 0;
static int cookTime = 0;
static int Event;
static int adcValue = 0;
static int invert = FALSE;
static uint8_t buttonEvents = FALSE;
static OvenData Oven = {SETUP, DEFAULT_TIME, DEFAULT_TEMP, BAKE, TIME_ARROW};


// **** Put any helper functions here ****

/* 
 * timeMin() Returns how many minutes are left to be used for printing. Similarly,
 * timeSec() returns how many seconds are left. The function input timeLeft is the time
 * in milliseconds left to cook.
 */
int timeMin(int timeLeft)
{
    return timeLeft / (60 * ONE_SECOND);
}

int timeSec(int timeLeft)
{
    return (timeLeft % 60000) / ONE_SECOND;
}

/*
 * LedTimer() will light up the LEDs depending on the total cook time with respect to
 * how much time has passed. To figure out how to set up the LEDs, we find the ratio
 * of the cook time with the elapsed time multiplied by 8. This value tells us how
 * much we need to shift the LEDs by.
 */
void LedTimer(int cookTime, int elapsedTime)
{
    uint8_t LedSetup = LEDS_ALL;
    ratio = (elapsedTime * LED_COUNT) / cookTime;
    LedSetup <<= ratio;
    LEDS_SET(LedSetup);
}



/*This function will update your OLED to reflect the state .*/
// Bake -> display and edit time and temp
// Toast -> display and edit time only (and no arrow)
// Broil -> display and edit time, only display temp at 500 (no arrow)

void updateOvenOLED(OvenData ovenData)
{
    if (ovenData.mode == BAKE) {
        strcpy(modeString, "BAKE");
    }
    else if (ovenData.mode == TOAST) {
        strcpy(modeString, "TOAST");
    }
    else if (ovenData.mode == BROIL) {
        strcpy(modeString, "BROIL");
    }
    if ((ovenData.state == SETUP) || (ovenData.state == SELECTOR_CHANGE_PENDING) || (ovenData.state == ALERT)) {
        if (ovenData.mode == BAKE) {
            if (ovenData.arrow == TEMP_ARROW) {
                sprintf(OledMsg, "%s MODE: %s \n%s %sTIME: %d:%02d\n%s %sTEMP: %d\n%s",
                        LINE_1_OFF, modeString, LINE_2, " ", timeMin(ovenData.timeLeft),
                        timeSec(ovenData.timeLeft), LINE_3, ">", ovenData.temp, LINE_4_OFF);
            }
            else if (ovenData.arrow == TIME_ARROW) {
                sprintf(OledMsg, "%s MODE: %s \n%s %sTIME: %d:%02d\n%s %sTEMP: %d\n%s",
                        LINE_1_OFF, modeString, LINE_2, ">", timeMin(ovenData.timeLeft),
                        timeSec(ovenData.timeLeft), LINE_3, " ", ovenData.temp, LINE_4_OFF);
            }
            // for extra credit
            else if (ovenData.arrow == NO_ARROW) {
                sprintf(OledMsg, "%s MODE: %s \n%s %sTIME: %d:%02d\n%s %sTEMP: %d\n%s",
                        LINE_1_OFF, modeString, LINE_2, " ", timeMin(ovenData.timeLeft),
                        timeSec(ovenData.timeLeft), LINE_3, " ", ovenData.temp, LINE_4_OFF);
            }
        }
        else if (ovenData.mode == TOAST) {
            sprintf(OledMsg, "%s MODE: %s\n%s  TIME: %d:%02d\n%s             \n%s",
                    LINE_1_OFF, modeString, LINE_2, timeMin(ovenData.timeLeft),
                    timeSec(ovenData.timeLeft), LINE_3, LINE_4_OFF);
        }
        else if (ovenData.mode == BROIL) {
            sprintf(OledMsg, "%s MODE: %s\n%s  TIME: %d:%02d\n%s  TEMP: %d\n%s",
                    LINE_1_OFF, modeString, LINE_2, timeMin(ovenData.timeLeft),
                    timeSec(ovenData.timeLeft), LINE_3, BROIL_TEMP, LINE_4_OFF);
        }
    }
    else if ((ovenData.state == COOKING) || (ovenData.state = RESET_PENDING)) {
        if (ovenData.mode == BAKE) {
            sprintf(OledMsg, "%s MODE: %s\n%s  TIME: %d:%02d\n%s  TEMP: %d\n%s",
                    LINE_1_COOK, modeString, LINE_2, timeMin(ovenData.timeLeft),
                    timeSec(ovenData.timeLeft), LINE_3, ovenData.temp, LINE_4_COOK);
        }
        else if (ovenData.mode == TOAST) {
            sprintf(OledMsg, "%s MODE: %s\n%s  TIME: %d:%02d\n%s\n%s",
                    LINE_1_OFF, modeString, LINE_2, timeMin(ovenData.timeLeft),
                    timeSec(ovenData.timeLeft), LINE_3, LINE_4_COOK);
        }
        else if (ovenData.mode == BROIL) {
            sprintf(OledMsg, "%s MODE: %s\n%s  TIME: %d:%02d\n%s  TEMP: %d\n%s",
                    LINE_1_COOK, modeString, LINE_2, timeMin(ovenData.timeLeft),
                    timeSec(ovenData.timeLeft), LINE_3, BROIL_TEMP, LINE_4_OFF);
        }
        LedTimer(cookTime, elapsedTime);
    }
    OledDrawString(OledMsg);
    if (invert == TRUE) {
        OledSetDisplayInverted();
    }
    else {
        OledSetDisplayNormal();
    }
    OledUpdate();
}

/*This function will execute your state machine.  
 * It should ONLY run if an event flag has been set.*/
void runOvenSM(void)
{
    // extra credit
    if (Oven.state == ALERT) {
        AlertTimeTwo = runtime;
        AlertElapsedTime = AlertTimeTwo - AlertTimeOne;
        if (AlertElapsedTime >= ALERT_TIME) {
            Oven.state = SETUP;
            Oven.timeLeft = cookTime;
            Oven.arrow = TIME_ARROW;
            Oven.temp = 350;
        }
        else if ((((AlertElapsedTime) / ALERT_SWITCH) % 2) == 0) {
            invert = TRUE;
        }
        else {
            invert = FALSE;
        }
    }
    if ((Oven.state == COOKING) || (Oven.state == RESET_PENDING)) {
        timeTwo = runtime;
        elapsedTime = timeTwo - timeOne;
        // check if elapsedTime is an integer, if so update Oven.timeLeft
        Oven.timeLeft = cookTime - elapsedTime;
        if (Oven.timeLeft < 0) {
            Oven.timeLeft = cookTime;
            Oven.state = ALERT;
            invert = TRUE;
            AlertTimeOne = runtime;
            Oven.arrow = NO_ARROW;
        }
        // Cancel cooking (CC) feature
        if (buttonEvents & BUTTON_EVENT_4DOWN) {
            CCTimeOne = runtime;
            Oven.state = RESET_PENDING;
        }
        else if ((buttonEvents & BUTTON_EVENT_4UP) && (Oven.state == RESET_PENDING)) {
            CCTimeTwo = runtime;
            CCElapsedTime = CCTimeTwo - CCTimeOne;
            // I went with the lab doc instead of the FSM diagram for the ElapsedTime bounds
            if (CCElapsedTime > LONG_PRESS * ONE_SECOND) {
                Oven.timeLeft = cookTime;
                Oven.state = SETUP;
                LEDS_SET(LEDS_OFF);
            }
            else {
                Oven.state = COOKING;
            }
        }
    }
    if ((Oven.state == SETUP) || (Oven.state == SELECTOR_CHANGE_PENDING)) {
        // check if button 3 is pressed down, if so record time and update OLED
        if (buttonEvents & BUTTON_EVENT_3DOWN) {
            timeOne = runtime;
            Oven.state = SELECTOR_CHANGE_PENDING;
        }
        else if ((buttonEvents & BUTTON_EVENT_3UP) && (Oven.state = SELECTOR_CHANGE_PENDING)) {
            timeTwo = runtime;
            elapsedTime = timeTwo - timeOne;
            /* 
             * FSM diagram slightly differs from this, but I decided to go with the
             * bounds that the Lab doc says to use evaluating LONG_PRESS
             */
            if (elapsedTime <= (LONG_PRESS * ONE_SECOND)) {
                if (Oven.mode == BROIL) {
                    Oven.mode = BAKE;
                    Oven.timeLeft = DEFAULT_TIME;
                    Oven.arrow = TIME_ARROW;
                    Oven.temp = DEFAULT_TEMP;
                }
                else {
                    Oven.mode += MODE_SWITCH;
                }
                Oven.state = SETUP;
            }
            else {
                if ((Oven.mode == BAKE) && (Oven.arrow == TIME_ARROW)) {
                    Oven.arrow = TEMP_ARROW;
                }
                else {
                    Oven.arrow = TIME_ARROW;
                }
                Oven.state = SETUP;
            }
        }
        // Check if we need to switch to cooking mode
        if (buttonEvents & BUTTON_EVENT_4DOWN) {
            Oven.state = COOKING;
            timeOne = runtime;
            cookTime = Oven.timeLeft;
        }

        // Update Time and Temperature values if needed
        if (Oven.mode == BAKE) {
            if (Oven.arrow == TIME_ARROW) {
                Oven.timeLeft = (adcValue * ONE_SECOND) + ONE_SECOND;
            }
            else if (Oven.arrow == TEMP_ARROW) {
                Oven.temp = adcValue + BASE_TEMP;
            }
        }
        else if (Oven.mode == TOAST) {
            Oven.timeLeft = (adcValue * ONE_SECOND) + ONE_SECOND;
        }
        else if (Oven.mode == BROIL) {
            Oven.timeLeft = (adcValue * ONE_SECOND) + ONE_SECOND;
        }
    }
    updateOvenOLED(Oven);
}

int main()
{
    //initalize timers and timer ISRs:
    // <editor-fold defaultstate="collapsed" desc="TIMER SETUP">

    // Configure Timer 2 using PBCLK as input. We configure it using a 1:16 prescalar, so each timer
    // tick is actually at F_PB / 16 Hz, so setting PR2 to F_PB / 16 / 100 yields a .01s timer.

    T2CON = 0; // everything should be off
    T2CONbits.TCKPS = 0b100; // 1:16 prescaler
    PR2 = BOARD_GetPBClock() / 16 / 100; // interrupt at .5s intervals
    T2CONbits.ON = 1; // turn the timer on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T2IF = 0; //clear the interrupt flag before configuring
    IPC2bits.T2IP = 4; // priority of  4
    IPC2bits.T2IS = 0; // subpriority of 0 arbitrarily 
    IEC0bits.T2IE = 1; // turn the interrupt on

    // Configure Timer 3 using PBCLK as input. We configure it using a 1:256 prescaler, so each timer
    // tick is actually at F_PB / 256 Hz, so setting PR3 to F_PB / 256 / 5 yields a .2s timer.

    T3CON = 0; // everything should be off
    T3CONbits.TCKPS = 0b111; // 1:256 prescaler
    PR3 = BOARD_GetPBClock() / 256 / 5; // interrupt at .5s intervals
    T3CONbits.ON = 1; // turn the timer on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T3IF = 0; //clear the interrupt flag before configuring
    IPC3bits.T3IP = 4; // priority of  4
    IPC3bits.T3IS = 0; // subpriority of 0 arbitrarily 
    IEC0bits.T3IE = 1; // turn the interrupt on;

    // </editor-fold>

    printf("Welcome to sbathaee's Lab07 (Toaster Oven).  Compiled on %s %s.", __TIME__, __DATE__);

    //initialize state machine (and anything else you need to init) here
    BOARD_Init();
    OledInit();
    LEDS_INIT();
    AdcInit();
    Event = FALSE;
    while (TRUE) {
        if (Event) {
            runOvenSM();
            Event = FALSE;
        }
    }
}

/*The 5hz timer is used to update the free-running timer and to generate TIMER_TICK events*/
void __ISR(_TIMER_3_VECTOR, ipl4auto) TimerInterrupt5Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 12;

    // timer interrupts at 5Hz (5 times per second) so we add 1/5 of a second
    runtime += TIME_INTERVAL;
    Event = TRUE;

}

/*The 100hz timer is used to check for button and ADC events*/
void __ISR(_TIMER_2_VECTOR, ipl4auto) TimerInterrupt100Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;
    // Check if ADC changed, if so read it
    if (AdcChanged()) {
        adcValue = AdcRead();
        adcValue >>= ADC_SHIFT;
    }
    buttonEvents = ButtonsCheckEvents();
    Event = TRUE;
}