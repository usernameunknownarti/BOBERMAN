#include <stdio.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION

#include "libs/stb_image.h"

#define MINIAUDIO_IMPLEMENTATION

#include "libs/miniaudio.h"

#define WINDOW_WIDTH 1100 // Szerokość okna gry.
#define WINDOW_HEIGHT 700 // Wysokość okna gry.

enum Menus { // Typ wyliczeniowy mówiący, w którym menu teraz jesteśmy.
    GAME, // Jesteśmy w grze.
    OPTIONS, // Jesteśmy w opcjach.
    MAIN_MANU // Jesteśmy w menu głównym.
} Menus = MAIN_MANU; // Definicja typu wyliczeniowego mówiąca o tym, w którym menu aktualnie jesteśmy.

enum BombStatus { // Typ wyliczeniowy mówiący, w jakim stanie bomby jest aktualnie bomba.
    BOMB, // Bomba została podłożona i nie wybuchła.
    EXPLODING, // Bomba jest w trakcie wybuchania.
    AFTERMATH // Bomba już wybuchła.
};

struct character_info { // Struktura przechowująca informacje o graczu.
    float y; // Pozycja na osi Y gracza (w przeciwieństwie do większosci elementów w tym programie, pozycja gracza jest liczona od środka nie od lewego górnego rogu).
    float x; // Pozycja na osi X gracza (w przeciwieństwie do większosci elementów w tym programie, pozycja gracza jest liczona od środka nie od lewego górnego rogu).
    int score; // Punkty gracza.
    bool moving_up; // Flaga, która mówi, czy gracz porusza się w górę.
    bool moving_down; // Flaga, która mówi, czy gracz porusza się w dół.
    bool moving_left; // Flaga, która mówi, czy gracz porusza się w lewo.
    bool moving_right; // Flaga, która mówi, czy gracz porusza się w prawo.
    bool died; // Flaga, która mówi, czy gracz nie żyje.
    bool ability_to_bomb; // Flaga, która mówi, czy gracz może położyć bombę
    bool can_play_death_sound_effect; // Flaga, która mówi, czy gracz może wydać dźwięk śmierci.
} character1, character2; // Dwie struktury globalne typu character_info, które przechowują informacje o pierwszym(niebieskim) i drugim(czerwonym) graczu.

struct queue_node { // Węzeł kolejki FIFO.
    unsigned long long bomb_timer; // Czas, który jest wykorzystywany do sprawdzania, kiedy wybucha bomba.
    float bomb_y; // Lokacja bomby na osi Y na macierzy map_data (typu float).
    float bomb_x; // Lokacja bomby na osi X na macierzy map_data (typu float).
    int aftermath_y; // Lokacja bomby na osi Y na macierzy map_data.
    int aftermath_x; // Lokacja bomby na osi X na macierzy map_data.
    enum BombStatus bomb_status; // Definicja typu wyliczeniowego mówiąca o tym, w którym stanie jest aktualnie bomba.
    bool can_play_explosion_sound_effect; // Flaga, która określa czy może zostać wydany dźwięk eksplozji.
    struct queue_node *next; // Wskaźnik na następny węzeł kolejki FIFO.
};

struct queue_pointers { // Węzeł wskazujący na początek i koniec kolejki FIFO.
    struct queue_node *head, *tail; // Wskaźniki wskazujące na początek i koniec kolejki.
} queue = {NULL, NULL}; // Kolejka FIFO używana przy tworzeniu bomb.

unsigned long long score_timer; // Zmienna, która przetrzymuje czas, o której zginął jeden z graczy, czas ten jest wykorzystywany podczas pokazywania się wyników oraz zwycięzcy.
unsigned long long paused_time; // Zmienna, która przetrzymuje czas, na jaki była zapauzowana gra.
bool proceed = false; // Flaga, która jest wykorzystywana podczas pokazywania wyników i restartowania gry.
bool paused = false; // Flaga, która mówi, czy gra jest zapauzowana.

char map_data[7][11]; // Macierz, na której odbywa się gra, przechowuje informacje o tym, jak wygląda aktualna mapa, gdzie są przeszkody, bomby, kule ognia.
// 0-Puste pole można przez nie przejść i nic się nie stanie.
// 1-Niezniszczalna ściana, której nie można zniszczyć i nie można przez nią przejść.
// 2-Zniszczalna przeszkoda, którą można wysadzić bombą, po wysadzeniu zamienia się w puste pole.
// 3-Kula ognia, powstała po postawieniu bomby, jest w stanie niszczyć zniszczalne ściany oraz wejście w nią przez gracza spowoduje śmierć.
// 4-Bomba, w tym miejscu znajduje się bomba, nie pozwala na postawienie w tym miejscu nowej bomby, służy do zaoszczędzenia pamięci w kolejce FIFO.
char original_map_data[7][11]; // Macierz, która przechowuje dane dotyczące mapy, gdzie znajdują się skrzynki, ściany i wolne miejsca, wykorzystywana
// do zrestartowania macierzy map_data po restarcie gry.

GLuint mapDisplayList; // Display List zawierająca ściany.
GLuint texture[17]; // Tablica zawierająca tekstury.
ma_sound sounds[5]; // Tablica zawierająca dźwięki.
ma_engine engine; // Silnik dźwiękowy.

// Funkcja loadTexture służy do wczytywania tekstury z pliku .png i zwraca takową teksturę, funkcja przyjmuje
// jako argument ścieżkę do podanej tekstury, tekstury są przechowywane w folderze graphics i są rozszerzenia .png,
// przez co podawany argument będzie miał postać "graphics/texture.png".
GLuint loadTexture(const char *filename) { // filename-ścieżka do tekstury.
    int width, height, channels; // Zmienne, które oznaczają szerokość, wysokość i liczby kanałów tekstury.
    unsigned char *pixel_data = stbi_load(filename, &width, &height, &channels, 0); // Wczytanie danych tekstury i zapisanie ich adresu do zmiennej pixel_data.
    if (!pixel_data) { // Sprawdzenie, czy dane tekstury są załadowane.
        return 0;
    }

    GLuint texture_gluint;
    glGenTextures(1, &texture_gluint); // Generacja identyfikatora tekstury i przypisanie go do texture_gluint;
    glBindTexture(GL_TEXTURE_2D, texture_gluint); // Ustawienie tekstury jako tekstury 2D.

    GLenum format;
    if (channels == 4) { // Sprawdzenie i przypisanie odpowiedniego formatu ddo tekstury.
        format = GL_RGBA; // Jeżeli są 4 kanały do zostanie ustalona wartość RGBA.
    } else {
        format = GL_RGB; // Jeżeli są 3 kanały do zostanie ustalona wartość RGB.
    }
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixel_data); // Przekazanie danych tekstury do GPU.

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Ustawienie jak będzie się zachowywać tekstura, jeżeli zostanie ustawiona na większy obiekt niż szerokość tekstury (tekstura będzie się powtarzać).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // Ustawienie jak będzie się zachowywać tekstura, jeżeli zostanie ustawiona na większy obiekt niż wysokość tekstury (tekstura będzie się powtarzać).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Ustawienie jak będzie się zachowywać tekstura, jeżeli zostanie zmniejszona (zachodzi interpolacja pomiędzy sąsiednimi pikselami).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Ustawienie jak będzie się zachowywać tekstura, jeżeli zostanie zwiększona (zachodzi interpolacja pomiędzy sąsiednimi pikselami).

    stbi_image_free(pixel_data); // Zwolnienie pamięci zajmowanej przez dane tekstury.
    return texture_gluint; // Zwrócenie tekstury.
}

// Funkcja entity_button, której zadaniem jest narysować prostokąt o rozmiarze 800 x 100 i nadać mu tekturę.
// Jako argument funkcja przyjmuje: koordynaty gdzie zostanie narysowany prostokąt oraz teksturę.
void entity_button(float x, float y, GLuint texture_gluint) { // float x - pozycja na osi X, float y - pozycja na osi Y, GLuint texture_gluint - tekstura.
    glBindTexture(GL_TEXTURE_2D, texture_gluint);  // Rozpoczęcie nakładania tekstury na objekt.
    glColor3f(1.0f, 1.0f, 1.0f); // Ustawienie koloru prostokąta na biały.
    glBegin(GL_QUADS); // Funkcja, która mówi, że tworzymy prostokąt.
    // Kod poniżej tworzy wierzchołki przy pomocy funkcji glVertex2f(), z tych wierzchołków tworzony jest kwadrat o rozmiarze 800 x 100, z czego
    // koordynaty tego prostokąta znajdują się w środku tego prostokąta. Funkcja glTexCoord2f() jest wykorzystana żeby nałożyć na każdy wierzchołek teksturę.
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x - 400, y - 50);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x + 400, y - 50);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x + 400, y + 50);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x - 400, y + 50);
    glEnd(); // Funkcja, która mówi, że prostokąt jest skończony.
    glBindTexture(GL_TEXTURE_2D, 0); // Zakończenie nakładania tekstury na prostokąt.
}

// Funkcja entity_background, której zadaniem jest narysować prostokąt o rozmiarze okna gry i nadać mu tekturę.
// Jako argument funkcja przyjmuje teksturę.
void entity_background(GLuint texture_gluint) { // GLuint texture_gluint - tekstura.
    glBindTexture(GL_TEXTURE_2D, texture_gluint); // Rozpoczęcie nakładania tekstury na prostokąt.
    glColor3f(1.0f, 1.0f, 1.0f); // Ustawienie koloru prostokąta na biały.
    glBegin(GL_QUADS); // Funkcja, która mówi, że tworzymy prostokąt.
    // Kod poniżej tworzy wierzchołki przy pomocy funkcji glVertex2f(), z tych wierzchołków tworzony jest prostokąt o rozmiarze WINDOW_WIDTH × WINDOW_HEIGHT, z czego
    // koordynaty tego prostokąta znajdują się w lewym górnym rogu tego prostokąta. Funkcja glTexCoord2f() jest wykorzystana żeby nałożyć na każdy wierzchołek teksturę.
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(WINDOW_WIDTH, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0, WINDOW_HEIGHT);
    glEnd(); // Funkcja, która mówi, że prostokąt jest skończony.
    glBindTexture(GL_TEXTURE_2D, 0); // Zakończenie nakładania tekstury na prostokąt.
}

