#ifndef THERMO_H
#define THERMO_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>              // for debugging

// STRUCTS_BEGIN

////////////////////////////////////////////////////////////////////////////////
// scale data and structs

#define UNIT_KG 1               // show kilograms
#define UNIT_LB 2               // show pounds (2.2 pounds per kg)

#define MODE_SHOW      1        // show measured weight as sensor_val - tare_val 
#define MODE_TARE      2        // store the sensor_val into tare_val, Tare Indicator on
#define MODE_RESET     3        // store 0 into tare_val, Tare Indicator off
#define MODE_ERROR     4        // error of some kind

// store state of the scale in a struct for easier access
typedef struct{
  short weight;            // calculated weight for use elsewhere
  char mode;               // one of the MODE_XX values indicating what to do/show
  char indicators;         // Bit field with 1's for different indicators
  // Bit 0: Ounce Indicator On/Off
  // Bit 1: Pound Indicator On/Off
  // Bit 2: Tare Indicator On/Off, on when stored Tare value is non-zero
} scale_t;

// STRUCTS_END

////////////////////////////////////////////////////////////////////////////////
// scale_update.c functions: student implemented
int scale_from_ports(scale_t *scale);
int scale_display_special(scale_t scale, int *display);
int scale_display_weight(scale_t scale, int *display);
int scale_update();

////////////////////////////////////////////////////////////////////////////////
// scale_sim.c data/structs/functions; provided as is, do not modify

// PORTS_BEGIN
extern short SCALE_SENSOR_PORT;
// Set by the hardware to indicate the load placed on the scale. The
// sensor is in units of 0.1 oz so that a sensor value of 238 would be
// 23.8 ounces. If negative or greater than 999, a hardware error has
// occurred. This port should only be read from.

extern short SCALE_TARE_PORT;
// Internal hardware that stores a previous sensor value for use later
// such as to subtract off the weight of a container on the scale to
// determine the weight of its contents. If this value is negative or
// greater than 999, a hardware error has occurred.  This port may be
// read or written to.

extern unsigned char SCALE_STATUS_PORT;
// A series of bits indicating various aspects of the scale's state as
// detected by the hardware. While there are 8 bits, only 3 of them
// are used by the software.  The bits are as follows:
// 
// Bit 0: Unused
// Bit 1: Unused
// Bit 2: Unit Select, 0 for ounces (oz) display, 1 for pounds (lb) display
// Bit 3: Unused
// Bit 4: Unused
// Bit 5: Tare Button, 1 for pressed, 0 for not
// Bit 6: Unused
// Bit 7: Unused
//
// This port should only be read from.

extern int SCALE_DISPLAY_PORT;
// Controls the scale display. Readable and writable. Routines
// wishing to change the display should alter the bits of this
// variable.

// PORTS_END

////////////////////////////////////////////////////////////////////////////////
// data and printing routines for displaying bits nicely
typedef struct {
  int nbits;
  int nclusters;
  int clusters[32];
} bitspec_t;

extern bitspec_t dispspec;
extern bitspec_t statspec;
extern bitspec_t indicatorspec;

char *bitstr(int x, bitspec_t *spec);
char *bitstr_index(bitspec_t *spec);

void print_display();
// Show the simulated display as ASCII on the screen

#endif
