#include <graphics.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include <algorithm>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#define _CRT_SECURE_NO_WARNINGS

// 游戏参数
#define BOARD_SIZE 15
#define CELL_SIZE 35
#define BOARD_OFFSET 50
#define PIECE_RADIUS 15
#define WIN_COUNT 6
#define AI_SEARCH_DEPTH 2
#define MAX_HISTORY 200

// 游戏模式
enum GameMode { MENU, PVP, PVE, REPLAY };

//加一个结构体保存回放用的棋谱
#define MAX_REPLAY_MOVES 256
typedef struct {
    int x, y, player;
} ReplayMove;

ReplayMove replayMovesArr[MAX_REPLAY_MOVES];
int replayMoveCount = 0;
int replayCurrentStep = 0;
DWORD replayLastTick = 0;

// 游戏状态
typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int currentPlayer;
    time_t startTime;
    time_t lastUpdateTime;
    int gameOver;
    int pvpUser1Wins;
    int pvpUser2Wins;
    int pveUserWins;
    int pveAIWins;
    GameMode mode;
    bool gameStarted;
    int aiDifficulty;
    bool aiIsBlack;
    bool isAITurn;
    int turnStep;
    int turnQuota;
    int lastWinner;
    int volume; // 0~100
} GameState;

// 初始化游戏
void initGame(GameState* game) {
    memset(game->board, 0, sizeof(game->board));
	game->currentPlayer = 1;// 默认用户1执黑
    game->startTime = 0;
    game->lastUpdateTime = 0;
    game->gameOver = 0;
    game->pvpUser1Wins = 0;
    game->pvpUser2Wins = 0;
    game->pveUserWins = 0;
    game->pveAIWins = 0;
	game->gameStarted = false;// 游戏未开始
	game->aiDifficulty = 2;// 默认中等难度
	game->aiIsBlack = false; // 默认AI执白
    game->isAITurn = false;
    game->turnStep = 0;
	game->lastWinner = 0;// 0表示没有胜者
    game->turnQuota = 1; // 黑方第一次只下一子
}

// 设置音量，vol 范围 0~100
void setVolume(int vol) {
    DWORD dwVolume = (DWORD)(0xFFFF * vol / 100); // 0~0xFFFF
    // 左右声道都设置
    waveOutSetVolume(0, (dwVolume & 0xFFFF) | (dwVolume << 16));
}

//把棋谱文件读到replayMovesArr里
bool loadReplayFile(const char* filename) {
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "r");
    if (!fp) return false;
    replayMoveCount = 0;
    int x, y, player;
    while (fscanf_s(fp, "%d %d %d", &x, &y, &player) == 3 && replayMoveCount < MAX_REPLAY_MOVES) {
        replayMovesArr[replayMoveCount].x = x;
        replayMovesArr[replayMoveCount].y = y;
        replayMovesArr[replayMoveCount].player = player;
        replayMoveCount++;
    }
    fclose(fp);
    replayCurrentStep = 0;
    return replayMoveCount > 0;
}
// 保存当前棋局
bool saveGame(const GameState* game, const char* filename) {
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "wb");
    if (!fp) return false;
    fwrite(game, sizeof(GameState), 1, fp);
    fclose(fp);
    return true;
}

bool loadGame(GameState* game, const char* filename) {
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "rb");
    if (!fp) return false;
    fread(game, sizeof(GameState), 1, fp);
    fclose(fp);
    return true;
}

// 追加记录每步落子
void saveMove(int x, int y, int player, const char* filename) {
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "a");
    if (!fp) return;
    fprintf(fp, "%d %d %d\n", x, y, player);
    fclose(fp);
}

// 重置游戏(不重置战绩)
void resetGame(GameState* game) {
    memset(game->board, 0, sizeof(game->board));
    game->startTime = time(NULL);
    game->lastUpdateTime = 0;
    game->gameOver = 0;
    game->gameStarted = true;
    game->lastWinner = 0;

    if (game->mode == PVE) {
        // 固定用户执黑(1)，AI执白(2)
        game->aiIsBlack = false;
        game->currentPlayer = 1; // 总是用户(黑棋)先手
        game->isAITurn = false;
    }
    else {
        game->currentPlayer = 1; // PVP模式用户1执黑
        game->isAITurn = false;
    }
    game->turnStep = 0;
    game->turnQuota = 1; // 黑方第一次只下一子
}

