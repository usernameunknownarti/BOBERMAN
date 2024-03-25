#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glut.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 640

float y = 0, x = 0;

void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Wartosci do ustawienie, jak na razie nie ma roznicy co sie ustawi, ale moze to spowodowac bledy w przyszlosci.
    gluOrtho2D(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0);
}

void keyboard(int key, int miceX, int miceY) {
    switch (key) {
        case GLUT_KEY_UP:
            y -= 10;
            glutPostRedisplay();
            break;
        case GLUT_KEY_DOWN:
            y += 10;
            glutPostRedisplay();
            break;
        case GLUT_KEY_RIGHT:
            x += 10;
            glutPostRedisplay();
            break;
        case GLUT_KEY_LEFT:
            x -= 10;
            glutPostRedisplay();
            break;
        default:
            break;
    }
    printf("%03.3f, %03.3f\n", x, y);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0f, 0.0f, 1.0f);
    glRectf(x, y, x + 100, y + 100);
    glFlush();
    glutPostRedisplay();
    glutSwapBuffers();
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Boberman");

    glutDisplayFunc(display);

    init();

    glutSpecialFunc(keyboard);

    glutMainLoop();
    return 0;
}