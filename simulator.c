#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "simulator.h"
#include "config.h"
#include "packet.h"
#include "window.h"
#include "ui.h"

static void stopAndWait(void);
static void goBackN(void);
static void selectiveRepeat(void);


static void drawSimulationMenu(const char *errorMessage);
static int readSimulationChoice(void);


static void clearScreen(void);
static void waitMs(int ms);
static void pauseEnter(void);
static int checkLoss(void);

static void drawStopAndWait(Packet packets[], int count, int currentIndex,
                            const char *eventMessage, int sentCount,
                            int ackCount, int retransmissionCount, int retryCount);

static void drawGoBackN(Packet packets[], int count, Window window,
                        int currentIndex, const char *eventMessage,
                        int sentCount, int ackCount, int retransmissionCount);

static void drawSelectiveRepeat(Packet packets[], int count, Window window,
                                int currentIndex, const char *eventMessage,
                                int sentCount, int ackCount, int retransmissionCount);

void runSimulation(void)
{
    int choice;

    while (1)
    {
        choice = readSimulationChoice();

        switch (choice)
        {
        case 1:
            stopAndWait();
            break;

        case 2:
            goBackN();
            break;

        case 3:
            selectiveRepeat();
            break;

        case 0:
            return;
        }
    }
}

static void drawSimulationMenu(const char *errorMessage)
{
    clearScreenUI();

    setColor(UI_GRAY);
    drawBox(2, 1, 112, 27);
    drawLine(5, 4, 109);
    drawLine(5, 9, 109);

    setColor(UI_CYAN);
    printCenter(6, "Simulation Protocol Select", 115);

    setColor(UI_LIGHT_GRAY);
    printCenter(7, "- 전송 프로토콜 선택 -", 115);

    setColor(UI_LIGHT_GRAY);
    drawBox(34, 12, 80, 23);

    setColor(UI_SELECTED);
    printAt(38, 14, " 1. Stop & Wait                    ");

    setColor(UI_WHITE);
    printAt(38, 16, " 2. Go-Back-N                      ");
    printAt(38, 18, " 3. Selective Repeat               ");
    printAt(38, 20, " 0. 메인 메뉴로 돌아가기           ");

    setColor(UI_GRAY);
    drawLine(37, 21, 77);

    setColor(UI_LIGHT_GRAY);
    printAt(38, 22, "프로토콜 선택 : ");

    if (errorMessage != NULL)
    {
        setColor(UI_RED);
        printAt(34, 25, errorMessage);
    }

    resetColor();
}

static int readSimulationChoice(void)
{
    char buffer[50];
    char extra;
    int choice;
    const char *errorMessage = NULL;

    while (1)
    {
        drawSimulationMenu(errorMessage);

        gotoxy(56, 22);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            errorMessage = "입력 오류가 발생했습니다. 다시 입력하세요.";
            continue;
        }

        if (sscanf(buffer, "%d %c", &choice, &extra) != 1)
        {
            errorMessage = "잘못된 입력입니다. 숫자를 입력하세요.";
            continue;
        }

        if (choice >= 0 && choice <= 3)
        {
            return choice;
        }

        errorMessage = "잘못된 메뉴입니다. 0, 1, 2, 3 중에서 입력하세요.";
    }
}

