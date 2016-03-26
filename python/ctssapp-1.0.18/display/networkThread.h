#ifndef NETWORKTHREADS_H
#define NETWORKTHREADS_H

#include <sys/types.h>
#include <stdint.h>

int setupSockets ();
void sendBroadcast (uint8_t *buf, size_t size);
uint8_t *receiveApiData (uint8_t *buf, ssize_t *size);
uint8_t *receiveSensorData (size_t *size);
void *networkThreadLoop (void *);

#endif
