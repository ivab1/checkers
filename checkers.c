#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include <glfw3.h> // Подключает библиотеку GLFW для работы с окнами и событиями
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> // Подключает функции для работы со временем
#include "menu.h" // Подключает заголовочный файл с функциями для работы с меню

#define BOARD_SIZE 8 // Размер доски (8х8)
#define CELL_SIZE 80 // Размер одной клетки в пикселях
#define WINDOW_SIZE (BOARD_SIZE * CELL_SIZE) // Общий размер окна, равный размеру доски в пикселях
#define MAX_DEPTH 6  // Глубина поиска для алгоритма минимакс

// Перечисление, представляющее типы шашек и пустые клетки
typedef enum {
    EMPTY,
    BLACK,
    WHITE,
    BLACK_KING,
    WHITE_KING
} Piece;

// Структура для хранения координат (x, y) на доске
typedef struct {
    int x, y;
} Position;

typedef struct {
    Position from;
    Position to;
    Position capture;
    bool is_capture;
} Move;

Piece board[BOARD_SIZE][BOARD_SIZE]; // Двумерный массив, представляющий игровую доску
Position selected = { -1, -1 }; // Хранит текущую выбранную шашку

bool isWhiteTurn = true; // Флаг, указывающий, чей сейчас ход (белых или черных)
bool gameOver = false; // Флаг, указывающий, закончена ли игра
bool windowShouldClose = false; // Флаг, указывающий, нужно ли закрыть окно

// Хранят последние ходы бота
Position lastBotMoveFrom = { -1, -1 };
Position lastBotMoveTo = { -1, -1 };

// Переменная, указывающая режим игры 
// (человек против человека, человек против бота)
enum GameMode {
    MODE_NONE,
    MODE_PVP,
    MODE_PVBOT
};
int gameMode = MODE_NONE;

// Хранят сообщения об окончании игры
static char endGameTitle[64] = "";
static char endGameMessage[64] = "";

// Проверяет, является ли шашка дамкой
bool isKing(Piece p) {
    return p == BLACK_KING || p == WHITE_KING;
}

// Проверяет, является ли шашка белой
bool isWhite(Piece p) {
    return p == WHITE || p == WHITE_KING;
}

// Проверяет, является ли шашка черной
bool isBlack(Piece p) {
    return p == BLACK || p == BLACK_KING;
}

// Проверяет, являются ли две шашки противниками
bool isOpponent(Piece a, Piece b) {
    if (a == EMPTY || b == EMPTY) return false;
    return (isWhite(a) && isBlack(b)) || (isBlack(a) && isWhite(b));
}

// Инициализирует игровую доску, расставляя шашки на начальные позиции
void initBoard() {
    for (int y = 0; y < BOARD_SIZE; y++) { // Строки
        for (int x = 0; x < BOARD_SIZE; x++) { // Столбцы
            // Является ли клетка черной или белой
            // Если сумма нечетная, клетка будет черной, если четная — белой
            if ((x + y) % 2 == 1) {

                // Если клетка черная и находится в первых трех рядах
                // В ней размещается черная шашка
                if (y < 3) board[x][y] = BLACK;

                // Если клетка черная и находится в последних трех рядах
                // В ней размещается белая шашка
                else if (y > 4) board[x][y] = WHITE;

                // Если клетка черная и находится в ряду 3 или 4
                // Она остается пустой
                else board[x][y] = EMPTY;
            }
            // Если клетка белая, то она также остается пустой
            else {
                board[x][y] = EMPTY;
            }
        }
    }
}

// Проверяет, может ли шашка на позиции (x, y) захватить шашку противника
bool canCaptureFrom(int x, int y) {
    Piece p = board[x][y]; // Получение шашки в клетке (x, y)
    if (p == EMPTY) return false; // Если клетка пуста, то захват невозможен

    // Массив dirs содержит четыре возможных направления для движения шашки
    // (1, 1) - вниз вправо
    // (-1, 1) - вниз влево
    // (1, -1) - вверх вправо
    // (-1, -1) - вверх влево
    int dirs[4][2] = { {1, 1}, {-1, 1}, {1, -1}, {-1, -1} };

    for (int d = 0; d < 4; d++) { // Перебираем все 4 направления
        int dx = dirs[d][0], dy = dirs[d][1]; // Получаем текущее направление
        int nx = x + dx, ny = y + dy; // Вычисляем координаты следующей клетки в заданном направлении

        // Цикл продолжается, пока координаты находятся в пределах доски
        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
            // Если следующая клетка не пуста
            if (board[nx][ny] != EMPTY) {
                // Если шашка в следующей клетке противник
                if (isOpponent(p, board[nx][ny])) {
                    int cx = nx + dx, cy = ny + dy; // Вычисляем координаты клетки, находящейся за шашкой противника
                    // Если эта клетка в пределах доски и пустая
                    if (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && board[cx][cy] == EMPTY) {
                        return true; // Захват возможен
                    }
                }
                // Если шашка в следующей клетке не противник, цикл прерывается
                break;
            }
            // Если следующая клетка пуста и шашка не является дамкой, цикл прерывается
            if (!isKing(p)) break;
            // Вычисляем координаты следующей клетки
            nx += dx;
            ny += dy;
        }
    }
    return false;
}

