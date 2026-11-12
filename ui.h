#ifndef UI_H
#define UI_H

#define UI_BLACK        0
#define UI_BLUE         1
#define UI_GREEN        10
#define UI_CYAN         11
#define UI_RED          12
#define UI_YELLOW       14
#define UI_WHITE        15
#define UI_GRAY         8
#define UI_LIGHT_GRAY   7
#define UI_SELECTED     31

void initConsoleWindow(void);
void maximizeConsoleWindow(void);
void applyConsoleLayout(void);
void clearScreenUI(void);

void setColor(int color);
void resetColor(void);

void gotoxy(int x, int y);

int getConsoleWidth(void);
int getConsoleHeight(void);

void drawBox(int left, int top, int right, int bottom);
void drawLine(int left, int y, int right);
void drawFullScreenBorder(void);

void printAt(int x, int y, const char *text);
void printCenter(int y, const char *text, int width);

void clearTextAt(int x, int y, int width);
int readIntRangeAt(int x, int y, int min, int max, int errorX, int errorY, const char *label);

#endif