static void stopAndWait(void)
{
    Packet packets[MAX_PACKET];

    int packetCount = config.packetCount;
    int sentCount = 0;
    int ackCount = 0;
    int retransmissionCount = 0;
    int i;

    if (packetCount < 1)
    {
        packetCount = 1;
    }
    else if (packetCount > MAX_PACKET)
    {
        packetCount = MAX_PACKET;
    }

    initializePackets(packets, packetCount);
    srand((unsigned int)time(NULL));

    for (i = 0; i < packetCount; i++)
    {
        int retryCount = 0;

        while (packets[i].acknowledged == 0)
        {
            packets[i].transmitted = 1;
            sentCount++;

            clearScreen();
            drawStopAndWait(
                packets,
                packetCount,
                i,
                "Sending packet...",
                sentCount,
                ackCount,
                retransmissionCount,
                retryCount
            );
            waitMs(config.delay);

            if (checkLoss())
            {
                packets[i].lost = 1;
                packets[i].retransmitted = 1;
                retransmissionCount++;
                retryCount++;

                clearScreen();
                drawStopAndWait(
                    packets,
                    packetCount,
                    i,
                    "Packet lost! Timeout -> Retransmit",
                    sentCount,
                    ackCount,
                    retransmissionCount,
                    retryCount
                );
                waitMs(config.delay);

                packets[i].lost = 0;
            }
            else
            {
                packets[i].acknowledged = 1;
                packets[i].lost = 0;
                ackCount++;

                clearScreen();
                drawStopAndWait(
                    packets,
                    packetCount,
                    i,
                    "ACK received! Move to next packet",
                    sentCount,
                    ackCount,
                    retransmissionCount,
                    retryCount
                );
                waitMs(config.delay);
            }
        }
    }

    clearScreen();
    drawStopAndWait(
        packets,
        packetCount,
        packetCount - 1,
        "Simulation complete!",
        sentCount,
        ackCount,
        retransmissionCount,
        0
    );

    setColor(UI_LIGHT_GRAY);
    printAt(34, 26, "Enter키를 누르면 프로토콜 선택 화면으로 돌아갑니다.");
    resetColor();

    pauseEnter();
}

