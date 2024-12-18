#include "contiki.h"

#define MAX_MOISTURE_VALUE 98
#define MIN_MOISTURE_VALUE 0 
#define DEFAULT_DRY_TRESHOLD 20
#define DEFAULT_WET_TRESHOLD 80
#define DEFAULT_SENSING_VALUE 50
#define MOISTURE_SENSING_PERIOD (CLOCK_SECOND >> 1)

struct fakeMoistureSensor
{
    uint8_t sensingValue;
    struct etimer sensingTimer;
    uint8_t dryTreshold ;
    uint8_t wetTreshold ;
    bool pumpState ;
};

void inizializeSensor();
void checkMoisture();
void changePumpState(bool );
void changeSensigvalue();