// Funkcja entity_pause, której zadaniem jest narysować prostokąt o rozmiarze 550 na 275 i nadać mu tekturę.
// Jako argument funkcja przyjmuje koordynaty gdzie ma zostać menu narysowane oraz teksturę.
void entity_pause(float x, float y, GLuint texture_gluint) {  // x-pozycja na osi X, y pozycja na osi Y, GLuint texture_gluint - tekstura.
    glBindTexture(GL_TEXTURE_2D, texture_gluint);  // Rozpoczęcie nakładania tekstury na prostokąt.
    glColor3f(1.0f, 1.0f, 1.0f); // Ustawienie koloru prostokąta na biały.
    glBegin(GL_QUADS); // Funkcja, która mówi, że tworzymy prostokąt.
    // Kod poniżej tworzy wierzchołki przy pomocy funkcji glVertex2f(), z tych wierzchołków tworzony jest prostokąt o rozmiarze 550 × 275, z czego
    // koordynaty tego prostokąta znajdują się w lewym górnym rogu tego prostokąta. Funkcja glTexCoord2f() jest wykorzystana żeby nałożyć na każdy wierzchołek teksturę.
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x, y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x + 550, y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x + 550, y + 275);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x, y + 275);
    glEnd(); // Funkcja, która mówi, że prostokąt jest skończony.
    glBindTexture(GL_TEXTURE_2D, 0); // Zakończenie nakładania tekstury na prostokąt.
}


// Funkcja entity_square, której zadaniem jest narysować kwadrat i nadać mu tekturę.
// Jako argumenty przyjmuje pozycję gdzie ma zostać narysowany kwadrat
// oraz teksturę.
void entity_square(float x, float y, GLuint texture_gluint) { // float x - pozycja na osi X, float y - pozycja na osi Y, GLuint texture_gluint - tekstura.
    glBindTexture(GL_TEXTURE_2D, texture_gluint); // Rozpoczęcie nakładania tekstury na kwadrat.
    glColor3f(1.0f, 1.0f, 1.0f); // Ustawienie koloru kwadratu na biały.
    glBegin(GL_QUADS); // Funkcja, która mówi, że tworzymy prostokąt.
    // Kod poniżej tworzy wierzchołki przy pomocy funkcji glVertex2f(), z tych wierzchołków tworzony jest kwadrat o rozmiarze 100 x 100, z czego
    // koordynaty tego kwadratu znajdują się w lewym górnym rogu tego kwadratu. Funkcja glTexCoord2f() jest wykorzystana żeby nałożyć na każdy wierzchołek teksturę.
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x, y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x + 100, y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x + 100, y + 100);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x, y + 100);
    glEnd(); // Funkcja, która mówi, że kwadrat jest skończony.
    glBindTexture(GL_TEXTURE_2D, 0); // Zakończenie nakładania tekstury na kwadrat.
}

// Funkcja entity_rectangle_alpha, jej zadaniem jest narysowanie na ekranie podłużnego prostokąta o rozmiarach
// 300 x 700, jako parametry funkcja przyjmuję: kolor RGB, alfę, oraz pozycję na osi X i Y.
void entity_rectangle_alpha(float r, float g, float b, float alpha, float x, float y) {
    // float r-kolor czerowny, float g-kolor zielony, float b-kolor niebieski.
    // float alpha-kanał alfa.
    // float x-pozycja na osi X, float y-pozycja na osi X.
    glColor4f(r, g, b, alpha); // Ustawienie koloru kwadratu na kolor RGB z kanałem alfa.
    glRectf(x, y, x + 300, WINDOW_HEIGHT); // Wywołanie funkcji, która stworzy prostokąt od punktu x,y do punktu x+300, WINDOW_HEIGHT (prostokąt o rozmiarach 300 x 700).
}

// Funkcja entity_big_rectangle_alpha, jej zadaniem jest narysowanie na ekranie podłużnego prostokąta o rozmiarach
// 1100 x 300, jako parametry funkcja przyjmuję: kolor RGB oraz alfę.
void entity_big_rectangle_alpha(float r, float g, float b, float alpha) {
    // float r-kolor czerowny, float g-kolor zielony, float b-kolor niebieski.
    // float alpha-kanał alfa.
    // float x-pozycja na osi X, float y-pozycja na osi X.
    glColor4f(r, g, b, alpha); // Ustawienie koloru kwadratu na kolor RGB z kanałem alfa.
    glRectf(0, 150, WINDOW_WIDTH, 550); // Wywołanie funkcji, która stworzy prostokąt od punktu 0,150 do WINDOW_WIDTH,550 (prostokąt o rozmiarach 1100 x 300).
}

// Funkcja entity_score, której zadaniem jest narysować dwa podłużne pionowe przezroczyste prostokąty, oraz
// na nich napisać wynik obydwu graczy. Jako parametry ta funkcja przyjmuje: pozycję, kolor RGB, oraz informację
// o graczu.
void entity_score(float x, float r, float g, float b, struct character_info *character) {
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT); // Zapisanie aktualnych stanów atrybutów.
    glPushMatrix(); // Zapisanie aktualnej macierzy transformacji.
    entity_rectangle_alpha(r, g, b, 0.5f, x, 0); // Wywołanie funkcji, która narysuje podłużny kwadrat o danym kolorze, alfie i określonej pozycji.
    glColor3f(1.0f, 1.0f, 1.0f); // Ustawienie koloru czćionki na biały.
    glTranslatef(x + 70, 450.0f, 0.0f); // Przesunięcie układu współrzędnego do podanej pozycji, od tej pozycji będzie wpisywane znaki.
    glScalef(2.0f, -2.0f, 2.0f); // Zwiększenie rozmiaru czćionki.
    glutStrokeCharacter(GLUT_STROKE_ROMAN, character->score + '0'); // Wywołanie funkcji, która narysuje wyniki graczy na ekranie.
    glPopMatrix(); // Przywrócenie poprzedniej macierzy transformacji.
    glPopAttrib(); // Przywrócenie poprzednich stanów atrybutów.
}

// Funkcja entity_score, której zadaniem jest narysować podłużny pionowy przezroczysty prostokąt, oraz
// na nim narysować napis. Funkcja jako paremtry przyjmuje: kolor RGB, napis oraz offset.
void entity_winner(float r, float g, float b, const char *winner, float offset) {
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT); // Zapisanie aktualnych stanów atrybutów.
    glPushMatrix(); // Zapisanie aktualnej macierzy transformacji.
    entity_big_rectangle_alpha(r, g, b, 0.5f); // Wywołanie funkcji, która narysuje podłużny poziomy kwadrat o danym kolorze i aflie.
    glColor3f(1.0f, 1.0f, 1.0f); // Ustawienie koloru czćionki na biały.
    glTranslatef(offset, 390.0f, 0.0f); // Przesunięcie układu współrzędnego do podanej pozycji, od tej pozycji będzie wpisywane znaki.
    glScalef(0.90f, -0.90f, 0.90f); // Zmniejszenie rozmiaru czćionki.
    glutStrokeString(GLUT_STROKE_ROMAN, (const unsigned char *) winner); // Wywołanie funkcji, która narysuje wyśrodkowany napis na ekranie.
    glPopMatrix(); // Przywrócenie poprzedniej macierzy transformacji.
    glPopAttrib(); // Przywrócenie poprzednich stanów atrybutów.
}

// Funkcja set_map przyjmuję jako argument ścieżkę do pliku tekstowego, który zawiera kombinacje numerów (0, 1 i 2),
// pliki znajdują się w folderze maps i mają rozszerzenie .txt przez co podawany argument będzie miał postać "maps/map.txt".
// Zadaniem funkcji jest zczytanie wartości z pliku tekstowego i zapisania ich do tablicy map_data i original_map_data.
void set_map(char *filename) { // filename-ścieżka do mapy.
    FILE *fp = fopen(filename, "r"); // Otwarcie pliku, tylko do zczytywania.
    if (fp == NULL) { // Sprawdzenie, czy plik się nie otworzył.
        return; // Wyjście z funkcji.
    }
    for (int i = 0; i < 7; i++) { // Przejście przez macierz map_data.
        for (int j = 0; j < 11; j++) {
            fscanf(fp, "%1s", &map_data[i][j]); // Zczytanie i zapisanie pojedynczego numeru z pliku tekstowego do macierzy.
        }
    }
    for (int i = 0; i < 7; i++) { // Przejście przez macierz map_data.
        for (int j = 0; j < 11; j++) {
            map_data[i][j] -= 48; // Odjęcie od wartości macierzy map_data liczby 48, ponieważ zczytywane liczby to znaki ASCII więc '1' to nie 1 tylko 49.
            original_map_data[i][j] = map_data[i][j]; // Zczytanie i zapisanie pojedynczego numeru z pliku tekstowego do macierzy.
        }
    }
    fclose(fp); // Zamknięcie pliku tekstowego.
}

// Funkcja restart_map_data służy do zresetowania macierzy map_data, to tego
// jest wykorzystywana macierz original_map_data, która przechowuje nietknięte
// informacje dotyczące mapy.
void restart_map_data() {
    for (int i = 0; i < 7; i++) { // Przejście przez macierz map_data.
        for (int j = 0; j < 11; j++) {
            map_data[i][j] = original_map_data[i][j]; // Ustawienie wartości map_data[i][j] na wartość z macierzy original_map_data[i][j].
        }
    }
}

