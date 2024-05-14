#include <stdio.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define WINDOW_WIDTH 1100
#define WINDOW_HEIGHT 700

struct character_info {
    float y;
    float x;
    bool moving_up;
    bool moving_down;
    bool moving_left;
    bool moving_right;
    bool want_to_bomb;
} character1, character2;

struct queue_node {
    unsigned long long bomb_timer;
    float bomb_y;
    float bomb_x;
    int aftermath_y;
    int aftermath_x;
    struct queue_node *next;
};

struct queue_pointers {
    struct queue_node *head, *tail;
} queue = {NULL, NULL};

// Ta tablica to abominacja,
// będzie trzeba napisać funkcję która zczytuje wartości z pliku "map1.txt"
// w folderze maps i wpisuje je do tej tablicy
char map_data[7][11] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 2, 0, 2, 0, 2, 0, 1,
        1, 0, 1, 2, 1, 2, 1, 2, 1, 2, 1,
        1, 0, 2, 0, 2, 0, 2, 0, 2, 0, 1,
        1, 2, 1, 2, 1, 2, 1, 2, 1, 0, 1,
        1, 0, 2, 0, 2, 0, 2, 0, 0, 0, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

GLuint mapDisplayList;

void entity_square(float x, float y, float r, float g, float b) {
    glColor3f(r, g, b);
    glRectf(x, y, x + 100, y + 100);
}

bool enqueue(struct queue_pointers *queue_bomb, float bomb_x, float bomb_y, int aftermath_x, int aftermath_y,
             unsigned long long bomb_timer) {
    struct queue_node *new_node = (struct queue_node *) malloc(sizeof(struct queue_node));
    if (new_node != NULL) {
        new_node->bomb_x = bomb_x;
        new_node->bomb_y = bomb_y;
        new_node->aftermath_x = aftermath_x;
        new_node->aftermath_y = aftermath_y;
        new_node->bomb_timer = bomb_timer;
        new_node->next = NULL;
        if (queue_bomb->head == NULL) {
            queue_bomb->head = queue_bomb->tail = new_node;
        } else {
            queue_bomb->tail->next = new_node;
            queue_bomb->tail = new_node;
        }
        return true;
    }
    return false;
}

bool dequeue(struct queue_pointers *queue_bomb) {
    if (queue_bomb->head != NULL) {
        struct queue_node *tmp = queue_bomb->head->next;
        free(queue_bomb->head);
        queue_bomb->head = tmp;
        if (tmp == NULL) {
            queue_bomb->tail = NULL;
        }
        return true;
    }
    return false;
}

void explosion(struct queue_node *queue_bomb) {
    float j = 0;
    entity_square((queue_bomb->bomb_x) * 100, (queue_bomb->bomb_y) * 100, 1.0f, 0.0f, 0.0f);
    map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x] = 3;
    for (int i = 1; i < 2; i++) {
        j = j + 1;
        entity_square((queue_bomb->bomb_x + j) * 100, queue_bomb->bomb_y * 100, 1.0f, 0.0f, 0.0f);
        if (map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x + i] != 1) {
            map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x + i] = 3;
        }
        entity_square((queue_bomb->bomb_x - j) * 100, queue_bomb->bomb_y * 100, 1.0f, 0.0f, 0.0f);
        if (map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x - i] != 1) {
            map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x - i] = 3;
        }
        entity_square(queue_bomb->bomb_x * 100, (queue_bomb->bomb_y + j) * 100, 1.0f, 0.0f, 0.0f);
        if (map_data[queue_bomb->aftermath_y + i][queue_bomb->aftermath_x] != 1) {
            map_data[queue_bomb->aftermath_y + i][queue_bomb->aftermath_x] = 3;
        }
        entity_square(queue_bomb->bomb_x * 100, (queue_bomb->bomb_y - j) * 100, 1.0f, 0.0f, 0.0f);
        if (map_data[queue_bomb->aftermath_y - i][queue_bomb->aftermath_x] != 1) {
            map_data[queue_bomb->aftermath_y - i][queue_bomb->aftermath_x] = 3;
        }
    }
}

void bomb_cleanup(struct queue_node *queue_bomb) {
    map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x] = 0;
    for (int i = 1; i < 2; i++) {
        if (map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x + i] != 1) {
            map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x + i] = 0;
        }
        if (map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x - i] != 1) {
            map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x - i] = 0;
        }
        if (map_data[queue_bomb->aftermath_y + i][queue_bomb->aftermath_x] != 1) {
            map_data[queue_bomb->aftermath_y + i][queue_bomb->aftermath_x] = 0;
        }
        if (map_data[queue_bomb->aftermath_y - i][queue_bomb->aftermath_x] != 1) {
            map_data[queue_bomb->aftermath_y - i][queue_bomb->aftermath_x] = 0;
        }
    }
}

void death_detection(struct character_info *character) {
    int character_x = (int) truncf(character->x / 100);
    int character_y = (int) truncf(character->y / 100);
    if (map_data[character_y][character_x] == 3) {
        printf("You died.\n");
        //exit(0);
    }
}

