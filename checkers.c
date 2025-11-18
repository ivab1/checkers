#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include <GLFW/glfw3.h> // ���������� ���������� GLFW ��� ������ � ������ � ���������
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> // ���������� ������� ��� ������ �� ��������
#include <string.h>
#include "menu.h" // ���������� ������������ ���� � ��������� ��� ������ � ����

#define BOARD_SIZE 8 // ������ ����� (8�8)
#define CELL_SIZE 80 // ������ ����� ������ � ��������
#define WINDOW_SIZE (BOARD_SIZE * CELL_SIZE) // ����� ������ ����, ������ ������� ����� � ��������
#define MAX_DEPTH 6  // ������� ������ ��� ��������� ��������

extern void runTests();

// ������������, �������������� ���� ����� � ������ ������
typedef enum {
    EMPTY,
    BLACK,
    WHITE,
    BLACK_KING,
    WHITE_KING
} Piece;

// ��������� ��� �������� ��������� (x, y) �� �����
typedef struct {
    int x, y;
} Position;

typedef struct {
    Position from;
    Position to;
    Position capture;
    bool is_capture;
} Move;

Piece board[BOARD_SIZE][BOARD_SIZE]; // ��������� ������, �������������� ������� �����
Position selected = { -1, -1 }; // ������ ������� ��������� �����

bool isWhiteTurn = true; // ����, �����������, ��� ������ ��� (����� ��� ������)
bool gameOver = false; // ����, �����������, ��������� �� ����
bool windowShouldClose = false; // ����, �����������, ����� �� ������� ����

// ������ ��������� ���� ����
Position lastBotMoveFrom = { -1, -1 };
Position lastBotMoveTo = { -1, -1 };

// ����������, ����������� ����� ���� 
// (������� ������ ��������, ������� ������ ����)
enum GameMode {
    MODE_NONE,
    MODE_PVP,
    MODE_PVBOT
};
int gameMode = MODE_NONE;

// ������ ��������� �� ��������� ����
static char endGameTitle[64] = "";
static char endGameMessage[64] = "";

// ���������, �������� �� ����� ������
bool isKing(Piece p) {
    return p == BLACK_KING || p == WHITE_KING;
}

// ���������, �������� �� ����� �����
bool isWhite(Piece p) {
    return p == WHITE || p == WHITE_KING;
}

// ���������, �������� �� ����� ������
bool isBlack(Piece p) {
    return p == BLACK || p == BLACK_KING;
}

// ���������, �������� �� ��� ����� ������������
bool isOpponent(Piece a, Piece b) {
    if (a == EMPTY || b == EMPTY) return false;
    return (isWhite(a) && isBlack(b)) || (isBlack(a) && isWhite(b));
}

// �������������� ������� �����, ���������� ����� �� ��������� �������
void initBoard() {
    for (int y = 0; y < BOARD_SIZE; y++) { // ������
        for (int x = 0; x < BOARD_SIZE; x++) { // �������
            // �������� �� ������ ������ ��� �����
            // ���� ����� ��������, ������ ����� ������, ���� ������ � �����
            if ((x + y) % 2 == 1) {

                // ���� ������ ������ � ��������� � ������ ���� �����
                // � ��� ����������� ������ �����
                if (y < 3) board[x][y] = BLACK;

                // ���� ������ ������ � ��������� � ��������� ���� �����
                // � ��� ����������� ����� �����
                else if (y > 4) board[x][y] = WHITE;

                // ���� ������ ������ � ��������� � ���� 3 ��� 4
                // ��� �������� ������
                else board[x][y] = EMPTY;
            }
            // ���� ������ �����, �� ��� ����� �������� ������
            else {
                board[x][y] = EMPTY;
            }
        }
    }
}

// ���������, ����� �� ����� �� ������� (x, y) ��������� ����� ����������
bool canCaptureFrom(int x, int y) {
    Piece p = board[x][y]; // ��������� ����� � ������ (x, y)
    if (p == EMPTY) return false; // ���� ������ �����, �� ������ ����������

    // ������ dirs �������� ������ ��������� ����������� ��� �������� �����
    // (1, 1) - ���� ������
    // (-1, 1) - ���� �����
    // (1, -1) - ����� ������
    // (-1, -1) - ����� �����
    int dirs[4][2] = { {1, 1}, {-1, 1}, {1, -1}, {-1, -1} };

    for (int d = 0; d < 4; d++) { // ���������� ��� 4 �����������
        int dx = dirs[d][0], dy = dirs[d][1]; // �������� ������� �����������
        int nx = x + dx, ny = y + dy; // ��������� ���������� ��������� ������ � �������� �����������

        // ���� ������������, ���� ���������� ��������� � �������� �����
        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
            // ���� ��������� ������ �� �����
            if (board[nx][ny] != EMPTY) {
                // ���� ����� � ��������� ������ ���������
                if (isOpponent(p, board[nx][ny])) {
                    int cx = nx + dx, cy = ny + dy; // ��������� ���������� ������, ����������� �� ������ ����������
                    // ���� ��� ������ � �������� ����� � ������
                    if (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && board[cx][cy] == EMPTY) {
                        return true; // ������ ��������
                    }
                }
                // ���� ����� � ��������� ������ �� ���������, ���� �����������
                break;
            }
            // ���� ��������� ������ ����� � ����� �� �������� ������, ���� �����������
            if (!isKing(p)) break;
            // ��������� ���������� ��������� ������
            nx += dx;
            ny += dy;
        }
    }
    return false;
}