// Funkcja restart_game ma za zadanie zresetować flagi postaci oraz ich pozycje
// na takie, jakie były na początku gry.
void restart_game() {
    character1.died = false; // Zresetowanie flagi czy pierwsza postać zginęła.
    character2.died = false; // Zresetowanie flagi czy druga postać zginęła.
    character1.ability_to_bomb = true; // Zresetowanie flagi czy pierwsza postać może stawiać bomby.
    character2.ability_to_bomb = true; // Zresetowanie flagi czy druga postać może stawiać bomby.
    character1.can_play_death_sound_effect = true; // Zresetowanie flagi czy pierwsza postać może wydać dźwięk śmierci.
    character2.can_play_death_sound_effect = true; // Zresetowanie flagi czy druga postać może wydać dźwięk śmierci.
    if (proceed) { // Sprawdzenie, czy wyniki graczy zostały ustawione.
        character1.x = 950.0f; // Zresetowanie pozycji na oxi X pierwszego gracza.
        character1.y = 550.0f; // Zresetowanie pozycji na oxi Y pierwszego gracza.
        character2.x = 150.0f; // Zresetowanie pozycji na oxi X drugiego gracza.
        character2.y = 150.0f; // Zresetowanie pozycji na oxi Y drugiego gracza.
        proceed = false; // Ustawienie flagi proceed na false.
    }
}

// Funkcja win przyjmuje jako parametry: informacje o graczu, kolor RGB, ciąg znaków określający kto jest zwycięzcą oraz offset.
// Funkcja ma za zadanie, wyświetlić na ekranie podłużny poziomy pasek a na nim napis kto zwyciężył. Zwycięzcą jest ten, który
// jako pierwszy osiągnął 5 punktów. Offset jest wykorzystywany do wyśrodkowania napisu, funkcja ta też zresetujewyniki oraz
// powróci graczy do menu głównego.
void win(struct character_info *character, float r, float g, float b, const char *winner, float offset) {
    // struct character_info *character-informacje o graczu,
    // float r-kolor czerwony, float g-kolor zielony, float b-kolor niebieski,
    // const char *winner-ciąg znaków, jest to kolor, który zostanie wyświetlony podczas wyświetlenia napisu.
    // float offset-offset wykorzystywany do wyśrodkowania napisu.
    if (character->score == 5) { // Sprawdzenie, czy gracz ma 5 punktów.
        entity_winner(r, g, b, winner, offset); // Wywołanie funkcji, która narysuje podłużny prostokąt, i wypisze wyśrodkowany napis.
        if (time(NULL) >= score_timer + 10) { // Sprawdzenie, czy aktualny czas, jest większy niż score_timer + 10, czekanie ok. 7 sekund, nie 10 sekund, ponieważ wcześniej czekaliśmy już 3 sekundy podczas pokazywania wyników.
            character1.score = 0; // Ustawienie wyniku pierwszego gracza na 1.
            character2.score = 0; // Ustawienie wyniku pierwszego gracza na 2.
            Menus = MAIN_MANU; // Powrót do menu głównego.
        }
    }
}

// Funkcja set_score ma za zadanie ustawić punkty obydwu graczy, jeżeli gracz pierwszy zginie, to do punktów drugiego gracza jest przydzielany 1 punkt,
// tak samo działa to dla śmierci drugiego gracza. Jeżeli obydwu graczy zginie, to od ich punktów jest odejmowany 1 punkt (efektem tego jest remis i niezwiększanie
// się punktów). Funkcja ta ma się wykonywać tylko raz, więc korzysta z flagi proceed.
void set_score() {
    if (!proceed) { // Sprawdzenie, które ma na celu upewnić się, że wynik zostanie dodany tylko raz.
        if (character1.died) { // Sprawdzenie, czy pierwszy gracz nie żyje.
            character2.score++; // Inkrementacja wyniku drugiego gracza.
            proceed = true; // Ustawienie flagi procced na true (teraz funkcja wykona się tylko raz).
        }
        if (character2.died) { // Sprawdzenie, czy drugi gracz nie żyje.
            character1.score++; // Inkrementacja wyniku pierwszego gracza.
            proceed = true; // Ustawienie flagi procced na true (teraz funkcja wykona się tylko raz).
        }
        if (character2.died && character1.died) { // Sprawdzenie, czy obydwu graczy nie żyje.
            character1.score--; // Dekrementacja wyniku pierwszego gracza.
            character2.score--; // Dekrementacja wyniku drugiego gracza.
            proceed = true; // Ustawienie flagi procced na true (teraz funkcja wykona się tylko raz).
        }
    }
}

// Funkcja view_scores ma na celu wywołanie funkcji, które narysują wyniki.
void view_scores() {
    entity_score(200.0f, 1.0f, 0.0f, 0.0f, &character2); // Wywołanie funkcji, która narysuje wyniki dla drugiego gracza.
    entity_score(600.0f, 0.0f, 0.0f, 1.0f, &character1); // Wywołanie funkcji, która narysuje wyniki dla pierwszego gracza.
}

// Funkcja enqueue dodaje nowy element do kolejki FIFO. Funkcja ta jest kluczowa do poprawnego działania bomb,
// funkcja ta zaalokuje nową pamięć dla nowego węzła, po czym do tego węzła przydzieli odpowiednie dane które zostały
// podane to funkcji jako argumenty, argumenty te to: wskaźniki wskazujące na początek i koniec kolejki, koordynaty bomby w typie float,
// koordynaty bomby w typie int, oraz czas, który będzie wykorzystywany podczas wybuchów bomby.
void enqueue(struct queue_pointers *queue_bomb, float bomb_x, float bomb_y, int aftermath_x, int aftermath_y, unsigned long long bomb_timer) {
    // struct queue_pointers *queue_bomb - wskaźniki wskazujące na początek i koniec kolejki,
    // float bomb_x - lokacja bomby na osi X o typie float,
    // float bomb_y - lokacja bomby na osi Y o typie float,
    // int aftermath_x - lokacja bomby na osi X macierzy map_data,
    // int aftermath_y - lokacja bomby na osi Y macierzy map_data,
    // unsigned long long bomb_timer - czas, kiedy została postawiona bomba.
    struct queue_node *new_node = (struct queue_node *) malloc(sizeof(struct queue_node)); // Alokacja pamięci dla węzła.
    if (new_node != NULL) { // Sprawdzenie, czy alokacja pamięci się nie powiodła.
        new_node->bomb_x = bomb_x; // Przypisanie wartość gdzie znajduje się bomba na osi X. (typ float, ponieważ jest wykorzystywana do rysowania bomby na ekranie).
        new_node->bomb_y = bomb_y; // Przypisanie wartość gdzie znajduje się bomba na osi Y. (typ float, ponieważ jest wykorzystywana do rysowania bomby na ekranie).
        new_node->aftermath_x = aftermath_x; // Przypisanie wartość gdzie znajduje się bomba na osi X macierzy map_data. (typ int, ponieważ wartości te będą wykorzysytwane do modyfikowania macierzy map_data).
        new_node->aftermath_y = aftermath_y; // Przypisanie wartość gdzie znajduje się bomba na osi Y macierzy map_data. (typ int, ponieważ wartości te będą wykorzysytwane do modyfikowania macierzy map_data).
        map_data[new_node->aftermath_y][new_node->aftermath_x] = 4; // Ustawienie, że w tej lokacji na macierzy map_data znajduje się bomba.
        new_node->bomb_timer = bomb_timer; // Przypisanie, kiedy została stworzona bomba.
        new_node->next = NULL; // "Usunięcie" wiszącego wskaźnika.
        new_node->bomb_status = BOMB; // Ustawienie statusu postawionej bomby na nie wybuchniętą bombę.
        new_node->can_play_explosion_sound_effect = true; // Ustawienie flagi can_play_explosion_sound_effect na true.
        if (queue_bomb->head == NULL) { // Sprawdzenie, czy czoło kolejki jest puste (czy ten dodany element będzie pierwszy).
            queue_bomb->head = queue_bomb->tail = new_node; // Ustawienie wskaźników wskazujących na początek i koniec kolejki na początek węzła.
        } else {
            queue_bomb->tail->next = new_node; // Ustawienie wskaźników wskazujących koniec kolejki na nowy węzeł.
            queue_bomb->tail = new_node; // Ustawienie wskaźników wskazujących na początek na nowy węzeł.
        }
    }
}

bool dequeue(struct queue_pointers *queue_bomb) {
    if (queue_bomb->head != NULL) {
        struct queue_node *tmp = queue_bomb->head->next;
        map_data[queue_bomb->head->aftermath_y][queue_bomb->head->aftermath_x] = 0;
        free(queue_bomb->head);
        queue_bomb->head = tmp;
        if (tmp == NULL) {
            queue_bomb->tail = NULL;
        }
        return true;
    }
    return false;
}

// Funkcja play_explosion_sound_effect puszcza dźwięk eksplozji bomby, ale tylko raz.
// Funkcja przyjmuje jako argument aktualny węzeł kolejki.
void play_explosion_sound_effect(struct queue_node **queue_node) { // struct queue_node **queue_node - aktualny węzeł.
    if (queue_node != NULL && (*queue_node)->can_play_explosion_sound_effect) { // Sprawdzenie, czy istnieje aktualny węzeł oraz, czy można puścić dźwięk eksplozji.
        ma_sound_start(&sounds[1]); // Puszczenie dźwięku eksplozji.
        (*queue_node)->can_play_explosion_sound_effect = false; // Zmiana flagi can_play_explosion_sound_effect na false, co nie pozwoli puścić dźwięku drugi raz.
    }
}

