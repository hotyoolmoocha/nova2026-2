#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "ui.h"

static int getDisplayWidth(const char *text);

void initConsoleWindow(void)
{
#ifdef _WIN32
    SetConsoleTitleA("Sliding Window Simulator");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    applyConsoleLayout();
    clearScreenUI();
}


void clearScreenUI(void)
{
    applyConsoleLayout();

#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void setColor(int color)
{
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#else
    (void)color;
#endif
}

void resetColor(void)
{
    setColor(UI_LIGHT_GRAY);
}

void gotoxy(int x, int y)
{
#ifdef _WIN32
    COORD pos;
    pos.X = x;
    pos.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
#else
    printf("\033[%d;%dH", y + 1, x + 1);
#endif
}

int getConsoleWidth(void)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
#endif

    return 120;
}

int getConsoleHeight(void)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    {
        return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#endif

    return 30;
}

void printAt(int x, int y, const char *text)
{
    gotoxy(x, y);
    printf("%s", text);
}

void drawLine(int left, int y, int right)
{
    int i;

    gotoxy(left, y);

    for (i = left; i <= right; i++)
    {
        printf("─");
    }
}

void drawBox(int left, int top, int right, int bottom)
{
    int i;

    gotoxy(left, top);
    printf("┌");

    for (i = left + 1; i < right; i++)
    {
        printf("─");
    }

    printf("┐");

    for (i = top + 1; i < bottom; i++)
    {
        gotoxy(left, i);
        printf("│");

        gotoxy(right, i);
        printf("│");
    }

    gotoxy(left, bottom);
    printf("└");

    for (i = left + 1; i < right; i++)
    {
        printf("─");
    }

    printf("┘");
}

void drawFullScreenBorder(void)
{
    int width = getConsoleWidth();
    int height = getConsoleHeight();

    if (width < 80)
    {
        width = 80;
    }

    if (height < 25)
    {
        height = 25;
    }

    drawBox(1, 1, width - 3, height - 3);
}

void printCenter(int y, const char *text, int width)
{
    int textWidth = getDisplayWidth(text);
    int x = (width - textWidth) / 2;

    if (x < 0)
    {
        x = 0;
    }

    gotoxy(x, y);
    printf("%s", text);
}

void clearTextAt(int x, int y, int width)
{
    int i;

    gotoxy(x, y);

    for (i = 0; i < width; i++)
    {
        printf(" ");
    }
}

int readIntRangeAt(int x, int y, int min, int max, int errorX, int errorY, const char *label)
{
    char buffer[100];
    char *endptr;
    long value;

    while (1)
    {
        clearTextAt(x, y, 20);
        gotoxy(x, y);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            continue;
        }

        errno = 0;
        endptr = buffer;

        while (isspace((unsigned char)*endptr))
        {
            endptr++;
        }

        if (*endptr == '\0')
        {
            setColor(UI_RED);
            clearTextAt(errorX, errorY, 80);
            printf("잘못된 입력입니다. 숫자를 입력하세요.");
            resetColor();
            continue;
        }

        value = strtol(endptr, &endptr, 10);

        while (isspace((unsigned char)*endptr))
        {
            endptr++;
        }

        if (errno == 0 && *endptr == '\0' && value >= min && value <= max)
        {
            clearTextAt(errorX, errorY, 80);
            clearTextAt(errorX, errorY + 1, 80);
            return (int)value;
        }

        setColor(UI_RED);
        clearTextAt(errorX, errorY, 80);
        clearTextAt(errorX, errorY + 1, 80);

        gotoxy(errorX, errorY);
        printf("잘못된 입력입니다. 올바른 범위의 숫자를 입력하세요.");

        gotoxy(errorX, errorY + 1);
        printf("%s 범위: %d ~ %d", label, min, max);

        resetColor();
    }
}

static int getDisplayWidth(const char *text)
{
    int width = 0;
    int i = 0;

    while (text[i] != '\0')
    {
        unsigned char c = (unsigned char)text[i];

        if (c < 128)
        {
            width += 1;
            i += 1;
        }
        else
        {
            width += 2;

            if ((c & 0xE0) == 0xC0)
            {
                i += 2;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                i += 3;
            }
            else if ((c & 0xF8) == 0xF0)
            {
                i += 4;
            }
            else
            {
                i += 1;
            }
        }
    }

    return width;
}

void maximizeConsoleWindow(void)
{
#ifdef _WIN32
    HWND consoleWindow = GetConsoleWindow();

    if (consoleWindow != NULL)
    {
        ShowWindow(consoleWindow, SW_MAXIMIZE);
    }
#endif
}

void applyConsoleLayout(void)
{
#ifdef _WIN32
    /*
        콘솔 내부 크기를 크게 설정.
        Windows Terminal에서는 완전한 F11 전체화면은 강제하기 어렵지만,
        창을 최대화하고 내부 출력 영역을 넓게 잡을 수 있다.
    */
    system("mode con: cols=150 lines=42");
    maximizeConsoleWindow();
#endif
}