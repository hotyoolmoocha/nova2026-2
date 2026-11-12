#ifndef CONFIG_H
#define CONFIG_H

typedef struct
{
    int protocol;
    int packetCount;
    int windowSize;
    int lossRate;
    int delay;
} Config;

extern Config config;

void initConfig(void);

#endif