static void drawStopAndWait(Packet packets[], int count, int currentIndex,
                            const char *eventMessage, int sentCount,
                            int ackCount, int retransmissionCount, int retryCount)
{
    double successRate = 0.0;
    const char *state = "WAIT";
    const char *ackState = "Waiting";

    if (sentCount > 0)
    {
        successRate = ((double)ackCount / sentCount) * 100.0;
    }

    if (currentIndex >= 0 && currentIndex < count)
    {
        if (packets[currentIndex].lost)
        {
            state = "LOST";
            ackState = "No ACK";
        }
        else if (packets[currentIndex].acknowledged)
        {
            state = "ACKED";
            ackState = "ACK Received";
        }
        else if (packets[currentIndex].transmitted)
        {
            state = "SENDING";
            ackState = "Waiting ACK";
        }
    }

    setColor(UI_GRAY);
    drawBox(2, 1, 112, 27);

    setColor(UI_CYAN);
    printAt(5, 3, "Stop & Wait Protocol");

    setColor(UI_LIGHT_GRAY);
    gotoxy(78, 3);
    printf("Packet %d / %d", currentIndex + 1, count);

    setColor(UI_GRAY);
    drawLine(4, 4, 110);

    /*
        상단 왼쪽: 전송 과정
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(4, 6, 72, 15);

    setColor(UI_CYAN);
    printAt(7, 7, "[ 전송 과정 ]");

    setColor(UI_CYAN);
    drawBox(8, 10, 20, 12);
    printAt(10, 11, "Sender");

    setColor(UI_GREEN);
    drawBox(58, 10, 69, 12);
    printAt(60, 11, "Receiver");

    if (currentIndex >= 0 && currentIndex < count)
    {
        if (packets[currentIndex].lost)
        {
            setColor(UI_RED);
            gotoxy(25, 10);
            printf("[Packet %02d] --------X", packets[currentIndex].seqNum);

            gotoxy(31, 12);
            printf("Packet Lost / Timeout");
        }
        else if (packets[currentIndex].acknowledged)
        {
            setColor(UI_GREEN);
            gotoxy(25, 10);
            printf("[Packet %02d] -------------------->", packets[currentIndex].seqNum);

            gotoxy(31, 12);
            printf("<-------------------- ACK %02d", packets[currentIndex].seqNum);
        }
        else
        {
            setColor(UI_YELLOW);
            gotoxy(25, 10);
            printf("[Packet %02d] -------------------->", packets[currentIndex].seqNum);

            gotoxy(31, 12);
            printf("<-------------------- Waiting ACK");
        }
    }

    /*
        상단 오른쪽: 현재 패킷 상태
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(75, 6, 110, 15);

    setColor(UI_CYAN);
    printAt(78, 7, "[ 패킷 상태 ]");

    setColor(UI_WHITE);
    gotoxy(78, 9);
    printf("Packet No   : %02d", packets[currentIndex].seqNum);

    gotoxy(78, 10);
    printf("State       : ");

    if (packets[currentIndex].lost)
    {
        setColor(UI_RED);
    }
    else if (packets[currentIndex].acknowledged)
    {
        setColor(UI_GREEN);
    }
    else
    {
        setColor(UI_YELLOW);
    }

    printf("%s", state);

    setColor(UI_WHITE);
    gotoxy(78, 11);
    printf("ACK         : %s", ackState);

    gotoxy(78, 12);
    printf("Retry Count : %d", retryCount);

    /*
        하단 왼쪽: 통계
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(4, 17, 45, 25);

    setColor(UI_CYAN);
    printAt(7, 18, "[ 통계 ]");

    setColor(UI_WHITE);
    gotoxy(7, 20);
    printf("전송 Packet 수 : %d", sentCount);

    gotoxy(7, 21);
    printf("수신 ACK 수    : %d", ackCount);

    gotoxy(7, 22);
    printf("재전송 횟수    : %d", retransmissionCount);

    gotoxy(7, 23);
    printf("성공률         : %.1f%%", successRate);

    /*
        하단 오른쪽: 이벤트 로그
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(48, 17, 110, 25);

    setColor(UI_CYAN);
    printAt(51, 18, "[ 이벤트 로그 ]");

    setColor(UI_WHITE);
    gotoxy(51, 20);
    printf("%s", eventMessage);

    setColor(UI_GRAY);
    drawLine(51, 22, 106);

    setColor(UI_LIGHT_GRAY);
    gotoxy(51, 23);
    printf("손실률: %d%%   지연 시간: %d ms", config.lossRate, config.delay);

    resetColor();
}

static void drawSelectiveRepeat(Packet packets[], int count, Window window,
                                int currentIndex, const char *eventMessage,
                                int sentCount, int ackCount, int retransmissionCount)
{
    int i;
    int windowEnd = window.base + window.size - 1;
    int visibleCount;
    double successRate = 0.0;

    if (windowEnd >= count)
    {
        windowEnd = count - 1;
    }

    if (sentCount > 0)
    {
        successRate = ((double)ackCount / sentCount) * 100.0;
    }

    visibleCount = count;
    if (visibleCount > 20)
    {
        visibleCount = 20;
    }

    setColor(UI_GRAY);
    drawBox(2, 1, 112, 27);

    setColor(UI_CYAN);
    printAt(5, 3, "Selective Repeat Protocol");

    setColor(UI_LIGHT_GRAY);
    gotoxy(75, 3);
    printf("Base %d / %d", window.base + 1, count);

    setColor(UI_GRAY);
    drawLine(4, 4, 110);

    /*
        상단 왼쪽: Receiver Window
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(4, 6, 72, 15);

    setColor(UI_CYAN);
    printAt(7, 7, "[ Receiver Window ]");

    setColor(UI_LIGHT_GRAY);
    gotoxy(30, 7);
    printf("Window Size = %d", window.size);

    for (i = 0; i < visibleCount; i++)
    {
        int x = 8 + (i % 10) * 6;
        int y = 10 + (i / 10) * 2;

        if (i == currentIndex && packets[i].lost)
        {
            setColor(UI_RED);
        }
        else if (i == currentIndex)
        {
            setColor(UI_SELECTED);
        }
        else if (packets[i].acknowledged)
        {
            setColor(UI_GREEN);
        }
        else if (packets[i].retransmitted)
        {
            setColor(UI_YELLOW);
        }
        else if (i >= window.base && i <= windowEnd)
        {
            setColor(UI_CYAN);
        }
        else
        {
            setColor(UI_GRAY);
        }

        gotoxy(x, y);
        printf("[%02d]", packets[i].seqNum);
        resetColor();
    }

    if (count > 20)
    {
        setColor(UI_LIGHT_GRAY);
        printAt(66, 12, "...");
    }

    setColor(UI_LIGHT_GRAY);
    printAt(8, 14, "Green=ACKED  Cyan=Window  Red=Lost  Yellow=Retransmit");

    /*
        상단 오른쪽: 현재 패킷 상태
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(75, 6, 110, 15);

    setColor(UI_CYAN);
    printAt(78, 7, "[ 패킷 상태 ]");

    setColor(UI_WHITE);
    gotoxy(78, 9);
    printf("Base         : %d", window.base + 1);

    gotoxy(78, 10);
    printf("Next Seq Num : %d", window.nextSeqNum + 1);

    gotoxy(78, 11);
    printf("Window Size  : %d", window.size);

    gotoxy(78, 12);
    printf("Current      : Packet %02d", packets[currentIndex].seqNum);

    gotoxy(78, 13);
    printf("State        : ");

    if (packets[currentIndex].lost)
    {
        setColor(UI_RED);
        printf("LOST");
    }
    else if (packets[currentIndex].acknowledged)
    {
        setColor(UI_GREEN);
        printf("ACKED");
    }
    else if (packets[currentIndex].retransmitted)
    {
        setColor(UI_YELLOW);
        printf("RETX");
    }
    else
    {
        setColor(UI_YELLOW);
        printf("SENDING");
    }

    /*
        하단 왼쪽: 통계
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(4, 17, 45, 25);

    setColor(UI_CYAN);
    printAt(7, 18, "[ 통계 ]");

    setColor(UI_WHITE);
    gotoxy(7, 20);
    printf("전송 Packet 수 : %d", sentCount);

    gotoxy(7, 21);
    printf("수신 ACK 수    : %d", ackCount);

    gotoxy(7, 22);
    printf("재전송 횟수    : %d", retransmissionCount);

    gotoxy(7, 23);
    printf("성공률         : %.1f%%", successRate);

    /*
        하단 오른쪽: 이벤트 로그
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(48, 17, 110, 25);

    setColor(UI_CYAN);
    printAt(51, 18, "[ 이벤트 로그 ]");

    setColor(UI_WHITE);
    gotoxy(51, 20);
    printf("%s", eventMessage);

    setColor(UI_GRAY);
    drawLine(51, 22, 106);

    setColor(UI_LIGHT_GRAY);
    gotoxy(51, 23);
    printf("Selective Repeat: 손실된 Packet만 선택적으로 재전송");

    resetColor();
}

static int checkLoss(void)
{
    if (config.lossRate <= 0)
    {
        return 0;
    }

    if (config.lossRate >= 100)
    {
        return 1;
    }

    return (rand() % 100) < config.lossRate;
}

static void waitMs(int ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

static void pauseEnter(void)
{
    getchar();
}

static void clearScreen(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void goBackN(void)
{
    Packet packets[MAX_PACKET];
    Window window;

    int packetCount = config.packetCount;
    int sentCount = 0;
    int ackCount = 0;
    int retransmissionCount = 0;

    if (packetCount < 1)
    {
        packetCount = 1;
    }
    else if (packetCount > MAX_PACKET)
    {
        packetCount = MAX_PACKET;
    }

    initializePackets(packets, packetCount);
    initializeWindow(&window, config.windowSize);

    if (window.size < 1)
    {
        window.size = 1;
    }

    while (window.base < packetCount)
    {
        int i;
        int windowEnd = window.base + window.size - 1;
        int lossOccurred = 0;

        if (windowEnd >= packetCount)
        {
            windowEnd = packetCount - 1;
        }

        for (i = window.base; i <= windowEnd; i++)
        {
            if (packets[i].acknowledged)
            {
                continue;
            }

            if (packets[i].transmitted)
            {
                packets[i].retransmitted = 1;
                retransmissionCount++;
            }

            packets[i].transmitted = 1;
            sentCount++;

            clearScreen();
            drawGoBackN(
                packets,
                packetCount,
                window,
                i,
                "Sending packets in current window...",
                sentCount,
                ackCount,
                retransmissionCount
            );
            waitMs(config.delay);

            if (checkLoss())
            {
                packets[i].lost = 1;
                lossOccurred = 1;

                clearScreen();
                drawGoBackN(
                    packets,
                    packetCount,
                    window,
                    i,
                    "Packet lost! Go-Back-N will retransmit from this packet.",
                    sentCount,
                    ackCount,
                    retransmissionCount
                );
                waitMs(config.delay);

                packets[i].lost = 0;

                window.base = i;
                break;
            }
            else
            {
                packets[i].acknowledged = 1;
                ackCount++;

                clearScreen();
                drawGoBackN(
                    packets,
                    packetCount,
                    window,
                    i,
                    "ACK received.",
                    sentCount,
                    ackCount,
                    retransmissionCount
                );
                waitMs(config.delay);
            }
        }

        if (!lossOccurred)
        {
            while (window.base < packetCount && packets[window.base].acknowledged)
            {
                window.base++;
            }
        }

        window.nextSeqNum = window.base;
    }

    clearScreen();
    drawGoBackN(
        packets,
        packetCount,
        window,
        packetCount - 1,
        "Go-Back-N simulation complete!",
        sentCount,
        ackCount,
        retransmissionCount
    );

    setColor(UI_LIGHT_GRAY);
    printAt(34, 26, "Enter키를 누르면 프로토콜 선택 화면으로 돌아갑니다.");
    resetColor();

    pauseEnter();

}

static void drawGoBackN(Packet packets[], int count, Window window,
                        int currentIndex, const char *eventMessage,
                        int sentCount, int ackCount, int retransmissionCount)
{
    int i;
    int windowEnd = window.base + window.size - 1;
    int visibleCount;
    double successRate = 0.0;

    if (windowEnd >= count)
    {
        windowEnd = count - 1;
    }

    if (sentCount > 0)
    {
        successRate = ((double)ackCount / sentCount) * 100.0;
    }

    visibleCount = count;
    if (visibleCount > 20)
    {
        visibleCount = 20;
    }

    setColor(UI_GRAY);
    drawBox(2, 1, 112, 27);

    setColor(UI_GREEN);
    printAt(5, 3, "Go-Back-N Protocol");

    setColor(UI_LIGHT_GRAY);
    gotoxy(75, 3);
    printf("Base %d / %d", window.base + 1, count);

    setColor(UI_GRAY);
    drawLine(4, 4, 110);

    /*
        상단 왼쪽: Sender Window
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(4, 6, 72, 15);

    setColor(UI_CYAN);
    printAt(7, 7, "[ Sender Window ]");

    setColor(UI_LIGHT_GRAY);
    gotoxy(28, 7);
    printf("Window Size = %d", window.size);

    for (i = 0; i < visibleCount; i++)
    {
        int x = 8 + (i % 10) * 6;
        int y = 10 + (i / 10) * 2;

        if (i == currentIndex && packets[i].lost)
        {
            setColor(UI_RED);
        }
        else if (i == currentIndex)
        {
            setColor(UI_SELECTED);
        }
        else if (packets[i].acknowledged)
        {
            setColor(UI_GREEN);
        }
        else if (i >= window.base && i <= windowEnd)
        {
            setColor(UI_CYAN);
        }
        else
        {
            setColor(UI_GRAY);
        }

        gotoxy(x, y);
        printf("[%02d]", packets[i].seqNum);
        resetColor();
    }

    if (count > 20)
    {
        setColor(UI_LIGHT_GRAY);
        printAt(66, 12, "...");
    }

    setColor(UI_LIGHT_GRAY);
    printAt(8, 14, "Green=ACKED  Cyan=Window  Red=Lost  Blue=Current");

    /*
        상단 오른쪽: 현재 상태
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(75, 6, 110, 15);

    setColor(UI_CYAN);
    printAt(78, 7, "[ 패킷 상태 ]");

    setColor(UI_WHITE);
    gotoxy(78, 9);
    printf("Base         : %d", window.base + 1);

    gotoxy(78, 10);
    printf("Next Seq Num : %d", window.nextSeqNum + 1);

    gotoxy(78, 11);
    printf("Window Size  : %d", window.size);

    gotoxy(78, 12);
    printf("Current      : Packet %02d", packets[currentIndex].seqNum);

    gotoxy(78, 13);
    printf("State        : ");

    if (packets[currentIndex].lost)
    {
        setColor(UI_RED);
        printf("LOST");
    }
    else if (packets[currentIndex].acknowledged)
    {
        setColor(UI_GREEN);
        printf("ACKED");
    }
    else
    {
        setColor(UI_YELLOW);
        printf("SENDING");
    }

    /*
        하단 왼쪽: 통계
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(4, 17, 45, 25);

    setColor(UI_CYAN);
    printAt(7, 18, "[ 통계 ]");

    setColor(UI_WHITE);
    gotoxy(7, 20);
    printf("전송 Packet 수 : %d", sentCount);

    gotoxy(7, 21);
    printf("수신 ACK 수    : %d", ackCount);

    gotoxy(7, 22);
    printf("재전송 횟수    : %d", retransmissionCount);

    gotoxy(7, 23);
    printf("성공률         : %.1f%%", successRate);

    /*
        하단 오른쪽: 이벤트 로그
    */
    setColor(UI_LIGHT_GRAY);
    drawBox(48, 17, 110, 25);

    setColor(UI_CYAN);
    printAt(51, 18, "[ 이벤트 로그 ]");

    setColor(UI_WHITE);
    gotoxy(51, 20);
    printf("%s", eventMessage);

    setColor(UI_GRAY);
    drawLine(51, 22, 106);

    setColor(UI_LIGHT_GRAY);
    gotoxy(51, 23);
    printf("Go-Back-N: 손실 발생 시 해당 Packet부터 다시 전송");

    resetColor();
}

