// scale_sim.c: DO NOT MODIFY
//
// Scale simulator support functions.

#include "scale.h"

// see scale.h for info on these variables
short SCALE_SENSOR_PORT = 0;
short SCALE_TARE_PORT = 0;
unsigned char SCALE_STATUS_PORT = 0;

#define DISP_MAX_BITS 32

int SCALE_DISPLAY_PORT = 0;
// Global variable used to control the scale display. Making changes to
// this variable will change the thermostat display. Type ensures 64 bits.

////////////////////////////////////////////////////////////////////////////////
// Data and functions to set the state of the thermo clock display 

#define NROWS 5
#define NCOLS 23
// Convenience struct for representing the thermometer display as
// characters
typedef struct {
  char chars[NROWS][NCOLS];
} display_t;

//      ~~   ~~   ~~      0
//     |    |  | |  | -T  1
//      ~~   ~~           2
//        | |  | |  | OZ  3
//      ~~   ~~ o ~~      4
//012345678901234567890123
//0         1         2   

// Initial display config with decimal place
display_t init_display = {
  .chars = {
    "                     ",
    "                     ",
    "                     ",
    "                     ",
    "              o      ",
  }
};

// Reset display to be empty 
void reset_display(display_t *disp){
  *disp = init_display;
}

// Print an display 
void internal_print_display(display_t *display){
  for(int i=0; i<NROWS; i++){
    printf("%s\n",display->chars[i]);
  }
}  

// Position and char in display 
typedef struct {
  int r,c; int ch;
} charpos;
    
// Collection of characters corresponding to one bit in the state being set 
typedef struct {
  int len;                      // number of chars corresponding to this bit
  charpos pos[2];               // position of chars for this bit
} charpos_coll;


#define TOP 0
#define LFT 0
#define LMD 5
#define RMD 10
#define RGT 15


// Correspondence of bit positions to which characters should be set 
charpos_coll bits2chars[DISP_MAX_BITS] = {
  { .len=2, .pos={{0+TOP,1+RGT,'~'}, {0+TOP,2+RGT,'~'}}}, //  0
  { .len=1, .pos={{1+TOP,0+RGT,'|'},                  }}, //  1
  { .len=2, .pos={{2+TOP,1+RGT,'~'}, {2+TOP,2+RGT,'~'}}}, //  2
  { .len=1, .pos={{1+TOP,3+RGT,'|'},                  }}, //  3
  { .len=1, .pos={{3+TOP,0+RGT,'|'},                  }}, //  4
  { .len=2, .pos={{4+TOP,1+RGT,'~'}, {4+TOP,2+RGT,'~'}}}, //  5
  { .len=1, .pos={{3+TOP,3+RGT,'|'},                  }}, //  6

  { .len=2, .pos={{0+TOP,1+RMD,'~'}, {0+TOP,2+RMD,'~'}}}, //  7
  { .len=1, .pos={{1+TOP,0+RMD,'|'},                  }}, //  8
  { .len=2, .pos={{2+TOP,1+RMD,'~'}, {2+TOP,2+RMD,'~'}}}, //  9
  { .len=1, .pos={{1+TOP,3+RMD,'|'},                  }}, // 10
  { .len=1, .pos={{3+TOP,0+RMD,'|'},                  }}, // 11
  { .len=2, .pos={{4+TOP,1+RMD,'~'}, {4+TOP,2+RMD,'~'}}}, // 12
  { .len=1, .pos={{3+TOP,3+RMD,'|'},                  }}, // 13

  { .len=2, .pos={{0+TOP,1+LMD,'~'}, {0+TOP,2+LMD,'~'}}}, // 14
  { .len=1, .pos={{1+TOP,0+LMD,'|'},                  }}, // 15
  { .len=2, .pos={{2+TOP,1+LMD,'~'}, {2+TOP,2+LMD,'~'}}}, // 16
  { .len=1, .pos={{1+TOP,3+LMD,'|'},                  }}, // 17
  { .len=1, .pos={{3+TOP,0+LMD,'|'},                  }}, // 18
  { .len=2, .pos={{4+TOP,1+LMD,'~'}, {4+TOP,2+LMD,'~'}}}, // 19
  { .len=1, .pos={{3+TOP,3+LMD,'|'},                  }}, // 20

  { .len=2, .pos={{0+TOP,1+LFT,'~'}, {0+TOP,2+LFT,'~'}}}, // 21
  { .len=1, .pos={{1+TOP,0+LFT,'|'},                  }}, // 22
  { .len=2, .pos={{2+TOP,1+LFT,'~'}, {2+TOP,2+LFT,'~'}}}, // 23
  { .len=1, .pos={{1+TOP,3+LFT,'|'},                  }}, // 24
  { .len=1, .pos={{3+TOP,0+LFT,'|'},                  }}, // 25
  { .len=2, .pos={{4+TOP,1+LFT,'~'}, {4+TOP,2+LFT,'~'}}}, // 26
  { .len=1, .pos={{3+TOP,3+LFT,'|'},                  }}, // 27

  { .len=2, .pos={{ 3, 20,'O'}, { 3, 21,'Z'},} }, // 28
  { .len=2, .pos={{ 4, 20,'L'}, { 4, 21,'B'},} }, // 29

  { .len=2, .pos={{ 1, 20,'-'}, { 1, 21,'T'},} }, // 30
  // { .len=2, .pos={{ 2, 20,'^'}, { 2, 21,'^'},} }, // 31
  // { .len=4, .pos={{ 0, 20,'B'}, { 0, 21,'A'},{ 0, 22,'T'},{ 0, 22,'!'}} }, // 32

};