// ���������, ���� �� � ������ (������ ��� �������) ��������� ����
bool hasValidMoves(bool forWhite) { // ��������� �� ���� ����������, ����� ����� ��� ������
    for (int y = 0; y < BOARD_SIZE; y++) { // ������
        for (int x = 0; x < BOARD_SIZE; x++) { // �������
            Piece p = board[x][y]; // �������� �����, ����������� � ������ (x, y)

            // ���� ��� ����� ��, ��� ������� �� ���� ��������� ����
            if ((forWhite && isWhite(p)) || (!forWhite && isBlack(p))) {
                // ������ dirs �������� ������ ��������� ����������� ��� �������� ����� 
                // (���� ������, ���� �����, ����� ������, ����� �����)
                int dirs[4][2] = { {1,1}, {-1,1}, {1,-1}, {-1,-1} };

                for (int d = 0; d < 4; d++) { // ���������� ��� 4 �����������
                    int dx = dirs[d][0], dy = dirs[d][1]; // �������� ������� �����������

                    // ���� ����� - �����
                    if (isKing(p)) {
                        int nx = x + dx, ny = y + dy; // ��������� ���������� ��������� ������ � �������� �����������
                        // ���� ������������, ���� ���������� ��������� � �������� �����
                        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) return true; // ���� ��������� ������ �����, �� ���� ��������� ���
                            if (board[nx][ny] != EMPTY) break; // ���� �� �����, ���� �����������
                            // ��������� ���������� ��������� ������
                            nx += dx;
                            ny += dy;
                        }
                    }
                    // ���� �� �����, ��������� ������ ���� ������ � �������� �����������
                    else {
                        int nx = x + dx, ny = y + dy; // ��������� ���������� ��������� ������ � �������� �����������
                        // ���� ������ ��������� � �������� ����� � ��� �����
                        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) {
                                // ���� ����� �����, �� ��� ����� ��������� ����� (-1), � ���� ������ - ���� (1)
                                if ((isWhite(p) && dy == -1) || (isBlack(p) && dy == 1)) {
                                    return true; // ���� ��������� ���
                                }
                            }
                        }
                    }
                }
                // ����� �������� ������� ����� ���������, ����� �� ����� �� ������� ������� ��������� ����� ����������
                if (canCaptureFrom(x, y)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// ������������ ������ �����, ������� �����
// �������� ��������� ������ � ��������� ������, �� ������� ������������ ���
void drawBoardSquares() {
    for (int y = 0; y < BOARD_SIZE; y++) { // ������
        for (int x = 0; x < BOARD_SIZE; x++) { // �������
            // ���� ����� ��������� ������ ������
            if ((x + y) % 2 == 0)
                glColor3f(0.8f, 0.8f, 0.8f); // ������������� ���� ��� ������� ������ - ����� (RGB: 0.8, 0.8, 0.8)
            else
                glColor3f(0.5f, 0.3f, 0.1f); // ������������� ���� ��� ������ ������ - ���������� (RGB: 0.5, 0.3, 0.1)

            // ���� ������� ������ �������
            if (selected.x == x && selected.y == y)
                glColor3f(0.2f, 0.8f, 0.2f); // ������������� ������� ���� ������ 

            // ���� ������� ������ ��, �� ������� ������������ ���
            if (lastBotMoveTo.x == x && lastBotMoveTo.y == y)
                glColor3f(0.8f, 0.8f, 0.2f); // ������������� ������ ����, ����� ��������, ���� ��� ���������� ���� �����

            glBegin(GL_QUADS); // �������� ��������� ���������������� (������)
            glVertex2f(x * CELL_SIZE, y * CELL_SIZE); // ������ ������ ������� (������ ����� ����) ������
            glVertex2f((x + 1) * CELL_SIZE, y * CELL_SIZE); // ������ ������ ������� (������ ������ ����)
            glVertex2f((x + 1) * CELL_SIZE, (y + 1) * CELL_SIZE); // ������ ������ ������� (������� ������ ����)
            glVertex2f(x * CELL_SIZE, (y + 1) * CELL_SIZE); // ������ ��������� ������� (������� ����� ����)
            glEnd(); // ��������� ��������� ����������������
        }
    }
}

// ������������ ������, � ������� ����� ��������� ������, �������������� ������� ������
void highlightCaptureMoves() {
    for (int y = 0; y < BOARD_SIZE; y++) { // ������
        for (int x = 0; x < BOARD_SIZE; x++) { // �������
            Piece p = board[x][y]; // �������� �����, ����������� � ������ (x, y)

            // ���� ���� ��������� � ������ "������� ������ ��������" (PVP) ��� "������� ������ ����" (PVBOT) � ������� ��� ����������� ����� ������
            if ((gameMode == MODE_PVP) || (gameMode == MODE_PVBOT && isWhiteTurn)) {
                // ���� ������� ����� ��, ������� ����������� ������, � ����� �� ��� ��������� ����� ����������
                if (((isWhiteTurn && isWhite(p)) || (!isWhiteTurn && isBlack(p))) && canCaptureFrom(x, y)) {
                    glEnable(GL_BLEND); // �������� ����� ����������, ������� ��������� ��������� �������������� �����
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ������������� ������� ����������, ������� ��������� ������������ �����-����� ��� ������������
                    glColor4f(1.0f, 0.0f, 0.0f, 0.3f); // ������������� ���� ��� ��������� ������ (������� � 30% �������������)
                    glBegin(GL_QUADS); // �������� ��������� ���������������� (������)
                    glVertex2f(x * CELL_SIZE, y * CELL_SIZE); // ������ ������ ������� (������ ����� ����)
                    glVertex2f((x + 1) * CELL_SIZE, y * CELL_SIZE); // ������ ������ ������� (������ ������ ����)
                    glVertex2f((x + 1) * CELL_SIZE, (y + 1) * CELL_SIZE); // ������ ������ ������� (������� ������ ����)
                    glVertex2f(x * CELL_SIZE, (y + 1) * CELL_SIZE); // ������ ��������� ������� (������� ����� ����)
                    glEnd(); // ��������� ��������� ����������������
                    glDisable(GL_BLEND); // ��������� ����� ���������� ����� ���������
                }
            }
        }
    }
}

// ������������ ����� �� ����� � ���� �����������
// ����� ����� � �����, ������ � �����-�����
// ����� ���������� ������� ������ ������
void drawPieces() {
    for (int y = 0; y < BOARD_SIZE; y++) { // ������
        for (int x = 0; x < BOARD_SIZE; x++) { // �������
            Piece p = board[x][y]; // �������� �����, ����������� � ������ (x, y)

            // ���� ������ �� ������
            if (p != EMPTY) {
                if (isWhite(p)) glColor3f(1.0f, 1.0f, 1.0f); // ���� ����� �����, ��������������� ����� ���� (RGB: 1.0, 1.0, 1.0)
                else glColor3f(0.1f, 0.1f, 0.1f); // ���� ����� ������, ��������������� �����-����� ���� (RGB: 0.1, 0.1, 0.1)

                float cx = x * CELL_SIZE + CELL_SIZE / 2; // ��������� ���������� ������ �� ��� X ��� ��������� �����
                float cy = y * CELL_SIZE + CELL_SIZE / 2; // ��������� ���������� ������ �� ��� Y ��� ��������� �����
                float radius = CELL_SIZE * 0.35f; // ������������� ������ ����������, �������������� �����, ������ 35% �� ������� ������

                glBegin(GL_TRIANGLE_FAN); // �������� ��������� ���������� � ������� �������������
                glVertex2f(cx, cy); // ������ ����������� ������� ����������

                // ���������� ���� �� 0 �� 360 �������� � ����� 10 �������� ��� �������� ����������
                for (int i = 0; i <= 360; i += 10) {
                    float angle = i * M_PI / 180.0f; // ����������� ���� � �������
                    glVertex2f(cx + cosf(angle) * radius, cy + sinf(angle) * radius); // ��������� ���������� ������ ������� ���������� � ��������� �� � ���������
                }
                glEnd(); // ��������� ��������� ����������

                // ���� ����� - �����
                if (isKing(p)) {
                    glColor3f(1.0f, 0.0f, 0.0f); // ������������� ������� ����
                    float inner = radius / 2; // ������������� ������ ���������� ����������, �������������� �����, ������ �������� ������� ������� �����

                    glBegin(GL_TRIANGLE_FAN); // �������� ��������� ���������� � ������� �������������
                    glVertex2f(cx, cy); // ������ ����������� ������� ����������

                    // ���������� ���� �� 0 �� 360 �������� � ����� 10 �������� ��� �������� ����������
                    for (int i = 0; i <= 360; i += 10) {
                        float angle = i * M_PI / 180.0f; // ����������� ���� � �������
                        glVertex2f(cx + cosf(angle) * inner, cy + sinf(angle) * inner); // ��������� ���������� ������ ������� ���������� � ��������� �� � ���������
                    }
                    glEnd(); // ��������� ��������� ����������
                }
            }
        }
    }
}

// ���������, ������ �� ������� ����� ��������� ������
bool playerMustCapture() {
    for (int y = 0; y < BOARD_SIZE; y++) { // ������
        for (int x = 0; x < BOARD_SIZE; x++) { // �������
            Piece p = board[x][y]; // �������� �����, ����������� � ������ (x, y)
            // ���� ����� ��, ������� ����������� ������, ��� ��� (����� ��� ������) � ����� �� ����� �� ������� ������� ��������� ����� ����������
            // �� ����� ������ ��������� ������
            if (isWhiteTurn && isWhite(p) && canCaptureFrom(x, y)) return true;
            if (!isWhiteTurn && isBlack(p) && canCaptureFrom(x, y)) return true;
        }
    }
    return false;
}

// ���������, �������� �� ��� ���������� (������� ��� ������)
// ���� ������, ���������� ���������� ����������� �����
// x0, y0 - ��������� ���������� ������� �����
// x1, y1 - �������� ���������� ������� �����
// *capX, capY* - ��������� �� ����������, ���� ����� �������� ���������� ����������� �����, ���� ������ ���, � ���� ��� �� ����, �� �������� ��������� -1
// *captured - ��������� �� ������� ����������, ������� ����� ����������� � true, ���� ������ ���������, � � false, ���� ������� �� ����
bool tryValidNormalOrCaptureMove(int x0, int y0, int x1, int y1, int* capX, int* capY, bool* captured) {
    // ������������� ���� ������� � false � �������������� ���������� ����������� �����
    *captured = false;
    *capX = -1;
    *capY = -1;

    Piece p = board[x0][y0]; // �������� �����, ����������� � ������ (x0, y0)
    if (p == EMPTY) return false; // ���� ������ �����, ��� ����������
    if (board[x1][y1] != EMPTY) return false; // ���� �������� ������ �� �����, ��� ����������

    int dx = x1 - x0, dy = y1 - y0; // ��������� ������� ����� ���. � ���. ������������

    // ���� ����� �� �����
    if (!isKing(p)) {
        // ���� ����� ������������ �� ���� ������ �� ���������
        if (abs(dx) == 1 && abs(dy) == 1) {
            if (playerMustCapture()) return false; // ���� ����� ������ ���������, ��� ����������
            if (isWhite(p) && dy == -1) return true; // ���� ����� ����� � ������������ �����, ��� ��������
            if (isBlack(p) && dy == 1) return true; // ���� ����� ������ � ������������ ����, ��� ��������
            return false;
        }
        // ���� ����� ������������ �� ��� ������ �� ���������
        if (abs(dx) == 2 && abs(dy) == 2) {
            int midX = x0 + dx / 2, midY = y0 + dy / 2; // ��������� ���������� ������� ������
            if (isOpponent(p, board[midX][midY])) { // ���� ����� � ������� ������ ���������
                //������������� ���� ������� � ���������� ���������� ����������� �����
                *captured = true;
                *capX = midX;
                *capY = midY;
                return true;
            }
            return false;
        }
        return false;
    }
    // ���� ����� �����
    else {
        if (abs(dx) != abs(dy)) return false; // ���� ����� �� ������������ �� ��������� (������� �� X ������ ���� ����� ������� �� Y)

        // ���������� ����������� �������� �� ���� X � Y
        int stepX = (dx > 0) ? 1 : -1;
        int stepY = (dy > 0) ? 1 : -1;
        // �������������� ������� ���������� ��� �������� ������������� ������
        int cx = x0 + stepX;
        int cy = y0 + stepY;
        int opponentCount = 0; // ������� ��� ����������� �����
        int opX = -1, opY = -1; // ���������� ��� �������� ��������� ����������� �����

        // ��������� ��� ������ ����� ��������� � �������� ���������
        while (cx != x1 && cy != y1) {
            if (board[cx][cy] != EMPTY) { // ���� ������ �� �����
                // ���� ����� ����������, ����������� ������� ����������� ����� � ���������� ���������� ����������� �����
                if (isOpponent(p, board[cx][cy])) {
                    opponentCount++;
                    opX = cx;
                    opY = cy;
                    if (opponentCount > 1) return false; // ���� ��������� ������ 1 �����
                }
                else return false;
            }
            // ���������� ������� ���������� � ��������� ������
            cx += stepX;
            cy += stepY;
        }

        // ���� ��������� 0
        if (opponentCount == 0) {
            if (playerMustCapture()) return false; // ������ �� ����� ���������
            return true;
        }
        // ���� ��������� �����, ������������� ���� ������� � ���������� ���������� ����������� �����
        else {
            *captured = true;
            *capX = opX;
            *capY = opY;
            return true;
        }
    }
}

// ��������� ����������� ����� � ����� ������� �� ������, �������� ��������� �����
// ���� ����� �������� ���������������� ����, ��� ���������� ������
void makeMove(int x0, int y0, int x1, int y1, bool captured, int capX, int capY) {
    Piece p = board[x0][y0]; // �������� ����� �� ��������� �������
    board[x1][y1] = p; // ���������� ����� � �������� �������
    board[x0][y0] = EMPTY; // ������� ��������� �������

    // ���� ��� ������ � ���������� ���������
    if (captured && capX >= 0 && capY >= 0) {
        board[capX][capY] = EMPTY; // ������� ����������� �����
    }

    // ���� ����� �������� ��������� ������
    if (p == WHITE && y1 == 0) board[x1][y1] = WHITE_KING; // ����������� ����� ����� � �����
    else if (p == BLACK && y1 == BOARD_SIZE - 1) board[x1][y1] = BLACK_KING; // ����������� ������ ����� � �����

    // ���� ���� � ������ "������� ������ ����" � ���� ������� ��� ����������� ������
    if (gameMode == MODE_PVBOT && !isWhiteTurn) {
        // ��������� ���������� ��������� � �������� ������� ���������� ���� ����
        lastBotMoveFrom.x = x0;
        lastBotMoveFrom.y = y0;
        lastBotMoveTo.x = x1;
        lastBotMoveTo.y = y1;
    }
}

// �������� �� �������� ��������� ���� � �����������, ����������� �� ���� � ���������� ������ ������ �� ������� ��� ������
void updateGameOver() {
    bool whiteExists = false, blackExists = false; // ���������� ��� ������������ ������� ����� ������ � ������� ������ �� �����

    // ���������� ��� ������ �����
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (isWhite(board[x][y])) whiteExists = true; // ���� � ������� ������ ����� �����, ������������� ����
            if (isBlack(board[x][y])) blackExists = true; // ���� � ������� ������ ������ �����, ������������� ����
        }
    }

    // ���� ����� ����� ������ ���
    if (!whiteExists) {
        // ���� ���������
        gameOver = true;
        snprintf(endGameTitle, sizeof(endGameTitle), "Game Over");
        snprintf(endGameMessage, sizeof(endGameMessage), "Black won!");
        return;
    }
    // ���� ������ ����� ������ ���
    else if (!blackExists) {
        // ���� ���������
        gameOver = true;
        snprintf(endGameTitle, sizeof(endGameTitle), "Game Over");
        snprintf(endGameMessage, sizeof(endGameMessage), "White won!");
        return;
    }

    // ���� �� � �������� ������ (������ ��� �������) ��������� ����
    bool currentPlayerHasMoves = hasValidMoves(isWhiteTurn);
    if (!currentPlayerHasMoves) { // ���� ���
        // ���� ���������
        gameOver = true;
        snprintf(endGameTitle, sizeof(endGameTitle), "Game Over");
        snprintf(endGameMessage, sizeof(endGameMessage), "Draw!");
    }
}

