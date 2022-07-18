#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "batt.h"
#include <string.h>
#include <stdint.h>

// Uses the two global variables (ports) BATT_VOLTAGE_PORT and
// BATT_STATUS_PORT to set the fields of the parameter 'batt'.  If
// BATT_VOLTAGE_PORT is negative, then battery has been wired wrong;
// no fields of 'batt' are changed and 1 is returned to indicate an
// error.  Otherwise, sets fields of batt based on reading the voltage
// value and converting to percent using the provided formula. Returns
// 0 on a successful execution with no errors. This function DOES NOT
// modify any global variables but may access global variables.
//
// CONSTRAINT: Uses only integer operations. No floating point
// operations are used as the target machine does not have a FPU.
//
// CONSTRAINT: Limit the complexity of code as much as possible. Do
// not use deeply nested conditional structures. Seek to make the code
// as short, and simple as possible. Code longer than 40 lines may be
// penalized for complexity.
int set_batt_from_ports(batt_t *batt)
{
    // If BATT_VOLTAGE_PORT is negative, then battery has been wired wrong;
    // no fields of 'batt' are changed and 1 is returned to indicate an
    // error.
    if (BATT_VOLTAGE_PORT < 0)
    {
        return 1;
    }

    // Otherwise, sets fields of batt based on reading the voltage
    // value and converting to percent using the provided formula. Returns
    // 0 on a successful execution with no errors.
    batt->mlvolts = BATT_VOLTAGE_PORT / 2;
    batt->percent = (batt->mlvolts - 3000) / 8;

    //ensure percentage does not go below zero
    if (batt->mlvolts < 3000)
    {
        batt->percent = 0;
    }

    //ensure percentage does not go above 100
    if (batt->mlvolts > 3800)
    {
        batt->percent = 100;
    }

    //check display mode
    if (BATT_STATUS_PORT & 1 << 2) //percent
    {
        batt->mode = 2;
    }
    else //volts
    {
        batt->mode = 1;
    }

    return 0;
}

// Alters the bits of integer pointed to by 'display' to reflect the
// data in struct param 'batt'.  Does not assume any specific bit
// pattern stored at 'display' and completely resets all bits in it on
// successfully completing.  Selects either to show Volts (mode=1) or
// Percent (mode=2). If Volts are displayed, only displays 3 digits
// rounding the lowest digit up or down appropriate to the last digit.
// Calculates each digit to display changes bits at 'display' to show
// the volts/percent according to the pattern for each digit. Modifies
// additional bits to show a decimal place for volts and a 'V' or '%'
// indicator appropriate to the mode. In both modes, places bars in
// the level display as indicated by percentage cutoffs in provided
// diagrams. This function DOES NOT modify any global variables but
// may access global variables. Always returns 0.
//
// CONSTRAINT: Limit the complexity of code as much as possible. Do
// not use deeply nested conditional structures. Seek to make the code
// as short, and simple as possible. Code longer than 65 lines may be
// penalized for complexity.
int set_display_from_batt(batt_t batt, int *display)
{
    int digits[10] = {0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110, 0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111};

    //reset display
    *display = 0;

    if (batt.mode == 1) //volt mode
    {
        *display |= digits[((batt.mlvolts + 5) / 10) % 10];   //right digit (+5 to round)
        *display |= digits[(batt.mlvolts / 100) % 10] << 7;   //center digit
        *display |= digits[(batt.mlvolts / 1000) % 10] << 14; //left digit

        *display |= 1 << 22; //v
        *display |= 1 << 23; //.
    }
    else //percent mode
    {
        *display |= digits[(batt.percent) % 10]; //right digit

        digits[0] = 0b0111111 * (batt.percent / 100); // leading zeroes don't display 

        *display |= digits[(batt.percent / 10) % 10] << 7;   //center digit
        *display |= digits[(batt.percent / 100) % 10] << 14; //left digit

        *display |= 1 << 21; //%
    }

    //battery bars
    if (batt.percent > 4)
    {
        *display |= 1 << 24;
    }
    if (batt.percent > 29)
    {
        *display |= 1 << 25;
    }
    if (batt.percent > 49)
    {
        *display |= 1 << 26;
    }
    if (batt.percent > 69)
    {
        *display |= 1 << 27;
    }
    if (batt.percent > 89)
    {
        *display |= 1 << 28;
    }

    return 0;
}

// Called to update the battery meter display.  Makes use of
// set_batt_from_ports() and set_display_from_batt() to access battery
// voltage sensor then set the display. Checks these functions and if
// they indicate an error, does NOT change the display.  If functions
// succeed, modifies BATT_DISPLAY_PORT to show current battery level.
//
// CONSTRAINT: Does not allocate any heap memory as malloc() is NOT
// available on the target microcontroller.  Uses stack and global
// memory only.
int batt_update()
{
    // create a batt
    batt_t batt = {};

    // Checks the function and if
    // it indicates an error, does NOT change the display.
    if (set_batt_from_ports(&batt))
    {
        return 1;
    }

    // changes the display
    set_display_from_batt(batt, &BATT_DISPLAY_PORT);

    return 0;
}