// 读取棋谱并回放
bool loadMoves(GameState* game, const char* filename) {
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "r");
    if (!fp) return false;
    resetGame(game);
    int x, y, player;
    while (fscanf_s(fp, "%d %d %d", &x, &y, &player) == 3) {
        game->board[x][y] = player;
    }
    fclose(fp);
    return true;
}

// 结束游戏(清空棋盘和计时，不计入战绩)
void endGame(GameState* game) {
    memset(game->board, 0, sizeof(game->board));
    game->currentPlayer = 1;
    game->startTime = 0;
    game->lastUpdateTime = 0;
    game->gameOver = 0;
    game->lastWinner = 0;
    game->gameStarted = false;
}

// 绘制棋盘
void drawChessBoard() {
    setbkcolor(RGB(240, 218, 181));
    cleardevice();

    setlinecolor(RGB(101, 67, 33));
    setlinestyle(PS_SOLID, 2);

    for (int i = 0; i < BOARD_SIZE; i++) {
        line(BOARD_OFFSET, BOARD_OFFSET + i * CELL_SIZE,
            BOARD_OFFSET + (BOARD_SIZE - 1) * CELL_SIZE, BOARD_OFFSET + i * CELL_SIZE);
        line(BOARD_OFFSET + i * CELL_SIZE, BOARD_OFFSET,
            BOARD_OFFSET + i * CELL_SIZE, BOARD_OFFSET + (BOARD_SIZE - 1) * CELL_SIZE);
    }

    // 绘制星位
    int stars[5][2] = { {3,3},{11,3},{7,7},{3,11},{11,11} };
    setfillcolor(BLACK);
    for (int i = 0; i < 5; i++) {
        fillcircle(BOARD_OFFSET + stars[i][0] * CELL_SIZE,
            BOARD_OFFSET + stars[i][1] * CELL_SIZE, 4);
    }
}

// 绘制棋子
void drawPiece(int x, int y, int player) {
    DWORD color, edgeColor;

    // PVE模式下明确颜色归属
    if (player == 1) {
        color = RGB(30, 30, 30);    // 黑棋
        edgeColor = RGB(10, 10, 10);
    }
    else {
        color = RGB(230, 230, 230); // 白棋
        edgeColor = RGB(200, 200, 200);
    }

    int posX = BOARD_OFFSET + x * CELL_SIZE;
    int posY = BOARD_OFFSET + y * CELL_SIZE;

    setfillcolor(color);
    setlinecolor(edgeColor);
    fillcircle(posX, posY, PIECE_RADIUS);
}

// 快速胜利检查
bool checkWinFast(GameState* game, int x, int y) {
    int dir[4][2] = { {1,0}, {0,1}, {1,1}, {1,-1} };
    int player = game->board[x][y];

    for (int d = 0; d < 4; d++) {
        int count = 1;
        for (int i = 1; i < WIN_COUNT; i++) {
            int nx = x + i * dir[d][0], ny = y + i * dir[d][1];
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE ||
                game->board[nx][ny] != player) break;
            count++;
        }
        for (int i = 1; i < WIN_COUNT; i++) {
            int nx = x - i * dir[d][0], ny = y - i * dir[d][1];
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE ||
                game->board[nx][ny] != player) break;
            count++;
        }
        if (count >= WIN_COUNT) return true;
    }
    return false;
}

// 评估函数
int evaluatePosition(GameState* game, int player) {
    int score = 0;
    int dir[4][2] = { {1,0}, {0,1}, {1,1}, {1,-1} };

    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            if (game->board[x][y] != 0) continue;

            for (int d = 0; d < 4; d++) {
                int count = 0, empty = 0;
                for (int i = 1; i < WIN_COUNT; i++) {
                    int nx = x + i * dir[d][0], ny = y + i * dir[d][1];
                    if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
                    if (game->board[nx][ny] == player) count++;
                    else if (game->board[nx][ny] == 0) empty++;
                    else break;
                }
                for (int i = 1; i < WIN_COUNT; i++) {
                    int nx = x - i * dir[d][0], ny = y - i * dir[d][1];
                    if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
                    if (game->board[nx][ny] == player) count++;
                    else if (game->board[nx][ny] == 0) empty++;
                    else break;
                }

                if (count >= WIN_COUNT - 1) return 100000;
                if (count == WIN_COUNT - 2 && empty >= 1) score += 5000;
                else if (count == WIN_COUNT - 3) score += 1000;
                else if (count > 0) score += count * 10;
            }
        }
    }
    return score;
}