static void selectiveRepeat(void)
{
    Packet packets[MAX_PACKET];
    Window window;

    int packetCount = config.packetCount;
    int sentCount = 0;
    int ackCount = 0;
    int retransmissionCount = 0;

    if (packetCount < 1)
    {
        packetCount = 1;
    }
    else if (packetCount > MAX_PACKET)
    {
        packetCount = MAX_PACKET;
    }

    initializePackets(packets, packetCount);
    initializeWindow(&window, config.windowSize);

    if (window.size < 1)
    {
        window.size = 1;
    }

    while (window.base < packetCount)
    {
        int i;
        int windowEnd = window.base + window.size - 1;

        if (windowEnd >= packetCount)
        {
            windowEnd = packetCount - 1;
        }

        for (i = window.base; i <= windowEnd; i++)
        {
            if (packets[i].acknowledged)
            {
                continue;
            }

            if (packets[i].transmitted)
            {
                packets[i].retransmitted = 1;
                retransmissionCount++;
            }

            packets[i].transmitted = 1;
            sentCount++;

            clearScreen();
            drawSelectiveRepeat(
                packets,
                packetCount,
                window,
                i,
                "Sending packet in receiver window...",
                sentCount,
                ackCount,
                retransmissionCount
            );
            waitMs(config.delay);

            if (checkLoss())
            {
                packets[i].lost = 1;

                clearScreen();
                drawSelectiveRepeat(
                    packets,
                    packetCount,
                    window,
                    i,
                    "Packet lost! Selective Repeat will retransmit only this packet.",
                    sentCount,
                    ackCount,
                    retransmissionCount
                );
                waitMs(config.delay);

                packets[i].lost = 0;
            }
            else
            {
                packets[i].acknowledged = 1;
                packets[i].lost = 0;
                ackCount++;

                clearScreen();
                drawSelectiveRepeat(
                    packets,
                    packetCount,
                    window,
                    i,
                    "ACK received. Other packets do not need retransmission.",
                    sentCount,
                    ackCount,
                    retransmissionCount
                );
                waitMs(config.delay);
            }
        }

        while (window.base < packetCount && packets[window.base].acknowledged)
        {
            window.base++;
        }

        window.nextSeqNum = window.base;
    }

    clearScreen();
    drawSelectiveRepeat(
        packets,
        packetCount,
        window,
        packetCount - 1,
        "Selective Repeat simulation complete!",
        sentCount,
        ackCount,
        retransmissionCount
    );

    setColor(UI_LIGHT_GRAY);
    printAt(34, 26, "Enter키를 누르면 프로토콜 선택 화면으로 돌아갑니다.");
    resetColor();

    pauseEnter();
}