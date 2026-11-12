#include <stdio.h>
#include <conio.h>

#include "menu.h"
#include "config.h"
#include "simulator.h"
#include "packet.h"
#include "ui.h"


static void drawMainMenu(const char *errorMessage);
static int readMenuChoice(void);
static void settings(void);
static void about(void);

static int readInt(void);
static void flushInput(void);
static int clampInt(int value, int min, int max);
static const char *getProtocolName(int protocol);

void runMenu(void)
{
    int menu;

    initConsoleWindow();

    while (1)
    {
        menu = readMenuChoice();

        switch (menu)
        {
        case 1:
            runSimulation();
            break;

        case 2:
            settings();
            break;

        case 3:
            about();
            break;

        case 0:
            clearScreenUI();
            resetColor();
            return;

        default:
            setColor(UI_RED);
            printAt(41, 30, "Wrong Input! Press Enter...");
            resetColor();
            getchar();
            break;
        }
    }
}

static void drawMainMenu(const char *errorMessage)
{
    int width = getConsoleWidth();
    int height = getConsoleHeight();

    int boxWidth = 52;
    int boxHeight = 13;

    int menuLeft = (width - boxWidth) / 2;
    int menuTop = (height - boxHeight) / 2 + 2;
    int menuRight = menuLeft + boxWidth;
    int menuBottom = menuTop + boxHeight;

    clearScreenUI();

    setColor(UI_GRAY);
    drawFullScreenBorder();

    drawLine(4, 4, width - 6);
    drawLine(4, 9, width - 6);

    setColor(UI_CYAN);
    printCenter(6, "Sliding Window Protocol Simulator", width);

    setColor(UI_LIGHT_GRAY);
    printCenter(7, "- 네트워크 전송 제어 시뮬레이터 -", width);

    setColor(UI_LIGHT_GRAY);
    drawBox(menuLeft, menuTop, menuRight, menuBottom);

    setColor(UI_SELECTED);
    printAt(menuLeft + 5, menuTop + 2, " 1. 시뮬레이션 시작                     ");

    setColor(UI_WHITE);
    printAt(menuLeft + 5, menuTop + 4, " 2. 설정                                ");
    printAt(menuLeft + 5, menuTop + 6, " 3. 프로그램 정보                       ");
    printAt(menuLeft + 5, menuTop + 8, " 0. 종료                                ");

    setColor(UI_GRAY);
    drawLine(menuLeft + 4, menuTop + 10, menuRight - 4);

    setColor(UI_LIGHT_GRAY);
    printAt(menuLeft + 5, menuTop + 11, "메뉴 선택 : ");

    if (errorMessage != NULL)
    {
        setColor(UI_RED);
        printAt(menuLeft + 3, menuBottom + 2, errorMessage);
    }

    setColor(UI_CYAN);
    printAt(5, height - 5, "팁");

    setColor(UI_LIGHT_GRAY);
    printAt(12, height - 5, "번호 입력 후 Enter키를 누르세요.");

    resetColor();
}

static int readMenuChoice(void)
{
    int menu;
    const char *errorMessage = NULL;

    while (1)
    {
        int width;
        int height;
        int boxWidth;
        int boxHeight;
        int menuLeft;
        int menuTop;

        drawMainMenu(errorMessage);

        width = getConsoleWidth();
        height = getConsoleHeight();

        boxWidth = 52;
        boxHeight = 13;

        menuLeft = (width - boxWidth) / 2;
        menuTop = (height - boxHeight) / 2 + 2;

        gotoxy(menuLeft + 18, menuTop + 11);

        if (scanf("%d", &menu) != 1)
        {
            flushInput();
            errorMessage = "잘못된 입력입니다. 숫자를 입력하세요.";
            continue;
        }

        flushInput();

        if (menu >= 0 && menu <= 3)
        {
            return menu;
        }

        errorMessage = "잘못된 메뉴입니다. 0, 1, 2, 3 중에서 입력하세요.";
    }
}

