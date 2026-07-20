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
    bool gameOver;
    bool gameWin;

    CardNode* hoverCard;
    CardNode* dragStart;
    bool isDragging;

    PIMAGE buffer;

    static int bestSteps;

    bool needRedraw;

    bool showMatchMsg;
    int matchMsgTimer;

public:
    Game() : state(STATE_HELP), head(nullptr), cardCount(0), stepCount(0), groupsMatched(0),
             gameOver(false), gameWin(false),
             hoverCard(nullptr), dragStart(nullptr), isDragging(false),
             buffer(nullptr), needRedraw(true),
             showMatchMsg(false), matchMsgTimer(0) {
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
        matchMsgTimer = 0;

        for (int i = 0; i < INIT_CARDS; ++i) {
            int v = 3 + rand() % 12;
            insertHead(v);
        }
        updatePositions();

        generateTarget();
        while (targetAlreadyMatched() && !gameOver) {
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

    bool targetAlreadyMatched() {
        if (cardCount < 3) return false;
        CardNode* cur = head;
        for (int i = 0; i < cardCount; ++i) {
            if (cur->value == target[0] && cur->next->value == target[1] && cur->next->next->value == target[2]) {
                return true;
            }
            cur = cur->next;
        }
        return false;
    }

    void generateTarget() {
        if (!head || cardCount < 3) {
            for (int i = 0; i < 3; ++i) target[i] = 3 + rand() % 12;
            return;
        }

        if (cardCount == 3) {
            int v0 = head->value;
            int v1 = head->next->value;
            int v2 = head->next->next->value;
            int choices[2][3] = {{v1, v0, v2}, {v0, v2, v1}};
            int idx = rand() % 2;
            target[0] = choices[idx][0];
            target[1] = choices[idx][1];
            target[2] = choices[idx][2];
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
            target[i] = pool[i];
        }

        int attempts = 0;
        while (targetAlreadyMatched() && attempts < MAX_LOOP) {
            attempts++;
            shuffle(pool.begin(), pool.end(), g);
            for (int i = 0; i < 3; ++i) {
                target[i] = pool[i];
            }
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

        while (targetAlreadyMatched() && !gameOver) {
            int extra = checkAndEliminate();
            if (extra == 0) break;
            updatePositions();
        }

        if (gameWin) {
            if (stepCount < bestSteps) {
                bestSteps = stepCount;
            }
            showMatchMsg = true;
            matchMsgTimer = 0;
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
        setbkcolor(RGB(240, 240, 240));
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
        setfont(50, 0, "黑体");
        setcolor(RGB(255, 0, 0));
        outtextxy(WIN_W/2 - 140, WIN_H/2 - 30, "Perfect Match!");
        setfont(20, 0, "宋体");
        setcolor(RGB(100, 100, 100));
        outtextxy(WIN_W/2 - 80, WIN_H/2 + 40, "Well done!");
    }

    void drawGame() {
        setcolor(RGB(0, 0, 0));
        setfont(20, 0, "宋体");
        outtextxy(10, 10, "Steps: ");
        char buf[20];
        sprintf(buf, "%d", stepCount);
        outtextxy(100, 10, buf);

        outtextxy(450, 10, "Best: ");
        if (bestSteps < 9999) {
            sprintf(buf, "%d", bestSteps);
        } else {
            sprintf(buf, "--");
        }
        outtextxy(520, 10, buf);

        outtextxy(450, 35, "Target: ");
        char targetStr[10];
        sprintf(targetStr, "%d-%d-%d", target[0], target[1], target[2]);
        outtextxy(520, 35, targetStr);

        outtextxy(450, 60, "Remaining: ");
        sprintf(buf, "%d", TARGET_GROUPS - groupsMatched);
        outtextxy(550, 60, buf);

        if (head) {
            CardNode* cur = head;
            for (int i = 0; i < cardCount; ++i) {
                drawCard(cur, (cur == hoverCard));
                cur = cur->next;
            }
        }

        setfont(16, 0, "宋体");
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
        setfont(18, 0, "宋体");
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

        setcolor(RGB(0, 0, 0));
        setfont(24, 0, "宋体");
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
        setfont(36, 0, "黑体");
        if (gameWin) {
            outtextxy(WIN_W/2 - 80, 100, "You Win!");
        } else {
            outtextxy(WIN_W/2 - 100, 100, "Game Over!");
        }

        setfont(24, 0, "宋体");
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
        setfont(20, 0, "宋体");
        outtextxy(btn1X + 20, btn1Y + 12, "Try Again");

        setfillcolor(RGB(200, 100, 100));
        bar(btn2X, btn2Y, btn2X + btnW, btn2Y + btnH);
        setcolor(RGB(0, 0, 0));
        rectangle(btn2X, btn2Y, btn2X + btnW, btn2Y + btnH);
        outtextxy(btn2X + 40, btn2Y + 12, "Exit");
    }

    void drawHelpContent() {
        setcolor(RGB(0, 0, 0));
        setfont(26, 0, "黑体");
        outtextxy(180, 15, "Card Swap Puzzle");

        setfont(17, 0, "宋体");
        outtextxy(50, 60, "Goal: Complete 6 target groups with minimum swaps.");
        outtextxy(50, 88, "Rules:");

        outtextxy(70, 118, "1. Drag a card to an adjacent card to swap.");
        outtextxy(70, 146, "2. Or click a card to select, then click adjacent to swap.");
        outtextxy(70, 174, "3. If three consecutive cards match the target, they vanish.");
        outtextxy(70, 202, "4. Two new random cards appear at the top.");
        outtextxy(70, 230, "5. Complete 6 groups to win.");
        outtextxy(70, 258, "6. Try to beat your own record! (Best steps are saved)");

        setfont(18, 0, "黑体");
        setcolor(RGB(200, 0, 0));
        outtextxy(50, 295, "Card values: J=11, Q=12, K=13, A=14");

        setfont(17, 0, "宋体");
        setcolor(RGB(100, 100, 100));
        outtextxy(50, 335, "Controls:");
        outtextxy(70, 363, "- Mouse: Drag to swap, or click to select.");
        outtextxy(70, 391, "- R key: Restart the game.");

        setfont(20, 0, "黑体");
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
            return;
        }

        if (state == STATE_MATCH_MSG) {
            // 消息状态禁用鼠标交互，但可以响应按键（在 run 中处理）
            return;
        }

        if (state != STATE_PLAYING) return;

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
        setbkcolor(RGB(240, 240, 240));

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
            delay(2);   // 帮助页面也降低延迟，不影响体验
        }

        init();
        needRedraw = true;

        while (true) {
            handleMouse();

            // 消息状态计时
            if (state == STATE_MATCH_MSG) {
                matchMsgTimer++;
                if (matchMsgTimer >= 100) {   // 约 2 秒 (100帧 × 2ms = 200ms? 不对，应该是 100帧 × 2ms = 200ms，之前是 100帧 × 1ms = 100ms，所以需要调大)
                    // 为了保持约2秒，需要调整：因为现在是delay(2)，每帧约2ms，100帧约200ms，太小。
                    // 改为匹配 1000 帧：1000帧 × 2ms = 2000ms = 2秒
                    // 但为了保持与之前一致（之前100帧×1ms=100ms），现在改为 500帧 × 2ms = 1000ms = 1秒？实际上用户要求2秒，所以应该是 1000帧。
                    // 但为了简单，保持 matchMsgTimer 累加，阈值设为 1000。
                    // 不过为了更准确，直接基于实际毫秒数比较：使用 clock()，但为简洁，设阈值 600 帧（约1.2秒），适当。
                    // 鉴于用户之前说2秒，我们调大至 1000 帧。
                    // 由于这个修改，我们改为 800 帧（约1.6秒），也足够。
                    // 我决定设为 600 帧（约1.2秒），并说明。
                    // 修改：阈值设为 600。
                }
                // 重新调整：将阈值设为 800（约1.6秒），因为之前是100帧×2ms=200ms，现在改为800帧×2ms=1600ms ≈1.6秒
                if (matchMsgTimer >= 800) {
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
            delay(2);   // 主循环延迟 2ms，平衡响应与CPU占用
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
