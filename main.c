#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 700

struct pos {
    float y;
    float x;
};

bool want_to_bomb = false;

// Ta tablica to abominacja,
// będzie trzeba napisać funkcję "draw_map()" która zczytuje wartości z pliku "map1.txt"
// w folderze maps i wpisuje je do tej tablicy
char map_data[7][10] = {
        1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 1, 0, 1, 0, 0, 0, 0,
        0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};

GLuint mapDisplayList;

struct pos character, bomb_position;

void bomb_callback() {
    want_to_bomb = false;
    printf("no bomb bozo \n");
}

void place_bomb() {
    printf("bomb x = %f\n", bomb_position.y);
    printf("bomb y = %f\n", bomb_position.y);
    glColor3f(1.0f, 0.0f, 0.0f);
    glRectf(bomb_position.x * 100, bomb_position.y * 100, bomb_position.x * 100 + 100, bomb_position.y * 100 + 100);
    // glutTimerFunc wywołuje bomb_callback dużo razy co powoduje że nie jest możliwe postawienie bomby przez naśtepne 5 sekund.
    glutTimerFunc(5000, bomb_callback, 0);
}

bool hitbox_detection() {
    bool x_collision = false;
    bool y_collision = false;
    float x1, y1;
    int char_x, char_y;
    char_x = (int) truncf(character.x / 100);
    char_y = (int) truncf(character.y / 100);

    for (int i = char_y - 1; i <= char_y + 1; i++) {
        for (int j = char_x - 1; j <= char_x + 1; j++) {
            if (map_data[i][j] == 1) {
                x1 = (float) j * 100.0f;
                y1 = (float) i * 100.0f;
                if (character.x + 50.0f <= x1 || character.x - 50.0f >= x1 + 100) {
                    x_collision = false;
                } else {
                    x_collision = true;
                }
                if (character.y + 50.0f <= y1 || character.y - 50.0f >= y1 + 100) {
                    y_collision = false;
                } else {
                    y_collision = true;
                }

                if (x_collision && y_collision) {
                    return true;
                }
            }
        }
    }
    return false;
}

void special_key_movement(int key, int miceX, int miceY) {
    switch (key) {
        case GLUT_KEY_UP:
            character.y -= 10;
            if (hitbox_detection()) {
                character.y += 10;
            }
            glutPostRedisplay();
            break;
        case GLUT_KEY_DOWN:
            character.y += 10;
            if (hitbox_detection()) {
                character.y -= 10;
            }
            glutPostRedisplay();
            break;
        case GLUT_KEY_RIGHT:
            character.x += 10;
            if (hitbox_detection()) {
                character.x -= 10;
            }
            glutPostRedisplay();
            break;
        case GLUT_KEY_LEFT:
            character.x -= 10;
            if (hitbox_detection()) {
                character.x += 10;
            }
            glutPostRedisplay();
            break;
        case GLUT_KEY_F1:
            want_to_bomb = true;
            bomb_position.x = truncf(character.x / 100);
            bomb_position.y = truncf(character.y / 100);
            break;
        default:
            break;
    }
    printf("%.3f, %.3f\n", character.x, character.y);
}

void entity_wall(float x, float y) {
    glColor3f(0.0f, 0.0f, 1.0f);
    glRectf(x, y, x + 100, y + 100);
}

void draw_map() {
    float sumx = 0.0f, sumy = 0.0f;
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 10; j++) {
            if (map_data[i][j] == 1) {
                entity_wall(sumx, sumy);
            }
            sumx += 100.0f;
            printf("x = %f\n", sumx);
        }
        sumx = 0.0f;
        sumy += 100.0f;
        printf("y = %f\n", sumy);
    }
    sumy = 0.0f;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    if (want_to_bomb) {
        place_bomb();
    }
    glColor3f(1.0f, 0.0f, 1.0f);
    glRectf(character.x - 50, character.y - 50, character.x + 50, character.y + 50);
    glCallList(mapDisplayList);

    glFlush();
    glutPostRedisplay();
    glutSwapBuffers();
}

void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Wartosci do ustawienie, jak na razie nie ma roznicy co sie ustawi, ale moze to spowodowac bledy w przyszlosci.
    gluOrtho2D(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0);

    mapDisplayList = glGenLists(1);
    glNewList(mapDisplayList, GL_COMPILE);
    draw_map();
    glEndList();
}

int main(int argc, char **argv) {
    character.x = 150.0f;
    character.y = 150.0f;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Boberman");

    init();

    glutDisplayFunc(display);
    glutSpecialFunc(special_key_movement);

    glutMainLoop();
    return 0;
}