// scale_update.c: functions that read scale hardware, convert its
// encode its state in a struct, and adjusts the display to show the
// user information.
#include "scale.h"

// Reads the scale hardware ports and updates the scale struct with the current state
int scale_from_ports(scale_t *scale) {
  // Check for out-of-range sensor or tare ports
  if (SCALE_SENSOR_PORT > 999 || SCALE_SENSOR_PORT < 0 || SCALE_TARE_PORT > 999 || SCALE_TARE_PORT < 0) {
    scale->mode = MODE_ERROR;  // Set mode to error
    scale->weight = 0;         // Reset weight
    scale->indicators = 0;     // Reset indicators
    return 1;                  // Return error code
  }

  // Check if the Tare button is pressed
  if (SCALE_STATUS_PORT & (1 << 5)) {
    scale->indicators = 0;    // Reset indicators
    scale->weight = 0;        // Reset weight
    scale->mode = MODE_TARE;  // Set mode to tare
    return 2;                 // Return tare code
  }

  // Normal operation: calculate weight and set indicators
  else {
    scale->mode = MODE_SHOW;                              // Set mode to show weight
    scale->weight = SCALE_SENSOR_PORT - SCALE_TARE_PORT;  // Calculate net weight

    // Check if the unit is set to pounds
    if (SCALE_STATUS_PORT & (1 << 2)) {
      scale->indicators = 1 << 1;          // Set pound indicator
      scale->weight += 8;                  // Round up if ounces are halfway to the next pound
      scale->weight = scale->weight >> 4;  // Convert ounces to pounds
    }
    // Otherwise, set unit to ounces
    else {
      scale->indicators = 1 << 0;  // Set ounce indicator
    }

    // Check if tare is active
    if (SCALE_STATUS_PORT & (1 << 5) || SCALE_TARE_PORT > 0) {
      scale->indicators = scale->indicators | (1 << 2);  // Set tare indicator
    }
  }
  return 0;  // Return success code
}

// Updates the display for special cases (error or tare mode)
int scale_display_special(scale_t scale, int *display) {
  // Display "ERR" if in error mode
  if (scale.mode == MODE_ERROR) {
    display[0] = 0b0110111101111110111110000000;  // Set display to "ERR.0"
    return 0;                                     // Return success code
  }

  // Display "STOR" if in tare mode
  else if (scale.mode == MODE_TARE) {
    display[0] = 0b1100111100100111110111011111;  // Set display to "STO.R"
    return 0;                                     // Return success code
  }
  return 1;  // Return error code if mode is not special
}

// Updates the display to show the weight in normal mode
int scale_display_weight(scale_t scale, int *display) {
  // Bit patterns for digits 0-9 and negative sign
  int my_patterns[11] = {0b1111011, 0b1001000, 0b0111101, 0b1101101, 0b1001110, 0b1100111,
                         0b1110111, 0b1001001, 0b1111111, 0b1101111, 0b0000100};

  int temp_display = 0b0;  // Initialize display buffer
  if (scale.mode == MODE_SHOW) {
    int weight = scale.weight;

    // Handle negative weight
    if (weight < 0) {
      weight *= -1;  // Convert to positive for processing
    }

    int j = 0;
    // Convert weight to display digits
    while (weight > 0) {
      temp_display |= (my_patterns[(weight % 10)] << j);  // Add digit to display
      weight /= 10;                                       // Move to next digit
      j += 7;                                             // Shift position for next digit
    }

    // Handle weights less than 1 (e.g., 0.1)
    if (j < 8) {
      temp_display |= (my_patterns[0] << j);  // Add leading zero
      j += 7;
    }

    // Add negative sign if weight is negative
    if (scale.weight < 0) {
      temp_display |= (my_patterns[10] << j);  // Add negative sign
      j += 7;
    }

    // Add unit and tare indicators
    temp_display |= scale.indicators << 28;  // Set indicator bits
    *display = temp_display;                 // Update display
    return 0;                                // Return success code
  }
  return 1;  // Return error code if not in show mode
}

// Updates the scale display based on the current state
int scale_update() {
  scale_t one;             // Create a scale struct
  scale_from_ports(&one);  // Populate struct with current port values

  int display;  // Initialize display buffer

  // If in tare mode, update the tare port
  if (one.mode == MODE_TARE) {
    SCALE_TARE_PORT = SCALE_SENSOR_PORT;  // Set tare port to current sensor value
  }

  // Update the display based on the scale mode
  if (scale_display_special(one, &display) == 0 || scale_display_weight(one, &display) == 0) {
    SCALE_DISPLAY_PORT = display;  // Set display port
    return 0;                      // Return success code
  }

  return 1;  // Return error code if display update fails
}