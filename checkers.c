#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "menu.h"

#define BOARD_SIZE 8
#define CELL_SIZE 80
#define WINDOW_SIZE (BOARD_SIZE * CELL_SIZE)
#define MAX_DEPTH 6

// Добавляем новые константы для TT
#define TT_SIZE 100000  // 100K записей - оптимальный размер
#define TT_EXACT 0
#define TT_LOWER_BOUND 1
#define TT_UPPER_BOUND 2

// Структура для таблицы транспозиций (компактная - 8 байт)
typedef struct {
    uint32_t hash_low;  // Младшие 32 бита хеша
    int16_t score;      // Оценка позиции
    uint8_t depth;      // Глубина поиска
    uint8_t flag;       // Тип оценки (EXACT/LOWER/UPPER)
} TTEntry;

// Глобальная таблица транспозиций
TTEntry transposition_table[TT_SIZE];

// Zobrist ключи для хеширования
uint64_t zobrist_table[BOARD_SIZE][BOARD_SIZE][5];
uint64_t turn_key;

extern void runTests();

typedef enum {
    EMPTY,
    BLACK,
    WHITE,
    BLACK_KING,
    WHITE_KING
} Piece;

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position from;
    Position to;
    Position capture;
    bool is_capture;
} Move;

Piece board[BOARD_SIZE][BOARD_SIZE];
Position selected = { -1, -1 };

bool isWhiteTurn = true;
bool gameOver = false;
bool windowShouldClose = false;

Position lastBotMoveFrom = { -1, -1 };
Position lastBotMoveTo = { -1, -1 };

enum GameMode {
    MODE_NONE,
    MODE_PVP,
    MODE_PVBOT
};
int gameMode = MODE_NONE;

static char endGameTitle[64] = "";
static char endGameMessage[64] = "";

bool isKing(Piece p) {
    return p == BLACK_KING || p == WHITE_KING;
}

bool isWhite(Piece p) {
    return p == WHITE || p == WHITE_KING;
}

bool isBlack(Piece p) {
    return p == BLACK || p == BLACK_KING;
}

bool isOpponent(Piece a, Piece b) {
    if (a == EMPTY || b == EMPTY) return false;
    return (isWhite(a) && isBlack(b)) || (isBlack(a) && isWhite(b));
}



// Генерация случайных 64-битных чисел
uint64_t random_64bit() {
    // Используем простой PRNG, можно заменить на более качественный
    static uint64_t seed = 0x123456789ABCDEF;
    seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    return seed;
}

// Инициализация Zobrist таблицы
void init_zobrist() {
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            for (int p = 0; p < 5; p++) { // 5 типов фигур: EMPTY, BLACK, WHITE, BLACK_KING, WHITE_KING
                zobrist_table[x][y][p] = random_64bit();
            }
        }
    }
    turn_key = random_64bit();
}

// Вычисление хеша для произвольной доски
uint64_t compute_board_hash_for_board(Piece tempBoard[BOARD_SIZE][BOARD_SIZE], bool is_white_turn) {
    uint64_t hash = 0;

    // Хешируем фигуры на переданной доске
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            Piece p = tempBoard[x][y];
            hash ^= zobrist_table[x][y][p];
        }
    }

    // Хешируем чей ход
    if (is_white_turn) {
        hash ^= turn_key;
    }

    return hash;
}

// Обертка для глобальной доски (оставляем для обратной совместимости)
uint64_t compute_board_hash() {
    return compute_board_hash_for_board(board, isWhiteTurn);
}

// Очистка таблицы транспозиций
void clear_transposition_table() {
    memset(transposition_table, 0, sizeof(transposition_table));
}

// Поиск в таблице транспозиций
TTEntry* tt_lookup(uint64_t hash) {
    uint32_t index = hash % TT_SIZE;
    uint32_t hash_low = (uint32_t)(hash & 0xFFFFFFFF);

    if (transposition_table[index].hash_low == hash_low) {
        return &transposition_table[index];
    }
    return NULL;
}

// Сохранение в таблицу транспозиций
void tt_store(uint64_t hash, int depth, int score, int flag) {
    if (depth < 0 || depth > 255) return;

    uint32_t index = hash % TT_SIZE;
    uint32_t hash_low = (uint32_t)(hash & 0xFFFFFFFF);

    // Всегда заменяем - простая стратегия
    transposition_table[index] = (TTEntry){
        .hash_low = hash_low,
        .score = (int16_t)score,
        .depth = (uint8_t)depth,
        .flag = (uint8_t)flag
    };
}



