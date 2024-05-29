#include <stdio.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION

#include "libs/stb_image.h"

#define WINDOW_WIDTH 1100
#define WINDOW_HEIGHT 700

struct character_info {
    float y;
    float x;
    int score;
    bool moving_up;
    bool moving_down;
    bool moving_left;
    bool moving_right;
    bool died;
    bool invincible;
} character1, character2;

struct queue_node {
    unsigned long long bomb_timer;
    float bomb_y;
    float bomb_x;
    int aftermath_y;
    int aftermath_x;
    struct queue_node *next;
};

enum Menus {
    GAME,
    OPTIONS,
    MAIN_MANU
} Menus = MAIN_MANU;

bool proceed = false;
bool paused = false;
unsigned long long score_timer;

struct queue_pointers {
    struct queue_node *head, *tail;
} queue = {NULL, NULL};

// Ta tablica to abominacja,
// będzie trzeba napisać funkcję która zczytuje wartości z pliku "map1.txt"
// w folderze maps i wpisuje je do tej tablicy
char map_data[7][11];
char original_map_data[7][11];

GLuint mapDisplayList;
GLuint texture[14];

void entity_square(float x, float y, GLuint texture_gluint) {
    glBindTexture(GL_TEXTURE_2D, texture_gluint);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x, y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x + 100, y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x + 100, y + 100);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x, y + 100);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
//    glColor3f(r, g, b);
//    glRectf(x, y, x + 100, y + 100);
}

void set_map(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("bozo");
        return;
    }
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 11; j++) {
            fscanf(fp, "%1s", &map_data[i][j]);
        }
    }
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 11; j++) {
            map_data[i][j] -= 48;
            original_map_data[i][j] = map_data[i][j];
        }
    }
    fclose(fp);
}

void entity_rectangle_alpha(float r, float g, float b, float alpha, float x, float y) {
    glColor4f(r, g, b, alpha);
    glRectf(x, 0, x + 300, 1100);
}

void entity_score(float x, float r, float g, float b, struct character_info *character) {
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
    glPushMatrix();

//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glEnable(GL_BLEND);
    entity_rectangle_alpha(r, g, b, 0.5f, x, 0);
    glColor3f(1.0f, 1.0f, 1.0f);
    glTranslatef(x + 70, 450.0f, 0.0f);
    glScalef(2.0f, -2.0f, 2.0f);
    glutStrokeCharacter(GLUT_STROKE_ROMAN, character->score + '0');
//    glDisable(GL_BLEND);

    glPopMatrix();
    glPopAttrib();
}

void win_check(struct character_info *character, char *color) {
    if (character->score == 5) {
        printf("%s wins!\n", color);
        character->score = 0;
        character->score = 0;
        Menus = MAIN_MANU;
    }
}

void restart_map_data() {
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 11; j++) {
            map_data[i][j] = original_map_data[i][j];
        }
    }
}

void restart_game() {
    character1.died = false;
    character2.died = false;
    if (proceed) {
        character1.x = 950.0f;
        character1.y = 550.0f;
        character2.x = 150.0f;
        character2.y = 150.0f;
        proceed = false;
    }
}

void set_score() {
    if (character1.died) {
        character2.score++;
        proceed = true;
    }
    if (character2.died) {
        character1.score++;
        proceed = true;
    }
    if (character2.died && character1.died) {
        character1.score--;
        character2.score--;
        proceed = true;
    }
}

void view_scores() {
    entity_score(200.0f, 1.0f, 0.0f, 0.0f, &character2);
    entity_score(600.0f, 0.0f, 0.0f, 1.0f, &character1);
}

