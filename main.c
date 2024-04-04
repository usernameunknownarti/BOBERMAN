#include <stdio.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 700

struct pos {
    float y;
    float x;
} character;

struct bomb_pos {
    int y;
    int x;
} bomb_position;

bool want_to_bomb = false;
bool explosion_allowed = false;

enum moving_direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NONE
} moving_direction = NONE;

// Ta tablica to abominacja,
// będzie trzeba napisać funkcję "draw_map()" która zczytuje wartości z pliku "map1.txt"
// w folderze maps i wpisuje je do tej tablicy
char map_data[7][10] = {
        1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 2, 1, 0, 1, 0, 0, 0, 0,
        0, 2, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 2, 0, 0, 0, 0, 0, 1, 0, 0,
        0, 2, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 2, 0, 0, 0, 0, 0, 0, 0, 1
};

GLuint mapDisplayList;
unsigned long long bomb_timer;
bool pressed_F1 = false;

void explosion() {
    // To cudo spawnuje bomby, nie wiem jak to zrobić lepiej ale działa, kiedyś się to poprawi :D
    glColor3f(1.0f, 0.0f, 0.0f);
    if (map_data[bomb_position.y - 1][bomb_position.x] != 1) {
        glRectf((float) bomb_position.x * 100, (float) (bomb_position.y - 1) * 100, (float) bomb_position.x * 100 + 100,
                (float) (bomb_position.y - 1) * 100 + 100);
        map_data[bomb_position.y - 1][bomb_position.x] = 3;
    }
    if (map_data[bomb_position.y + 1][bomb_position.x] != 1) {
        glRectf((float) bomb_position.x * 100, (float) (bomb_position.y + 1) * 100, (float) bomb_position.x * 100 + 100,
                (float) (bomb_position.y + 1) * 100 + 100);
        map_data[bomb_position.y + 1][bomb_position.x] = 3;
    }
    if (map_data[bomb_position.y][bomb_position.x] != 1) {
        glRectf((float) bomb_position.x * 100, (float) bomb_position.y * 100, (float) bomb_position.x * 100 + 100,
                (float) bomb_position.y * 100 + 100);
        map_data[bomb_position.y + 1][bomb_position.x] = 3;
    }
    if (map_data[bomb_position.y][bomb_position.x - 1] != 1) {
        glRectf((float) (bomb_position.x - 1) * 100, (float) bomb_position.y * 100,
                (float) (bomb_position.x - 1) * 100 + 100,
                (float) bomb_position.y * 100 + 100);
        map_data[bomb_position.y][bomb_position.x - 1] = 3;
    }
    if (map_data[bomb_position.y][bomb_position.x + 1] != 1) {
        glRectf((float) (bomb_position.x + 1) * 100, (float) bomb_position.y * 100,
                (float) (bomb_position.x + 1) * 100 + 100,
                (float) bomb_position.y * 100 + 100);
        map_data[bomb_position.y][bomb_position.x + 1] = 3;
    }
}

void death_detection() {
    int char_x = (int) truncf(character.x / 100);
    int char_y = (int) truncf(character.y / 100);
    if (map_data[char_x][char_y] == 3) {
        exit(0);
    }
}

void place_bomb() {
    glColor3f(0.5f, 0.5f, 0.5f);
    glRectf((float)bomb_position.x * 100, (float)bomb_position.y * 100, (float)bomb_position.x * 100 + 100,
            (float)bomb_position.y * 100 + 100);
    if (time(NULL) == bomb_timer) {
        want_to_bomb = false;
        explosion_allowed = true;
        printf("%llu", bomb_timer);
    }
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
            if (map_data[i][j] == 1 || map_data[i][j] == 2) {
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

void key_start_movement(int key, int miceX, int miceY) {
    switch (key) {
        case GLUT_KEY_UP:
            moving_direction = UP;
            printf("Going up\n");
            break;
        case GLUT_KEY_DOWN:
            moving_direction = DOWN;
            printf("Going down\n");
            break;
        case GLUT_KEY_RIGHT:
            moving_direction = RIGHT;
            printf("Going right\n");
            break;
        case GLUT_KEY_LEFT:
            moving_direction = LEFT;
            printf("Going left\n");
            break;
        case GLUT_KEY_F1:
            if (pressed_F1 == false) {
                bomb_position.x = (int) truncf(character.x / 100);
                bomb_position.y = (int) truncf(character.y / 100);
                want_to_bomb = true;
                bomb_timer = time(NULL) + 6;
            }
            pressed_F1 = true;
            break;
        default:
            break;
    }
}

void key_stop_movement(int key, int miceX, int miceY) {
    if (key != GLUT_KEY_F1) {
        moving_direction = NONE;
    }
    printf("you stopped moving\n");
}

void update_movement() {
    switch (moving_direction) {
        case UP:
            character.y -= 5;
            if (hitbox_detection()) {
                character.y += 5;
            }
            break;
        case DOWN:
            character.y += 5;
            if (hitbox_detection()) {
                character.y -= 5;
            }
            break;
        case LEFT:
            character.x -= 5;
            if (hitbox_detection()) {
                character.x += 5;
            }
            break;
        case RIGHT:
            character.x += 5;
            if (hitbox_detection()) {
                character.x -= 5;
            }
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

void entity_wall(float x, float y) {
    glColor3f(0.0f, 0.0f, 1.0f);
    glRectf(x, y, x + 100, y + 100);
}

void entity_crate(float x, float y) {
    glColor3f(1.0f, 1.0f, 0.0f);
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
        }
        sumx = 0.0f;
        sumy += 100.0f;
    }
}

void draw_crates() {
    float sumx = 0.0f, sumy = 0.0f;
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 10; j++) {
            if (map_data[i][j] == 2) {
                entity_crate(sumx, sumy);
            }
            sumx += 100.0f;
        }
        sumx = 0.0f;
        sumy += 100.0f;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    if (want_to_bomb) {
        place_bomb();
    } else if (explosion_allowed == true) {
        if (time(NULL) < bomb_timer + 1) {
            explosion();
        } else {
            pressed_F1 = false;
            bomb_timer = 0;
            explosion_allowed = false;
            map_data[bomb_position.y - 1][bomb_position.x] = 0;
            map_data[bomb_position.y + 1][bomb_position.x] = 0;
            map_data[bomb_position.y][bomb_position.x] = 0;
            map_data[bomb_position.y][bomb_position.x - 1] = 0;
            map_data[bomb_position.y][bomb_position.x + 1] = 0;
        }
    }
    death_detection();
    draw_crates();
    glColor3f(1.0f, 0.0f, 1.0f);
    glRectf(character.x - 50, character.y - 50, character.x + 50, character.y + 50);
    update_movement();

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
    glutSpecialFunc(key_start_movement);
    glutSpecialUpFunc(key_stop_movement);

    glutMainLoop();
    return 0;
}