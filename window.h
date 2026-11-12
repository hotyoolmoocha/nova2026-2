#ifndef WINDOW_H
#define WINDOW_H

typedef struct
{
    int base;
    int nextSeqNum;
    int size;
} Window;

void initializeWindow(Window *window, int size);

#endif