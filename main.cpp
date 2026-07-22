// ============================================================
// Card Swap Puzzle - Steps Challenge Mode (Drag & Drop)
// Controls: Drag a card to adjacent card to swap, or click to select.
//           R: Restart
// ============================================================

#include <graphics.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <windows.h>

using namespace std;

// ---------- Constants ----------
const int WIN_W = 640;
const int WIN_H = 700;
const int CARD_W = 60;
const int CARD_H = 55;
const int CARD_SPACING = 5;
const int TOP_OFFSET = 70;
const int MAX_CARDS = 10;
const int TARGET_GROUPS = 6;
const int INIT_CARDS = 8;
const int MAX_LOOP = 100;

// ---------- Doubly Circular Linked List Node ----------
struct CardNode {
    int value;              // 3~14 (J=11, Q=12, K=13, A=14)
    int x, y;
    bool selected;
    CardNode* prev;
    CardNode* next;

    CardNode(int v = 3) : value(v), x(0), y(0), selected(false), prev(nullptr), next(nullptr) {}
};

// ---------- Game Class ----------
class Game {
private:
    enum GameState { STATE_HELP, STATE_PLAYING, STATE_END, STATE_MATCH_MSG };
    GameState state;

    CardNode* head;
    int cardCount;
    int stepCount;
    int groupsMatched;
    int target[3];
    int nextTarget[3];   // 预览：下一个目标组合，供玩家提前规�?
    bool gameOver;
    bool gameWin;

    CardNode* hoverCard;
    CardNode* dragStart;
    bool isDragging;

    PIMAGE buffer;

    static int bestSteps;

    bool needRedraw;

    bool showMatchMsg;
    clock_t matchMsgStart;   // ����ʵʱ���ʱ����֡����??

public:
    Game() : state(STATE_HELP), head(nullptr), cardCount(0), stepCount(0), groupsMatched(0),
             gameOver(false), gameWin(false),
             hoverCard(nullptr), dragStart(nullptr), isDragging(false),
             buffer(nullptr), needRedraw(true),
             showMatchMsg(false), matchMsgStart(0) {
        srand((unsigned)time(nullptr));
    }

    ~Game() {
        freeAllMemory();
        if (buffer) {
            delimage(buffer);
            buffer = nullptr;
        }
    }

    // ---------- Initialization ----------
    void init() {
        freeAllMemory();
        stepCount = 0;
        groupsMatched = 0;
        gameOver = false;
        gameWin = false;
        hoverCard = nullptr;
        dragStart = nullptr;
        isDragging = false;
        showMatchMsg = false;
        matchMsgStart = 0;

        for (int i = 0; i < INIT_CARDS; ++i) {
            int v = 3 + rand() % 12;
            insertHead(v);
        }
        updatePositions();

        generateTarget();
        while (targetAlreadyMatched(target) && !gameOver) {
            int elim = checkAndEliminate();
            if (elim == 0) break;
            if (gameOver) break;
        }
        if (gameOver) {
            if (gameWin && stepCount < bestSteps) bestSteps = stepCount;
            state = STATE_END;
        } else {
            state = STATE_PLAYING;
        }
        needRedraw = true;
    }

    // ---------- Linked List Operations ----------
    void insertHead(int value) {
        CardNode* newNode = new CardNode(value);
        if (!head) {
            head = newNode;
            head->next = head;
            head->prev = head;
        } else {
            CardNode* tail = head->prev;
            newNode->next = head;
            newNode->prev = tail;
            head->prev = newNode;
            tail->next = newNode;
            head = newNode;
        }
        cardCount++;
    }

    void removeNode(CardNode* node) {
        if (!node || cardCount == 0) return;
        if (cardCount == 1) {
            delete node;
            head = nullptr;
            cardCount = 0;
            return;
        }
        node->prev->next = node->next;
        node->next->prev = node->prev;
        if (node == head) {
            head = node->next;
        }
        delete node;
        cardCount--;
    }