void getAllMoves(bool forWhite, Move* moves, int* moveCount);

// ������� ������ ������� ��� ��������� ��������
int evaluatePosition() {
    int score = 0; // ���������� ��� �������� ������ ������� ������� �� �����

    // ������� ���������
    for (int y = 0; y < BOARD_SIZE; y++) { // ������
        for (int x = 0; x < BOARD_SIZE; x++) { // �������
            Piece p = board[x][y]; // �������� ����� �� ������ � ����������� (�, �)
            // ������ ����� �� �����
            if (p == WHITE) {
                score += 50; // ������� ����� �����
            }
            else if (p == WHITE_KING) {
                score += 100; // �����
            }
            else if (p == BLACK) {
                score -= 50; // ������� ������ �����
            }
            else if (p == BLACK_KING) {
                score -= 100; // �����
            }
        }
    }

    // ���� �����������
    int whiteMobility = 0, blackMobility = 0; // ���������� ��� �������� ���������� ��������� ����� 
    Move moves[100]; // ������ ��� �������� ��������� �����
    int moveCount; // ���������� ��� �������� ���������� ��������� ����� 

    // ������� ��������� ����� ��� �����
    getAllMoves(true, moves, &moveCount);
    whiteMobility += moveCount; // ����������� ������� ��������� ����� ��� �����

    // ������� ��������� ����� ��� ������
    getAllMoves(false, moves, &moveCount);
    blackMobility += moveCount; // ����������� ������� ��������� ����� ��� ������

    score += whiteMobility - blackMobility; // ����������� ������ �� �����������
    // ��� ��������� ���������, ��������� ������� ������ ����� � ������� �������
    return score;
}