// AI移动函数
void makeAIMove(GameState* game) {
    if (!game->isAITurn || game->gameOver) return;

    int aiPlayer = game->aiIsBlack ? 1 : 2;
    int humanPlayer = game->aiIsBlack ? 2 : 1;

    // 1. 检查AI能否直接获胜
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            if (game->board[x][y] == 0) {
                game->board[x][y] = aiPlayer;
                if (checkWinFast(game, x, y)) {

                    saveMove(x, y, aiPlayer, "record.txt");
                    //PlaySound(TEXT("move.wav"), NULL, SND_FILENAME | SND_ASYNC);

                    game->gameOver = 1;
                    game->lastUpdateTime = time(NULL);
                    game->pveAIWins++;
                    game->currentPlayer = 2;
                    game->lastWinner = 2; // AI胜利
                    return;
                }
                game->board[x][y] = 0;
            }
        }
    }

    // 2. 检查是否需要防守玩家即将获胜的位置
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            if (game->board[x][y] == 0) {
                game->board[x][y] = humanPlayer;
                bool needBlock = checkWinFast(game, x, y);
                game->board[x][y] = 0;
                if (needBlock) {
                    game->board[x][y] = aiPlayer;
                    saveMove(x, y, aiPlayer, "record.txt");
                    return;
                }
            }
        }
    }

    // 3. 进攻与防守双重评分，选最优点
    int bestX = -1, bestY = -1;
    int bestScore = -1000000;
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            if (game->board[x][y] == 0) {
                game->board[x][y] = aiPlayer;
                int attackScore = evaluatePosition(game, aiPlayer);
                game->board[x][y] = 0;
                game->board[x][y] = humanPlayer;
                int defendScore = evaluatePosition(game, humanPlayer);
                game->board[x][y] = 0;
                int centerDist = abs(x - BOARD_SIZE / 2) + abs(y - BOARD_SIZE / 2);
                int centerScore = (BOARD_SIZE - centerDist) * 5;
                int score = attackScore * 2 + defendScore + centerScore;
                if (game->aiDifficulty == 1) score += rand() % 1000;
                else if (game->aiDifficulty == 2) score += rand() % 100;
                if (score > bestScore || (score == bestScore && rand() % 2 == 0)) {
                    bestScore = score;
                    bestX = x;
                    bestY = y;
                }
            }
        }
    }

    // 4. 落子
    if (bestX != -1 && bestY != -1) {
        game->board[bestX][bestY] = aiPlayer;
        saveMove(bestX, bestY, aiPlayer, "record.txt");
        if (checkWinFast(game, bestX, bestY)) {
            game->gameOver = 1;
            game->lastUpdateTime = time(NULL);
            game->pveAIWins++;
            game->currentPlayer = 2;
            game->lastWinner = 2; // AI胜利
        }
        return;
    }
}