void initBoard() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if ((x + y) % 2 == 1) {
                if (y < 3) board[x][y] = BLACK;
                else if (y > 4) board[x][y] = WHITE;
                else board[x][y] = EMPTY;
            }
            else {
                board[x][y] = EMPTY;
            }
        }
    }
}

bool canCaptureFrom(int x, int y) {
    Piece p = board[x][y];
    if (p == EMPTY) return false;

    int dirs[4][2] = { {1, 1}, {-1, 1}, {1, -1}, {-1, -1} };

    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0], dy = dirs[d][1];
        int nx = x + dx, ny = y + dy;

        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
            if (board[nx][ny] != EMPTY) {
                if (isOpponent(p, board[nx][ny])) {
                    int cx = nx + dx, cy = ny + dy;
                    if (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && board[cx][cy] == EMPTY) {
                        return true;
                    }
                }
                break;
            }
            if (!isKing(p)) break;
            nx += dx;
            ny += dy;
        }
    }
    return false;
}

bool hasValidMoves(bool forWhite) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Piece p = board[x][y];
            if ((forWhite && isWhite(p)) || (!forWhite && isBlack(p))) {
                int dirs[4][2] = { {1,1}, {-1,1}, {1,-1}, {-1,-1} };

                for (int d = 0; d < 4; d++) {
                    int dx = dirs[d][0], dy = dirs[d][1];

                    if (isKing(p)) {
                        int nx = x + dx, ny = y + dy;
                        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) return true;
                            if (board[nx][ny] != EMPTY) break;
                            nx += dx;
                            ny += dy;
                        }
                    }
                    else {
                        int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) {
                                if ((isWhite(p) && dy == -1) || (isBlack(p) && dy == 1)) {
                                    return true;
                                }
                            }
                        }
                    }
                }
                if (canCaptureFrom(x, y)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void drawBoardSquares() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if ((x + y) % 2 == 0)
                glColor3f(0.8f, 0.8f, 0.8f);
            else
                glColor3f(0.5f, 0.3f, 0.1f);

            if (selected.x == x && selected.y == y)
                glColor3f(0.2f, 0.8f, 0.2f);

            if (lastBotMoveTo.x == x && lastBotMoveTo.y == y)
                glColor3f(0.8f, 0.8f, 0.2f);

            glBegin(GL_QUADS);
            glVertex2f(x * CELL_SIZE, y * CELL_SIZE);
            glVertex2f((x + 1) * CELL_SIZE, y * CELL_SIZE);
            glVertex2f((x + 1) * CELL_SIZE, (y + 1) * CELL_SIZE);
            glVertex2f(x * CELL_SIZE, (y + 1) * CELL_SIZE);
            glEnd();
        }
    }
}

void highlightCaptureMoves() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Piece p = board[x][y];
            if ((gameMode == MODE_PVP) || (gameMode == MODE_PVBOT && isWhiteTurn)) {
                if (((isWhiteTurn && isWhite(p)) || (!isWhiteTurn && isBlack(p))) && canCaptureFrom(x, y)) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glColor4f(1.0f, 0.0f, 0.0f, 0.3f);
                    glBegin(GL_QUADS);
                    glVertex2f(x * CELL_SIZE, y * CELL_SIZE);
                    glVertex2f((x + 1) * CELL_SIZE, y * CELL_SIZE);
                    glVertex2f((x + 1) * CELL_SIZE, (y + 1) * CELL_SIZE);
                    glVertex2f(x * CELL_SIZE, (y + 1) * CELL_SIZE);
                    glEnd();
                    glDisable(GL_BLEND);
                }
            }
        }
    }
}

void drawPieces() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Piece p = board[x][y];
            if (p != EMPTY) {
                if (isWhite(p)) glColor3f(1.0f, 1.0f, 1.0f);
                else glColor3f(0.1f, 0.1f, 0.1f);

                float cx = x * CELL_SIZE + CELL_SIZE / 2;
                float cy = y * CELL_SIZE + CELL_SIZE / 2;
                float radius = CELL_SIZE * 0.35f;

                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(cx, cy);
                for (int i = 0; i <= 360; i += 10) {
                    float angle = i * M_PI / 180.0f;
                    glVertex2f(cx + cosf(angle) * radius, cy + sinf(angle) * radius);
                }
                glEnd();

                if (isKing(p)) {
                    glColor3f(1.0f, 0.0f, 0.0f);
                    float inner = radius / 2;

                    glBegin(GL_TRIANGLE_FAN);
                    glVertex2f(cx, cy);
                    for (int i = 0; i <= 360; i += 10) {
                        float angle = i * M_PI / 180.0f;
                        glVertex2f(cx + cosf(angle) * inner, cy + sinf(angle) * inner);
                    }
                    glEnd();
                }
            }
        }
    }
}

bool playerMustCapture() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Piece p = board[x][y];
            if (isWhiteTurn && isWhite(p) && canCaptureFrom(x, y)) return true;
            if (!isWhiteTurn && isBlack(p) && canCaptureFrom(x, y)) return true;
        }
    }
    return false;
}

bool tryValidNormalOrCaptureMove(int x0, int y0, int x1, int y1, int* capX, int* capY, bool* captured) {
    *captured = false;
    *capX = -1;
    *capY = -1;

    Piece p = board[x0][y0];
    if (p == EMPTY) return false;
    if (board[x1][y1] != EMPTY) return false;

    int dx = x1 - x0, dy = y1 - y0;

    if (!isKing(p)) {
        if (abs(dx) == 1 && abs(dy) == 1) {
            if (playerMustCapture()) return false;
            if (isWhite(p) && dy == -1) return true;
            if (isBlack(p) && dy == 1) return true;
            return false;
        }
        if (abs(dx) == 2 && abs(dy) == 2) {
            int midX = x0 + dx / 2, midY = y0 + dy / 2;
            if (isOpponent(p, board[midX][midY])) {
                *captured = true;
                *capX = midX;
                *capY = midY;
                return true;
            }
            return false;
        }
        return false;
    }
    else {
        if (abs(dx) != abs(dy)) return false;

        int stepX = (dx > 0) ? 1 : -1;
        int stepY = (dy > 0) ? 1 : -1;
        int cx = x0 + stepX;
        int cy = y0 + stepY;
        int opponentCount = 0;
        int opX = -1, opY = -1;

        while (cx != x1 && cy != y1) {
            if (board[cx][cy] != EMPTY) {
                if (isOpponent(p, board[cx][cy])) {
                    opponentCount++;
                    opX = cx;
                    opY = cy;
                    if (opponentCount > 1) return false;
                }
                else return false;
            }
            cx += stepX;
            cy += stepY;
        }

        if (opponentCount == 0) {
            if (playerMustCapture()) return false;
            return true;
        }
        else {
            *captured = true;
            *capX = opX;
            *capY = opY;
            return true;
        }
    }
}

void makeMove(int x0, int y0, int x1, int y1, bool captured, int capX, int capY) {
    Piece p = board[x0][y0];
    board[x1][y1] = p;
    board[x0][y0] = EMPTY;

    if (captured && capX >= 0 && capY >= 0) {
        board[capX][capY] = EMPTY;
    }

    if (p == WHITE && y1 == 0) board[x1][y1] = WHITE_KING;
    else if (p == BLACK && y1 == BOARD_SIZE - 1) board[x1][y1] = BLACK_KING;

    if (gameMode == MODE_PVBOT && !isWhiteTurn) {
        lastBotMoveFrom.x = x0;
        lastBotMoveFrom.y = y0;
        lastBotMoveTo.x = x1;
        lastBotMoveTo.y = y1;
    }
}

void updateGameOver() {
    bool whiteExists = false, blackExists = false;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (isWhite(board[x][y])) whiteExists = true;
            if (isBlack(board[x][y])) blackExists = true;
        }
    }

    if (!whiteExists) {
        gameOver = true;
        snprintf(endGameTitle, sizeof(endGameTitle), "Game Over");
        snprintf(endGameMessage, sizeof(endGameMessage), "Black won!");
        return;
    }
    else if (!blackExists) {
        gameOver = true;
        snprintf(endGameTitle, sizeof(endGameTitle), "Game Over");
        snprintf(endGameMessage, sizeof(endGameMessage), "White won!");
        return;
    }

    bool currentPlayerHasMoves = hasValidMoves(isWhiteTurn);
    if (!currentPlayerHasMoves) {
        gameOver = true;
        snprintf(endGameTitle, sizeof(endGameTitle), "Game Over");
        snprintf(endGameMessage, sizeof(endGameMessage), "Draw!");
    }
}

void getAllMoves(bool forWhite, Move* moves, int* moveCount);

