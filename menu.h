#ifndef MENU_H // Начало условной компиляции
#define MENU_H // Определяет макрос MENU_H
#include <glfw3.h> // Подключает библиотеку GLFW для работы с окнами и событиями

void drawMenu(); // Отрисовываем меню
void handleMenuClick(double xpos, double ypos, int* gameMode); // Обрабатываем нажатия мыши в меню
void drawEasyText(float x, float y, const char* text, float scale); // Отрисовываем текст на экране (принимает координаты х и у, строку текста и масштаб для изменения размера текста)
void drawGameOverMessage(const char* gameOverMessage, const char* winnerMessage); // Отрисовка сообщения о завершении игры
void runBenchmarks();

#endif // Завершает условную компиляцию