    bool swapAdjacent(CardNode* a, CardNode* b) {
        if (!a || !b || a == b) return false;
        if (a->next != b && a->prev != b) return false;
        CardNode* left = (a->next == b) ? a : b;
        CardNode* right = (a->next == b) ? b : a;

        CardNode* leftPrev = left->prev;
        CardNode* rightNext = right->next;

        left->prev = right;
        left->next = rightNext;
        right->prev = leftPrev;
        right->next = left;

        if (leftPrev) leftPrev->next = right;
        if (rightNext) rightNext->prev = left;

        if (head == left) head = right;
        else if (head == right) head = left;

        return true;
    }

    bool removeMatch(CardNode* first) {
        if (!first) return false;
        CardNode* second = first->next;
        CardNode* third = second->next;
        if (!second || !third) return false;
        if (first->value != target[0] || second->value != target[1] || third->value != target[2])
            return false;

        if (cardCount == 3) {
            delete first;
            delete second;
            delete third;
            head = nullptr;
            cardCount = 0;
            return true;
        }

        CardNode* before = first->prev;
        CardNode* after = third->next;
        before->next = after;
        after->prev = before;
        delete first;
        delete second;
        delete third;
        cardCount -= 3;
        if (head == first || head == second || head == third) {
            head = after;
        }
        return true;
    }

    // ---------- Game Logic ----------
    void getValueCount(int count[15]) {
        for (int i = 3; i <= 14; ++i) count[i] = 0;
        if (!head) return;
        CardNode* cur = head;
        for (int i = 0; i < cardCount; ++i) {
            count[cur->value]++;
            cur = cur->next;
        }
    }

    bool targetAlreadyMatched(const int t[3]) {
        if (cardCount < 3) return false;
        CardNode* cur = head;
        for (int i = 0; i < cardCount; ++i) {
            if (cur->value == t[0] && cur->next->value == t[1] && cur->next->next->value == t[2]) {
                return true;
            }
            cur = cur->next;
        }
        return false;
    }

    // 生成一个目标组合写�?out，避免与当前牌面已连续匹�?
    void generateOneTarget(int out[3]) {
        if (!head || cardCount < 3) {
            for (int i = 0; i < 3; ++i) out[i] = 3 + rand() % 12;
            return;
        }

        if (cardCount == 3) {
            int v0 = head->value;
            int v1 = head->next->value;
            int v2 = head->next->next->value;
            int choices[2][3] = {{v1, v0, v2}, {v0, v2, v1}};
            int idx = rand() % 2;
            out[0] = choices[idx][0];
            out[1] = choices[idx][1];
            out[2] = choices[idx][2];
            return;
        }

        vector<int> pool;
        CardNode* cur = head;
        for (int i = 0; i < cardCount; ++i) {
            pool.push_back(cur->value);
            cur = cur->next;
        }

        random_device rd;
        mt19937 g(rd());
        shuffle(pool.begin(), pool.end(), g);
        for (int i = 0; i < 3; ++i) {
            out[i] = pool[i];
        }

        int attempts = 0;
        while (targetAlreadyMatched(out) && attempts < MAX_LOOP) {
            attempts++;
            shuffle(pool.begin(), pool.end(), g);
            for (int i = 0; i < 3; ++i) {
                out[i] = pool[i];
            }
        }
    }

    void generateTarget() {
        generateOneTarget(target);
        // 生成 nextTarget 预览，确保与当前 target 不同（否则预览无意义�?
        generateOneTarget(nextTarget);
        int attempts = 0;
        while (nextTarget[0] == target[0] && nextTarget[1] == target[1] && nextTarget[2] == target[2]
               && attempts < MAX_LOOP) {
            attempts++;
            generateOneTarget(nextTarget);
        }
    }

    int checkAndEliminate() {
        int eliminated = 0;
        bool found = true;
        int loopGuard = 0;
        while (found && !gameOver && loopGuard < MAX_LOOP) {
            loopGuard++;
            found = false;
            if (cardCount < 3) break;
            CardNode* cur = head;
            for (int i = 0; i < cardCount; ++i) {
                if (cur->value == target[0] && cur->next->value == target[1] && cur->next->next->value == target[2]) {
                    CardNode* first = cur;
                    if (removeMatch(first)) {
                        eliminated++;
                        groupsMatched++;
                        found = true;
                        insertHead(3 + rand() % 12);
                        insertHead(3 + rand() % 12);
                        updatePositions();
                        if (groupsMatched >= TARGET_GROUPS) {
                            gameWin = true;
                            gameOver = true;
                            return eliminated;
                        }
                        if (cardCount > MAX_CARDS) {
                            gameOver = true;
                            return eliminated;
                        }
                        break;
                    }
                }
                cur = cur->next;
            }
        }
        if (eliminated > 0 && !gameOver) {
            generateTarget();
        }
        return eliminated;
    }

    void updatePositions() {
        if (!head) return;
        CardNode* cur = head;
        int y = TOP_OFFSET;
        for (int i = 0; i < cardCount; ++i) {
            cur->x = (WIN_W - CARD_W) / 2;
            cur->y = y;
            y += CARD_H + CARD_SPACING;
            cur = cur->next;
        }
    }

    // ---------- Player Actions ----------
    bool playerSwap(CardNode* a, CardNode* b) {
        if (!a || !b || a == b) return false;
        if (a->next != b && a->prev != b) return false;

        clearSelected();

        if (!swapAdjacent(a, b)) return false;
        stepCount++;
        updatePositions();

        int elimCount = checkAndEliminate();
        updatePositions();

        while (targetAlreadyMatched(target) && !gameOver) {
            int extra = checkAndEliminate();
            if (extra == 0) break;
            updatePositions();
        }

        if (gameWin) {
            if (stepCount < bestSteps) {
                bestSteps = stepCount;
            }
            showMatchMsg = true;
            matchMsgStart = clock();   // ��¼��Ϣ��ʼʱ�䣬������ʵ��ʱ
            state = STATE_MATCH_MSG;
        } else if (gameOver) {
            state = STATE_END;
        }

        clearSelected();
        needRedraw = true;
        return true;
    }

    // ---------- Memory Free ----------
    void freeAllMemory() {
        if (!head) return;
        CardNode* cur = head;
        CardNode* nextNode;
        head->prev->next = nullptr;
        while (cur) {
            nextNode = cur->next;
            delete cur;
            cur = nextNode;
        }
        head = nullptr;
        cardCount = 0;
    }

    // ---------- Drawing ----------
    void draw() {
        settarget(buffer);
        setbkcolor(RGB(240, 240, 240));   // 灰色背景
        cleardevice();

        if (state == STATE_PLAYING) {
            drawGame();
        } else if (state == STATE_MATCH_MSG) {
            drawGame();
            drawMatchMessage();
        } else if (state == STATE_END) {
            drawEnd();
        } else if (state == STATE_HELP) {
            drawHelpContent();
        }

        settarget(NULL);
        putimage(0, 0, buffer);
        needRedraw = false;
    }

    void drawMatchMessage() {
        setfont(50, 0, "????");
        setcolor(RGB(255, 0, 0));
        outtextxy(WIN_W/2 - 140, WIN_H/2 - 30, "Perfect Match!");
        setfont(20, 0, "????");
        setcolor(RGB(100, 100, 100));
        outtextxy(WIN_W/2 - 80, WIN_H/2 + 40, "Well done!");
    }

    void drawGame() {
        setcolor(RGB(0, 0, 0));
        setfont(20, 0, "????");
        outtextxy(10, 10, "Steps: ");
        char buf[20];
        sprintf(buf, "%d", stepCount);
        outtextxy(100, 10, buf);

        outtextxy(390, 10, "Best: ");
        if (bestSteps < 9999) {
            sprintf(buf, "%d", bestSteps);
        } else {
            sprintf(buf, "--");
        }
        outtextxy(525, 10, buf);

        setcolor(RGB(218, 165, 32));   // 金色显示当前 Target
        outtextxy(390, 35, "Target: ");
        char targetStr[10];
        sprintf(targetStr, "%d-%d-%d", target[0], target[1], target[2]);
        outtextxy(525, 35, targetStr);
        setcolor(RGB(0, 0, 0));   // 恢复黑色，避免后续文字继承金色

        outtextxy(390, 60, "Remaining: ");
        sprintf(buf, "%d", TARGET_GROUPS - groupsMatched);
        outtextxy(525, 60, buf);

        // 显示下一个目标预览，帮助玩家提前规划移动
        outtextxy(390, 85, "Next Target: ");
        char nextStr[10];
        sprintf(nextStr, "%d-%d-%d", nextTarget[0], nextTarget[1], nextTarget[2]);
        outtextxy(525, 85, nextStr);

        if (head) {
            // ����ֻ����һ�Σ�����ÿ���ƶ����¼������壨EGE ??setfont ������
            setfont(24, 0, "????");
            CardNode* cur = head;
            for (int i = 0; i < cardCount; ++i) {
                drawCard(cur, (cur == hoverCard));
                cur = cur->next;
            }
        }

        setfont(16, 0, "????");
        setcolor(RGB(100, 100, 100));
        outtextxy(10, WIN_H - 80, "Drag card to adjacent card to swap | Click to select | R: Restart");

        int btnW = 100, btnH = 35;
        int btnY = WIN_H - 45;
        int btn1X = WIN_W/2 - btnW - 15;
        int btn2X = WIN_W/2 + 15;

        setfillcolor(RGB(150, 220, 150));
        bar(btn1X, btnY, btn1X + btnW, btnY + btnH);
        setcolor(RGB(0,0,0));
        rectangle(btn1X, btnY, btn1X + btnW, btnY + btnH);
        setfont(18, 0, "????");
        outtextxy(btn1X + 10, btnY + 7, "Restart");

        setfillcolor(RGB(220, 150, 150));
        bar(btn2X, btnY, btn2X + btnW, btnY + btnH);
        setcolor(RGB(0,0,0));
        rectangle(btn2X, btnY, btn2X + btnW, btnY + btnH);
        outtextxy(btn2X + 24, btnY + 7, "Exit");
    }

    void drawCard(CardNode* card, bool hover) {
        int x = card->x, y = card->y;
        if (card->selected) {
            setfillcolor(RGB(200, 200, 255));
        } else if (hover) {
            setfillcolor(RGB(220, 220, 220));
        } else {
            setfillcolor(RGB(255, 255, 255));
        }
        bar(x, y, x + CARD_W, y + CARD_H);

        if (card->selected) {
            setcolor(RGB(255, 0, 0));
            setlinestyle(SOLID_LINE, 3);
        } else {
            setcolor(RGB(0, 0, 0));
            setlinestyle(SOLID_LINE, 1);
        }
        rectangle(x, y, x + CARD_W, y + CARD_H);

        // �������� drawGame �Ŀ���ѭ��ǰͳһ���ã����ﲻ���ظ���??setfont
        setcolor(RGB(0, 0, 0));
        char val[5];
        int v = card->value;
        if (v <= 10) sprintf(val, "%d", v);
        else if (v == 11) sprintf(val, "J");
        else if (v == 12) sprintf(val, "Q");
        else if (v == 13) sprintf(val, "K");
        else if (v == 14) sprintf(val, "A");
        outtextxy(x + CARD_W/2 - 12, y + CARD_H/2 - 12, val);
    }

    void drawEnd() {
        setfillcolor(RGB(200, 200, 200));
        bar(0, 0, WIN_W, WIN_H);

        setcolor(RGB(0, 0, 0));
        setfont(36, 0, "????");
        if (gameWin) {
            outtextxy(WIN_W/2 - 80, 100, "You Win!");
        } else {
            outtextxy(WIN_W/2 - 100, 100, "Game Over!");
        }

        setfont(24, 0, "????");
        char buf[100];
        sprintf(buf, "Steps used: %d", stepCount);
        outtextxy(WIN_W/2 - 100, 180, buf);

        if (bestSteps < 9999) {
            sprintf(buf, "Best record: %d steps", bestSteps);
        } else {
            sprintf(buf, "Best record: --");
        }
        outtextxy(WIN_W/2 - 100, 220, buf);

        int btnW = 140, btnH = 50;
        int btn1X = WIN_W/2 - btnW - 20;
        int btn1Y = 320;
        int btn2X = WIN_W/2 + 20;
        int btn2Y = 320;

        setfillcolor(RGB(100, 200, 100));
        bar(btn1X, btn1Y, btn1X + btnW, btn1Y + btnH);
        setcolor(RGB(0, 0, 0));
        rectangle(btn1X, btn1Y, btn1X + btnW, btn1Y + btnH);
        setfont(20, 0, "????");
        outtextxy(btn1X + 20, btn1Y + 12, "Try Again");

        setfillcolor(RGB(200, 100, 100));
        bar(btn2X, btn2Y, btn2X + btnW, btn2Y + btnH);
        setcolor(RGB(0, 0, 0));
        rectangle(btn2X, btn2Y, btn2X + btnW, btn2Y + btnH);
        outtextxy(btn2X + 40, btn2Y + 12, "Exit");
    }

    void drawHelpContent() {
        setcolor(RGB(0, 0, 0));
        setfont(26, 0, "????");
        outtextxy(180, 15, "Card Swap Puzzle");

        setfont(17, 0, "????");
        outtextxy(50, 60, "Goal: Complete 6 target groups with minimum swaps.");
        outtextxy(50, 88, "Rules:");

        outtextxy(70, 118, "1. Drag a card to an adjacent card to swap.");
        outtextxy(70, 146, "2. Or click a card to select, then click adjacent to swap.");
        outtextxy(70, 174, "3. If three consecutive cards match the target, they vanish.");
        outtextxy(70, 202, "4. Two new random cards appear at the top.");
        outtextxy(70, 230, "5. Complete 6 groups to win.");
        outtextxy(70, 258, "6. Try to beat your own record! (Best steps are saved)");
        outtextxy(70, 286, "7. The Next box previews the upcoming target - plan ahead!");

        setfont(18, 0, "????");
        setcolor(RGB(200, 0, 0));
        outtextxy(50, 305, "Card values: J=11, Q=12, K=13, A=14");

        setfont(17, 0, "????");
        setcolor(RGB(100, 100, 100));
        outtextxy(50, 345, "Controls:");
        outtextxy(70, 373, "- Mouse: Drag to swap, or click to select.");
        outtextxy(70, 401, "- R key: Restart the game.");

        setfont(20, 0, "????");
        setcolor(RGB(200, 0, 0));
        outtextxy(180, 470, "Press any key to start...");
    }

    CardNode* getCardAt(int mx, int my) {
        if (!head) return nullptr;
        CardNode* cur = head;
        for (int i = 0; i < cardCount; ++i) {
            if (mx >= cur->x && mx <= cur->x + CARD_W &&
                my >= cur->y && my <= cur->y + CARD_H) {
                return cur;
            }
            cur = cur->next;
        }
        return nullptr;
    }