bool hitbox_detection(struct character_info *character) {
    bool x_collision = false;
    bool y_collision = false;
    float x1, y1;
    int char_x = (int) truncf(character->x / 100);
    int char_y = (int) truncf(character->y / 100);

    for (int i = char_y - 1; i <= char_y + 1; i++) {
        for (int j = char_x - 1; j <= char_x + 1; j++) {
            if (map_data[i][j] == 1 || map_data[i][j] == 2 || map_data[i][j] == 5) {
                x1 = (float) j * 100.0f;
                y1 = (float) i * 100.0f;
                if (character->x + 50.0f <= x1 || character->x - 50.0f >= x1 + 100) {
                    x_collision = false;
                } else {
                    x_collision = true;
                }
                if (character->y + 50.0f <= y1 || character->y - 50.0f >= y1 + 100) {
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

void key_start_movement_character1(int key, int miceX, int miceY) {
    switch (key) {
        case GLUT_KEY_UP:
            character1.moving_up = true;
            printf("Going up\n");
            break;
        case GLUT_KEY_DOWN:
            character1.moving_down = true;
            printf("Going down\n");
            break;
        case GLUT_KEY_RIGHT:
            character1.moving_right = true;
            printf("Going right\n");
            break;
        case GLUT_KEY_LEFT:
            character1.moving_left = true;
            printf("Going left\n");
            break;
        default:
            break;
    }
}

void key_stop_movement_character1(int key, int miceX, int miceY) {
    switch (key) {
        case GLUT_KEY_UP:
            character1.moving_up = false;
            printf("Stopped going up\n");
            break;
        case GLUT_KEY_DOWN:
            character1.moving_down = false;
            printf("Stopped going down\n");
            break;
        case GLUT_KEY_RIGHT:
            character1.moving_right = false;
            printf("Stopped going right\n");
            break;
        case GLUT_KEY_LEFT:
            character1.moving_left = false;
            printf("Stopped going left\n");
            break;
        default:
            break;
    }
}

void key_start_movement_character2(unsigned char key, int miceX, int miceY) {
    switch (key) {
        case 'w':
            character2.moving_up = true;
            printf("Going up\n");
            break;
        case 's':
            character2.moving_down = true;
            printf("Going down\n");
            break;
        case 'd':
            character2.moving_right = true;
            printf("Going right\n");
            break;
        case 'a':
            character2.moving_left = true;
            printf("Going left\n");
            break;
        case 'z':
            enqueue(&queue, truncf(character2.x / 100), truncf(character2.y / 100), (int) truncf(character2.x / 100),
                    (int) truncf(character2.y / 100), time(NULL));
            break;
        case '0':
            enqueue(&queue, truncf(character1.x / 100), truncf(character1.y / 100), (int) truncf(character1.x / 100),
                    (int) truncf(character1.y / 100), time(NULL));
            break;
        default:
            break;
    }
}

void key_stop_movement_character2(unsigned char key, int miceX, int miceY) {
    switch (key) {
        case 'w':
            character2.moving_up = false;
            printf("Stopped going up\n");
            break;
        case 's':
            character2.moving_down = false;
            printf("Stopped going down\n");
            break;
        case 'd':
            character2.moving_right = false;
            printf("Stopped going right\n");
            break;
        case 'a':
            character2.moving_left = false;
            printf("Stopped going left\n");
            break;
        default:
            break;
    }
}

void update_movement(struct character_info *character) {
    if (character->moving_up == true) {
        character->y -= 5;
        if (hitbox_detection(character)) {
            character->y += 5;
        }
    }
    if (character->moving_down == true) {
        character->y += 5;
        if (hitbox_detection(character)) {
            character->y -= 5;
        }
    }
    if (character->moving_right == true) {
        character->x += 5;
        if (hitbox_detection(character)) {
            character->x -= 5;
        }
    }
    if (character->moving_left == true) {
        character->x -= 5;
        if (hitbox_detection(character)) {
            character->x += 5;
        }
    }
    glutPostRedisplay();
}

void draw_map() {
    float sumx = 0.0f, sumy = 0.0f;
    for (int i = 0; i < WINDOW_HEIGHT / 100; i++) {
        for (int j = 0; j < WINDOW_WIDTH / 100; j++) {
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

void bombing(struct queue_pointers *queue_bomb) {
    struct queue_node *current = queue_bomb->head;
    bool exploded = false;
    while (current != NULL) {
        if (current->bomb_timer + 1 >= time(NULL)) {
            entity_square(current->bomb_x * 100, current->bomb_y * 100, 0.5f, 0.5f, 0.5f);
        }
        if (current->bomb_timer + 2 >= time(NULL) && current->bomb_timer + 1 < time(NULL)) {
            explosion(current);
        }
        if (current->bomb_timer + 2 < time(NULL)) {
            bomb_cleanup(current);
            dequeue(queue_bomb);
        }
        current = current->next;
    }
}


void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    bombing(&queue);
    bombing(&queue);

    draw_crates();
    entity_square(character1.x - 50, character1.y - 50, 1.0f, 0.0f, 1.0f);
    entity_square(character2.x - 50, character2.y - 50, 1.0f, 0.0f, 1.0f);
    update_movement(&character1);
    update_movement(&character2);

    death_detection(&character1);
    death_detection(&character2);
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
    character1.x = 950.0f;
    character1.y = 550.0f;
    character2.x = 150.0f;
    character2.y = 150.0f;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Boberman");

    init();

    glutDisplayFunc(display);
    glutSpecialFunc(key_start_movement_character1);
    glutSpecialUpFunc(key_stop_movement_character1);

    glutKeyboardFunc(key_start_movement_character2);
    glutKeyboardUpFunc(key_stop_movement_character2);

    glutMainLoop();
    return 0;
}