// 绘制按钮
void drawButton(int x, int y, int width, int height, const char* text, bool hover) {
    // 按钮背景色
    static DWORD bgColors[2] = { RGB(255, 255, 255), RGB(230, 240, 255) };//白色，RGB(230, 240, 255)蓝色
    // 按钮边框色
    static DWORD edgeColors[2] = { RGB(150, 150, 150), RGB(0, 100, 200) };//灰色，RGB(0, 100, 200)淡蓝色
    // 文字颜色
    static DWORD textColors[2] = { RGB(50, 50, 50), RGB(0, 0, 100) };//深灰色，RGB(0, 0, 100)淡蓝色

    // 绘制按钮背景
    setfillcolor(bgColors[hover ? 1 : 0]);
    fillrectangle(x, y, x + width, y + height);

    // 绘制按钮边框
    setlinecolor(edgeColors[hover ? 1 : 0]);//hover为1时边框颜色为淡蓝色表示被选中
    setlinestyle(PS_SOLID, 2);
    rectangle(x, y, x + width, y + height);

    // 绘制按钮文字
    setbkmode(TRANSPARENT); // 设置文字背景为透明
    settextcolor(textColors[hover ? 1 : 0]);
    settextstyle(25, 0, _T("微软雅黑"));
    RECT r = { x, y, x + width, y + height };
    drawtext(text, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//居中对齐
}

// 绘制菜单界面
void drawMenu(GameState* game) {
    setbkcolor(RGB(240, 218, 181));
    cleardevice();

    // 标题
    settextcolor(RGB(80, 40, 0));
    settextstyle(80, 0, _T("华文隶书"));
    RECT titleRect = { 0, 60, 800, 160 };
    drawtext(_T("六子棋"), &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 音量显示
    TCHAR volText[32];
    _stprintf_s(volText, _countof(volText), _T("音量: %d"), game->volume);
    settextstyle(23, 0, _T("华文琥珀"));
    outtextxy(30, 18, volText);

    // “-”按钮
    drawButton(120, 15, 28, 28, _T("-"), false);
    // “+”按钮
    drawButton(155, 15, 28, 28, _T("+"), false);

    // 按钮悬停检测
    MOUSEMSG mouseMsg;
    bool pvpHover = false, pveHover = false, exitHover = false;
    bool diffHover[3] = { false };

    if (MouseHit()) {
        mouseMsg = GetMouseMsg();
        pvpHover = (mouseMsg.x >= 300 && mouseMsg.x <= 500 && mouseMsg.y >= 200 && mouseMsg.y <= 250);
        pveHover = (mouseMsg.x >= 300 && mouseMsg.x <= 500 && mouseMsg.y >= 270 && mouseMsg.y <= 320);
        exitHover = (mouseMsg.x >= 300 && mouseMsg.x <= 500 && mouseMsg.y >= 340 && mouseMsg.y <= 390);

        for (int i = 0; i < 3; i++) {
            diffHover[i] = (mouseMsg.x >= 420 + i * 60 && mouseMsg.x <= 470 + i * 60 &&
                mouseMsg.y >= 420 && mouseMsg.y <= 470);
        }
    }
    // 绘制按钮
    drawButton(300, 200, 200, 50, _T("双人对战"), pvpHover);
    drawButton(300, 270, 200, 50, _T("人机对战"), pveHover);
    drawButton(300, 340, 200, 50, _T("退出游戏"), exitHover);

    // 难度选择
    settextstyle(26, 0, _T("华文琥珀"));
    int buttonWidth = 50;
    int buttonHeight = 50;
    int buttonGap = 20;
    int totalWidth = 3 * buttonWidth + 2 * buttonGap;
    int startX = (800 - totalWidth) / 2;
    int y = 420;

    outtextxy(startX - 110, y + 10, _T("AI难度:"));

    for (int i = 0; i < 3; i++) {
        int btnX1 = startX + i * (buttonWidth + buttonGap);
        int btnY1 = y;
        int btnX2 = btnX1 + buttonWidth;
        int btnY2 = btnY1 + buttonHeight;

        setfillcolor(diffHover[i] ? RGB(200, 230, 255) :
            (game->aiDifficulty == i + 1 ? RGB(180, 210, 240) : RGB(250, 250, 250)));
        fillrectangle(btnX1, btnY1, btnX2, btnY2);

        setlinecolor(diffHover[i] ? RGB(0, 100, 200) : RGB(150, 150, 150));
        rectangle(btnX1, btnY1, btnX2, btnY2);

        TCHAR level[2] = { _T('1') + i, 0 };// 1,2,3
        settextcolor(BLACK);
        RECT r = { btnX1, btnY1, btnX2, btnY2 };
        drawtext(level, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // 历史战绩
    TCHAR scoreText[150] = { 0 };
    _stprintf_s(scoreText, _countof(scoreText),
        _T("双人对战 : 用户A-用户B  %d - %d   人机对战 : 用户-AI  %d - %d "),
        game->pvpUser1Wins, game->pvpUser2Wins,
        game->pveUserWins, game->pveAIWins);
    settextstyle(24, 0, _T("微软雅黑"));
    outtextxy(180, 500, scoreText);
}

// 绘制游戏界面（优化版）
void drawGame(GameState* game) {
    setbkcolor(RGB(240, 218, 181));
    cleardevice();

    drawChessBoard();

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] != 0) {
                drawPiece(i, j, game->board[i][j]);
            }
        }
    }

    setfillcolor(RGB(255, 255, 255));
    solidrectangle(600, 0, 800, 600);

    setfillcolor(RGB(245, 245, 245));
    fillrectangle(610, 20, 790, 300);

    settextcolor(RGB(50, 50, 50));
    settextstyle(25, 0, _T("华文行楷"));

    time_t currentTime = game->gameStarted ?
        (game->gameOver ? game->lastUpdateTime - game->startTime : time(NULL) - game->startTime) : 0;

    TCHAR infoText[256] = { 0 };
    _stprintf_s(infoText, _countof(infoText),
        _T("游戏模式: %s\n\n当前回合: %s\n\n用时: %02d:%02d"),
        game->mode == PVP ? _T("双人对战") : _T("人机对战"),
        game->gameOver ? _T("游戏结束") :
        (game->mode == PVP ?
            (game->currentPlayer == 1 ? _T("用户A(黑)") : _T("用户B(白)")) :
            (game->isAITurn ? _T("AI(白)") : _T("用户(黑)"))),
        (int)(currentTime / 60), (int)(currentTime % 60));

    RECT infoRect = { 620, 30, 780, 200 };
    drawtext(infoText, &infoRect, DT_LEFT | DT_TOP | DT_WORDBREAK);

    // 胜负结果
    int resultY = 220;
    if (game->gameOver) {
        settextstyle(28, 0, _T("华文琥珀"));
        settextcolor(game->lastWinner == 1 ? RGB(180, 0, 0) : RGB(0, 130, 0));
        TCHAR resultText[50] = { 0 };
        if (game->mode == PVP) {
            _stprintf_s(resultText, _countof(resultText),
                _T("%s胜利!"),
                game->lastWinner == 1 ? _T("用户A") : _T("用户B"));
        }
        else {
            _stprintf_s(resultText, _countof(resultText),
                _T("%s胜利!"),
                game->lastWinner == 1 ? _T("用户") : _T("AI"));
        }
        outtextxy(620 + (150 - 100) / 2, resultY, resultText);
    }

    // 记谱和打谱按钮各60宽，间隔10像素
    int btnTotalW = 130; // 60+10+60
    int btnStartX = 620 + (150 - btnTotalW) / 2;
    int btnY = 260;
    int btnW = 60, btnH = 36;
    int btnGap = 10;

    // 记谱按钮
    bool exportHover = false, replayHover = false;
    MOUSEMSG mouseMsg;
    if (MouseHit()) {
        mouseMsg = GetMouseMsg();
        exportHover = (mouseMsg.x >= btnStartX && mouseMsg.x <= btnStartX + btnW &&
            mouseMsg.y >= btnY && mouseMsg.y <= btnY + btnH);
        replayHover = (mouseMsg.x >= btnStartX + btnW + btnGap && mouseMsg.x <= btnStartX + btnW + btnGap + btnW &&
            mouseMsg.y >= btnY && mouseMsg.y <= btnY + btnH);
    }
    drawButton(btnStartX, btnY, btnW, btnH, _T("记谱"), exportHover);
    drawButton(btnStartX + btnW + btnGap, btnY, btnW, btnH, _T("打谱"), replayHover);

    // 其它功能按钮
    setfillcolor(RGB(245, 245, 245));
    fillrectangle(610, 320, 790, 598);

    bool menuHover = false, startHover = false, endHover = false;
    if (MouseHit()) {
        mouseMsg = GetMouseMsg();
        menuHover = (mouseMsg.x >= 620 && mouseMsg.x <= 770 && mouseMsg.y >= 330 && mouseMsg.y <= 370);
        startHover = (mouseMsg.x >= 620 && mouseMsg.x <= 770 && mouseMsg.y >= 380 && mouseMsg.y <= 420);
        endHover = (mouseMsg.x >= 620 && mouseMsg.x <= 770 && mouseMsg.y >= 430 && mouseMsg.y <= 470);
    }
    drawButton(620, 330, 150, 40, _T("返回菜单"), menuHover);
    drawButton(620, 380, 150, 40, game->gameStarted ? _T("重新开始") : _T("开始游戏"), startHover);
    drawButton(620, 430, 150, 40, _T("结束对战"), endHover);

    // 存盘、读盘按钮（下方，宽150，左对齐620）
    int saveBtnW = 150, saveBtnH = 40;
    int saveBtnX = 620;
    int saveBtnY = 500;
    int loadBtnY = saveBtnY + saveBtnH + 10;

    bool saveHover = false, loadHover = false;
    if (MouseHit()) {
        mouseMsg = GetMouseMsg();
        saveHover = (mouseMsg.x >= saveBtnX && mouseMsg.x <= saveBtnX + saveBtnW &&
            mouseMsg.y >= saveBtnY && mouseMsg.y <= saveBtnY + saveBtnH);
        loadHover = (mouseMsg.x >= saveBtnX && mouseMsg.x <= saveBtnX + saveBtnW &&
            mouseMsg.y >= loadBtnY && mouseMsg.y <= loadBtnY + saveBtnH);
    }
    drawButton(saveBtnX, saveBtnY, saveBtnW, saveBtnH, _T("存盘"), saveHover);
    drawButton(saveBtnX, loadBtnY, saveBtnW, saveBtnH, _T("读盘"), loadHover);
}