// Funkcja explosion, która ma za zadanie narysować kule ognia eksplozji, oraz zmienić macierz map_data w taki sposób,
// żeby wejście w eksplozje powodowało śmierć.
void explosion(struct queue_node *queue_bomb) { // struct queue_node *queue_bomb - aktualny węzeł
    entity_square(queue_bomb->bomb_x * 100, queue_bomb->bomb_y * 100, texture[11]); // Wywołanie funkcji, która narysuje środek kuli ognia.
    map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x] = 3; // Ustawienie pola na macierzy map_data, na eksplozję.
    play_explosion_sound_effect(&queue_bomb); // Wywołanie funkcji, która puszcza dźwięk eksplozji, ale tylko raz.

    entity_square((queue_bomb->bomb_x + 1) * 100, queue_bomb->bomb_y * 100, texture[11]); // Wywołanie funkcji, która narysuje kulę ognia na prawo od środka i nada teksturę kuli ognia.
    if (map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x + 1] != 1) { // Sprawdzenie, czy pole, na którym znajduje się kula ognia w macierzy map_data, nie jest ścianą.
        map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x + 1] = 3; // Ustawienie pola w macierzy map_data, na którym znajduje się kula ognia na kulę ognia.
    }
    entity_square((queue_bomb->bomb_x - 1) * 100, queue_bomb->bomb_y * 100, texture[11]); // Wywołanie funkcji, która narysuje kulę ognia na lewo od środka i nada teksturę kuli ognia.
    if (map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x - 1] != 1) { // Sprawdzenie, czy pole, na którym znajduje się kula ognia w macierzy map_data, nie jest ścianą.
        map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x - 1] = 3; // Ustawienie pola w macierzy map_data, na którym znajduje się kula ognia na kulę ognia.
    }
    entity_square(queue_bomb->bomb_x * 100, (queue_bomb->bomb_y + 1) * 100, texture[11]); // Wywołanie funkcji, która narysuje kulę ognia w dół od środka i nada teksturę kuli ognia.
    if (map_data[queue_bomb->aftermath_y + 1][queue_bomb->aftermath_x] != 1) { // Sprawdzenie, czy pole, na którym znajduje się kula ognia w macierzy map_data, nie jest ścianą.
        map_data[queue_bomb->aftermath_y + 1][queue_bomb->aftermath_x] = 3; // Ustawienie pola w macierzy map_data, na którym znajduje się kula ognia na kulę ognia.
    }
    entity_square(queue_bomb->bomb_x * 100, (queue_bomb->bomb_y - 1) * 100, texture[11]); // Wywołanie funkcji, która narysuje kulę ognia w górę od środka i nada teksturę kuli ognia.
    if (map_data[queue_bomb->aftermath_y - 1][queue_bomb->aftermath_x] != 1) { // Sprawdzenie, czy pole, na którym znajduje się kula ognia w macierzy map_data, nie jest ścianą.
        map_data[queue_bomb->aftermath_y - 1][queue_bomb->aftermath_x] = 3; // Ustawienie pola w macierzy map_data, na którym znajduje się kula ognia na kulę ognia.
    }
}

// Funkcja bomb_cleanup ma za zadanie wyczyścić macierz map_data z ekslozji.
void bomb_cleanup(struct queue_node *queue_bomb) { // struct queue_node *queue_bomb - aktualny węzeł
    map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x] = 0; // Ustawienie środka eksplozji na puste pole.
    if (map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x + 1] != 1) { // Sprawdzenie, czy pole, które ma zostać wyczyszczone w macierzy map_data, nie jest ścianą.
        map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x + 1] = 0; // Ustawienie pola w dół od środka eksplozji na puste pole.
    }
    if (map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x - 1] != 1) { // Sprawdzenie, czy pole, które ma zostać wyczyszczone w macierzy map_data, nie jest ścianą.
        map_data[queue_bomb->aftermath_y][queue_bomb->aftermath_x - 1] = 0; // Ustawienie pola w górę od środka eksplozji na puste pole.
    }
    if (map_data[queue_bomb->aftermath_y + 1][queue_bomb->aftermath_x] != 1) { // Sprawdzenie, czy pole, które ma zostać wyczyszczone w macierzy map_data, nie jest ścianą.
        map_data[queue_bomb->aftermath_y + 1][queue_bomb->aftermath_x] = 0; // Ustawienie pola na prawo od środka eksplozji na puste pole..
    }
    if (map_data[queue_bomb->aftermath_y - 1][queue_bomb->aftermath_x] != 1) { // Sprawdzenie, czy pole, które ma zostać wyczyszczone w macierzy map_data, nie jest ścianą.
        map_data[queue_bomb->aftermath_y - 1][queue_bomb->aftermath_x] = 0; // Ustawienie pola na lewo od środka eksplozji na puste pole.
    }
}

// Funkcja play_death_sound_effect puszcza dźwięk śmierci gracza, ale tylko raz.
// Funkcja przyjmuje jako argument informacje o graczu.
void play_death_sound_effect(struct character_info **character) { // struct character_info **character - informacje o graczu.
    if ((*character)->can_play_death_sound_effect) { // Sprawdzenie, czy można puścić dźwięk śmierci.
        ma_sound_start(&sounds[3]); // Puszczenie dźwięku śmierci.
        (*character)->can_play_death_sound_effect = false; // Zmiana flagi can_play_death_sound_effect na false, co nie pozwoli puścić dźwięku drugi raz.
    }
}

// Funkcja death_detection ma za zadanie sprawdzić, czy gracz nie żyje.
// Funkcja ta przyjmuje jako parametr informacje o graczu.
void death_detection(struct character_info *character) { // struct character_info *character - informacje o graczu.
    int character_x = (int) truncf(character->x / 100); // Pozycja gracza na osi X na macierzy map_data.
    int character_y = (int) truncf(character->y / 100); // Pozycja gracza na osi Y na macierzy map_data.
    if (map_data[character_y][character_x] == 3 && !character->died) { // Sprawdzenie, czy pole, na którym się aktualnie znajduje gracz to kula ognia oraz czy postać jest martwa.
        play_death_sound_effect(&character); // Wywołanie funkcji, która puści dźwięk śmierci.
        character->died = true; // Gracz umiera.
        score_timer = time(NULL) + 3; // Ustawienie zmiennej score_timer na aktualny czas + 3, tablica wyników będzie widoczna przez ok. 3 sekundy.
    }
}

// Funkcja yeet_bober przyjmuje jako parametr informacje dotyczące gracza. Funkcja ta sprawdzi, czy
// gracz nie żyje i wyrzuci go poza mapę.
void yeet_bober(struct character_info *character) { // struct character_info *character - informacej o graczu.
    if (character->died) { // Sprawdzenie, czy gracz nie żyje.
        character->x = 9999.0f; // Ustawienie pozycji gracza na osi X na 9999.0f (Pozycja poza mapą).
        character->y = 9999.0f; // Ustawienie pozycji gracza na osi Y na 9999.0f (Pozycja poza mapą).
    }
}

// Funkcja player_hitbox_detection, jako argumenty przyjmuje informacje o pierwszym graczu i o drugim graczu.
// Funkcja ma za zadanie sprawdzać, czy gracze są w kolizji ze sobą i zwraca wartość true, jeżeli się kolidują
// lub false, jeżeli się nie kolidują.
bool player_hitbox_detection(struct character_info *characterr1, struct character_info *characterr2) {
    // struct character_info *characterr1 - informację o pierwszym graczu.
    // struct character_info *characterr2 - informację o drugim graczu.
    bool x_collision = false;
    bool y_collision = false;
    if (characterr1->x + 50.0f <= characterr2->x - 50.0f || characterr1->x - 50.0f >= characterr2->x + 50.0f) { // Sprawdzenie, czy jeden gracz nie koliduje z drugim na osi X.
        x_collision = false; // Nie ma kolizji graczy na osi X.
    } else {
        x_collision = true; // Nie ma kolizji graczy na osi X.
    }
    if (characterr1->y + 50.0f <= characterr2->y - 50.0f || characterr1->y - 50.0f >= characterr2->y + 50.0f) { // Sprawdzenie, czy jeden gracz nie koliduje z drugim na osi Y.
        y_collision = false; // Nie ma kolizji graczy na osi Y.
    } else {
        y_collision = true; // Nie ma kolizji graczy na osi Y.
    }

    if (x_collision && y_collision) { // Sprawdzenie, czy gracze kolidują się na osi X i Y.
        return true; // Gracze się kolidują.
    }
    return false; // Gracze się nie kolidują.
}

// Funkcja hitbox_detection ma za zadanie sprawdzić, czy gracz jest w kolizji ze ścianą.
// Funkcja ta będzie brać pod uwagę tylko ściany wokół gracza. Funkcja przyjmuje jako argument
// informacje o graczu i zwraca wartość true, jeżeli jest kolizja lub false, jeżeli takiej nie ma.
bool hitbox_detection(struct character_info *character) { // struct character_info *character - informacje o graczu
    bool x_collision = false; // Flaga czy jest kolizja z przeszkodą na osi X.
    bool y_collision = false; // Flaga czy jest kolizja z przeszkodą na osi Y.
    float x1, y1; // Zmienne będące koordynatami ścian.
    int char_x = (int) truncf(character->x / 100); // Pozycja na osi X gracza na macierzy map_data.
    int char_y = (int) truncf(character->y / 100); // Pozycja na osi Y gracza na macierzy map_data.

    for (int i = char_y - 1; i <= char_y + 1; i++) { // Przejścię przez macierz map_data wokół gracza.
        for (int j = char_x - 1; j <= char_x + 1; j++) {
            if (map_data[i][j] == 1 || map_data[i][j] == 2) { // Sprawdzenie, czy pole obok gracza na macierzy map_data to ściana lub skrzynka
                x1 = (float) j * 100.0f; // Ustawienie osi X ścianki na górną ściankę.
                y1 = (float) i * 100.0f; // Ustawienie osi Y ścianki na lewą ściankę.
                if (character->x + 50.0f <= x1 || character->x - 50.0f >= x1 + 100) { // Sprawdzenie, czy postać znajduje się pomiędzy lewą a prawą ścianką.
                    x_collision = false; // Nie ma kolizji z ośią X ścianki.
                } else {
                    x_collision = true; // Jest kolizja z ośią X ścianki
                }
                if (character->y + 50.0f <= y1 || character->y - 50.0f >= y1 + 100) { // Sprawdzenie, czy postać znajduje się pomiędzy górną a dolną ścianką.
                    y_collision = false; // Nie ma kolizji z ośią Y ścianki.
                } else {
                    y_collision = true; // Jest kolizja z ośią Y ścianki.
                }

                if (x_collision && y_collision) { // Sprawdzenie, czy nastąpiła kolizja z osią X przeszkody i osią Y przeszkody.
                    return true; // Jest kolizja z przeszkodą.
                }
            }
        }
    }
    return false; // Nie ma kolizji z przeszkodą.
}