static void settings(void)
{
    int newProtocol;
    int newPacketCount;
    int newWindowSize;
    int newLossRate;
    int newDelay;

    clearScreenUI();

    setColor(UI_GRAY);
    drawBox(2, 1, 112, 27);

    setColor(UI_CYAN);
    printAt(6, 3, "[ 설정 ]");

    setColor(UI_LIGHT_GRAY);
    printAt(78, 3, "네트워크 전송 환경 설정");

    setColor(UI_GRAY);
    drawLine(4, 5, 110);

    /* 왼쪽: 현재 설정 */
    setColor(UI_WHITE);
    printAt(8, 7, "현재 설정");

    setColor(UI_GRAY);
    drawLine(8, 8, 50);

    setColor(UI_LIGHT_GRAY);
    printAt(8, 10, "프로토콜      : ");
    setColor(UI_CYAN);
    printAt(27, 10, getProtocolName(config.protocol));

    setColor(UI_LIGHT_GRAY);
    printAt(8, 12, "패킷 개수     : ");
    gotoxy(27, 12);
    printf("%d", config.packetCount);

    printAt(8, 13, "윈도우 크기   : ");
    gotoxy(27, 13);
    printf("%d", config.windowSize);

    printAt(8, 14, "손실률 (%)    : ");
    gotoxy(27, 14);
    printf("%d", config.lossRate);

    printAt(8, 15, "지연 시간(ms) : ");
    gotoxy(27, 15);
    printf("%d", config.delay);

    /* 오른쪽: 새 설정 입력 */
    setColor(UI_YELLOW);
    printAt(58, 7, "새 설정값 입력");

    setColor(UI_GRAY);
    drawLine(58, 8, 105);

    setColor(UI_LIGHT_GRAY);
    printAt(58, 10, "1=Stop&Wait, 2=Go-Back-N, 3=Selective Repeat");

    setColor(UI_WHITE);

    printAt(58, 12, "새 프로토콜      : ");
    newProtocol = readIntRangeAt(80, 12, 1, 3, 58, 19, "프로토콜");

    printAt(58, 13, "새 패킷 개수     : ");
    newPacketCount = readIntRangeAt(80, 13, 1, MAX_PACKET, 58, 19, "패킷 개수");

    printAt(58, 14, "새 윈도우 크기   : ");
    newWindowSize = readIntRangeAt(80, 14, 1, newPacketCount, 58, 19, "윈도우 크기");

    printAt(58, 15, "새 손실률 (%)    : ");
    newLossRate = readIntRangeAt(80, 15, 0, 100, 58, 19, "손실률");

    printAt(58, 16, "새 지연 시간(ms) : ");
    newDelay = readIntRangeAt(80, 16, 100, 3000, 58, 19, "지연 시간");

    config.protocol = newProtocol;
    config.packetCount = newPacketCount;
    config.windowSize = newWindowSize;
    config.lossRate = newLossRate;
    config.delay = newDelay;

    setColor(UI_GRAY);
    drawLine(4, 21, 110);

    setColor(UI_GREEN);
    printAt(48, 23, "[ 저장 완료 ]");

    setColor(UI_LIGHT_GRAY);
    printAt(34, 24, "Enter키를 누르면 메인 메뉴로 돌아갑니다.");

    resetColor();
    _getch();
}

static void about(void)
{
    clearScreenUI();

    setColor(UI_GRAY);
    drawBox(2, 1, 112, 27);

    setColor(UI_CYAN);
    printCenter(5, "Sliding Window Protocol Simulator", 115);

    setColor(UI_LIGHT_GRAY);
    printCenter(6, "- 네트워크 전송 제어 시뮬레이터 -", 115);

    setColor(UI_GRAY);
    drawLine(5, 8, 109);

    setColor(UI_LIGHT_GRAY);
    drawBox(27, 10, 88, 23);

    setColor(UI_WHITE);
    printAt(32, 13, "프로젝트 : 네트워크 전송 제어 시뮬레이터");
    printAt(32, 15, "프로토콜 : Stop & Wait / Go-Back-N / Selective Repeat");

    printAt(32, 17, "목적     : Packet 전송, ACK, 손실, Timeout,");
    printAt(43, 18, "Retransmission 과정을 시각적으로 표현");

    setColor(UI_GRAY);
    drawLine(30, 20, 85);

    setColor(UI_CYAN);
    printAt(40, 22, "Enter키를 누르면 메인 메뉴로 돌아갑니다.");

    resetColor();
    _getch();
}

static int readInt(void)
{
    int value;

    if (scanf("%d", &value) != 1)
    {
        flushInput();
        return -1;
    }

    flushInput();
    return value;
}

static void flushInput(void)
{
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}

static int clampInt(int value, int min, int max)
{
    if (value < min)
    {
        return min;
    }

    if (value > max)
    {
        return max;
    }

    return value;
}

static const char *getProtocolName(int protocol)
{
    switch (protocol)
    {
    case 1:
        return "Stop & Wait";
    case 2:
        return "Go-Back-N";
    case 3:
        return "Selective Repeat";
    default:
        return "Unknown";
    }
}