void entity_button(float x, float y, GLuint texture_gluint) {
    glBindTexture(GL_TEXTURE_2D, texture_gluint);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x - 250, y - 50);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x + 250, y - 50);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x + 250, y + 50);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x - 250, y + 50);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void entity_pause(float x, float y, GLuint texture_gluint) {
    glBindTexture(GL_TEXTURE_2D, texture_gluint);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x, y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x + 550, y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x + 550, y + 275);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x, y + 275);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
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
    entity_square((queue_bomb->bomb_x) * 100, (queue_bomb->bomb_y) * 100, texture[11]);
    map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x] = 3;
    for (int i = 1; i < 2; i++) {
        j = j + 1;
        entity_square((queue_bomb->bomb_x + j) * 100, queue_bomb->bomb_y * 100, texture[11]);
        if (map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x + i] != 1) {
            map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x + i] = 3;
        }
        entity_square((queue_bomb->bomb_x - j) * 100, queue_bomb->bomb_y * 100, texture[11]);
        if (map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x - i] != 1) {
            map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x - i] = 3;
        }
        entity_square(queue_bomb->bomb_x * 100, (queue_bomb->bomb_y + j) * 100, texture[11]);
        if (map_data[queue_bomb->aftermath_y + i][queue_bomb->aftermath_x] != 1) {
            map_data[queue_bomb->aftermath_y + i][queue_bomb->aftermath_x] = 3;
        }
        entity_square(queue_bomb->bomb_x * 100, (queue_bomb->bomb_y - j) * 100, texture[11]);
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
        character->died = true;
        score_timer = time(NULL) + 3;
    }
}