// Funkcja draw_map ma za zadanie narysowanie drzew na ekranie zgodnie z macierzą map_data.
void draw_map() {
    for (int i = 0; i < 7; i++) { // Przejście przez macierz map_data.
        for (int j = 0; j < 11; j++) {
            if (map_data[i][j] == 1) { // Sprawdzenie, czy na tej pozycji w macierzy map_data ma się znajdować drzewo.
                entity_square((float) (j * 100), (float) (i * 100), texture[rand() % 2 + 12]); // Wywołanie funkcja, która narysuje drzewo w danych koordynatach, texture[rand() % 2 + 12] - wybranie które tekstura drzewa powinna zostać uźyta.
            }
        }
    }
}

// Funkcja draw_map ma za zadanie narysowanie skrzynek na ekranie zgodnie z macierzą map_data.
void draw_crates() {
    for (int i = 0; i < 7; i++) {  // Przejście przez macierz map_data.
        for (int j = 0; j < 11; j++) {
            if (map_data[i][j] == 2) {  // Sprawdzenie, czy na tej pozycji w macierzy map_data ma się znajdować skrzynka.
                entity_square((float) (j * 100), (float) (i * 100), texture[10]); // Wywołanie funkcji, która narysuje drzewo w danych koordynatach i o danej teksturze.
            }
        }
    }
}

// Funkcja update_movement ma za zadanie zaimplementować poruszanie się postaci oraz kolizje wraz ze ścianami i
// drugim graczem. Przyjmowane argumenty to informacje o pierwszym i drugim graczu, z czego informacje drugiego
// gracza nie są w żaden sposób modyfikowane i służą tylko do sprawdzania kolizji z drugim graczem.
void update_movement(struct character_info *characterr1, struct character_info *characterr2) {
    // struct character_info *characterr1 - informacje o pierwszym graczu.
    // struct character_info *characterr2 - informacje o drugim graczu.
    if (characterr1->moving_up == true && !paused) { // Sprawdzenie, czy gracz się porusza w górę i czy gra nie jest zapauzowana.
        characterr1->y -= 4; // Przesunięcie gracza w górę o 4.
        if (hitbox_detection(characterr1) || player_hitbox_detection(characterr1, characterr2)) { // Sprawdzenie, czy gracz jest w kolizji ze ścianą lub z drugim graczem.
            characterr1->y += 4; // Przesunięcie gracza w dół o 4.
        }
    } else {
        characterr1->moving_up = false; // Zatrzymanie postaci.
    }
    if (characterr1->moving_down == true && !paused) { // Sprawdzenie, czy gracz się porusza w dół i czy gra nie jest zapauzowana.
        characterr1->y += 4; // Przesunięcie gracza w dół o 4.
        if (hitbox_detection(characterr1) || player_hitbox_detection(characterr1, characterr2)) { // Sprawdzenie, czy gracz jest w kolizji ze ścianą lub z drugim graczem.
            characterr1->y -= 4; // Przesunięcie gracza w górę o 4.
        }
    } else {
        characterr1->moving_down = false; // Zatrzymanie postaci.
    }
    if (characterr1->moving_right == true && !paused) { // Sprawdzenie, czy gracz się porusza w prawo i czy gra nie jest zapauzowana.
        characterr1->x += 4; // Przesunięcie gracza w prawo o 4.
        if (hitbox_detection(characterr1) || player_hitbox_detection(characterr1, characterr2)) { // Sprawdzenie, czy gracz jest w kolizji ze ścianą lub z drugim graczem.
            characterr1->x -= 4; // Przesunięcie gracza w lewo o 4.
        }
    } else {
        characterr1->moving_right = false; // Zatrzymanie postaci.
    }
    if (characterr1->moving_left == true && !paused) { // Sprawdzenie, czy gracz się porusza w lewo i czy gra nie jest zapauzowana.
        characterr1->x -= 4; // Przesunięcie gracza w lewo o 4.
        if (hitbox_detection(characterr1) || player_hitbox_detection(characterr1, characterr2)) { // Sprawdzenie, czy gracz jest w kolizji ze ścianą lub z drugim graczem.
            characterr1->x += 4; // Przesunięcie gracza w prawo o 4.
        }
    } else {
        characterr1->moving_left = false; // Zatrzymanie postaci.
    }
}

// Funkcja bombing służy do zajmowania się bombami. Funkcja ta będzie przechodzić przez kolejkę bomb
// i zależnie od wartości tych bomb, będzie wykonywać odpowiednie akcje, którymi są: rysowanie bomb,
// jeżeli nie minęła ok. 1 sekunda; eksplodować, jeżeli już minęła ta 1 sekunda; wyczyścić macierz
// map_data z pozostałości po wybuchu bomby.
void bombing(struct queue_pointers *queue_bomb) { // struct queue_pointers *queue_bomb - wskaźniki na początek i koniec kolejki.
    struct queue_node *current = queue_bomb->head; // Ustawienie wskaźnika na aktualny węzeł na początek kolejki.
    struct queue_node *next = NULL;
    if (paused && queue.tail != NULL && current->bomb_status == BOMB) { // Sprawdzenie, czy gra została zapauzowana, czy ostatni element kolejki istnieje i czy bomba nie eksplodowała.
        paused_time = time(NULL) - queue.tail->bomb_timer; // Ustawienie zmiennej paused_time na ilość sekund, podczas której gra była zapauzowana.
    }
    if (paused && queue.tail != NULL && current->bomb_status == EXPLODING) { // Sprawdzenie, czy gra została zapauzowana, czy ostatni element kolejki istnieje i czy bomba eksplodowała.
        paused_time = time(NULL) - queue.tail->bomb_timer - 2; // Ustawienie zmiennej paused_time na ilość sekund, podczas której gra była zapauzowana.
    }
    while (current != NULL) { // Sprawdzenie, czy aktualny węzeł istnieje.
        next = current->next; // Ustawienie wskaźnika next na następny węzeł.
        if (current->bomb_timer + 1 + paused_time >= time(NULL)) { // Sprawdzenie, czy nie mineła 1 sekunda + czas, w którym gra była zapauzowana.
            current->bomb_status = BOMB; // Ustawienie statusu bomby aktualnego węzła na nieeksplodowaną bombę.
            entity_square(current->bomb_x * 100, current->bomb_y * 100, texture[9]); // Wywołanie funkcji, która narysuje nieeksplodowaną bombę.
        }
        if (current->bomb_timer + 2 + paused_time == time(NULL)) { // Sprawdzenie, czy jest 2 sekunda + czas, w którym gra była zapauzowana.
            current->bomb_status = EXPLODING; // Ustawienie statusu bomby aktualnego węzła na eksplozje.
            explosion(current); // Wywołanie funkcji, która wykona eksplozje.
        }
        if (current->bomb_timer + 3 + paused_time == time(NULL)) {  // Sprawdzenie, czy jest 3 sekunda + czas, w którym gra była zapauzowana.
            current->bomb_status = AFTERMATH; // Ustawienie statusu bomby aktualnego węzła na już eksplodowaną.
            bomb_cleanup(current); // Wywołanie funkcji, która wyczyści pozostałości po wybuchu bomby.
            paused_time = 0; // Wyzerowanie zmiennej paused_time.
        }
        current = next; // Ustawienie aktualnego węzła kolejki na następny.
    }
}

// Funkcja game jest wykorzystywana do stworzenia faktycznej gry. Funkcja ta odpowiada za ustawienie tła, logikę za bombami,
// kolizje graczy, narysowanie graczy, poruszanie się graczy, śmierci graczy, pauzowanie gry oraz radzenie sobie z
// akcjami, które mają nastąpic po śmierci gracza.
void game() {
    entity_background(texture[16]); // Wywołanie funkcji, która narysuje tło.
    bombing(&queue); // Wywolanie funkcji, która odpowiada za logikę za bombami.
    player_hitbox_detection(&character1, &character2); // Wwowłanie funkcji, która wykrywa czy jest kolizja jedno gracza z drugim.
    draw_crates(); // Wywołanie funkcji, która narysuje skrzynki.

    entity_square(character1.x - 50, character1.y - 50, texture[7]); // Wywołanie funkcji, która narysuje pierwszego gracza.
    entity_square(character2.x - 50, character2.y - 50, texture[8]); // Wywołanie funkcji, która narysuje drugiego gracza.
    update_movement(&character1, &character2); // Zaktualizowanie pozycji dla pierwszego gracza.
    update_movement(&character2, &character1); // Zaktualizowanie pozycji dla pierwszego gracza.
    death_detection(&character1); // Sprawdzenie, czy pierwszy gracz nie żyje.
    death_detection(&character2); // Sprawdzenie, czy pierwszy gracz nie żyje.
    glCallList(mapDisplayList); // Wywołanie DisplayList-a, narysowanie wszystkich drzew na mapie.
    if (paused) { // Sprawdzenie, czy gra jest zapauzowana.
        entity_pause(275, 225, texture[14]); // Wywołanie funkcji, która narysuje manu pauzy.
    }
    if (character1.died || character2.died) { // Sprawdzenie, czy pierwszy lub drugi gracz zgineli.
        character1.ability_to_bomb = false; // Wyłączenie możliwości kładzenia bomb przez pierwszego gracza.
        character2.ability_to_bomb = false; // Wyłączenie możliwości kładzenia bomb przez drugiego gracza.
        yeet_bober(&character1); // Wywołanie funkcji która "pozbędzie" się pierwszego gracza.
        yeet_bober(&character2); // Wywołanie funkcji która "pozbędzie" się pierwszego gracza.

        set_score(); // Wywołanie funkcji, która ustala punkty dla graczy.
        if (time(NULL) <= score_timer) { // Sprawdzenie, czy aktualny czas jest mniejszy od score_timer, czekanie ok. 3 sekund.
            view_scores(); // Wywołanie funkcji, która narysuje wyniki.
        } else { // Kod, który zostanie wykonany, jeżeli minie więcej niż te ok. 3 sekundy.
            win(&character1, 0.0f, 0.0f, 1.0f, "Niebieski wygrywa!", 11.8f); // Wywołanie funkcji dla zwycięzcy całej gry.
            win(&character2, 1.0f, 0.0f, 0.0f, "Czerwony wygrywa!", 11.5f); // Wywołanie funkcji dla zwycięzcy całej gry.
        }
        if (time(NULL) > score_timer && character1.score != 5 && character2.score != 5) { // Sprawdzenie czy aktualny czas jest większy od score_timer i czy punkty obydwu graczy nie są równe 5.
            restart_game(); // Wywołanie funkcji, która restartuje grę.
            restart_map_data(); // Wywołanie funkcji, która restartuje macierz map_data.
        }
    }
}

