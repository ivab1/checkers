#include "menu.h" // Подключаем заголовочный файл меню menu.h (с объявлениями функций меню)
#include "stb_easy_font.h" // Подключаем библиотеку stb_easy_font.h — легковесный шрифт для быстрой отрисовки текста с примитивным OpenGL
// Эта директива нужна для того, чтобы в данном .c файле была сгенерирована реализация функций из stb_easy_font.h
#define STB_EASY_FONT_IMPLEMENTATION

void drawEasyText(float x, float y, const char* text, float scale) {
    static char buffer[99999]; // Большой буфер для вершинных данных текста
    int quads = stb_easy_font_print(0, 0, (char*)text, NULL, buffer, sizeof(buffer)); // Массив вершин для текста, возвращающий число четырехугольников (quads) для отрисовки

    glPushMatrix(); // Сохраняем текущую матрицу
    glTranslatef(x, y, 0); // Сдвигаем ее на (х, у) (z = 0)
    glScalef(scale, scale, 1); // Масштабируем по размеру текста х и у на scale, а z на 1

    glColor3f(1, 1, 1); // Белый текст
    glEnableClientState(GL_VERTEX_ARRAY); // Включаем клиентский режим для использования массива вершин
    glVertexPointer(2, GL_FLOAT, 16, buffer); // Определяем указатель на вершины (2 - координаты (х, у), 16 - шаг между вершинами (16 байт = 4 float * 4 байта)
    glDrawArrays(GL_QUADS, 0, quads * 4); // Рисуем массив квадов (0 - начальный индекс в массиве, quads * 4 -  общее кол-во вершин)
    glDisableClientState(GL_VERTEX_ARRAY); // Отключаем клиентский режим

    glPopMatrix(); // Восстанавливаем предыдущую матрицу
}

void drawMenu() {
    glClear(GL_COLOR_BUFFER_BIT); // Очищаем экран

    // Background
    // Рисуем квадрат-заливку на всю область (640x640), цвет — темно-серый
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(640, 0);
    glVertex2f(640, 640);
    glVertex2f(0, 640);
    glEnd();

    // Title
    // Рисуем название игры большими цифрами с масштабом 5 в координатах (200,120)
    drawEasyText(200, 120, "CHECKERS", 5);

    // Button 1 (2 игрока)
    // Рисуем кнопку (серый прямоугольник) для "2 PLAYERS" в заданных координатах
    // Надпись "2 PLAYERS" рисуется с масштабом 3 в середине кнопки
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(125, 250);
    glVertex2f(525, 250);
    glVertex2f(525, 350);
    glVertex2f(125, 350);
    glEnd();
    drawEasyText(255, 290, "2 PLAYERS", 3);

    // Button 2 (против ии)
    // Рисуем вторую кнопку (потемнее)
    // Надпись "VS AI" с масштабом 3 чуть ниже первой кнопки
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(125, 400);
    glVertex2f(525, 400);
    glVertex2f(525, 500);
    glVertex2f(125, 500);
    glEnd();
    drawEasyText(285, 440, "VS AI", 3);

    glfwSwapBuffers(glfwGetCurrentContext()); // Меняем буферы окна, чтобы отрисованное появилось на экране
}

// Проверяет координаты клика мыши
void handleMenuClick(double xpos, double ypos, int* gameMode) {
    // Если кликнут первый прямоугольник (кнопка "2 PLAYERS")
    if (xpos >= 125 && xpos <= 525 && ypos >= 250 && ypos <= 350) {
        *gameMode = 1; // меняет *gameMode на 1
    }
    // Если кликнут второй прямоугольник (кнопка "VS AI")
    else if (xpos >= 125 && xpos <= 525 && ypos >= 400 && ypos <= 500) {
        *gameMode = 2; // меняет *gameMode на 2
    }
}

void drawGameOverMessage(const char* gameOverMessage, const char* winnerMessage) {
    glClear(GL_COLOR_BUFFER_BIT); // Очистка экрана

    // Темно-серый фон
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(640, 0);
    glVertex2f(640, 640);
    glVertex2f(0, 640);
    glEnd();

    // Рисуем три текста
    drawEasyText(230, 250, gameOverMessage, 3);
    drawEasyText(205, 300, winnerMessage, 4);
    drawEasyText(150, 500, "Press ESC or R to return to menu", 2);
}