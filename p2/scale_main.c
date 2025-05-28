// thermo_main.c: DO NOT MODIFY (bug fix on Wed Feb 28 04:47:43 PM EST 2024)
//
// Thermometer simulator main program and supporting functions. Read
// temperatuer sensor value and mode (C / F) from the command line and
// show the results of running functions from thermo_update.c on the
// screen. 


#include "scale.h"

#define LB_FLAG      (1<<2)
#define TARE_FLAG    (1<<5)

int main(int argc, char **argv){
  if(argc < 5){
    printf("usage: %s {sensor_val} {tare_val} {oz|lb} {tare_no|tare_yes}\n",argv[0]);
    printf("  sensor_val: integer\n");
    return 0;
  }

  int argind = 0;

  argind++;
  SCALE_SENSOR_PORT = atoi(argv[argind]);

  argind++;
  SCALE_TARE_PORT = atoi(argv[argind]);

  argind++;
  if(strcmp(argv[argind],"lb")==0){
    SCALE_STATUS_PORT |= LB_FLAG;
  }
  else if(strcmp(argv[argind],"oz")!=0){
    printf("argument '%s' is invalid\n",argv[argind]);
    return 1;
  }

  argind++;
  if(strcmp(argv[argind],"tare_yes")==0){
    SCALE_STATUS_PORT |= TARE_FLAG;
  }
  else if(strcmp(argv[argind],"tare_no")!=0){
    printf("argument '%s' is invalid\n",argv[argind]);
    return 1;
  }

  printf("SCALE_SENSOR_PORT: %hd\n", SCALE_SENSOR_PORT);
  printf("SCALE_TARE_PORT:   %hd\n", SCALE_TARE_PORT);
  printf("SCALE_STAUS_PORT:  %s\n",bitstr(SCALE_STATUS_PORT, &statspec));
  printf("index:             %s\n",bitstr_index(&statspec));

  scale_t *scale = malloc(sizeof(scale));
  int result;

  printf("result = scale_from_ports(&scale);\n");
  result = scale_from_ports(scale);
  printf("result: %d\n",result);
  // printf("%s", (result == 0) ? "" : "WARNING: Non-zero value returned\n");

  printf("scale = {\n"); 
  printf("  weight     = %hd\n",scale->weight);
  printf("  mode       = %hhd\n",scale->mode);
  printf("  indicators = %s\n",bitstr(scale->indicators, &indicatorspec));
  printf("}\n");

  printf("result = scale_update();\n");
  result = scale_update();
  printf("result: %d\n", result);
  // printf("%s", (result == 0) ? "" : "WARNING: Non-zero value returned\n");

  printf("SCALE_SENSOR_PORT: %hd\n", SCALE_SENSOR_PORT);
  printf("SCALE_TARE_PORT:   %hd\n", SCALE_TARE_PORT);
  printf("SCALE_DISPLAY_PORT:\n");
  printf("bits:  %s\n",bitstr(SCALE_DISPLAY_PORT, &dispspec));
  printf("index: %s\n",bitstr_index(&dispspec));
  printf("Scale Display:\n");
  print_display();

  free(scale);
  return 0;
}