void inputFileName(char* filename, size_t size, const char* prompt) {
    // 用 InputBox 弹窗输入
    TCHAR tfilename[260] = { 0 };
    int ret = InputBox(tfilename, size, _T("请输入文件名（如 save1.dat）"), _T("文件名输入"), _T(""), 0, 0, false);
    if (ret == 0 || tfilename[0] == 0) {
        filename[0] = '\0'; // 用户取消或未输入
        return;
    }
#ifdef UNICODE
    // 如果你的项目是 UNICODE，需要转换
    wcstombs(filename, tfilename, size);
#else
    strncpy_s(filename, size, tfilename, _TRUNCATE);
    filename[size - 1] = '\0';
#endif
}

// 处理菜单点击
void handleMenuClick(GameState* game, int x, int y) {

    // 音量减
    if (x >= 120 && x <= 148 && y >= 15 && y <= 43) {
        if (game->volume > 0) game->volume -= 10;
        if (game->volume < 0) game->volume = 0;
        setVolume(game->volume);
        return;
    }
    // 音量加
    if (x >= 155 && x <= 183 && y >= 15 && y <= 43) {
        if (game->volume < 100) game->volume += 10;
        if (game->volume > 100) game->volume = 100;
        setVolume(game->volume);
        return;
    }

	// 菜单按钮点击
    if (x >= 300 && x <= 500) {
        if (y >= 200 && y <= 250) {
            game->mode = PVP;
            resetGame(game);
        }
        else if (y >= 270 && y <= 320) {
            game->mode = PVE;
            resetGame(game);
        }
        else if (y >= 340 && y <= 390) {
            exit(0);
        }
    }

    // 难度选择
    for (int i = 0; i < 3; i++) {
        if (x >= 305 + i * 70 && x <= 355 + i * 70 && y >= 420 && y <= 470) {
            game->aiDifficulty = i + 1;
            break;
        }
    }
}

