#include <stdio.h>
#include <time.h>
#include "menu.h" 

// Бенчмарк для функции drawMenu
void benchmark_DrawMenu() {
    clock_t start = clock(); // Запоминаем время начала
    drawMenu(); // Вызов тестируемой функции
    clock_t end = clock(); // Запоминаем время окончания
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC; // Вычисляем прошедшее время
    printf("Benchmark for drawMenu: %f seconds\n", time_spent);
}

// Бенчмарк для обработки клика мыши
void benchmark_HandleMouseClick() {
    int gameMode = 0;
    clock_t start = clock(); // Запоминаем время начала
    handleMenuClick(300, 300, &gameMode); // Пример координат для кнопки "2 PLAYERS"
    clock_t end = clock(); // Запоминаем время окончания
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC; // Вычисляем прошедшее время
    printf("Benchmark for handleMenuClick: %f seconds\n", time_spent);
}

void runBenchmarks() {
    benchmark_DrawMenu(); // Запуск бенчмарка для функции drawMenu
    benchmark_HandleMouseClick(); // Запуск бенчмарка для функции обработки клика
    return 0; // Успешный выход
}