bool player_hitbox_detection(struct character_info *characterr1, struct character_info *characterr2) {
    bool x_collision = false;
    bool y_collision = false;
    if (characterr1->x + 50.0f <= characterr2->x - 50.0f || characterr1->x - 50.0f >= characterr2->x + 50.0f) {
        x_collision = false;
    } else {
        x_collision = true;
    }
    if (characterr1->y + 50.0f <= characterr2->y - 50.0f || characterr1->y - 50.0f >= characterr2->y + 50.0f) {
        y_collision = false;
    } else {
        y_collision = true;
    }

    if (x_collision && y_collision) {
        return true;
    }
    return false;
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

GLuint loadTexture(const char *filename) {
    int width, height, channels;
    unsigned char *pixel_data = stbi_load(filename, &width, &height, &channels, 0);
    if (!pixel_data) {
        printf("nie ma tekstury\n");
        return 0;
    }

    GLuint texture_gluint;
    glGenTextures(1, &texture_gluint);
    glBindTexture(GL_TEXTURE_2D, texture_gluint);

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixel_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(pixel_data);
    return texture_gluint;
}

void key_start_movement_character1(int key, int miceX, int miceY) {
    if (!paused) {
        switch (key) {
            case GLUT_KEY_UP:
                character1.moving_up = true;
                texture[7] = loadTexture("graphics/block_beaver_blue_back.png");
                printf("Going up\n");
                break;
            case GLUT_KEY_DOWN:
                character1.moving_down = true;
                texture[7] = loadTexture("graphics/block_beaver_blue_front.png");
                printf("Going down\n");
                break;
            case GLUT_KEY_RIGHT:
                character1.moving_right = true;
                texture[7] = loadTexture("graphics/block_beaver_blue_right.png");
                printf("Going right\n");
                break;
            case GLUT_KEY_LEFT:
                character1.moving_left = true;
                texture[7] = loadTexture("graphics/block_beaver_blue_left.png");
                printf("Going left\n");
                break;
            default:
                break;
        }
    }
}

void key_stop_movement_character1(int key, int miceX, int miceY) {
    if (!paused) {
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
}

void key_start_movement_character2(unsigned char key, int miceX, int miceY) {
    if (!paused) {
        switch (key) {
            case 'w':
                character2.moving_up = true;
                texture[8] = loadTexture("graphics/block_beaver_red_back.png");
                printf("Going up\n");
                break;
            case 's':
                character2.moving_down = true;
                texture[8] = loadTexture("graphics/block_beaver_red_front.png");
                printf("Going down\n");
                break;
            case 'd':
                character2.moving_right = true;
                texture[8] = loadTexture("graphics/block_beaver_red_right.png");
                printf("Going right\n");
                break;
            case 'a':
                character2.moving_left = true;
                texture[8] = loadTexture("graphics/block_beaver_red_left.png");
                printf("Going left\n");
                break;
            case 'z':
                enqueue(&queue, truncf(character2.x / 100), truncf(character2.y / 100),
                        (int) truncf(character2.x / 100),
                        (int) truncf(character2.y / 100), time(NULL));
                break;
            case '0':
                enqueue(&queue, truncf(character1.x / 100), truncf(character1.y / 100),
                        (int) truncf(character1.x / 100),
                        (int) truncf(character1.y / 100), time(NULL));
                break;
            case 27:
                paused = true;
                break;
            default:
                break;
        }
    }
}

void key_stop_movement_character2(unsigned char key, int miceX, int miceY) {
    if (!paused) {
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
}

void draw_map() {
    float sumx = 0.0f, sumy = 0.0f;
    for (int i = 0; i < WINDOW_HEIGHT / 100; i++) {
        for (int j = 0; j < WINDOW_WIDTH / 100; j++) {
            if (map_data[i][j] == 1) {
                entity_square(sumx, sumy, texture[12]);
            }
            sumx += 100.0f;
        }
        sumx = 0.0f;
        sumy += 100.0f;
    }
}

void mouse(int button, int state, int miceX, int miceY) {
    if (button == GLUT_LEFT_BUTTON && Menus == MAIN_MANU && state == GLUT_DOWN) {
        if (miceX <= 800 && miceX >= 300 && miceY <= 400 && miceY >= 300) {
            Menus = GAME;
        }
        if (miceX <= 800 && miceX >= 300 && miceY <= 525 && miceY >= 425) {
            Menus = OPTIONS;
        }
        if (miceX <= 800 && miceX >= 300 && miceY <= 650 && miceY >= 550) {
            exit(0);
        }
    }
    if (button == GLUT_LEFT_BUTTON && Menus == OPTIONS && state == GLUT_DOWN) {
        if (miceX <= 800 && miceX >= 300 && miceY <= 200 && miceY >= 100) {
            printf("map1\n");
            set_map("maps/map1.txt");
            glNewList(mapDisplayList, GL_COMPILE);
            draw_map();
            glEndList();
        }
        if (miceX <= 800 && miceX >= 300 && miceY <= 325 && miceY >= 225) {
            printf("map2\n");
            set_map("maps/map2.txt");
            glNewList(mapDisplayList, GL_COMPILE);
            draw_map();
            glEndList();
        }
        if (miceX <= 800 && miceX >= 300 && miceY <= 450 && miceY >= 350) {
            printf("map3\n");
            set_map("maps/map3.txt");
            glNewList(mapDisplayList, GL_COMPILE);
            draw_map();
            glEndList();
        }
        if (miceX <= 800 && miceX >= 300 && miceY <= 600 && miceY >= 500) {
            Menus = MAIN_MANU;
        }
    }
    if (button == GLUT_LEFT_BUTTON && Menus == GAME && state == GLUT_DOWN) {
        if (paused && miceX <= 750 && miceX >= 350 && miceY <= 350 && miceY >= 275) {
            paused = false;
        }
        if (paused && miceX <= 750 && miceX >= 350 && miceY <= 500 && miceY >= 375) {
            proceed = true;
            restart_game();
            restart_map_data();
            paused = false;
            Menus = MAIN_MANU;
        }
    }
}

void update_movement(struct character_info *characterr1, struct character_info *characterr2) {
    if (characterr1->moving_up == true) {
        characterr1->y -= 4;
        if (hitbox_detection(characterr1) || player_hitbox_detection(characterr1, characterr2)) {
            characterr1->y += 4;
        }
    }
    if (characterr1->moving_down == true) {
        characterr1->y += 4;
        if (hitbox_detection(characterr1) || player_hitbox_detection(characterr1, characterr2)) {
            characterr1->y -= 4;
        }
    }
    if (characterr1->moving_right == true) {
        characterr1->x += 4;
        if (hitbox_detection(characterr1) || player_hitbox_detection(characterr1, characterr2)) {
            characterr1->x -= 4;
        }
    }
    if (characterr1->moving_left == true) {
        characterr1->x -= 4;
        if (hitbox_detection(characterr1) || player_hitbox_detection(characterr1, characterr2)) {
            characterr1->x += 4;
        }
    }
    glutPostRedisplay();
}

void draw_crates() {
    float sumx = 0.0f, sumy = 0.0f;
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 10; j++) {
            if (map_data[i][j] == 2) {
                entity_square(sumx, sumy, texture[10]);
            }
            sumx += 100.0f;
        }
        sumx = 0.0f;
        sumy += 100.0f;
    }
}

void bombing(struct queue_pointers *queue_bomb) {
    struct queue_node *current = queue_bomb->head;
    while (current != NULL) {
        if (current->bomb_timer + 1 >= time(NULL)) {
            entity_square(current->bomb_x * 100, current->bomb_y * 100, texture[9]);
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

void game() {
    bombing(&queue);
    bombing(&queue);
    player_hitbox_detection(&character1, &character2);
    draw_crates();

    entity_square(character1.x - 50, character1.y - 50, texture[7]);
    entity_square(character2.x - 50, character2.y - 50, texture[8]);
    update_movement(&character1, &character2);
    update_movement(&character2, &character1);

    death_detection(&character1);
    death_detection(&character2);
    glCallList(mapDisplayList);
    if (paused) {
        entity_pause(275, 225, texture[13]);
    }
    if (character1.died || character2.died) {
        if (!proceed) {
            set_score();
        }
        if (time(NULL) <= score_timer) {
            view_scores();
        } else {
            win_check(&character1, "Blue");
            win_check(&character2, "Red");
        }
        if (time(NULL) > score_timer) {
            restart_game();
            restart_map_data();
        }
    }
}

void main_menu() {
    entity_button(550, 350, texture[0]);
    entity_button(550, 475, texture[1]);
    entity_button(550, 600, texture[2]);
}

void options() {
    entity_button(550, 150, texture[3]);
    entity_button(550, 275, texture[4]);
    entity_button(550, 400, texture[5]);
    entity_button(550, 550, texture[6]);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    switch (Menus) {
        case GAME:
            game();
            break;
        case OPTIONS:
            options();
            break;
        case MAIN_MANU:
            main_menu();
            break;
        default:
            break;
    }
    glFlush();
    glutPostRedisplay();
    glutSwapBuffers();
}

void init() {
    glClearColor(0.294f, 0.388f, 0.165f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    // Wartosci do ustawienie, jak na razie nie ma roznicy co sie ustawi, ale moze to spowodowac bledy w przyszlosci.
    gluOrtho2D(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    set_map("maps/map1.txt");

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
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Boberman");

    texture[0] = loadTexture("graphics/button_start_game.png");
    texture[1] = loadTexture("graphics/button_options.png");
    texture[2] = loadTexture("graphics/button_exit.png");
    texture[3] = loadTexture("graphics/button_map1.png");
    texture[4] = loadTexture("graphics/button_map2.png");
    texture[5] = loadTexture("graphics/button_map3.png");
    texture[6] = loadTexture("graphics/button_back.png");


    texture[7] = loadTexture("graphics/block_beaver_blue_front.png");
    texture[8] = loadTexture("graphics/block_beaver_red_front.png");
    texture[9] = loadTexture("graphics/block_bomb.png");
    texture[10] = loadTexture("graphics/block_crate.png");
    texture[11] = loadTexture("graphics/block_fire.png");
    texture[12] = loadTexture("graphics/block_wall.png");

    texture[13] = loadTexture("graphics/ingame_pause.png");
    init();

    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutSpecialFunc(key_start_movement_character1);
    glutSpecialUpFunc(key_stop_movement_character1);

    glutKeyboardFunc(key_start_movement_character2);
    glutKeyboardUpFunc(key_stop_movement_character2);

    glutMainLoop();
    return 0;
}