// Funkja main_menu ma za zadanie wywoływanie funkcji, które zostaną wykorzystane do narysowania menu głównego.
void main_menu() {
    entity_background(texture[15]); // Wywołanie funkcji, która narysuje tło ekranu i nałoży na nie teksturę.
    entity_button(550, 350, texture[0]); // Wywołanie funkcji, która narysuje przycisk PLAY ekranu i nałoży na niego odpowiednią teksturę.
    entity_button(550, 475, texture[1]); // Wywołanie funkcji, która narysuje przycisk OPTIONS ekranu i nałoży na niego odpowiednią teksturę.
    entity_button(550, 600, texture[2]); // Wywołanie funkcji, która narysuje przycisk EXIT ekranu i nałoży na niego odpowiednią teksturę.
}

// Funkja main_menu ma za zadanie wywoływanie funkcji, które zostaną wykorzystane do narysowania menu opcji.
void options() {
    entity_background(texture[15]); // Wywołanie funkcji, która narysuje tło ekranu i nałoży na nie teksturę.
    entity_button(550, 150, texture[3]); // Wywołanie funkcji, która narysuje przycisk MAPA1 ekranu i nałoży na niego odpowiednią teksturę.
    entity_button(550, 275, texture[4]); // Wywołanie funkcji, która narysuje przycisk MAPA2 ekranu i nałoży na niego odpowiednią teksturę.
    entity_button(550, 400, texture[5]); // Wywołanie funkcji, która narysuje przycisk MAPA3 ekranu i nałoży na niego odpowiednią teksturę.
    entity_button(550, 550, texture[6]); // Wywołanie funkcji, która narysuje przycisk BACK ekranu i nałoży na niego odpowiednią teksturę.
}

// Funkcja display jest to nasza funkcja znajdująca się w pętli głównej, jest odpowiedzialna za praktycznie wszystkie akcje związane z wyświetlaniem i faktyczną grą.
// Sama funkcja display ma za zadanie tylko wyczyścić ekran, przenieść nas do odpowiedniego menu i odświerzyć obraz.
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Funkcja, która usuwa dane znajdujące się w buforach koloru i głębokości.
    switch (Menus) { // Switch, który przyjmuje jako warunek menu.
        case GAME: // Kod dla menu gry.
            game(); // Wywołanie funkcji, która odpowiada za akcje w grze.
            break;
        case OPTIONS: // Kod dla menu opcji.
            options(); // Wywołanie funkcji, która odpowiada za akcje w menu opcji.
            break;
        case MAIN_MANU: // Kod dla menu głównego.
            main_menu(); // Wywołanie funkcji, która odpowiada za akcje w menu głównym.
            break;
        default:
            break;
    }
    glFlush(); // Wywołanie funkcji, która wymusza wykonanie wszystkich poprzednich operacji graficznych.
    glutPostRedisplay(); // Wywołanie funkcji, która odświeża okno gry.
    glutSwapBuffers(); // Wywołanie funkcji, która podmienia bufory.
}

// Funkcja key_start_movement_character1 jest to funkcja zwrotna, która ma za zadanie zaimplementować poruszanie się postaci
// przy pomocy klawiatury dla pierwszego gracza. Przyjmuje ona: klawisz, który został naciśnięty oraz pozycje kursora.
// Funkcja ta jako klawisze, które zostały naciśnięte przyjmuje tylko klawisze specialne jak np. INSERT.
// Pozycja kursora nigdzie nie jest wykorzystywana, ale jest wygamagana do poprawnego działania funkcji.
void key_start_movement_character1(int key, int miceX, int miceY) { // int key-klawisz, który został naciśnięty, int miceX-pozycja kursora na osi X, miceY-pozycja kursora na osi Y.
    if (Menus == GAME && !paused) { // Sprawdzenie, czy gra się rozgrywa oraz, czy gra nie jest zapauzowana.
        switch (key) { // Switch, który przyjmuje jako warunek klawisz.
            case GLUT_KEY_UP: // Kod dla strzałki w górę.
                character1.moving_up = true; // Zmiana flagi o poruszaniu się w górę dla pierwszego gracza na true.
                texture[7] = loadTexture("graphics/block_beaver_blue_back.png"); // Ustawienie grafiki pierwszego gracza na teksturę tyłu gracza.
                break;
            case GLUT_KEY_DOWN: // Kod dla strzałki w dół.
                character1.moving_down = true; // Zmiana flagi o poruszaniu się w dół dla pierwszego gracza na true.
                texture[7] = loadTexture("graphics/block_beaver_blue_front.png"); // Ustawienie grafiki pierwszego gracza na teksturę przodu gracza.
                break;
            case GLUT_KEY_RIGHT: // Kod dla strzałki w prawo.
                character1.moving_right = true; // Zmiana flagi o poruszaniu się w prawo dla pierwszego gracza na true.
                texture[7] = loadTexture("graphics/block_beaver_blue_right.png"); // Ustawienie grafiki pierwszego gracza na teksturę prawej strony gracza.
                break;
            case GLUT_KEY_LEFT: // Kod dla strzałki w lewo.
                character1.moving_left = true; // Zmiana flagi o poruszaniu się w lewo dla pierwszego gracza na true.
                texture[7] = loadTexture("graphics/block_beaver_blue_left.png"); // Ustawienie grafiki pierwszego gracza na teksturę lewej strony gracza.
                break;
            default: // Kod dla innych klawiszy specjalnych.
                break;
        }
    }
}

// Funkcja key_stop_movement_character1 jest to funkcja zwrotna, która ma za zadanie zaimplementować zatrzymywanie się postaci
// przy pomocy klawiatury dla pierwszego gracza. Przyjmuje ona: klawisz, który został naciśnięty oraz pozycje kursora.
// Funkcja ta jako klawisze, które zostały naciśnięte przyjmuje tylko klawisze specialne jak np. INSERT.
// Pozycja kursora nigdzie nie jest wykorzystywana, ale jest wygamagana do poprawnego działania funkcji.
void key_stop_movement_character1(int key, int miceX, int miceY) {
    if (!paused) { // Sprawdzenie, czy gra nie jest zapauzowana.
        switch (key) { // Switch, który przyjmuje jako warunek klawisz.
            case GLUT_KEY_UP: // Kod dla strzałki w górę.
                character1.moving_up = false; // Zmiana flagi o poruszaniu się w górę dla pierwszego gracza na false.
                break;
            case GLUT_KEY_DOWN:  // Kod dla strzałki w doł.
                character1.moving_down = false; // Zmiana flagi o poruszaniu się w dół dla pierwszego gracza na false.
                break;
            case GLUT_KEY_RIGHT:  // Kod dla strzałki w prawo.
                character1.moving_right = false; // Zmiana flagi o poruszaniu się w prawo dla pierwszego gracza na false.
                break;
            case GLUT_KEY_LEFT: // Kod dla strzałki w lewo.
                character1.moving_left = false; // Zmiana flagi o poruszaniu się w lewo dla pierwszego gracza na false.
                break;
            default: // Kod dla innych klawiszy specjalnych.
                break;
        }
    }
}