// Проверяет, есть ли у игрока (белого или черного) доступные ходы
bool hasValidMoves(bool forWhite) { // Принимаем на вход информацию, белая шашка или черная
    for (int y = 0; y < BOARD_SIZE; y++) { // Строки
        for (int x = 0; x < BOARD_SIZE; x++) { // Столбцы
            Piece p = board[x][y]; // Получаем шашку, находящуюся в клетке (x, y)

            // Если это шашка та, для которой мы ищем доступные ходы
            if ((forWhite && isWhite(p)) || (!forWhite && isBlack(p))) {
                // Массив dirs содержит четыре возможных направления для движения шашки 
                // (вниз вправо, вниз влево, вверх вправо, вверх влево)
                int dirs[4][2] = { {1,1}, {-1,1}, {1,-1}, {-1,-1} };

                for (int d = 0; d < 4; d++) { // Перебираем все 4 направления
                    int dx = dirs[d][0], dy = dirs[d][1]; // Получаем текущее направление

                    // Если шашка - дамка
                    if (isKing(p)) {
                        int nx = x + dx, ny = y + dy; // Вычисляем координаты следующей клетки в заданном направлении
                        // Цикл продолжается, пока координаты находятся в пределах доски
                        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) return true; // Если следующая клетка пуста, то есть доступный ход
                            if (board[nx][ny] != EMPTY) break; // Если не пуста, цикл прерывается
                            // Вычисляем координаты следующей клетки
                            nx += dx;
                            ny += dy;
                        }
                    }
                    // Если не дамка, проверяем только одну клетку в заданном направлении
                    else {
                        int nx = x + dx, ny = y + dy; // Вычисляем координаты следующей клетки в заданном направлении
                        // Если клетка находится в пределах доски и она пуста
                        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) {
                                // Если шашка белая, то она может двигаться вверх (-1), а если черная - вниз (1)
                                if ((isWhite(p) && dy == -1) || (isBlack(p) && dy == 1)) {
                                    return true; // Есть доступный ход
                                }
                            }
                        }
                    }
                }
                // После проверки обычних ходов проверяем, может ли шашка на текущей позиции захватить шашку противника
                if (canCaptureFrom(x, y)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Отрисовывает клетки доски, чередуя цвета
// Выделяет выбранную клетку и последнюю клетку, на которую переместился бот
void drawBoardSquares() {
    for (int y = 0; y < BOARD_SIZE; y++) { // Строки
        for (int x = 0; x < BOARD_SIZE; x++) { // Столбцы
            // Если сумма координат клетки четная
            if ((x + y) % 2 == 0)
                glColor3f(0.8f, 0.8f, 0.8f); // Устанавливаем цвет для светлой клетки - серый (RGB: 0.8, 0.8, 0.8)
            else
                glColor3f(0.5f, 0.3f, 0.1f); // Устанавливаем цвет для темной клетки - коричневый (RGB: 0.5, 0.3, 0.1)

            // Если текущая клетка выбрана
            if (selected.x == x && selected.y == y)
                glColor3f(0.2f, 0.8f, 0.2f); // Устанавливаем зеленый цвет клетки 

            // Если текущая клетка та, на которую переместился бот
            if (lastBotMoveTo.x == x && lastBotMoveTo.y == y)
                glColor3f(0.8f, 0.8f, 0.2f); // Устанавливаем желтый цвет, чтобы показать, куда бот переместил свою шашку

            glBegin(GL_QUADS); // Начинаем отрисовку четырехугольника (клетки)
            glVertex2f(x * CELL_SIZE, y * CELL_SIZE); // Задаем первую вершину (нижний левый угол) клетки
            glVertex2f((x + 1) * CELL_SIZE, y * CELL_SIZE); // Задаем вторую вершину (нижний правый угол)
            glVertex2f((x + 1) * CELL_SIZE, (y + 1) * CELL_SIZE); // Задаем третью вершину (верхний правый угол)
            glVertex2f(x * CELL_SIZE, (y + 1) * CELL_SIZE); // Задаем четвертую вершину (верхний левый угол)
            glEnd(); // Завершаем отрисовку четырехугольника
        }
    }
}

// Подсвечивает клетки, с которых можно выполнить захват, полупрозрачным красным цветом
void highlightCaptureMoves() {
    for (int y = 0; y < BOARD_SIZE; y++) { // Строки
        for (int x = 0; x < BOARD_SIZE; x++) { // Столбцы
            Piece p = board[x][y]; // Получаем шашку, находящуюся в клетке (x, y)

            // Если игра находится в режиме "человек против человека" (PVP) или "человек против бота" (PVBOT) и текущий ход принадлежит белым шашкам
            if ((gameMode == MODE_PVP) || (gameMode == MODE_PVBOT && isWhiteTurn)) {
                // Если текущая шашка та, которая принадлежит игроку, и может ли она захватить шашку противника
                if (((isWhiteTurn && isWhite(p)) || (!isWhiteTurn && isBlack(p))) && canCaptureFrom(x, y)) {
                    glEnable(GL_BLEND); // Включаем режим смешивания, который позволяет создавать полупрозрачные цвета
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Устанавливаем функцию смешивания, которая позволяет использовать альфа-канал для прозрачности
                    glColor4f(1.0f, 0.0f, 0.0f, 0.3f); // Устанавливаем цвет для подсветки клетки (красный с 30% прозрачностью)
                    glBegin(GL_QUADS); // Начинаем отрисовку четырехугольника (клетки)
                    glVertex2f(x * CELL_SIZE, y * CELL_SIZE); // Задаем первую вершину (нижний левый угол)
                    glVertex2f((x + 1) * CELL_SIZE, y * CELL_SIZE); // Задаем вторую вершину (нижний правый угол)
                    glVertex2f((x + 1) * CELL_SIZE, (y + 1) * CELL_SIZE); // Задаем третью вершину (верхний правый угол)
                    glVertex2f(x * CELL_SIZE, (y + 1) * CELL_SIZE); // Задаем четвертую вершину (верхний левый угол)
                    glEnd(); // Завершаем отрисовку четырехугольника
                    glDisable(GL_BLEND); // Отключаем режим смешивания после отрисовки
                }
            }
        }
    }
}

// Отрисовывает шашки на доске в виде окружностей
// Белые шашки — белые, черные — темно-серые
// Дамки выделяются красным кругом внутри
void drawPieces() {
    for (int y = 0; y < BOARD_SIZE; y++) { // Строки
        for (int x = 0; x < BOARD_SIZE; x++) { // Столбцы
            Piece p = board[x][y]; // Получаем шашку, находящуюся в клетке (x, y)

            // Если клетка не пустая
            if (p != EMPTY) {
                if (isWhite(p)) glColor3f(1.0f, 1.0f, 1.0f); // Если шашка белая, устанавливается белый цвет (RGB: 1.0, 1.0, 1.0)
                else glColor3f(0.1f, 0.1f, 0.1f); // Если шашка черная, устанавливается темно-серый цвет (RGB: 0.1, 0.1, 0.1)

                float cx = x * CELL_SIZE + CELL_SIZE / 2; // Вычисляем координату центра по оси X для отрисовки шашки
                float cy = y * CELL_SIZE + CELL_SIZE / 2; // Вычисляем координату центра по оси Y для отрисовки шашки
                float radius = CELL_SIZE * 0.35f; // Устанавливаем радиус окружности, представляющей шашку, равный 35% от размера клетки

                glBegin(GL_TRIANGLE_FAN); // Начинаем отрисовку окружности с помощью треугольников
                glVertex2f(cx, cy); // Задаем центральную вершину окружности

                // Перебираем углы от 0 до 360 градусов с шагом 10 градусов для создания окружности
                for (int i = 0; i <= 360; i += 10) {
                    float angle = i * M_PI / 180.0f; // Преобразуем угол в радианы
                    glVertex2f(cx + cosf(angle) * radius, cy + sinf(angle) * radius); // Вычисляем координаты каждой вершины окружности и добавляет их в отрисовку
                }
                glEnd(); // Завершаем отрисовку окружности

                // Если шашка - дамка
                if (isKing(p)) {
                    glColor3f(1.0f, 0.0f, 0.0f); // Устанавливаем красный цвет
                    float inner = radius / 2; // Устанавливаем радиус внутренней окружности, представляющей дамку, равный половине радиуса обычной шашки

                    glBegin(GL_TRIANGLE_FAN); // Начинаем отрисовку окружности с помощью треугольников
                    glVertex2f(cx, cy); // Задаем центральную вершину окружности

                    // Перебираем углы от 0 до 360 градусов с шагом 10 градусов для создания окружности
                    for (int i = 0; i <= 360; i += 10) {
                        float angle = i * M_PI / 180.0f; // Преобразуем угол в радианы
                        glVertex2f(cx + cosf(angle) * inner, cy + sinf(angle) * inner); // Вычисляем координаты каждой вершины окружности и добавляет их в отрисовку
                    }
                    glEnd(); // Завершаем отрисовку окружности
                }
            }
        }
    }
}

// Проверяет, должен ли текущий игрок выполнить захват
bool playerMustCapture() {
    for (int y = 0; y < BOARD_SIZE; y++) { // Строки
        for (int x = 0; x < BOARD_SIZE; x++) { // Столбцы
            Piece p = board[x][y]; // Получаем шашку, находящуюся в клетке (x, y)
            // Если шашка та, которая принадлежит игроку, чей ход (белые или черные) и может ли шашка на текущей позиции захватить шашку противника
            // то игрок обязан выполнить захват
            if (isWhiteTurn && isWhite(p) && canCaptureFrom(x, y)) return true;
            if (!isWhiteTurn && isBlack(p) && canCaptureFrom(x, y)) return true;
        }
    }
    return false;
}

// Проверяет, является ли ход допустимым (обычный или захват)
// Если захват, возвращает координаты захваченной шашки
// x0, y0 - начальные координаты позиции шашки
// x1, y1 - конечные координаты позиции шашки
// *capX, capY* - указатели на переменные, куда будут записаны координаты захваченной шашки, если захват был, а если его не было, то значение останется -1
// *captured - указатель на булевую переменную, которая будет установлена в true, если захват произошел, и в false, если захвата не было
bool tryValidNormalOrCaptureMove(int x0, int y0, int x1, int y1, int* capX, int* capY, bool* captured) {
    // Устанавливаем флаг захвата в false и инициализируем координаты захваченной шашки
    *captured = false;
    *capX = -1;
    *capY = -1;

    Piece p = board[x0][y0]; // Получаем шашку, находящуюся в клетке (x0, y0)
    if (p == EMPTY) return false; // Если клетка пуста, ход невозможен
    if (board[x1][y1] != EMPTY) return false; // Если конечная клетка не пуста, ход невозможен

    int dx = x1 - x0, dy = y1 - y0; // Вычисляем разницу между нач. и кон. координатами

    // Если шашка не дамка
    if (!isKing(p)) {
        // Если шашка перемещается на одну клетку по диагонали
        if (abs(dx) == 1 && abs(dy) == 1) {
            if (playerMustCapture()) return false; // Если игрок обязан захватить, ход невозможен
            if (isWhite(p) && dy == -1) return true; // Если шашка белая и перемещается вверх, ход возможен
            if (isBlack(p) && dy == 1) return true; // Если шашка черная и перемещается вниз, ход возможен
            return false;
        }
        // Если шашка перемещается на две клетки по диагонали
        if (abs(dx) == 2 && abs(dy) == 2) {
            int midX = x0 + dx / 2, midY = y0 + dy / 2; // Вычисляем координаты средней клетки
            if (isOpponent(p, board[midX][midY])) { // Если шашка в средней клетке противник
                //Устанавливаем флаг захвата и записываем переменные захваченной шашки
                *captured = true;
                *capX = midX;
                *capY = midY;
                return true;
            }
            return false;
        }
        return false;
    }
    // Если шашка дамка
    else {
        if (abs(dx) != abs(dy)) return false; // Если дамка не перемещается по диагонали (разница по X должна быть равна разнице по Y)

        // Определяем направление движения по осям X и Y
        int stepX = (dx > 0) ? 1 : -1;
        int stepY = (dy > 0) ? 1 : -1;
        // Инициализируем текущие координаты для проверки промежуточных клеток
        int cx = x0 + stepX;
        int cy = y0 + stepY;
        int opponentCount = 0; // Счетчик для захваченных шашек
        int opX = -1, opY = -1; // Переменные для хранения координат захваченной шашки

        // Проверяем все клетки между начальной и конечной позициями
        while (cx != x1 && cy != y1) {
            if (board[cx][cy] != EMPTY) { // Если клетка не пуста
                // Если шашка противника, увеличивает счетчик захваченных шашек и записываем координаты захваченной шашки
                if (isOpponent(p, board[cx][cy])) {
                    opponentCount++;
                    opX = cx;
                    opY = cy;
                    if (opponentCount > 1) return false; // Если захвачено больше 1 шашки
                }
                else return false;
            }
            // Перемещаем текущие координаты к следующей клетке
            cx += stepX;
            cy += stepY;
        }

        // Если захвачено 0
        if (opponentCount == 0) {
            if (playerMustCapture()) return false; // Обязан ли игрок захватить
            return true;
        }
        // Если захвачена шашка, устанавливает флаг захвата и записываем координаты захваченной шашки
        else {
            *captured = true;
            *capX = opX;
            *capY = opY;
            return true;
        }
    }
}

// Выполняет перемещение шашки с одной позиции на другую, обновляя состояние доски
// Если шашка достигла противоположного края, она становится дамкой
void makeMove(int x0, int y0, int x1, int y1, bool captured, int capX, int capY) {
    Piece p = board[x0][y0]; // Получаем шашку из начальной позиции
    board[x1][y1] = p; // Перемещаем шашку в конечную позицию
    board[x0][y0] = EMPTY; // Очищаем начальную позицию

    // Если был захват и координаты корректны
    if (captured && capX >= 0 && capY >= 0) {
        board[capX][capY] = EMPTY; // Удаляем захваченную шашку
    }

    // Если шашка достигла последней строки
    if (p == WHITE && y1 == 0) board[x1][y1] = WHITE_KING; // Превращение белой шашки в дамку
    else if (p == BLACK && y1 == BOARD_SIZE - 1) board[x1][y1] = BLACK_KING; // Превращение черной шашки в дамку

    // Если игра в режиме "человек против бота" и если текущий ход принадлежит черным
    if (gameMode == MODE_PVBOT && !isWhiteTurn) {
        // Сохраняем координаты начальной и конечной позиций последнего хода бота
        lastBotMoveFrom.x = x0;
        lastBotMoveFrom.y = y0;
        lastBotMoveTo.x = x1;
        lastBotMoveTo.y = y1;
    }
}

// Отвечает за проверку состояния игры и определение, закончилась ли игра в результате победы одного из игроков или ничьей
void updateGameOver() {
    bool whiteExists = false, blackExists = false; // Переменные для отслеживания наличия шашек белого и черного игрока на доске

    // Перебираем все клетки доски
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (isWhite(board[x][y])) whiteExists = true; // Если в текущей клетке белая шашка, устанавливаем флаг
            if (isBlack(board[x][y])) blackExists = true; // Если в текущей клетке черная шашка, устанавливаем флаг
        }
    }

    // Если белых шашек больше нет
    if (!whiteExists) {
        // Игра закончена
        gameOver = true;
        snprintf(endGameTitle, sizeof(endGameTitle), "Game Over");
        snprintf(endGameMessage, sizeof(endGameMessage), "Black won!");
        return;
    }
    // Если черных шашек больше нет
    else if (!blackExists) {
        // Игра закончена
        gameOver = true;
        snprintf(endGameTitle, sizeof(endGameTitle), "Game Over");
        snprintf(endGameMessage, sizeof(endGameMessage), "White won!");
        return;
    }

    // Есть ли у текущего игрока (белого или черного) доступные ходы
    bool currentPlayerHasMoves = hasValidMoves(isWhiteTurn);
    if (!currentPlayerHasMoves) { // Если нет
        // Игра закончего
        gameOver = true;
        snprintf(endGameTitle, sizeof(endGameTitle), "Game Over");
        snprintf(endGameMessage, sizeof(endGameMessage), "Draw!");
    }
}

void getAllMoves(bool forWhite, Move* moves, int* moveCount);

// Функция оценки позиции для алгоритма минимакс
int evaluatePosition() {
    int score = 0; // Переменная для хранения оценки текущей позиции на доске

    // Подсчет материала
    for (int y = 0; y < BOARD_SIZE; y++) { // Строки
        for (int x = 0; x < BOARD_SIZE; x++) { // Столбцы
            Piece p = board[x][y]; // Получаем шашку из клетки в координатах (х, у)
            // Оценка шашек на доске
            if (p == WHITE) {
                score += 50; // Обычная белая шашка
            }
            else if (p == WHITE_KING) {
                score += 100; // Дамка
            }
            else if (p == BLACK) {
                score -= 50; // Обычная черная шашка
            }
            else if (p == BLACK_KING) {
                score -= 100; // Дамка
            }
        }
    }

    // Учет мобильности
    int whiteMobility = 0, blackMobility = 0; // Переменные для хранения количества доступных ходов 
    Move moves[100]; // Массив для хранения возможных ходов
    int moveCount; // Переменные для хранения количества возможных ходов 

    // Подсчет доступных ходов для белых
    getAllMoves(true, moves, &moveCount);
    whiteMobility += moveCount; // Увеличиваем счетчик доступных ходов для белых

    // Подсчет доступных ходов для черных
    getAllMoves(false, moves, &moveCount);
    blackMobility += moveCount; // Увеличиваем счетчик доступных ходов для черных

    score += whiteMobility - blackMobility; // Увеличиваем оценку за мобильность
    // Это позволяет учитывать, насколько активен каждый игрок в текущей позиции
    return score;
}