// 逐步打谱回放
bool replayMoves(GameState* game, const char* filename) {
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "r");
    if (!fp) return false;
    resetGame(game);
    int x, y, player;
    // 先清空棋盘并刷新
    drawGame(game);
    FlushBatchDraw();
    Sleep(500);

    while (fscanf_s(fp, "%d %d %d", &x, &y, &player) == 3) {
        game->board[x][y] = player;
        drawGame(game);
        FlushBatchDraw();
        Sleep(500); // 每步间隔0.5秒
    }
    fclose(fp);
    return true;
}

// 处理游戏点击
void handleGameClick(GameState* game, int x, int y) {
    // 右侧面板按钮区域参数
    // “返回菜单”“重新开始”“结束对战”按钮
    int btnMenuX = 620, btnMenuW = 150, btnMenuH = 40;
    int btnMenuY = 330, btnStartY = 380, btnEndY = 430;

    // 胜利提示下方“打谱回放”按钮
    int replayBtnW = 120, replayBtnH = 36;
    int replayBtnX = 620 + (150 - replayBtnW) / 2;
    int replayBtnY = 260;

    // 存盘、读盘按钮（下方，宽150，左对齐620）
    int saveBtnW = 150, saveBtnH = 40;
    int saveBtnX = 620;
    int saveBtnY = 500;
    int loadBtnY = saveBtnY + saveBtnH + 10;

	// 记谱打谱按钮
    int btnTotalW = 130; // 60+10+60
    int btnStartX = 620 + (150 - btnTotalW) / 2;
    int btnY = 260;
    int btnW = 60, btnH = 36;
    int btnGap = 10;

    // 右侧面板区域
    if (x >= 600) {
        // 返回菜单
        if (x >= btnMenuX && x <= btnMenuX + btnMenuW && y >= btnMenuY && y <= btnMenuY + btnMenuH) {
            game->mode = MENU;
            return;
        }
        // 重新开始/开始游戏
        else if (x >= btnMenuX && x <= btnMenuX + btnMenuW && y >= btnStartY && y <= btnStartY + btnMenuH) {
            if (!game->gameStarted) {
                game->gameStarted = true;
                game->startTime = time(NULL);
                game->currentPlayer = 1;
                game->isAITurn = false;
                if (game->mode == PVE) {
                    game->aiIsBlack = false;
                }
                FILE* fp = nullptr;
                fopen_s(&fp, "record.txt", "w");
                if (fp) fclose(fp);
            }
            else {
                resetGame(game);
                FILE* fp = nullptr;
                fopen_s(&fp, "record.txt", "w");
                if (fp) fclose(fp);
            }
            return;
        }
        // 结束对战
        else if (x >= btnMenuX && x <= btnMenuX + btnMenuW && y >= btnEndY && y <= btnEndY + btnMenuH) {
            endGame(game);
            return;
        }
        // 记谱按钮
        if (x >= btnStartX && x <= btnStartX + btnW && y >= btnY && y <= btnY + btnH) {
            char filename[100];
            inputFileName(filename, sizeof(filename), "输入要保存的棋谱文件名: ");
            if (filename[0] == '\0') return;
            FILE* src = nullptr, * dst = nullptr;
            fopen_s(&src, "record.txt", "r");
            fopen_s(&dst, filename, "w");
            if (src && dst) {
                char buf[256];
                while (fgets(buf, sizeof(buf), src)) {
                    fputs(buf, dst);
                }
                fclose(src);
                fclose(dst);
                MessageBox(GetHWnd(), _T("棋谱保存成功！"), _T("提示"), MB_OK);
            }
            else {
                if (src) fclose(src);
                if (dst) fclose(dst);
                MessageBox(GetHWnd(), _T("棋谱保存失败！"), _T("错误"), MB_OK);
            }
            return;
        }
        // 打谱按钮
        else if (x >= btnStartX + btnW + btnGap && x <= btnStartX + btnW + btnGap + btnW &&
            y >= btnY && y <= btnY + btnH) {
            char filename[100];
            inputFileName(filename, sizeof(filename), "输入棋谱文件名: ");
            if (loadReplayFile(filename)) {
                resetGame(game);
                game->mode = REPLAY;
                replayLastTick = GetTickCount();
            }
            else {
                MessageBox(GetHWnd(), _T("打谱失败！"), _T("错误"), MB_OK);
            }
            return;
        }

        // 存盘
        else if (x >= saveBtnX && x <= saveBtnX + saveBtnW && y >= saveBtnY && y <= saveBtnY + saveBtnH) {
            char filename[100];
            inputFileName(filename, sizeof(filename), "输入存盘文件名: ");
            if (saveGame(game, filename)) {
                MessageBox(GetHWnd(), _T("存盘成功！"), _T("提示"), MB_OK);
            }
            else {
                MessageBox(GetHWnd(), _T("存盘失败！"), _T("错误"), MB_OK);
            }
            return;
        }
        // 读盘
        else if (x >= saveBtnX && x <= saveBtnX + saveBtnW && y >= loadBtnY && y <= loadBtnY + saveBtnH) {
            char filename[100];
            inputFileName(filename, sizeof(filename), "输入读盘文件名: ");
            if (loadGame(game, filename)) {
                MessageBox(GetHWnd(), _T("读盘成功！"), _T("提示"), MB_OK);
            }
            else {
                MessageBox(GetHWnd(), _T("读盘失败！"), _T("错误"), MB_OK);
            }
            return;
        }
    }

    // 棋盘落子处理
    if (game->gameStarted && !game->gameOver && !game->isAITurn) {
        int boardX = (x - BOARD_OFFSET + CELL_SIZE / 2) / CELL_SIZE;
        int boardY = (y - BOARD_OFFSET + CELL_SIZE / 2) / CELL_SIZE;

        if (boardX >= 0 && boardX < BOARD_SIZE &&
            boardY >= 0 && boardY < BOARD_SIZE &&
            game->board[boardX][boardY] == 0) {

            int currentPiece = (game->mode == PVE) ? 1 : game->currentPlayer;
            game->board[boardX][boardY] = currentPiece;
            saveMove(boardX, boardY, currentPiece, "record.txt");
            //PlaySound(TEXT("move.wav"), NULL, SND_FILENAME | SND_ASYNC);

            if (checkWinFast(game, boardX, boardY)) {
                game->gameOver = 1;
                game->lastUpdateTime = time(NULL);
                if (game->mode == PVE) {
                    game->pveUserWins++;
                    game->currentPlayer = 1;
                    game->lastWinner = 1; // 用户胜利
                }
                else if (game->mode == PVP) {
                    if (game->currentPlayer == 1)
                        game->pvpUser1Wins++;
                    else
                        game->pvpUser2Wins++;
                    game->lastWinner = game->currentPlayer;
                }
                FlushBatchDraw();
                return;
            }

            game->turnStep++;

            // 判断是否轮到对方
            if (game->turnStep >= game->turnQuota) {
                game->turnStep = 0;
                game->turnQuota = 2;

                if (game->mode == PVE) {
                    game->isAITurn = true;
                    game->currentPlayer = 2;
                    FlushBatchDraw();
                    for (int i = 0; i < 2 && !game->gameOver; ++i) {
                        makeAIMove(game);
                    }
                    game->currentPlayer = 1;
                    game->isAITurn = false;
                }
                else if (game->mode == PVP) {
                    game->currentPlayer = 3 - game->currentPlayer;
                }
            }
        }
    }
}

