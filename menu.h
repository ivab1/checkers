#ifndef MENU_H // ������ �������� ����������
#define MENU_H // ���������� ������ MENU_H
#include <GLFW/glfw3.h> // ���������� ���������� GLFW ��� ������ � ������ � ���������

void drawMenu(); // ������������ ����
void handleMenuClick(double xpos, double ypos, int* gameMode); // ������������ ������� ���� � ����
void drawEasyText(float x, float y, const char* text, float scale); // ������������ ����� �� ������ (��������� ���������� � � �, ������ ������ � ������� ��� ��������� ������� ������)
void drawGameOverMessage(const char* gameOverMessage, const char* winnerMessage); // ��������� ��������� � ���������� ����
void runBenchmarks();

#endif // ��������� �������� ����������
