#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

// Добавляем определения из checkers.c
#define BOARD_SIZE 8
typedef enum {
    EMPTY, BLACK, WHITE, BLACK_KING, WHITE_KING
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

// Объявления функций из checkers.c
extern void initBoard();
extern int evaluatePosition();
extern bool canCaptureFrom(int x, int y);
extern bool hasValidMoves(bool forWhite);
extern bool playerMustCapture();
extern void getAllMoves(bool forWhite, Move* moves, int* moveCount);
extern int minimax(Piece tempBoard[BOARD_SIZE][BOARD_SIZE], int depth, bool isMaximizing, int alpha, int beta);
extern void botMove();
extern void makeMove(int x0, int y0, int x1, int y1, bool captured, int capX, int capY);
extern bool tryValidNormalOrCaptureMove(int x0, int y0, int x1, int y1, int* capX, int* capY, bool* captured);
extern void updateGameOver();

// Глобальные переменные из checkers.c
extern Piece board[BOARD_SIZE][BOARD_SIZE];

void bench_minimax_debug() {
    Piece tempBoard[BOARD_SIZE][BOARD_SIZE];

    // Копируем доску
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            tempBoard[x][y] = board[x][y];
        }
    }

    printf("Testing minimax depth=2...\n");
    uint64_t start = get_time_ns();
    int result2 = minimax(tempBoard, 2, true, -10000, 10000);
    uint64_t end = get_time_ns();
    printf("minimax depth=2: result=%d, time=%.2f ms\n", result2, (end - start) / 1e6);

    printf("Testing minimax depth=6...\n");
    start = get_time_ns();
    int result6 = minimax(tempBoard, 6, true, -10000, 10000);
    end = get_time_ns();
    printf("minimax depth=6: result=%d, time=%.2f ms\n", result6, (end - start) / 1e6);
}

// Основная функция для запуска тестов
void runTests() {
    bench_minimax_debug();
}