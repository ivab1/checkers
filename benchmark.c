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
#define TT_SIZE 100000
#define TT_EXACT 0
#define TT_LOWER_BOUND 1  
#define TT_UPPER_BOUND 2

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

typedef struct {
    uint32_t hash_low;
    int16_t score;
    uint8_t depth;
    uint8_t flag;
} TTEntry;

// Объявления функций из checkers.c
extern void initBoard();
extern int evaluatePosition();
extern bool canCaptureFrom(int x, int y);
extern bool hasValidMoves(bool forWhite);
extern bool playerMustCapture();
extern void getAllMoves(bool forWhite, Move* moves, int* moveCount);
extern uint64_t compute_board_hash_for_board(Piece tempBoard[BOARD_SIZE][BOARD_SIZE], bool is_white_turn);
extern int minimax(Piece tempBoard[BOARD_SIZE][BOARD_SIZE], int depth, int max_depth, bool isMaximizing, int alpha, int beta, uint64_t current_hash);
extern void botMove();
extern void makeMove(int x0, int y0, int x1, int y1, bool captured, int capX, int capY);
extern bool tryValidNormalOrCaptureMove(int x0, int y0, int x1, int y1, int* capX, int* capY, bool* captured);
extern void updateGameOver();

// Глобальные переменные из checkers.c
extern Piece board[BOARD_SIZE][BOARD_SIZE];
extern TTEntry transposition_table[TT_SIZE];
extern bool isWhiteTurn;

uint64_t get_time_ns() {
#ifdef _WIN32
    LARGE_INTEGER frequency, time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&time);
    return (uint64_t)(time.QuadPart * 1e9 / frequency.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
#endif
}

void run_benchmark(const char* name, void (*func)(void)) {
    const int warmup = 1000;
    const int runs = 10000;

    // Прогрев
    for (int i = 0; i < warmup; i++) {
        func();
    }

    uint64_t start = get_time_ns();
    for (int i = 0; i < runs; i++) {
        func();
    }
    uint64_t end = get_time_ns();

    double avg_ns = (double)(end - start) / runs;
    printf("%-30s: %8.2f ns/call\n", name, avg_ns);
}

// Функции-обертки для бенчмарков
void bench_evaluate() {
    evaluatePosition();
}

void bench_canCapture() {
    canCaptureFrom(2, 2);
}

void bench_hasValidMovesWhite() {
    hasValidMoves(true);
}

void bench_hasValidMovesBlack() {
    hasValidMoves(false);
}

void bench_playerMustCapture() {
    playerMustCapture();
}

void bench_updateGameOver() {
    updateGameOver();
}

void bench_tryValidMove() {
    int capX, capY;
    bool captured;
    tryValidNormalOrCaptureMove(2, 2, 3, 3, &capX, &capY, &captured);
}

void bench_getAllMovesWhite() {
    Move moves[100];
    int moveCount;
    getAllMoves(true, moves, &moveCount);
}

void bench_getAllMovesBlack() {
    Move moves[100];
    int moveCount;
    getAllMoves(false, moves, &moveCount);
}

void bench_minimax_with_tt() {
    Piece tempBoard[BOARD_SIZE][BOARD_SIZE];
    // Копируем текущую доску
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            tempBoard[x][y] = board[x][y];
        }
    }

    uint64_t current_hash = compute_board_hash_for_board(tempBoard, true);
    minimax(tempBoard, 0, 6, true, -10000, 10000, current_hash); // Уменьшили глубину для бенчмарка
}

void bench_compute_hash() {
    compute_board_hash();
}

void bench_tt_lookup() {
    uint64_t hash = compute_board_hash();
    tt_lookup(hash);
}

void bench_simple_move() {
    makeMove(2, 2, 3, 3, false, -1, -1);
}

void runBenchmarks() {
    initBoard();

    printf("=== HIGH-PRECISION BENCHMARKS ===\n\n");

    run_benchmark("initBoard", initBoard);
    run_benchmark("evaluatePosition", bench_evaluate);
    run_benchmark("canCaptureFrom", bench_canCapture);
    run_benchmark("hasValidMoves (white)", bench_hasValidMovesWhite);
    run_benchmark("hasValidMoves (black)", bench_hasValidMovesBlack);
    run_benchmark("playerMustCapture", bench_playerMustCapture);
    run_benchmark("updateGameOver", bench_updateGameOver);
    run_benchmark("tryValidNormalOrCaptureMove", bench_tryValidMove);
    run_benchmark("getAllMoves (white)", bench_getAllMovesWhite);
    run_benchmark("getAllMoves (black)", bench_getAllMovesBlack);
    run_benchmark("compute_board_hash", bench_compute_hash);
    run_benchmark("tt_lookup", bench_tt_lookup);
    run_benchmark("minimax", bench_minimax_with_tt);
    run_benchmark("makeMove", bench_simple_move);

    printf("\n=== BENCHMARKS COMPLETED ===\n");
}