#include <stdio.h>
#include <stdlib.h>
#include "menu.h" 

// Функция для тестирования drawMenu
void test_DrawMenu() {
    printf("Testing drawMenu...\n");
    // Предположим, drawMenu не возвращает значения, поэтому просто проверяем вызов
    drawMenu();
    printf("Passed drawMenu test.\n");
}

// Тест для проверки обработки клика мыши
void test_HandleMouseClick() {
    int gameMode = 0;
    printf("Testing handleMenuClick...\n");
    handleMenuClick(300, 300, &gameMode); // Координаты для "2 PLAYERS"
    if (gameMode == 1) {
        printf("Passed handleMenuClick test.\n");
    }
    else {
        printf("Failed handleMenuClick test. Expected 1, got %d\n", gameMode);
    }
}

// Тест для проверки обработки второго клика на ИИ
void test_HandleAIClick() {
    int gameMode = 0;
    printf("Testing handleMenuClick for AI...\n");
    handleMenuClick(300, 450, &gameMode); // Координаты для "VS AI"
    if (gameMode == 2) {
        printf("Passed handleMenuClick test for AI.\n");
    }
    else {
        printf("Failed handleMenuClick test for AI. Expected 2, got %d\n", gameMode);
    }
}

// Основная функция для запуска тестов
void runTests() {
    test_DrawMenu();
    test_HandleMouseClick();
    test_HandleAIClick();
}