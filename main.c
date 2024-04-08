#include <stdio.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 700

struct float_pos {
    float y;
    float x;
} character, bomb_position;

struct int_pos {
    int y;
    int x;
} bomb_aftermath;

bool want_to_bomb = false;
bool explosion_allowed = false;
bool moving_up = false;
bool moving_down = false;
bool moving_left = false;
bool moving_right = false;

// Ta tablica to abominacja,
// będzie trzeba napisać funkcję która zczytuje wartości z pliku "map1.txt"
// w folderze maps i wpisuje je do tej tablicy
char map_data[7][10] = {
        1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        2, 2, 2, 2, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

GLuint mapDisplayList;
unsigned long long bomb_timer;
bool pressed_F1 = false;

void entity_square(float x, float y, float r, float g, float b) {
    glColor3f(r, g, b);
    glRectf(x, y, x + 100, y + 100);
}

void explosion() {
    float j = 0;
    entity_square((bomb_position.x) * 100, (bomb_position.y) * 100, 1.0f, 0.0f, 0.0f);
    map_data[bomb_aftermath.y][bomb_aftermath.x] = 3;
    for (int i = 1; i < 2; i++) {
        j = j + 1;
        entity_square((bomb_position.x + j) * 100, bomb_position.y * 100, 1.0f, 0.0f, 0.0f);
        if (map_data[bomb_aftermath.y][bomb_aftermath.x + i] != 1) {
            map_data[bomb_aftermath.y][bomb_aftermath.x + i] = 3;
        }
        entity_square((bomb_position.x - j) * 100, bomb_position.y * 100, 1.0f, 0.0f, 0.0f);
        if (map_data[bomb_aftermath.y][bomb_aftermath.x - i] != 1) {
            map_data[bomb_aftermath.y][bomb_aftermath.x - i] = 3;
        }
        entity_square(bomb_position.x * 100, (bomb_position.y + j) * 100, 1.0f, 0.0f, 0.0f);
        if (map_data[bomb_aftermath.y + i][bomb_aftermath.x] != 1) {
            map_data[bomb_aftermath.y + i][bomb_aftermath.x] = 3;
        }
        entity_square(bomb_position.x * 100, (bomb_position.y - j) * 100, 1.0f, 0.0f, 0.0f);
        if (map_data[bomb_aftermath.y - i][bomb_aftermath.x] != 1) {
            map_data[bomb_aftermath.y - i][bomb_aftermath.x] = 3;
        }
    }
}

void bomb_cleanup() {
    map_data[bomb_aftermath.y][bomb_aftermath.x] = 0;
    for (int i = 1; i < 2; i++) {
        if (map_data[bomb_aftermath.y][bomb_aftermath.x + i] != 1) {
            map_data[bomb_aftermath.y][bomb_aftermath.x + i] = 0;
        }
        if (map_data[bomb_aftermath.y][bomb_aftermath.x - i] != 1) {
            map_data[bomb_aftermath.y][bomb_aftermath.x - i] = 0;
        }
        if (map_data[bomb_aftermath.y + i][bomb_aftermath.x] != 1) {
            map_data[bomb_aftermath.y + i][bomb_aftermath.x] = 0;
        }
        if (map_data[bomb_aftermath.y - i][bomb_aftermath.x] != 1) {
            map_data[bomb_aftermath.y - i][bomb_aftermath.x] = 0;
        }
    }
}

void death_detection() {
    int character_x = (int) truncf(character.x / 100);
    int character_y = (int) truncf(character.y / 100);
    printf("%d, %d\n", character_x, character_y);
    if (map_data[character_y][character_x] == 3) {
        printf("You died.");
        exit(0);
    }
}

void place_bomb() {
    entity_square(bomb_position.x * 100, bomb_position.y * 100, 0.5f, 0.5f, 0.5f);
    if (time(NULL) == bomb_timer) {
        want_to_bomb = false;
        explosion_allowed = true;
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
            moving_up = true;
            printf("Going up\n");
            break;
        case GLUT_KEY_DOWN:
            moving_down = true;
            printf("Going down\n");
            break;
        case GLUT_KEY_RIGHT:
            moving_right = true;
            printf("Going right\n");
            break;
        case GLUT_KEY_LEFT:
            moving_left = true;
            printf("Going left\n");
            break;
        case GLUT_KEY_F1:
            if (pressed_F1 == false) {
                bomb_position.x = truncf(character.x / 100);
                bomb_position.y = truncf(character.y / 100);
                bomb_aftermath.x = (int) truncf(character.x / 100);
                bomb_aftermath.y = (int) truncf(character.y / 100);

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
    switch (key) {
        case GLUT_KEY_UP:
            moving_up = false;
            printf("Stopped going up\n");
            break;
        case GLUT_KEY_DOWN:
            moving_down = false;
            printf("Stopped going down\n");
            break;
        case GLUT_KEY_RIGHT:
            moving_right = false;
            printf("Stopped going right\n");
            break;
        case GLUT_KEY_LEFT:
            moving_left = false;
            printf("Stopped going left\n");
            break;
        default:
            break;
    }
}

void update_movement() {
    if (moving_up == true) {
        character.y -= 5;
        if (hitbox_detection()) {
            character.y += 5;
        }
    }
    if (moving_down == true) {
        character.y += 5;
        if (hitbox_detection()) {
            character.y -= 5;
        }
    }
    if (moving_right == true) {
        character.x += 5;
        if (hitbox_detection()) {
            character.x -= 5;
        }
    }
    if (moving_left == true) {
        character.x -= 5;
        if (hitbox_detection()) {
            character.x += 5;
        }
    }
    glutPostRedisplay();
}

void draw_map() {
    float sumx = 0.0f, sumy = 0.0f;
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 10; j++) {
            if (map_data[i][j] == 1) {
                entity_square(sumx, sumy, 0.0f, 0.0f, 1.0f);
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
                entity_square(sumx, sumy, 1.0f, 1.0f, 0.0f);
            }
            sumx += 100.0f;
        }
        sumx = 0.0f;
        sumy += 100.0f;
    }
}

void map_data_draw() {
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 10; j++) {
            printf("%d ",map_data[i][j]);
        }
        printf("\n");
    }
    printf("\n");
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
            bomb_cleanup();
        }
    }
    draw_crates();
    entity_square(character.x - 50, character.y - 50, 1.0f, 0.0f, 1.0f);
    update_movement();
    map_data_draw();

    death_detection();
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