// Функция для получения всех возможных ходов
// forWhite - логическое значение, указывающее, для какого игрока (белого или черного) нужно получить ходы
// moves - указатель на массив, в который будут записаны возможные ходы
// moveCount - указатель на переменную, в которую будет записано количество возможных ходов
void getAllMoves(bool forWhite, Move* moves, int* moveCount) {
    *moveCount = 0; // moveCount: указатель на переменную, в которую будет записано количество возможных ходов
    bool mustCapture = false; // Переменная, указывающая, есть ли обязательные захваты для текущего игрока

    // Сначала проверяем, есть ли обязательные взятия
    for (int y = 0; y < BOARD_SIZE; y++) { // Строки
        for (int x = 0; x < BOARD_SIZE; x++) { // Столбцы
            Piece p = board[x][y]; // Получаем шашку
            // Если шашка принадлежит текущему игроку и если она может захватить
            if ((forWhite && (p == WHITE || p == WHITE_KING)) || (!forWhite && (p == BLACK || p == BLACK_KING))) { 
                if (canCaptureFrom(x, y)) {
                    mustCapture = true;
                    break;
                }
            }
        }
        if (mustCapture) break;
    }

    // Собираем все возможные ходы
    for (int y = 0; y < BOARD_SIZE; y++) { // Строки
        for (int x = 0; x < BOARD_SIZE; x++) { // Столбцы
            Piece p = board[x][y]; // Получаем шашку
            // Если она принадлежит тек. игроку
            if ((forWhite && (p == WHITE || p == WHITE_KING)) || (!forWhite && (p == BLACK || p == BLACK_KING))) {
                int dirs[4][2] = { {1,1}, {-1,1}, {1,-1}, {-1,-1} }; // Массив направлений
                // Проверяем все четыре направления
                for (int d = 0; d < 4; d++) {
                    int dx = dirs[d][0], dy = dirs[d][1];
                    int nx = x + dx, ny = y + dy; // Координаты следующей клетки в заданном направлении
                    // Обработка дамок
                    if (isKing(p)) {
                        // Для дамки проверяем все клетки по диагонали
                        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) { // Если следующая клетка пустая
                                if (!mustCapture) { // И нет возможности захвата
                                    // Добавляем этот ход в массив
                                    moves[*moveCount].from = (Position){ x, y };
                                    moves[*moveCount].to = (Position){ nx, ny };
                                    moves[*moveCount].is_capture = false;
                                    (*moveCount)++;
                                }
                            }
                            else { // Если в следующей клетке шашка противника
                                if (isOpponent(p, board[nx][ny])) {
                                    int cx = nx + dx, cy = ny + dy; // Вычисляем следующую за ней клетку
                                    if (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && board[cx][cy] == EMPTY) {
                                        // Добавляем этот захват в массив
                                        moves[*moveCount].from = (Position){ x, y };
                                        moves[*moveCount].to = (Position){ cx, cy };
                                        moves[*moveCount].capture = (Position){ nx, ny };
                                        moves[*moveCount].is_capture = true;
                                        (*moveCount)++;
                                    }
                                }
                                break;
                            }
                            // Смотрим следующий ход
                            nx += dx;
                            ny += dy;
                        }
                    }
                    // Обработка обычных шашек
                    else {
                        // Для обычной шашки проверяем только одну клетку в заданном направлении
                        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                            if (board[nx][ny] == EMPTY) { // Если следующая клетка пуста и шашка может двигаться
                                if (!mustCapture && ((forWhite && dy == -1) || (!forWhite && dy == 1))) {
                                   // Добавляем этот ход в массив
                                    moves[*moveCount].from = (Position){ x, y };
                                    moves[*moveCount].to = (Position){ nx, ny };
                                    moves[*moveCount].is_capture = false;
                                    (*moveCount)++;
                                }
                            }
                            // Если в следующей клетке противник
                            else if (isOpponent(p, board[nx][ny])) {
                                // Вычисляем координаты следующей за ней клетки
                                int cx = nx + dx, cy = ny + dy;
                                if (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && board[cx][cy] == EMPTY) {
                                    // Добавляем в массив этот захват
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

// Функция для выполнения хода на копии доски
// tempBoard - двумерный массив, представляющий временную доску, на которой будет выполнен ход
// move - структура Move, содержащая информацию о ходе, который нужно выполнить (откуда и куда перемещается шашка, а также информация о захвате)
void makeTempMove(Piece tempBoard[BOARD_SIZE][BOARD_SIZE], Move move) {
    Piece p = tempBoard[move.from.x][move.from.y]; // Получаем шашку, находящуюся в клетке, из которой будет выполнен ход (начальная позиция)
    tempBoard[move.to.x][move.to.y] = p; // Перемещаем шашку из начальной позиции в конечную позицию на временной доске
    tempBoard[move.from.x][move.from.y] = EMPTY; // Очищаем начальную позицию, устанавливая ее в состояние пустой клетки
    
    // Если захват был
    if (move.is_capture) {
        // Удаляем захваченную шашку, устанавливая соответствующую клетку на временной доске в состояние пустой клетки
        tempBoard[move.capture.x][move.capture.y] = EMPTY;
    }

    // Превращение в дамку
    // Если шашка достигла противоположного края доски, она превращается в даску
    if (p == WHITE && move.to.y == 0) tempBoard[move.to.x][move.to.y] = WHITE_KING;
    else if (p == BLACK && move.to.y == BOARD_SIZE - 1) tempBoard[move.to.x][move.to.y] = BLACK_KING;
}

// Рекурсивная функция минимакс
// tempBoard - временная доска, на которой будут оцениваться ходы
// depth - текущая глубина рекурсии
// isMaximizing - логическое значение, указывающее, является ли текущий игрок максимизирующим или минимизирующим 
// alpha и beta - значения для альфа-бета отсечения, которые помогают оптимизировать процесс поиска
int minimax(Piece tempBoard[BOARD_SIZE][BOARD_SIZE], int depth, bool isMaximizing, int alpha, int beta) {
    // Достигли ли максимальной глубины
    if (depth == MAX_DEPTH) {
        return evaluatePosition(); // Возвращаем оценку текущей позиции
    }

    Move moves[100]; // Массив для хранения возможных ходов
    int moveCount = 0; // Количество возможных ходов
    getAllMoves(isMaximizing, moves, &moveCount); // Получаем все возможные ходы

    if (moveCount == 0) { // Если ходов нет
        return isMaximizing ? -1000 : 1000; // Проигрыш для максимизирующего игрока (-1000) или выигрыш для минимизирующего (1000)
    }
    // Если тек. игрок максимизированный
    if (isMaximizing) {
        int maxEval = -10000; // Максимальная оценка
        // Перебираем все возможные ходы
        for (int i = 0; i < moveCount; i++) {
            // Создаем новую доску для оценки
            Piece newBoard[BOARD_SIZE][BOARD_SIZE];
            // Копируем тек. состояние временной доски в новую
            memcpy(newBoard, tempBoard, sizeof(Piece) * BOARD_SIZE * BOARD_SIZE);
            makeTempMove(newBoard, moves[i]); // Выполняем временный ход

            // Смотрим следующий уровень глубины уже для минимизированного игрока
            int eval = minimax(newBoard, depth + 1, false, alpha, beta);
            // Обновляем максимальную оценку и альфа
            maxEval = eval > maxEval ? eval : maxEval;
            alpha = alpha > eval ? alpha : eval;
            if (beta <= alpha) break; // Если нашли, то выходим
        }
        return maxEval;
    }
    // Если тек. игрок минимизированный
    else {
        int minEval = 10000; // Минимальная оценка
        // Перебираем все возможные ходы
        for (int i = 0; i < moveCount; i++) {
            // Создаем новую доску для оценки
            Piece newBoard[BOARD_SIZE][BOARD_SIZE];
            // Копируем тек. состояние временной доски в новую
            memcpy(newBoard, tempBoard, sizeof(Piece) * BOARD_SIZE * BOARD_SIZE);
            makeTempMove(newBoard, moves[i]); // Выполняем временный ход

            // Смотрим следующий уровень глубины уже для максимизированного игрока
            int eval = minimax(newBoard, depth + 1, true, alpha, beta);
            // Обновляем минимальную оценку и бета
            minEval = eval < minEval ? eval : minEval;
            beta = beta < eval ? beta : eval;
            if (beta <= alpha) break; // Если нашли, то выходим
        }
        return minEval;
    }
}

void botMove() {
    if (gameMode != MODE_PVBOT || isWhiteTurn || gameOver) return;

    Move moves[100]; // Массив для хранения ходов
    int moveCount = 0; // Количество ходов
    getAllMoves(false, moves, &moveCount);  // Получаем все возможные ходы для черного игрока (бота) и сохраняем их в массиве moves, false - для черных (бота)

    // Если ходов нет, то завершаем игру
    if (moveCount == 0) {
        updateGameOver();
        return;
    }

    int bestScore = -10000; // Лучшая оценка
    Move bestMove = moves[0]; // Лучший ход

    // Перебираем все возможные ходы
    for (int i = 0; i < moveCount; i++) {
        Piece newBoard[BOARD_SIZE][BOARD_SIZE]; // Создаем новую доску для оценки
        // Копируем текущее состояние основной доски в новую доску
        memcpy(newBoard, board, sizeof(Piece) * BOARD_SIZE * BOARD_SIZE);
        makeTempMove(newBoard, moves[i]); // Выполняем временный ход на новой доске

        // Вызываем функцию minimax() для оценки позиции после выполнения хода
        int score = minimax(newBoard, 0, true, -10000, 10000);  // true - следующий ход белых

        // Если текущая оценка лучше, чем предыдущая наилучшая оценка
        if (score > bestScore) {
            // То меняем
            bestScore = score;
            bestMove = moves[i];
        }
    }

    // Выполняем лучший ход
    makeMove(bestMove.from.x, bestMove.from.y, bestMove.to.x, bestMove.to.y,
        bestMove.is_capture, bestMove.capture.x, bestMove.capture.y);

    // Если был захват, проверяем возможность продолжения захватов в любом направлении
    while (bestMove.is_capture) {
        Move captureMoves[20]; // Массив для хранения возможных захватов
        int captureMoveCount = 0; // Количество захватов
        // Получаем все возможные захваты для черного игрока (бота)
        getAllMoves(false, captureMoves, &captureMoveCount);

        // Инициализируем флаг, указывающий, был ли найден продолжительный захват
        bool foundContinuation = false;

        // Ищем любой возможный захват с новой позиции
        for (int i = 0; i < captureMoveCount; i++) {
            //  Если начальная позиция захвата совпадает с конечной позицией предыдущего хода и это захват
            if (captureMoves[i].from.x == bestMove.to.x &&
                captureMoves[i].from.y == bestMove.to.y &&
                captureMoves[i].is_capture) {

                // Выполняем захват
                makeMove(captureMoves[i].from.x, captureMoves[i].from.y,
                    captureMoves[i].to.x, captureMoves[i].to.y,
                    captureMoves[i].is_capture,
                    captureMoves[i].capture.x, captureMoves[i].capture.y);

                bestMove = captureMoves[i]; // Обновляем текущий ход на новый захват
                foundContinuation = true; // Устанавливаем флаг, указывающий, что был найден продолжительный захват
                break;
            }
        }

        if (!foundContinuation) break;
    }
    // После всего черед белых и проверяем закончилась ли игра
    isWhiteTurn = true;
    updateGameOver();
}
// Отвечает за взаимодействие пользователя с графическим интерфейсом игры, включая выбор шашек и выполнение ходов
// GLFWwindow* window - указатель на окно GLFW, в котором произошло событие нажатия кнопки мыши, позволяет функции узнать, для какого окна произошло событие
// button - целочисленное значение, представляющее кнопку мыши, которая была нажата или отпущена
// action - кнопка нажата или опущена
// mods - показывает, какие дополнительные клавиши были нажаты во время клика: Shift, Ctrl, Alt и т.д.
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Была ли нажата левая кнопка мыши, произошло ли нажатие (не отпускание) и не закончена ли игра
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !gameOver) {
        double xpos, ypos; // Переменные для хранения координат курсора
        glfwGetCursorPos(window, &xpos, &ypos); // Получаем текущие координаты курсора мыши

        if (gameMode == MODE_NONE) { // Если игра еще не началась
            handleMenuClick(xpos, ypos, &gameMode); // Обработка клика по меню
        }
        else {
            // Вычисляем координаты на доске, деля позицию курсора на размер клетки
            int boardX = (int)(xpos / CELL_SIZE);
            int boardY = (int)(ypos / CELL_SIZE);

            // Находятся ли координаты в пределах доски
            if (boardX < 0 || boardX >= BOARD_SIZE || boardY < 0 || boardY >= BOARD_SIZE)
                return; // Если нет

            Piece clicked = board[boardX][boardY]; // Получаем шашку, находящуюся в клетке (boardX, boardY)
            bool playerTurn = (gameMode == MODE_PVBOT) ? isWhiteTurn : true; // Определяем, принадлежит ли текущий ход игроку (если игра против бота, то проверяет, чей сейчас ход)

            // Если текущий ход не принадлежит игроку в режиме "человек против бота"
            if (!playerTurn && gameMode == MODE_PVBOT) return;

            if (selected.x == -1) { // Если шашка не выбрана
                if (clicked != EMPTY) { // Если нажатая клетка не пуста
                    // Если шашка принадлежит тек. игроку
                    if ((isWhiteTurn && isWhite(clicked)) || (!isWhiteTurn && isBlack(clicked))) {
                        // Устанавливаем координаты выбранной шашки
                        selected.x = boardX;
                        selected.y = boardY;
                    }
                }
            }
            else { // Если выбрана
                // Переменные для захвата
                int capX, capY;
                bool captured;

                // Является ли ход допустимым (нормальный или захватный)
                bool valid = tryValidNormalOrCaptureMove(selected.x, selected.y, boardX, boardY, &capX, &capY, &captured);
                if (valid) { // Если допустимый
                    makeMove(selected.x, selected.y, boardX, boardY, captured, capX, capY); // Выполняем ход

                    if (captured && canCaptureFrom(boardX, boardY)) { // Если игрок может продолжить захватывать
                        // Продолжаем захват
                        selected.x = boardX;
                        selected.y = boardY;
                    }
                    else {
                        // Сбрасываем выбор
                        selected.x = -1;
                        selected.y = -1;
                        isWhiteTurn = !isWhiteTurn; // Переключаем ход на др. игрока
                    }
                    updateGameOver(); // Проверяем, закончилась ли игра после хода
                }
                else {
                    // Сбрасываем выбор
                    selected.x = -1;
                    selected.y = -1;
                }
            }
        }
    }
}

// Отвечает за выполнение действий, связанных с клавиатурным вводом
// GLFWwindow* window - указатель на окно GLFW, в котором произошло событие нажатия кнопки мыши, позволяет функции узнать, для какого окна произошло событие
// key - нажатая клавиша
// scancode - скан-код нажатой клавиши
// action - клавиша нажата или нет
// mods - показывает, какие дополнительные клавиши были нажаты во время клика: Shift, Ctrl, Alt и т.д.
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Если была ли нажата клавиша Escape
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        initBoard(); // Сбрасываем состояние доски к начальному
        // Сбрасываем выбранную шашку
        selected.x = -1;
        selected.y = -1;
        isWhiteTurn = true; // Ход белого
        gameOver = false; // Игра не закончена
        windowShouldClose = false; // Сбрасывает флаг, указывающий, что окно не должно закрываться

        // Сбрасываем координаты последнего хода бота
        lastBotMoveFrom.x = -1;
        lastBotMoveFrom.y = -1;
        // Сбрасываем координаты конечной позиции последнего хода бота
        lastBotMoveTo.x = -1;
        lastBotMoveTo.y = -1;

        gameMode = MODE_NONE; // Игра еще не началась или вернулась в меню
    }
}

int main() {
    // Инициализируем библиотеку GLFW
    // Если инициализация не удалась, программа завершает работу с кодом -1
    if (!glfwInit()) return -1;

    // Создаем окно с заданными размерами и заголовком
    GLFWwindow* window = glfwCreateWindow(WINDOW_SIZE, WINDOW_SIZE, "Checkers", NULL, NULL);
    // Если окно не создалось
    if (!window) {
        glfwTerminate(); // Завершаем работу
        return -1;
    }

    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE); // Устанавливаем атрибут окна, запрещая его изменение размера

    glfwMakeContextCurrent(window); // Делаем созданное окно текущим контекстом OpenGL, что позволяет выполнять OpenGL команды для этого окна
    glfwSetMouseButtonCallback(window, mouseButtonCallback); // Устанавливаем функцию обратного вызова для обработки событий нажатия кнопок мыши
    glfwSetKeyCallback(window, keyCallback); // Устанавливаем функцию обратного вызова для обработки событий нажатия клавиш

    initBoard(); // Инициализируем игровую доску, устанавливая начальные позиции шашек
    srand((unsigned int)time(NULL)); // Инициализирует генератор случайных чисел, используя текущее время, чтобы обеспечить случайность

    glMatrixMode(GL_PROJECTION); // Устанавливаем текущую матрицу проекции
    glLoadIdentity(); // Сбрасываем текущую матрицу проекции
    glOrtho(0, WINDOW_SIZE, WINDOW_SIZE, 0, -1, 1); // Задаем ортографическую проекцию, где (0, 0) — это нижний левый угол, а (WINDOW_SIZE, WINDOW_SIZE) — верхний правый угол
    glMatrixMode(GL_MODELVIEW); // Переключаем обратно на матрицу модели/вида

    // Пока окно не закрыто
    while (!glfwWindowShouldClose(window)) {
        // Если игра еще не началась, отрисовывает меню
        if (gameMode == MODE_NONE) {
            drawMenu();
        }
        else {
            glClear(GL_COLOR_BUFFER_BIT); // Очищаем экран

            if (gameOver) { // Если игра окончена
                drawGameOverMessage(endGameTitle, endGameMessage); // Отрисовываем сообщение о завершении игры
            }
            else {
                drawBoardSquares(); // Отрисовываем игровую доску
                highlightCaptureMoves(); // Выделяем возможные захваты
                drawPieces(); // Отрисовываем шашки
            }

            glfwSwapBuffers(window); // Меняем передний и задний буферы, отображая отрисованное содержимое на экране
        }
        glfwPollEvents(); // Обрабатываем все события, такие как нажатия клавиш и движения мыши

        // Если игра не окончена и режим игры "человек против бота", и сейчас ход черного игрока
        if (!gameOver) {
            if (gameMode == MODE_PVBOT && !isWhiteTurn) {
                static double lastMoveTime = 0.0; // Переменная для отслеживания времени последнего хода бота
                double currentTime = glfwGetTime(); // Получаем текущее время
                // Если прошло более 0.7 секунд с последнего хода
                if (currentTime - lastMoveTime > 0.7) {
                    double start_time = glfwGetTime();
                    botMove(); // Ход бота
                    double end_time = glfwGetTime();
                    double time_spent = end_time - start_time;
                    printf("botMove: time = %.4f sec\n", time_spent);
                    lastMoveTime = currentTime; // Обновляется время
                }
            }
        }

        // Если игра окончена и нажата клавиша R
        if (gameOver && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            // Снова инициализирует доску и сбрасывает все необходимые переменные, чтобы начать новую игру
            initBoard();
            selected.x = -1;
            selected.y = -1;
            isWhiteTurn = true;
            gameOver = false;
            gameMode = MODE_NONE;
        }
    }
    // Завершает работу GLFW, освобождая все ресурсы, связанные с библиотекой
    glfwTerminate();
    return 0;
}