void set_display(display_t *disp, int bits){
  int i,j;
  int mask = 0x1;
  reset_display(disp);
  for(i=0; i<DISP_MAX_BITS; i++){
    //    printf("Checking %d\n",i);
    if( bits & (mask << i) ){ // ith bit set, fill in characters 
      //      printf("%d IS SET\n",i);
      charpos_coll coll = bits2chars[i];
      //      printf("Coll len: %d\n",coll.len);
      for(j=0; j<coll.len; j++){
        //        printf("Inner iter %d\n",j);
        charpos pos = coll.pos[j];
        disp->chars[pos.r][pos.c] = pos.ch;
        // print_thermo_display(thermo);
      }
    }
  }
}


// Use the global SCALE_DISPLAY_PORT to show the display
void print_display(){
  display_t display;
  set_display(&display, SCALE_DISPLAY_PORT);
  internal_print_display(&display);
  return;
}

bitspec_t dispspec = {
  .nbits = 32,
  .nclusters = 6,
  .clusters = {1, 3, 7, 7, 7, 7},
};

bitspec_t statspec = {
  .nbits = 8,
  .nclusters = 2,
  .clusters = {4,4},
};

bitspec_t indicatorspec = {
  .nbits = 8,
  .nclusters = 2,
  .clusters = {5,3},
};

// Generate a string version of the bits which clusters the bits based
// on the logical groupings in the problem
char *bitstr(int x, bitspec_t *spec){
  static char buffer[512];
  int idx = 0;
  int clust_idx = 0;
  int clust_break = spec->clusters[0];
  int max = spec->nbits-1;
  for(int i=0; i<spec->nbits; i++){
    if(i==clust_break){
      buffer[idx] = ' ';
      idx++;
      clust_idx++;
      clust_break += spec->clusters[clust_idx];
    }
    buffer[idx] = x & (1 << (max-i)) ? '1' : '0';
    idx++;
  }
  buffer[idx] = '\0';
  return buffer;
}

// Creates a string of bit indices matching the clustering pattern
// above
char *bitstr_index(bitspec_t *spec){
  static char buffer[512];
  char format[256];
  int pos = 0;
  int idx = spec->nbits;
  for(int i=0; i<spec->nclusters; i++){
    idx -= spec->clusters[i];
    if(spec->clusters[i] > 1){
      sprintf(format, "%s%dd ","%",spec->clusters[i]);
      pos += sprintf(buffer+pos, format, idx);
    }
    else{                       // cluster of 1 bit gets no index
      pos += sprintf(buffer+pos, "  ");
    }
  }
  buffer[pos-1] = '\0';         // eliminate trailing space
  return buffer;
}


// // Print the most signficant (right-most) to least signficant bit in
// // the integer passed in 
// void showbits(int x, int bits){
//   int i;
//   int mask = 0x1;
//   for(i=bits-1; i>=0; i--){
//     int shifted_mask = mask << i;
//     if(shifted_mask & x){
//       putchar('1');
//     } else {
//       putchar('0');
//     }
//   }
//   // Equivalent short version 
//   //    (x&(1<<i)) ? putchar('1'): putchar('0');
// }

// char *asbits(int x, int bits){
//   static char buffer[256];
//   int i,idx=0;
//   int mask = 0x1;
//   for(i=bits-1; i>=0; i--, idx++){
//     int shifted_mask = mask << i;
//     buffer[idx]= (shifted_mask & x) ? '1' : '0';
//   }
//   buffer[idx] = '\0';
//   return buffer;
// }

