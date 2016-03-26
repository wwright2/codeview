#ifndef ARRIVAL_FUNCS_H
#define ARRIVAL_FUNCS_H

#include "apiThread.h"

int splitArrival (const char *buf, char *bus, char *route, char *time);
int splitShortArrival (const char *line, char *bus, char *route1, char *route2,
    char *time);
int getArrivals (char *msg, char *line[MAX_ARRIVALS]);
#endif
