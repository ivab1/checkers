#include "menu.h" // ���������� ������������ ���� ���� menu.h (� ������������ ������� ����)
#include "stb_easy_font.h" // ���������� ���������� stb_easy_font.h � ����������� ����� ��� ������� ��������� ������ � ����������� OpenGL
// ��� ��������� ����� ��� ����, ����� � ������ .c ����� ���� ������������� ���������� ������� �� stb_easy_font.h
#define STB_EASY_FONT_IMPLEMENTATION

void drawEasyText(float x, float y, const char* text, float scale) {
    static char buffer[99999]; // ������� ����� ��� ��������� ������ ������
    int quads = stb_easy_font_print(0, 0, (char*)text, NULL, buffer, sizeof(buffer)); // ������ ������ ��� ������, ������������ ����� ����������������� (quads) ��� ���������

    glPushMatrix(); // ��������� ������� �������
    glTranslatef(x, y, 0); // �������� �� �� (�, �) (z = 0)
    glScalef(scale, scale, 1); // ������������ �� ������� ������ � � � �� scale, � z �� 1

    glColor3f(1, 1, 1); // ����� �����
    glEnableClientState(GL_VERTEX_ARRAY); // �������� ���������� ����� ��� ������������� ������� ������
    glVertexPointer(2, GL_FLOAT, 16, buffer); // ���������� ��������� �� ������� (2 - ���������� (�, �), 16 - ��� ����� ��������� (16 ���� = 4 float * 4 �����)
    glDrawArrays(GL_QUADS, 0, quads * 4); // ������ ������ ������ (0 - ��������� ������ � �������, quads * 4 -  ����� ���-�� ������)
    glDisableClientState(GL_VERTEX_ARRAY); // ��������� ���������� �����

    glPopMatrix(); // ��������������� ���������� �������
}

void drawMenu() {
    glClear(GL_COLOR_BUFFER_BIT); // ������� �����

    // Background
    // ������ �������-������� �� ��� ������� (640x640), ���� � �����-�����
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(640, 0);
    glVertex2f(640, 640);
    glVertex2f(0, 640);
    glEnd();

    // Title
    // ������ �������� ���� �������� ������� � ��������� 5 � ����������� (200,120)
    drawEasyText(200, 120, "CHECKERS", 5);

    // Button 1 (2 ������)
    // ������ ������ (����� �������������) ��� "2 PLAYERS" � �������� �����������
    // ������� "2 PLAYERS" �������� � ��������� 3 � �������� ������
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(125, 250);
    glVertex2f(525, 250);
    glVertex2f(525, 350);
    glVertex2f(125, 350);
    glEnd();
    drawEasyText(255, 290, "2 PLAYERS", 3);

    // Button 2 (������ ��)
    // ������ ������ ������ (��������)
    // ������� "VS AI" � ��������� 3 ���� ���� ������ ������
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(125, 400);
    glVertex2f(525, 400);
    glVertex2f(525, 500);
    glVertex2f(125, 500);
    glEnd();
    drawEasyText(285, 440, "VS AI", 3);

    glfwSwapBuffers(glfwGetCurrentContext()); // ������ ������ ����, ����� ������������ ��������� �� ������
}

// ��������� ���������� ����� ����
void handleMenuClick(double xpos, double ypos, int* gameMode) {
    // ���� ������� ������ ������������� (������ "2 PLAYERS")
    if (xpos >= 125 && xpos <= 525 && ypos >= 250 && ypos <= 350) {
        *gameMode = 1; // ������ *gameMode �� 1
    }
    // ���� ������� ������ ������������� (������ "VS AI")
    else if (xpos >= 125 && xpos <= 525 && ypos >= 400 && ypos <= 500) {
        *gameMode = 2; // ������ *gameMode �� 2
    }
}

void drawGameOverMessage(const char* gameOverMessage, const char* winnerMessage) {
    glClear(GL_COLOR_BUFFER_BIT); // ������� ������

    // �����-����� ���
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(640, 0);
    glVertex2f(640, 640);
    glVertex2f(0, 640);
    glEnd();

    // ������ ��� ������
    drawEasyText(230, 250, gameOverMessage, 3);
    drawEasyText(205, 300, winnerMessage, 4);
    drawEasyText(150, 500, "Press ESC or R to return to menu", 2);
}