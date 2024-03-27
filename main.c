#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glut.h>

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 700

struct pos {
    float y;
    float x;
};

// Ta tablica to abominacja,
// będzie trzeba napisać funkcję "draw_map()" która zczytuje wartości z pliku "map1.txt"
// w folderze maps i wpisuje je do tej tablicy
char map_data[7][10] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 1, 1, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

GLuint mapDisplayList;

struct pos character;

void keyboard(int key, int miceX, int miceY) {
    switch (key) {
        case GLUT_KEY_UP:
            character.y -= 10;
            glutPostRedisplay();
            break;
        case GLUT_KEY_DOWN:
            character.y += 10;
            glutPostRedisplay();
            break;
        case GLUT_KEY_RIGHT:
            character.x += 10;
            glutPostRedisplay();
            break;
        case GLUT_KEY_LEFT:
            character.x -= 10;
            glutPostRedisplay();
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
    glColor3f(1.0f, 0.0f, 1.0f);
    glRectf(character.x + 100, character.y + 100, character.x + 200, character.y + 200);
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
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Boberman");

    init();

    glutDisplayFunc(display);
    glutSpecialFunc(keyboard);

    glutMainLoop();
    return 0;
}