// Funkcja key_start_movement_character2 jest to funkcja zwrotna, która ma za zadanie zaimplementować poruszanie się postaci
// przy pomocy klawiatury dla drugiego gracza oraz stawianie bomb dla obydwu graczy, ponieważ dla pierwszego gracza nie ma takiego
// klawisza specjalnego, które byłyby wygodny dla gracza poruszającego się na strzałkach, oraz pauzowania gry, ponieważ ESC nie jest.
// znakiem specjalnym. Przyjmuje ona: klawisz, który został naciśnięty oraz pozycje kursora.
// Funkcja ta jako klawisze, które zostały naciśnięte przyjmuje tylko klawisze niespecjalne jak np. A.
// Pozycja kursora nigdzie nie jest wykorzystywana, ale jest wymagana do poprawnego działania funkcji.
void key_start_movement_character2(unsigned char key, int miceX, int miceY) {
    if (Menus == GAME && !paused) { // Sprawdzenie, czy gra się rozgrywa oraz czy gra nie jest zapauzowana.
        switch (key) { // Switch, który przyjmuje jako warunek klawisz.
            case 'w': // Kod dla 'w'.
                character2.moving_up = true; // Zmiana flagi o poruszaniu się w górę dla drugiego gracza na true.
                texture[8] = loadTexture("graphics/block_beaver_red_back.png"); // Ustawienie grafiki drugiego gracza na teksturę przodu gracza.
                break;
            case 's': // Kod dla 's'.
                character2.moving_down = true; // Zmiana flagi o poruszaniu się w dół dla drugiego gracza na true.
                texture[8] = loadTexture("graphics/block_beaver_red_front.png"); // Ustawienie grafiki drugiego gracza na teksturę tyłu gracza.
                break;
            case 'd': // Kod dla 'd'.
                character2.moving_right = true; // Zmiana flagi o poruszaniu się w prawo dla drugiego gracza na true.
                texture[8] = loadTexture("graphics/block_beaver_red_right.png"); // Ustawienie grafiki drugiego gracza na teksturę prawej strony gracza.
                break;
            case 'a': // Kod dla 'a'.
                character2.moving_left = true; // Zmiana flagi o poruszaniu się w lewo dla drugiego gracza na true.
                texture[8] = loadTexture("graphics/block_beaver_red_left.png"); // Ustawienie grafiki drugiego gracza na teksturę lewej strony gracza.
                break;
            case 'z': // Kod dla 'z'.
                if (map_data[(int) truncf(character2.y / 100)][(int) truncf(character2.x / 100)] == 0 && !character2.died) { // Sprawdzenie, czy miejsce, na którym znajduje się gracz drugi jest wolne, oraz z gracz drugi żyje
                    if (character1.ability_to_bomb || character2.ability_to_bomb) { // Sprawdzenie, czy chociaż jeden z graczy może stawiać bomby.
                        enqueue( // Funkcja dodająca bombę do kolejki.
                                &queue, // Kolejka
                                truncf(character2.x / 100), // Pozycja gracza na osi X.
                                truncf(character2.y / 100), // Pozycja gracza na osi Y.
                                (int) truncf(character2.x / 100), // Pozycja gracza na osi X, ale zmieniona na integer.
                                (int) truncf(character2.y / 100), // Pozycja gracza na osi Y, ale zmieniona na integer.
                                time(NULL)); // Aktualny czas.
                        ma_sound_start(&sounds[0]); // Puszczenie dźwięku podłożenia bomby.
                    }
                }
                break;
            case '0': // Kod dla '0'.
                if (map_data[(int) truncf(character1.y / 100)][(int) truncf(character1.x / 100)] == 0 && !character1.died) { // Sprawdzenie, czy miejsce, na którym znajduje się gracz pierwszy jest wolne, oraz że gracz pierwszy żyje
                    if (character1.ability_to_bomb || character2.ability_to_bomb) { // Sprawdzenie, czy chociaż jeden z graczy może stawiać bomby.
                        enqueue(// Funkcja dodająca bombę do kolejki
                                &queue, // Kolejka
                                truncf(character1.x / 100), // Pozycja gracza na osi X.
                                truncf(character1.y / 100), // Pozycja gracza na osi Y.
                                (int) truncf(character1.x / 100), // Pozycja gracza na osi X ale zmieniona na integer.
                                (int) truncf(character1.y / 100), // Pozycja gracza na osi Y ale zmieniona na integer.
                                time(NULL)); // Aktualny czas.
                        ma_sound_start(&sounds[0]); // Puszczenie dźwięku podłożenia bomby.
                    }
                }
                break;
            case 27: // Kod dla ESC.
                if (!paused) { // Sprawdzenie, czy gra jest zapauzowana.
                    paused = true; // Zmienienie flagi paused na true.
                    ma_sound_start(&sounds[4]); // Puszczenie// dźwięku pauzy.
                }
                break;
            default: // Kod dla innych klawiszy niespecjalnych.
                break;
        }
    }
}

// Funkcja key_stop_movement_character2 jest to funkcja zwrotna, która ma za zadanie zaimplementować zatrzymywanie się postaci
// przy pomocy klawiatury dla drugiego gracza. Przyjmuje ona: klawisz, który został naciśnięty oraz pozycje kursora.
// Funkcja ta jako klawisze które zostały naciśnięte przyjmuje tylko klawisze niespecialne jak np. A.
// Pozycja kursora nigdzie nie jest wykorzystywana, ale jest wygamagana do poprawnego działania funkcji.
void key_stop_movement_character2(unsigned char key, int miceX, int miceY) {
    if (!paused) { // Sprawdzenie, czy gra nie jest zapauzowana.
        switch (key) { // Switch, który przyjmuje jako warunek klawisz.
            case 'w': // Kod dla 'w'.
                character2.moving_up = false; // Zmiana flagi o poruszaniu się w górę dla drugiego gracza na false.
                break;
            case 's': // Kod dla 's'.
                character2.moving_down = false; // Zmiana flagi o poruszaniu się w dół dla drugiego gracza na false.
                break;
            case 'd': // Kod dla 'd'.
                character2.moving_right = false; // Zmiana flagi o poruszaniu się w prawo dla drugiego gracza na false.
                break;
            case 'a': // Kod dla 'a'.
                character2.moving_left = false; // Zmiana flagi o poruszaniu się w lewo dla drugiego gracza na false.
                break;
            default:
                break;
        }
    }
}

// Funkcja mouse jest to funkcja zwrotna, która ma za zadanie zaimplementować operacje na myszy.
// Parametrami funkcji jest przycisk, który został naciśnięty, stan przycisku, oraz lokacja kursora,
// dzięki nimi można zaimplementować przyciski, które są wykorzystywane w funkcji do poruszania się
// pomiędzy menu lub wykonywania różnych akcji po naciśnięciu przycisku.
void mouse(int button, int state, int miceX, int miceY) { // button-przycisk który został naciśnięty, state-stan przycisku (naciśnięty lub puszczony), miceX - lokacja kursora na osi X, miceY - lokacja kursora na osi Y.
    if (button == GLUT_LEFT_BUTTON && Menus == MAIN_MANU && state == GLUT_DOWN) { // Sprawdzenie, czy został naciśnięty LPM, czy znajdujesz się w menu głównym oraz i czy przycisk został naciśnięty (bez stanu kliknięcie by było uznawane za dwa kliknięcia).
        if (miceX <= 950 && miceX >= 150 && miceY <= 400 && miceY >= 300) { // Sprawdzenie, czy kursor znajduje się na przycisku PLAY.
            Menus = GAME; // Zamiana menu na rozpoczęcie gry.
            ma_sound_start(&sounds[2]); // Puszczenie dźwięku kliknięcia, w menu.
        }
        if (miceX <= 950 && miceX >= 150 && miceY <= 525 && miceY >= 425) { // Sprawdzenie, czy kursor znajduje się na przycisku OPTIONS.
            Menus = OPTIONS; // Zamiana menu na opcje.
            ma_sound_start(&sounds[2]); // Puszczenie dźwięku kliknięcia, w menu.
        }
        if (miceX <= 950 && miceX >= 150 && miceY <= 650 && miceY >= 550) { // Sprawdzenie, czy kursor znajduje się na przycisku EXIT.
            exit(0); // Zamknięcie programu. Wyjście z gry.
        }
    }
    if (button == GLUT_LEFT_BUTTON && Menus == OPTIONS && state == GLUT_DOWN) { // Sprawdzenie, czy został naciśnięty LPM, czy znajdujesz się w opcjach oraz czy przycisk został naciśnięty.
        if (miceX <= 950 && miceX >= 150 && miceY <= 200 && miceY >= 100) { // Sprawdzenie, czy kursor znajduje się na przycisku MAP1.
            ma_sound_start(&sounds[2]); // Puszczenie dźwięku kliknięcia, w menu.
            set_map("maps/map1.txt"); // Ustawienie mapy na map1.
            glNewList(mapDisplayList, GL_COMPILE); // Zaktualizowanie DisplayList'a.
            draw_map(); // Narysowanie nowej mapy.
            glEndList(); // Koniec Display List'a.
        }
        if (miceX <= 950 && miceX >= 150 && miceY <= 325 && miceY >= 225) { // Sprawdzenie, czy został naciśnięty LPM, czy znajdujesz się w opcjach oraz czy przycisk został naciśnięty.
            ma_sound_start(&sounds[2]); // Puszczenie dźwięku kliknięcia, w menu.
            set_map("maps/map2.txt"); // Ustawienie mapy na map2.
            glNewList(mapDisplayList, GL_COMPILE); // Zaktualizowanie DisplayList'a.
            draw_map(); // Narysowanie nowej mapy.
            glEndList(); // Koniec Display List'a.
        }
        if (miceX <= 950 && miceX >= 150 && miceY <= 450 && miceY >= 350) { // Sprawdzenie, czy został naciśnięty LPM, czy znajdujesz się w opcjach oraz czy przycisk został naciśnięty.
            ma_sound_start(&sounds[2]); // Puszczenie dźwięku kliknięcia, w menu.
            set_map("maps/map3.txt"); // Ustawienie mapy na map2.
            glNewList(mapDisplayList, GL_COMPILE); // Zaktualizowanie DisplayList'a.
            draw_map(); // Narysowanie nowej mapy.
            glEndList(); // Koniec Display List'a.
        }
        if (miceX <= 950 && miceX >= 150 && miceY <= 600 && miceY >= 500) { // Sprawdzenie, czy został naciśnięty LPM, czy znajdujesz się w opcjach oraz czy przycisk został naciśnięty.
            ma_sound_start(&sounds[2]); // Puszczenie dźwięku kliknięcia, w menu.
            Menus = MAIN_MANU; // Zmiana menu na menu główne..
        }
    }
    if (button == GLUT_LEFT_BUTTON && Menus == GAME && state == GLUT_DOWN) { // Sprawdzenie, czy został naciśnięty LPM, czy znajdujesz się w grze oraz czy przycisk został naciśnięty.
        if (paused && miceX <= 750 && miceX >= 350 && miceY <= 320 && miceY >= 230) { // Sprawdzenie, czy gra jest zapauzowana i czy znajduje się na przycisku CONTINUE.
            ma_sound_start(&sounds[2]); // Puszczenie dźwięku kliknięcia, w menu.
            paused = false; // Zmiana flagi paused na false;
        }
        if (paused && miceX <= 750 && miceX >= 350 && miceY <= 490 && miceY >= 280) { // Sprawdzenie, czy gra jest zapauzowana i czy znajduje się na przycisku SAVE YOURSELF.
            ma_sound_start(&sounds[2]); // Puszczenie dźwięku kliknięcia, w menu.
            proceed = true; // Ustawienie flagi proceed na true.
            character1.score = 0; // Wyzerowanie wynika dla gracza 1.
            character2.score = 0; // Wyzerowanie wynika dla gracza 2.
            restart_game(); // Wywołanie funkcji, która restartuje grę.
            restart_map_data(); // Wywołanie funkcji, która restartuje mapę gry.
            paused = false; // Zmiana flagi paused na false;
            Menus = MAIN_MANU; // Zmiana menu na menu główne.
        }
    }
}