int evaluatePosition() {
    int score = 0;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Piece p = board[x][y];
            if (p == WHITE) {
                score += 50;
            }
            else if (p == WHITE_KING) {
                score += 100;
            }
            else if (p == BLACK) {
                score -= 50;
            }
            else if (p == BLACK_KING) {
                score -= 100;
            }
        }
    }

    int whiteMobility = 0, blackMobility = 0;
    Move moves[100];
    int moveCount;

    getAllMoves(true, moves, &moveCount);
    whiteMobility += moveCount;

    getAllMoves(false, moves, &moveCount);
    blackMobility += moveCount;

    score += whiteMobility - blackMobility;
    return score;
}

void getAllMoves(bool forWhite, Move* moves, int* moveCount) {
    *moveCount = 0;
    bool mustCapture = false;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Piece p = board[x][y];
            if ((forWhite && (p == WHITE || p == WHITE_KING)) || (!forWhite && (p == BLACK || p == BLACK_KING))) {
                if (canCaptureFrom(x, y)) {
                    mustCapture = true;
                    break;
                }
            }
        }
        if (mustCapture) break;
    }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Piece p = board[x][y];
            if ((forWhite && (p == WHITE || p == WHITE_KING)) || (!forWhite && (p == BLACK || p == BLACK_KING))) {
                int dirs[4][2] = { {1,1}, {-1,1}, {1,-1}, {-1,-1} };
                for (int d = 0; d < 4; d++) {
                    int dx = dirs[d][0], dy = dirs[d][1];
                    int nx = x + dx, ny = y + dy;
                    if (isKing(p)) {
                        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) {
                                if (!mustCapture) {
                                    moves[*moveCount].from = (Position){ x, y };
                                    moves[*moveCount].to = (Position){ nx, ny };
                                    moves[*moveCount].is_capture = false;
                                    (*moveCount)++;
                                }
                            }
                            else {
                                if (isOpponent(p, board[nx][ny])) {
                                    int cx = nx + dx, cy = ny + dy;
                                    if (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && board[cx][cy] == EMPTY) {
                                        moves[*moveCount].from = (Position){ x, y };
                                        moves[*moveCount].to = (Position){ cx, cy };
                                        moves[*moveCount].capture = (Position){ nx, ny };
                                        moves[*moveCount].is_capture = true;
                                        (*moveCount)++;
                                    }
                                }
                                break;
                            }
                            nx += dx;
                            ny += dy;
                        }
                    }
                    else {
                        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) {
                                if (!mustCapture && ((forWhite && dy == -1) || (!forWhite && dy == 1))) {
                                    moves[*moveCount].from = (Position){ x, y };
                                    moves[*moveCount].to = (Position){ nx, ny };
                                    moves[*moveCount].is_capture = false;
                                    (*moveCount)++;
                                }
                            }
                            else if (isOpponent(p, board[nx][ny])) {
                                int cx = nx + dx, cy = ny + dy;
                                if (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && board[cx][cy] == EMPTY) {
                                    moves[*moveCount].from = (Position){ x, y };
                                    moves[*moveCount].to = (Position){ cx, cy };
                                    moves[*moveCount].capture = (Position){ nx, ny };
                                    moves[*moveCount].is_capture = true;
                                    (*moveCount)++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void makeTempMove(Piece tempBoard[BOARD_SIZE][BOARD_SIZE], Move move) {
    Piece p = tempBoard[move.from.x][move.from.y];
    tempBoard[move.to.x][move.to.y] = p;
    tempBoard[move.from.x][move.from.y] = EMPTY;

    if (move.is_capture) {
        tempBoard[move.capture.x][move.capture.y] = EMPTY;
    }

    if (p == WHITE && move.to.y == 0) tempBoard[move.to.x][move.to.y] = WHITE_KING;
    else if (p == BLACK && move.to.y == BOARD_SIZE - 1) tempBoard[move.to.x][move.to.y] = BLACK_KING;
}

int minimax(Piece tempBoard[BOARD_SIZE][BOARD_SIZE], int depth, int max_depth, bool isMaximizing, int alpha, int beta, uint64_t current_hash) {
    // Проверяем таблицу транспозиций
    TTEntry* entry = tt_lookup(current_hash);
    if (entry != NULL && entry->depth >= max_depth - depth) {
        // Нашли подходящую запись в кэше
        switch (entry->flag) {
        case TT_EXACT:
            return entry->score;
        case TT_LOWER_BOUND:
            alpha = (alpha > entry->score) ? alpha : entry->score;
            break;
        case TT_UPPER_BOUND:
            beta = (beta < entry->score) ? beta : entry->score;
            break;
        }
        if (alpha >= beta) {
            return entry->score;
        }
    }

    // Стандартная терминальная проверка
    if (depth == max_depth) {
        // Используем переданную доску для оценки
        // Временно сохраняем глобальное состояние
        Piece saved_board[BOARD_SIZE][BOARD_SIZE];
        bool saved_isWhiteTurn = isWhiteTurn;

        // Подменяем глобальное состояние на временное
        memcpy(saved_board, board, sizeof(board));
        memcpy(board, tempBoard, sizeof(board));
        isWhiteTurn = isMaximizing; // Для корректной оценки позиции

        int score = evaluatePosition();

        // Восстанавливаем глобальное состояние
        memcpy(board, saved_board, sizeof(board));
        isWhiteTurn = saved_isWhiteTurn;

        return score;
    }

    Move moves[100];
    int moveCount = 0;

    // Временно подменяем глобальную доску для генерации ходов
    Piece saved_board[BOARD_SIZE][BOARD_SIZE];
    bool saved_isWhiteTurn = isWhiteTurn;
    memcpy(saved_board, board, sizeof(board));
    memcpy(board, tempBoard, sizeof(board));
    isWhiteTurn = isMaximizing;

    getAllMoves(isMaximizing, moves, &moveCount);

    // Восстанавливаем глобальную доску
    memcpy(board, saved_board, sizeof(board));
    isWhiteTurn = saved_isWhiteTurn;

    if (moveCount == 0) {
        return isMaximizing ? -1000 : 1000;
    }

    int originalAlpha = alpha;
    int originalBeta = beta;
    int bestScore = isMaximizing ? -10000 : 10000;

    // Перебираем ходы
    for (int i = 0; i < moveCount; i++) {
        // Создаем копию доски для симуляции хода
        Piece newBoard[BOARD_SIZE][BOARD_SIZE];
        memcpy(newBoard, tempBoard, sizeof(Piece) * BOARD_SIZE * BOARD_SIZE);

        // Симулируем ход
        makeTempMove(newBoard, moves[i]);

        // Вычисляем хеш для новой позиции
        uint64_t new_hash = compute_board_hash_for_board(newBoard, !isMaximizing);

        int eval = minimax(newBoard, depth + 1, max_depth, !isMaximizing, alpha, beta, new_hash);

        if (isMaximizing) {
            if (eval > bestScore) bestScore = eval;
            if (eval > alpha) alpha = eval;
            if (beta <= alpha) break;
        }
        else {
            if (eval < bestScore) bestScore = eval;
            if (eval < beta) beta = eval;
            if (beta <= alpha) break;
        }
    }

    // Сохраняем результат в таблицу транспозиций
    int flag = TT_EXACT;
    if (bestScore <= originalAlpha) {
        flag = TT_UPPER_BOUND;
    }
    else if (bestScore >= originalBeta) {
        flag = TT_LOWER_BOUND;
    }

    tt_store(current_hash, max_depth - depth, bestScore, flag);

    return bestScore;
}

void botMove() {
    if (gameMode != MODE_PVBOT || isWhiteTurn || gameOver) return;

    Move moves[100];
    int moveCount = 0;
    getAllMoves(false, moves, &moveCount);

    if (moveCount == 0) {
        updateGameOver();
        return;
    }

    int bestScore = -10000;
    Move bestMove = moves[0];

    // Вычисляем хеш текущей позиции (используем глобальную доску)
    uint64_t current_hash = compute_board_hash();

    for (int i = 0; i < moveCount; i++) {
        // Создаем копию глобальной доски
        Piece newBoard[BOARD_SIZE][BOARD_SIZE];
        memcpy(newBoard, board, sizeof(Piece) * BOARD_SIZE * BOARD_SIZE);

        // Симулируем ход на копии
        makeTempMove(newBoard, moves[i]);

        // Вычисляем хеш для новой позиции (ход перешел к белым)
        uint64_t new_hash = compute_board_hash_for_board(newBoard, true);

        int score = minimax(newBoard, 0, MAX_DEPTH, true, -10000, 10000, new_hash);

        if (score > bestScore) {
            bestScore = score;
            bestMove = moves[i];
        }
    }

    // Реальный ход на глобальной доске
    makeMove(bestMove.from.x, bestMove.from.y, bestMove.to.x, bestMove.to.y,
        bestMove.is_capture, bestMove.capture.x, bestMove.capture.y);

    // Обработка множественных взятий
    while (bestMove.is_capture) {
        Move captureMoves[20];
        int captureMoveCount = 0;
        getAllMoves(false, captureMoves, &captureMoveCount);

        bool foundContinuation = false;
        for (int i = 0; i < captureMoveCount; i++) {
            if (captureMoves[i].from.x == bestMove.to.x &&
                captureMoves[i].from.y == bestMove.to.y &&
                captureMoves[i].is_capture) {

                makeMove(captureMoves[i].from.x, captureMoves[i].from.y,
                    captureMoves[i].to.x, captureMoves[i].to.y,
                    captureMoves[i].is_capture,
                    captureMoves[i].capture.x, captureMoves[i].capture.y);

                bestMove = captureMoves[i];
                foundContinuation = true;
                break;
            }
        }
        if (!foundContinuation) break;
    }

    isWhiteTurn = true;
    updateGameOver();
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !gameOver) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (gameMode == MODE_NONE) {
            handleMenuClick(xpos, ypos, &gameMode);
        }
        else {
            int boardX = (int)(xpos / CELL_SIZE);
            int boardY = (int)(ypos / CELL_SIZE);

            if (boardX < 0 || boardX >= BOARD_SIZE || boardY < 0 || boardY >= BOARD_SIZE)
                return;

            Piece clicked = board[boardX][boardY];
            bool playerTurn = (gameMode == MODE_PVBOT) ? isWhiteTurn : true;

            if (!playerTurn && gameMode == MODE_PVBOT) return;

            if (selected.x == -1) {
                if (clicked != EMPTY) {
                    if ((isWhiteTurn && isWhite(clicked)) || (!isWhiteTurn && isBlack(clicked))) {
                        selected.x = boardX;
                        selected.y = boardY;
                    }
                }
            }
            else {
                int capX, capY;
                bool captured;

                bool valid = tryValidNormalOrCaptureMove(selected.x, selected.y, boardX, boardY, &capX, &capY, &captured);
                if (valid) {
                    makeMove(selected.x, selected.y, boardX, boardY, captured, capX, capY);

                    if (captured && canCaptureFrom(boardX, boardY)) {
                        selected.x = boardX;
                        selected.y = boardY;
                    }
                    else {
                        selected.x = -1;
                        selected.y = -1;
                        isWhiteTurn = !isWhiteTurn;
                    }
                    updateGameOver();
                }
                else {
                    selected.x = -1;
                    selected.y = -1;
                }
            }
        }
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        initBoard();
        selected.x = -1;
        selected.y = -1;
        isWhiteTurn = true;
        gameOver = false;
        windowShouldClose = false;

        lastBotMoveFrom.x = -1;
        lastBotMoveFrom.y = -1;
        lastBotMoveTo.x = -1;
        lastBotMoveTo.y = -1;

        gameMode = MODE_NONE;
    }
}

int main() {
    runTests();
    runBenchmarks();
    if (!glfwInit()) return -1;

    init_zobrist();
    clear_transposition_table();

    GLFWwindow* window = glfwCreateWindow(WINDOW_SIZE, WINDOW_SIZE, "Checkers", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);

    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    initBoard();
    srand((unsigned int)time(NULL));

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_SIZE, WINDOW_SIZE, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    while (!glfwWindowShouldClose(window)) {
        if (gameMode == MODE_NONE) {
            drawMenu();
        }
        else {
            glClear(GL_COLOR_BUFFER_BIT);

            if (gameOver) {
                drawGameOverMessage(endGameTitle, endGameMessage);
            }
            else {
                drawBoardSquares();
                highlightCaptureMoves();
                drawPieces();
            }

            glfwSwapBuffers(window);
        }
        glfwPollEvents();

        if (!gameOver) {
            if (gameMode == MODE_PVBOT && !isWhiteTurn) {
                static double lastMoveTime = 0.0;
                double currentTime = glfwGetTime();
                if (currentTime - lastMoveTime > 0.7) {
                    double start_time = glfwGetTime();
                    botMove();
                    double end_time = glfwGetTime();
                    double time_spent = end_time - start_time;
                    printf("botMove: time = %.4f sec\n", time_spent);
                    lastMoveTime = currentTime;
                }
            }
        }

        if (gameOver && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            initBoard();
            selected.x = -1;
            selected.y = -1;
            isWhiteTurn = true;
            gameOver = false;
            gameMode = MODE_NONE;
        }
    }

    glfwTerminate();
    return 0;
}