// ������� ��� ��������� ���� ��������� �����
// forWhite - ���������� ��������, �����������, ��� ������ ������ (������ ��� �������) ����� �������� ����
// moves - ��������� �� ������, � ������� ����� �������� ��������� ����
// moveCount - ��������� �� ����������, � ������� ����� �������� ���������� ��������� �����
void getAllMoves(bool forWhite, Move* moves, int* moveCount) {
    *moveCount = 0; // moveCount: ��������� �� ����������, � ������� ����� �������� ���������� ��������� �����
    bool mustCapture = false; // ����������, �����������, ���� �� ������������ ������� ��� �������� ������

    // ������� ���������, ���� �� ������������ ������
    for (int y = 0; y < BOARD_SIZE; y++) { // ������
        for (int x = 0; x < BOARD_SIZE; x++) { // �������
            Piece p = board[x][y]; // �������� �����
            // ���� ����� ����������� �������� ������ � ���� ��� ����� ���������
            if ((forWhite && (p == WHITE || p == WHITE_KING)) || (!forWhite && (p == BLACK || p == BLACK_KING))) { 
                if (canCaptureFrom(x, y)) {
                    mustCapture = true;
                    break;
                }
            }
        }
        if (mustCapture) break;
    }

    // �������� ��� ��������� ����
    for (int y = 0; y < BOARD_SIZE; y++) { // ������
        for (int x = 0; x < BOARD_SIZE; x++) { // �������
            Piece p = board[x][y]; // �������� �����
            // ���� ��� ����������� ���. ������
            if ((forWhite && (p == WHITE || p == WHITE_KING)) || (!forWhite && (p == BLACK || p == BLACK_KING))) {
                int dirs[4][2] = { {1,1}, {-1,1}, {1,-1}, {-1,-1} }; // ������ �����������
                // ��������� ��� ������ �����������
                for (int d = 0; d < 4; d++) {
                    int dx = dirs[d][0], dy = dirs[d][1];
                    int nx = x + dx, ny = y + dy; // ���������� ��������� ������ � �������� �����������
                    // ��������� �����
                    if (isKing(p)) {
                        // ��� ����� ��������� ��� ������ �� ���������
                        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) { // ���� ��������� ������ ������
                                if (!mustCapture) { // � ��� ����������� �������
                                    // ��������� ���� ��� � ������
                                    moves[*moveCount].from = (Position){ x, y };
                                    moves[*moveCount].to = (Position){ nx, ny };
                                    moves[*moveCount].is_capture = false;
                                    (*moveCount)++;
                                }
                            }
                            else { // ���� � ��������� ������ ����� ����������
                                if (isOpponent(p, board[nx][ny])) {
                                    int cx = nx + dx, cy = ny + dy; // ��������� ��������� �� ��� ������
                                    if (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && board[cx][cy] == EMPTY) {
                                        // ��������� ���� ������ � ������
                                        moves[*moveCount].from = (Position){ x, y };
                                        moves[*moveCount].to = (Position){ cx, cy };
                                        moves[*moveCount].capture = (Position){ nx, ny };
                                        moves[*moveCount].is_capture = true;
                                        (*moveCount)++;
                                    }
                                }
                                break;
                            }
                            // ������� ��������� ���
                            nx += dx;
                            ny += dy;
                        }
                    }
                    // ��������� ������� �����
                    else {
                        // ��� ������� ����� ��������� ������ ���� ������ � �������� �����������
                        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) { // ���� ��������� ������ ����� � ����� ����� ���������
                                if (!mustCapture && ((forWhite && dy == -1) || (!forWhite && dy == 1))) {
                                   // ��������� ���� ��� � ������
                                    moves[*moveCount].from = (Position){ x, y };
                                    moves[*moveCount].to = (Position){ nx, ny };
                                    moves[*moveCount].is_capture = false;
                                    (*moveCount)++;
                                }
                            }
                            // ���� � ��������� ������ ���������
                            else if (isOpponent(p, board[nx][ny])) {
                                // ��������� ���������� ��������� �� ��� ������
                                int cx = nx + dx, cy = ny + dy;
                                if (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && board[cx][cy] == EMPTY) {
                                    // ��������� � ������ ���� ������
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

// ������� ��� ���������� ���� �� ����� �����
// tempBoard - ��������� ������, �������������� ��������� �����, �� ������� ����� �������� ���
// move - ��������� Move, ���������� ���������� � ����, ������� ����� ��������� (������ � ���� ������������ �����, � ����� ���������� � �������)
void makeTempMove(Piece tempBoard[BOARD_SIZE][BOARD_SIZE], Move move) {
    Piece p = tempBoard[move.from.x][move.from.y]; // �������� �����, ����������� � ������, �� ������� ����� �������� ��� (��������� �������)
    tempBoard[move.to.x][move.to.y] = p; // ���������� ����� �� ��������� ������� � �������� ������� �� ��������� �����
    tempBoard[move.from.x][move.from.y] = EMPTY; // ������� ��������� �������, ������������ �� � ��������� ������ ������
    
    // ���� ������ ���
    if (move.is_capture) {
        // ������� ����������� �����, ������������ ��������������� ������ �� ��������� ����� � ��������� ������ ������
        tempBoard[move.capture.x][move.capture.y] = EMPTY;
    }

    // ����������� � �����
    // ���� ����� �������� ���������������� ���� �����, ��� ������������ � �����
    if (p == WHITE && move.to.y == 0) tempBoard[move.to.x][move.to.y] = WHITE_KING;
    else if (p == BLACK && move.to.y == BOARD_SIZE - 1) tempBoard[move.to.x][move.to.y] = BLACK_KING;
}

// ����������� ������� ��������
// tempBoard - ��������� �����, �� ������� ����� ����������� ����
// depth - ������� ������� ��������
// isMaximizing - ���������� ��������, �����������, �������� �� ������� ����� ��������������� ��� �������������� 
// alpha � beta - �������� ��� �����-���� ���������, ������� �������� �������������� ������� ������
int minimax(Piece tempBoard[BOARD_SIZE][BOARD_SIZE], int depth, bool isMaximizing, int alpha, int beta) {
    // �������� �� ������������ �������
    if (depth == MAX_DEPTH) {
        return evaluatePosition(); // ���������� ������ ������� �������
    }

    Move moves[100]; // ������ ��� �������� ��������� �����
    int moveCount = 0; // ���������� ��������� �����
    getAllMoves(isMaximizing, moves, &moveCount); // �������� ��� ��������� ����

    if (moveCount == 0) { // ���� ����� ���
        return isMaximizing ? -1000 : 1000; // �������� ��� ���������������� ������ (-1000) ��� ������� ��� ��������������� (1000)
    }
    // ���� ���. ����� �����������������
    if (isMaximizing) {
        int maxEval = -10000; // ������������ ������
        // ���������� ��� ��������� ����
        for (int i = 0; i < moveCount; i++) {
            // ������� ����� ����� ��� ������
            Piece newBoard[BOARD_SIZE][BOARD_SIZE];
            // �������� ���. ��������� ��������� ����� � �����
            memcpy(newBoard, tempBoard, sizeof(Piece) * BOARD_SIZE * BOARD_SIZE);
            makeTempMove(newBoard, moves[i]); // ��������� ��������� ���

            // ������� ��������� ������� ������� ��� ��� ����������������� ������
            int eval = minimax(newBoard, depth + 1, false, alpha, beta);
            // ��������� ������������ ������ � �����
            maxEval = eval > maxEval ? eval : maxEval;
            alpha = alpha > eval ? alpha : eval;
            if (beta <= alpha) break; // ���� �����, �� �������
        }
        return maxEval;
    }
    // ���� ���. ����� ����������������
    else {
        int minEval = 10000; // ����������� ������
        // ���������� ��� ��������� ����
        for (int i = 0; i < moveCount; i++) {
            // ������� ����� ����� ��� ������
            Piece newBoard[BOARD_SIZE][BOARD_SIZE];
            // �������� ���. ��������� ��������� ����� � �����
            memcpy(newBoard, tempBoard, sizeof(Piece) * BOARD_SIZE * BOARD_SIZE);
            makeTempMove(newBoard, moves[i]); // ��������� ��������� ���

            // ������� ��������� ������� ������� ��� ��� ������������������ ������
            int eval = minimax(newBoard, depth + 1, true, alpha, beta);
            // ��������� ����������� ������ � ����
            minEval = eval < minEval ? eval : minEval;
            beta = beta < eval ? beta : eval;
            if (beta <= alpha) break; // ���� �����, �� �������
        }
        return minEval;
    }
}

void botMove() {
    if (gameMode != MODE_PVBOT || isWhiteTurn || gameOver) return;

    Move moves[100]; // ������ ��� �������� �����
    int moveCount = 0; // ���������� �����
    getAllMoves(false, moves, &moveCount);  // �������� ��� ��������� ���� ��� ������� ������ (����) � ��������� �� � ������� moves, false - ��� ������ (����)

    // ���� ����� ���, �� ��������� ����
    if (moveCount == 0) {
        updateGameOver();
        return;
    }

    int bestScore = -10000; // ������ ������
    Move bestMove = moves[0]; // ������ ���

    // ���������� ��� ��������� ����
    for (int i = 0; i < moveCount; i++) {
        Piece newBoard[BOARD_SIZE][BOARD_SIZE]; // ������� ����� ����� ��� ������
        // �������� ������� ��������� �������� ����� � ����� �����
        memcpy(newBoard, board, sizeof(Piece) * BOARD_SIZE * BOARD_SIZE);
        makeTempMove(newBoard, moves[i]); // ��������� ��������� ��� �� ����� �����

        // �������� ������� minimax() ��� ������ ������� ����� ���������� ����
        int score = minimax(newBoard, 0, true, -10000, 10000);  // true - ��������� ��� �����

        // ���� ������� ������ �����, ��� ���������� ��������� ������
        if (score > bestScore) {
            // �� ������
            bestScore = score;
            bestMove = moves[i];
        }
    }

    // ��������� ������ ���
    makeMove(bestMove.from.x, bestMove.from.y, bestMove.to.x, bestMove.to.y,
        bestMove.is_capture, bestMove.capture.x, bestMove.capture.y);

    // ���� ��� ������, ��������� ����������� ����������� �������� � ����� �����������
    while (bestMove.is_capture) {
        Move captureMoves[20]; // ������ ��� �������� ��������� ��������
        int captureMoveCount = 0; // ���������� ��������
        // �������� ��� ��������� ������� ��� ������� ������ (����)
        getAllMoves(false, captureMoves, &captureMoveCount);

        // �������������� ����, �����������, ��� �� ������ ��������������� ������
        bool foundContinuation = false;

        // ���� ����� ��������� ������ � ����� �������
        for (int i = 0; i < captureMoveCount; i++) {
            //  ���� ��������� ������� ������� ��������� � �������� �������� ����������� ���� � ��� ������
            if (captureMoves[i].from.x == bestMove.to.x &&
                captureMoves[i].from.y == bestMove.to.y &&
                captureMoves[i].is_capture) {

                // ��������� ������
                makeMove(captureMoves[i].from.x, captureMoves[i].from.y,
                    captureMoves[i].to.x, captureMoves[i].to.y,
                    captureMoves[i].is_capture,
                    captureMoves[i].capture.x, captureMoves[i].capture.y);

                bestMove = captureMoves[i]; // ��������� ������� ��� �� ����� ������
                foundContinuation = true; // ������������� ����, �����������, ��� ��� ������ ��������������� ������
                break;
            }
        }

        if (!foundContinuation) break;
    }
    // ����� ����� ����� ����� � ��������� ����������� �� ����
    isWhiteTurn = true;
    updateGameOver();
}
// �������� �� �������������� ������������ � ����������� ����������� ����, ������� ����� ����� � ���������� �����
// GLFWwindow* window - ��������� �� ���� GLFW, � ������� ��������� ������� ������� ������ ����, ��������� ������� ������, ��� ������ ���� ��������� �������
// button - ������������� ��������, �������������� ������ ����, ������� ���� ������ ��� ��������
// action - ������ ������ ��� �������
// mods - ����������, ����� �������������� ������� ���� ������ �� ����� �����: Shift, Ctrl, Alt � �.�.
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // ���� �� ������ ����� ������ ����, ��������� �� ������� (�� ����������) � �� ��������� �� ����
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !gameOver) {
        double xpos, ypos; // ���������� ��� �������� ��������� �������
        glfwGetCursorPos(window, &xpos, &ypos); // �������� ������� ���������� ������� ����

        if (gameMode == MODE_NONE) { // ���� ���� ��� �� ��������
            handleMenuClick(xpos, ypos, &gameMode); // ��������� ����� �� ����
        }
        else {
            // ��������� ���������� �� �����, ���� ������� ������� �� ������ ������
            int boardX = (int)(xpos / CELL_SIZE);
            int boardY = (int)(ypos / CELL_SIZE);

            // ��������� �� ���������� � �������� �����
            if (boardX < 0 || boardX >= BOARD_SIZE || boardY < 0 || boardY >= BOARD_SIZE)
                return; // ���� ���

            Piece clicked = board[boardX][boardY]; // �������� �����, ����������� � ������ (boardX, boardY)
            bool playerTurn = (gameMode == MODE_PVBOT) ? isWhiteTurn : true; // ����������, ����������� �� ������� ��� ������ (���� ���� ������ ����, �� ���������, ��� ������ ���)

            // ���� ������� ��� �� ����������� ������ � ������ "������� ������ ����"
            if (!playerTurn && gameMode == MODE_PVBOT) return;

            if (selected.x == -1) { // ���� ����� �� �������
                if (clicked != EMPTY) { // ���� ������� ������ �� �����
                    // ���� ����� ����������� ���. ������
                    if ((isWhiteTurn && isWhite(clicked)) || (!isWhiteTurn && isBlack(clicked))) {
                        // ������������� ���������� ��������� �����
                        selected.x = boardX;
                        selected.y = boardY;
                    }
                }
            }
            else { // ���� �������
                // ���������� ��� �������
                int capX, capY;
                bool captured;

                // �������� �� ��� ���������� (���������� ��� ���������)
                bool valid = tryValidNormalOrCaptureMove(selected.x, selected.y, boardX, boardY, &capX, &capY, &captured);
                if (valid) { // ���� ����������
                    makeMove(selected.x, selected.y, boardX, boardY, captured, capX, capY); // ��������� ���

                    if (captured && canCaptureFrom(boardX, boardY)) { // ���� ����� ����� ���������� �����������
                        // ���������� ������
                        selected.x = boardX;
                        selected.y = boardY;
                    }
                    else {
                        // ���������� �����
                        selected.x = -1;
                        selected.y = -1;
                        isWhiteTurn = !isWhiteTurn; // ����������� ��� �� ��. ������
                    }
                    updateGameOver(); // ���������, ����������� �� ���� ����� ����
                }
                else {
                    // ���������� �����
                    selected.x = -1;
                    selected.y = -1;
                }
            }
        }
    }
}

// �������� �� ���������� ��������, ��������� � ������������ ������
// GLFWwindow* window - ��������� �� ���� GLFW, � ������� ��������� ������� ������� ������ ����, ��������� ������� ������, ��� ������ ���� ��������� �������
// key - ������� �������
// scancode - ����-��� ������� �������
// action - ������� ������ ��� ���
// mods - ����������, ����� �������������� ������� ���� ������ �� ����� �����: Shift, Ctrl, Alt � �.�.
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // ���� ���� �� ������ ������� Escape
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        initBoard(); // ���������� ��������� ����� � ����������
        // ���������� ��������� �����
        selected.x = -1;
        selected.y = -1;
        isWhiteTurn = true; // ��� ������
        gameOver = false; // ���� �� ���������
        windowShouldClose = false; // ���������� ����, �����������, ��� ���� �� ������ �����������

        // ���������� ���������� ���������� ���� ����
        lastBotMoveFrom.x = -1;
        lastBotMoveFrom.y = -1;
        // ���������� ���������� �������� ������� ���������� ���� ����
        lastBotMoveTo.x = -1;
        lastBotMoveTo.y = -1;

        gameMode = MODE_NONE; // ���� ��� �� �������� ��� ��������� � ����
    }
}

int main() {
    runTests(); // ������ ������
    runBenchmarks(); // ������ ����������
    // �������������� ���������� GLFW
    // ���� ������������� �� �������, ��������� ��������� ������ � ����� -1
    if (!glfwInit()) return -1;

    // ������� ���� � ��������� ��������� � ����������
    GLFWwindow* window = glfwCreateWindow(WINDOW_SIZE, WINDOW_SIZE, "Checkers", NULL, NULL);
    // ���� ���� �� ���������
    if (!window) {
        glfwTerminate(); // ��������� ������
        return -1;
    }

    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE); // ������������� ������� ����, �������� ��� ��������� �������

    glfwMakeContextCurrent(window); // ������ ��������� ���� ������� ���������� OpenGL, ��� ��������� ��������� OpenGL ������� ��� ����� ����
    glfwSetMouseButtonCallback(window, mouseButtonCallback); // ������������� ������� ��������� ������ ��� ��������� ������� ������� ������ ����
    glfwSetKeyCallback(window, keyCallback); // ������������� ������� ��������� ������ ��� ��������� ������� ������� ������

    initBoard(); // �������������� ������� �����, ������������ ��������� ������� �����
    srand((unsigned int)time(NULL)); // �������������� ��������� ��������� �����, ��������� ������� �����, ����� ���������� �����������

    glMatrixMode(GL_PROJECTION); // ������������� ������� ������� ��������
    glLoadIdentity(); // ���������� ������� ������� ��������
    glOrtho(0, WINDOW_SIZE, WINDOW_SIZE, 0, -1, 1); // ������ ��������������� ��������, ��� (0, 0) � ��� ������ ����� ����, � (WINDOW_SIZE, WINDOW_SIZE) � ������� ������ ����
    glMatrixMode(GL_MODELVIEW); // ����������� ������� �� ������� ������/����

    // ���� ���� �� �������
    while (!glfwWindowShouldClose(window)) {
        // ���� ���� ��� �� ��������, ������������ ����
        if (gameMode == MODE_NONE) {
            drawMenu();
        }
        else {
            glClear(GL_COLOR_BUFFER_BIT); // ������� �����

            if (gameOver) { // ���� ���� ��������
                drawGameOverMessage(endGameTitle, endGameMessage); // ������������ ��������� � ���������� ����
            }
            else {
                drawBoardSquares(); // ������������ ������� �����
                highlightCaptureMoves(); // �������� ��������� �������
                drawPieces(); // ������������ �����
            }

            glfwSwapBuffers(window); // ������ �������� � ������ ������, ��������� ������������ ���������� �� ������
        }
        glfwPollEvents(); // ������������ ��� �������, ����� ��� ������� ������ � �������� ����

        // ���� ���� �� �������� � ����� ���� "������� ������ ����", � ������ ��� ������� ������
        if (!gameOver) {
            if (gameMode == MODE_PVBOT && !isWhiteTurn) {
                static double lastMoveTime = 0.0; // ���������� ��� ������������ ������� ���������� ���� ����
                double currentTime = glfwGetTime(); // �������� ������� �����
                // ���� ������ ����� 0.7 ������ � ���������� ����
                if (currentTime - lastMoveTime > 0.7) {
                    double start_time = glfwGetTime();
                    botMove(); // ��� ����
                    double end_time = glfwGetTime();
                    double time_spent = end_time - start_time;
                    printf("botMove: time = %.4f sec\n", time_spent);
                    lastMoveTime = currentTime; // ����������� �����
                }
            }
        }

        // ���� ���� �������� � ������ ������� R
        if (gameOver && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            // ����� �������������� ����� � ���������� ��� ����������� ����������, ����� ������ ����� ����
            initBoard();
            selected.x = -1;
            selected.y = -1;
            isWhiteTurn = true;
            gameOver = false;
            gameMode = MODE_NONE;
        }
    }
    // ��������� ������ GLFW, ���������� ��� �������, ��������� � �����������
    glfwTerminate();
    return 0;
}