// Funkcja texture_init której zadaniem jest wypełnić tablicę tekstur odpowiedznimi teksturami przy użyciu funkcji
// loadTexture która ładuje teksturę z pliku .png.
void textures_init() {
    texture[0] = loadTexture("graphics/button_start_game.png"); // Tekstura: Przycisk rozpoczęcia gry.
    texture[1] = loadTexture("graphics/button_options.png"); // Tekstura: Przycisk przejścia do opcji.
    texture[2] = loadTexture("graphics/button_exit.png"); // Tekstura: Przycisk do wyjścia z gry.
    texture[3] = loadTexture("graphics/button_map1.png"); // Tekstura: Przycisk do wybrania mapy 1.
    texture[4] = loadTexture("graphics/button_map2.png"); // Tekstura: Przycisk do wybrania mapy 2.
    texture[5] = loadTexture("graphics/button_map3.png"); // Tekstura: Przycisk do wybrania mapy 3.
    texture[6] = loadTexture("graphics/button_back.png"); // Tekstura: Przycisk do powrotu do poprzedniego menu.

    texture[7] = loadTexture("graphics/block_beaver_blue_front.png"); // Tekstura: Przód pierwszego gracza (tekstura ta będzie zmieniana później podczas obracania się).
    texture[8] = loadTexture("graphics/block_beaver_red_front.png"); // Tekstura: Przód drugiego gracza (tekstura ta będzie zmieniana później podczas obracania się).
    texture[9] = loadTexture("graphics/block_bomb.png"); // Tekstura: Bomba.
    texture[10] = loadTexture("graphics/block_crate.png"); // Tekstura: Skrzynka, zniszczalna przeszkoda.
    texture[11] = loadTexture("graphics/block_fire.png"); // Tekstura: Kula ognia eksplozji.
    texture[12] = loadTexture("graphics/block_wall.png"); // Tekstura: Drzewo, niezniszczalna przeszkoda.
    texture[13] = loadTexture("graphics/block_wall_alt.png"); // Tekstura: Alternatywne drzewo, niezniszczalna przeszkoda, służy do zrużnorodnienia tekstur.

    texture[14] = loadTexture("graphics/ingame_pause.png"); // Tekstura: Menu pauzy podczas gry.
    texture[15] = loadTexture("graphics/background_main_menu.png"); // Tekstura: Tło menu głównego.
    texture[16] = loadTexture("graphics/background_game.png"); // Tekstura: Tło podczas gry.
}

// Funkcja character_init służy do ustawienia graczy w odpowiedznich pozycjach oraz ustawienia odpowiednich flag.
void character_init() {
    character1.x = 950.0f; // Gracz pierwszy zostanie przeniesiony do miejsce 950.0 na ośi X.
    character1.y = 550.0f; // Gracz pierwszy zostanie przeniesiony do miejsce 550.0 na ośi Y (Teraz gracz pierwszy znajduje się w prawym dolnym rogu mapy).
    character2.x = 150.0f; // Gracz drugi zostanie przeniesiony do miejsce 150.0 na ośi X.
    character2.y = 150.0f; // Gracz drugi zostanie przeniesiony do miejsce 150.0 na ośi Y (Teraz gracz drugi znajduje się w lewym górnym rogu mapy).
    character1.ability_to_bomb = true; // Ustawienie flagi "ability_to_bomb" pierwszego gracza na true;
    character2.ability_to_bomb = true; // Ustawienie flagi "ability_to_bomb" drugiego gracza na true;
    character1.can_play_death_sound_effect = true; // Ustawienie flagi "can_play_death_sound_effect" pierwszego gracza na true;
    character2.can_play_death_sound_effect = true; // Ustawienie flagi "can_play_death_sound_effect" drugiego gracza na true;
}

// Funkcja init., inicjalizuje resztę funkcji, które są potrzebne do dalszego działania programu, w szczególności dotyczących ustawianiu
// macierzy stanu, przestrzeni ortogonalnej. Włączaniu lub wyłączaniu "GL capabilities", dźwięków, i ustawianiu display lists.
void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Wyczyszczenie tła kolorem białym.
    glMatrixMode(GL_PROJECTION); // Ustawienie macierzy stanu na macierz projekcji.
    glLoadIdentity(); // Ustawienie bieżącej macierzy na macierz jednostkową.

    ma_engine_init(NULL, &engine); // Inicjalizacja silnika dźwiękowego.

    ma_sound_init_from_file(&engine, "sounds/sound_place_bomb.wav", 0, NULL, NULL, &sounds[0]); // Wczytanie dźwięku podkładania bomby do tablicy dźwięków i ustalenie z którego silnika dźwiękowego ma korzystać.
    ma_sound_init_from_file(&engine, "sounds/sound_explosion.wav", 0, NULL, NULL, &sounds[1]); // Wczytanie dźwięku eksplozji do tablicy dźwięków i ustalenie z którego silnika dźwiękowego ma korzystać.
    ma_sound_init_from_file(&engine, "sounds/sound_menu_click.wav", 0, NULL, NULL, &sounds[2]); // Wczytanie dźwięku kliknięcia w menu do tablicy dźwięków i ustalenie z którego silnika dźwiękowego ma korzystać.
    ma_sound_init_from_file(&engine, "sounds/sound_death.wav", 0, NULL, NULL, &sounds[3]); // Wczytanie dźwięku śmierci do tablicy dźwięków i ustalenie z którego silnika dźwiękowego ma korzystać.
    ma_sound_init_from_file(&engine, "sounds/sound_pause.wav", 0, NULL, NULL, &sounds[4]); // Wczytanie dźwięku pauzy do tablicy dźwięków i ustalenie z którego silnika dźwiękowego ma korzystać.

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Ustawienie sposobu mieszania kolorów.
    glEnable(GL_BLEND); // Włączenie obsługi mieszania kolorów.
    gluOrtho2D(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0); // Ustawienie przestrzeni projekcyjnej na dwuwymiarową ortogonalną.
    glEnable(GL_TEXTURE_2D); // Włączenie obsługi tekstur 2D.
    glDisable(GL_DEPTH_TEST); // Wyłączenie testu głębokości.
    set_map("maps/map1.txt"); // Ustawienie mapy na map1.
    mapDisplayList = glGenLists(1); // Stworzenie jednego unikalnego indentyfikatora dla Display List'a.
    glNewList(mapDisplayList, GL_COMPILE); // Stworzenie nowego Display List'a, który zostanie wykorzystany do szybkiego wyświetlania drzewek na planszy.
    draw_map(); // Funkcja, która narysuje mapę, ta mapa zostanie zapamiętana do Display List'a do szybkiego wczytywania.
    glEndList(); // Koniec Display List'a.
}

// Funkcja main ustawia generator liczb pseudolosowych, inicializuje GLUT-a i wszystkie funkcje potrzebne do
// dalszego działania programu, tworzy okno, ustawia początkowe wartości dla postaci, ustawia głowną pętlę gry,
// i będzie reagować na wejścia z klawiatury i myszy, po czym odinicjalizuje silnik dźwięków i tablicę dźwięków.
int main(int argc, char **argv) {
    srand(time(NULL)); // Inicjalizacja generatora liczb pseudolosowych.

    glutInit(&argc, argv); // Inicjalizacia GLUT-a.
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA); // Ustawienie display mode'a, żeby korzystał z podwójnego bufora oraz z kolorów RGB i kanału alfa.
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT); // Ustawienie rozmiaru okna na WINDOW_WIDTH (1100) i WINDOW_HEIGHT (700).
    glutInitWindowPosition(0, 0); // Ustawienie pozycji okna (lewy górny róg ekranu).
    glutCreateWindow("Boberman"); // Stworzenie okna o nazwie "Boberman".
    character_init(); // Wywołanie funkcji, która ustawia parametry graczy.
    textures_init(); // Wywołanie funkcji, która wstawia tekstury do tablicy.
    init(); // Wywołanie funkcji, która inicjalizuje resztę funkcji potrzebnych do działania programu.

    glutDisplayFunc(display); // Funkcja, która jest odpowiedzialna za rysowanie rzeczy w oknie. Główna pętla.
    glutMouseFunc(mouse); // Funkcja, która wywołuję funkcję mouse za każdym razem, kiedy jest wykryty event związany z myszą.
    glutSpecialFunc(key_start_movement_character1); // Funkcja, która wywołuję funkcję key_start_movement_character1 za każdym razem, kiedy jest wykryte zdarzenie związane ze wciśnięciem klawisza na specjalnych klawiszach klawiatury(np. INSERT).
    glutSpecialUpFunc(key_stop_movement_character1); // Funkcja, która wywołuję funkcję key_stop_movement_character1 za każdym razem, kiedy jest wykryty zdarzenie związane z puszczeniem klawisza na specjalnych klawiszach klawiatury(np. INSERT).

    glutKeyboardFunc(key_start_movement_character2); // Funkcja, która wywołuję funkcję key_start_movement_character2 za każdym razem, kiedy jest wykryty zdarzenie związane ze wciśnięciem klawisza na normalnych klawiszach klawiatury(np. A).
    glutKeyboardUpFunc(key_stop_movement_character2); // Funkcja, która wywołuję funkcję key_stop_movement_character2 za każdym razem, kiedy jest wykryty zdarzenie związane z puszczeniem klawisza na normalnych klawiszach klawiatury(np. A).

    glutMainLoop(); // Funkcja, która tworzy pętlę zdarzeń.
    ma_engine_uninit(&engine); // Funkcja, która odinicjalizuje silnik wykorzystywany do dźwięków.
    ma_sound_uninit(sounds); // Funkcja, która odinicjalizuje tablicę dźwięków.
    return 0;
}