    // ---------- Mouse & Keyboard ----------
    void handleMouse() {
        if (state == STATE_END) {
            // �ſ������У�����������ѭ��
            while (mousemsg()) {
                mouse_msg msg = getmouse();
                if (msg.is_left() && msg.is_up()) {
                    int mx = msg.x, my = msg.y;
                    int btnW = 140, btnH = 50;
                    int btn1X = WIN_W/2 - btnW - 20;
                    int btn1Y = 320;
                    int btn2X = WIN_W/2 + 20;
                    int btn2Y = 320;

                    if (mx >= btn1X && mx <= btn1X + btnW && my >= btn1Y && my <= btn1Y + btnH) {
                        init();
                        needRedraw = true;
                        return;
                    }
                    if (mx >= btn2X && mx <= btn2X + btnW && my >= btn2Y && my <= btn2Y + btnH) {
                        closegraph();
                        exit(0);
                    }
                }
            }
            return;
        }

        if (state == STATE_MATCH_MSG) {
            // ??????????????????????????????????run ?��?????
            return;
        }

        if (state != STATE_PLAYING) return;

        // �ſ������У�һ֡�ڴ������л�ѹ�������Ϣ��������קʱ��Ӧ�ͺ�?
        while (mousemsg()) {
        mouse_msg msg = getmouse();

        if (msg.is_down() && msg.is_left()) {
            int mx = msg.x, my = msg.y;
            int btnW = 100, btnH = 35;
            int btnY = WIN_H - 45;
            int btn1X = WIN_W/2 - btnW - 15;
            int btn2X = WIN_W/2 + 15;
            if (mx >= btn1X && mx <= btn1X + btnW && my >= btnY && my <= btnY + btnH) {
                init();
                needRedraw = true;
                return;
            }
            if (mx >= btn2X && mx <= btn2X + btnW && my >= btnY && my <= btnY + btnH) {
                closegraph();
                exit(0);
            }

            CardNode* clicked = getCardAt(mx, my);
            if (clicked) {
                dragStart = clicked;
                isDragging = true;
                clearSelected();
                clicked->selected = true;
                needRedraw = true;
            } else {
                clearSelected();
                if (isDragging) {
                    isDragging = false;
                    dragStart = nullptr;
                }
                needRedraw = true;
            }
        }

        if (msg.is_up() && msg.is_left()) {
            if (isDragging && dragStart) {
                int mx = msg.x, my = msg.y;
                CardNode* target = getCardAt(mx, my);
                if (target && target != dragStart) {
                    if (dragStart->next == target || dragStart->prev == target) {
                        playerSwap(dragStart, target);
                        clearSelected();
                        needRedraw = true;
                    } else {
                        clearSelected();
                        needRedraw = true;
                    }
                } else {
                    clearSelected();
                    needRedraw = true;
                }
                isDragging = false;
                dragStart = nullptr;
            }
        }

        if (msg.is_move()) {
            int mx = msg.x, my = msg.y;
            if (!isDragging) {
                CardNode* newHover = getCardAt(mx, my);
                if (newHover != hoverCard) {
                    hoverCard = newHover;
                    needRedraw = true;
                }
            }
        }
        } // end while(mousemsg())
    }

    CardNode* getSelected() {
        if (!head) return nullptr;
        CardNode* cur = head;
        for (int i = 0; i < cardCount; ++i) {
            if (cur->selected) return cur;
            cur = cur->next;
        }
        return nullptr;
    }

    bool hasSelected() {
        return getSelected() != nullptr;
    }

    void clearSelected() {
        if (!head) return;
        CardNode* cur = head;
        for (int i = 0; i < cardCount; ++i) {
            cur->selected = false;
            cur = cur->next;
        }
    }

    // ---------- Main Loop ----------
    void run() {
        initgraph(WIN_W, WIN_H);
        buffer = newimage(WIN_W, WIN_H);
        setbkcolor(RGB(240, 240, 240));   // 灰色背景

        state = STATE_HELP;
        needRedraw = true;
        while (true) {
            if (needRedraw) {
                draw();
                needRedraw = false;
            }
            if (kbhit()) {
                getch();
                break;
            }
            delay_fps(60);   // cap at 60 FPS, lower CPU usage
        }

        init();
        needRedraw = true;

        while (true) {
            handleMouse();
            // 消息状态计时：使用真实时间，与帧率无关�?.5 秒后切换到结束界面）
            if (state == STATE_MATCH_MSG) {
                if (clock() - matchMsgStart >= 1500) {
                    state = STATE_END;
                    needRedraw = true;
                }
            }

            if (kbhit()) {
                char key = getch();
                if (key == 'r' || key == 'R') {
                    if (state == STATE_PLAYING || state == STATE_END || state == STATE_MATCH_MSG) {
                        init();
                        needRedraw = true;
                    }
                }
            }
            if (needRedraw) {
                draw();
                needRedraw = false;
            }
            delay_fps(60);   // cap at 60 FPS, lower CPU usage
        }
        closegraph();
    }
};

// ---------- Static member initialization ----------
int Game::bestSteps = 9999;

// ---------- Main ----------
int main() {
    Game game;
    game.run();
    return 0;
}