// 修改后的主游戏循环
int main() {
    initgraph(800, 600);
    BeginBatchDraw();

    //循环播放背景音乐
    PlaySound(_T("bgm.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

    GameState game;
    initGame(&game);
    game.mode = MENU;
	game.volume = 30; // 默认音量30%
    setVolume(game.volume);
    srand((unsigned int)time(NULL));

    while (true) {
        // 处理输入
        while (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                if (game.mode == MENU) {
                    handleMenuClick(&game, msg.x, msg.y);
                }
                else {
                    handleGameClick(&game, msg.x, msg.y);
                }
            }
        }

        // --- 回放模式处理 ---
        if (game.mode == REPLAY) {
            DWORD now = GetTickCount();
            if (replayCurrentStep <= replayMoveCount && now - replayLastTick > 500) {
                // 清空棋盘
                memset(game.board, 0, sizeof(game.board));
                // 依次下到当前步
                for (int i = 0; i < replayCurrentStep; ++i) {
                    int x = replayMovesArr[i].x;
                    int y = replayMovesArr[i].y;
                    int player = replayMovesArr[i].player;
                    game.board[x][y] = player;
                }
                drawGame(&game);
                FlushBatchDraw();
                replayCurrentStep++;
                replayLastTick = now;
                if (replayCurrentStep > replayMoveCount) {
                    MessageBox(GetHWnd(), _T("回放结束！"), _T("提示"), MB_OK);
                    game.mode = MENU;
                }
            }
            // 跳过后续绘制和帧率控制
            continue;
        }

        // 绘制
        cleardevice();
        if (game.mode == MENU) {
            drawMenu(&game);
        }
        else {
            drawGame(&game);
        }
        FlushBatchDraw();

        // 控制帧率
        static DWORD lastTick = GetTickCount();
        DWORD currentTick = GetTickCount();
        if (currentTick - lastTick < 16) {
            Sleep(16 - (currentTick - lastTick));
        }
        lastTick = currentTick;
    }

    EndBatchDraw();
    closegraph();
    return 0;
}