#include <SDL.h>
#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>
#include <map>
#include <fstream>  // Obsługa zapisu/odczytu plików (postęp gry)
#include <string>   // Obsługa tekstów
#include <iomanip>
#include <sstream>
#include <SDL_ttf.h>
#include <random>   // Generator liczb losowych (np. dla boosterów)

// --- STAŁE KONFIGURACYJNE ---
// Wymiary okna są obliczone na podstawie siatki gry:
// 16 kolumn * 70 pikseli = 1120 szerokości
// 12 wierszy * 70 pikseli = 840 wysokości
const int SCREEN_WIDTH = 1120;
const int SCREEN_HEIGHT = 840;
const int TILE_SIZE = 70;      // Rozmiar jednej kratki na mapie (kwadrat 70x70)
const int MAP_ROWS = 12;       // Liczba wierszy w siatce mapy
const int MAP_COLS = 16;       // Liczba kolumn w siatce mapy
const int LICZBA_LEVELI = 3;   // Całkowita liczba poziomów w grze

// --- STRUKTURY DLA POZIOMU Z BOSSEM ---
// Struktura reprezentująca spadającą belę siana
struct HayBale {
    SDL_Rect rect; // Przechowuje pozycję X, Y oraz szerokość i wysokość beli
    float y;       // Przechowuje dokładną pozycję pionową (float pozwala na płynniejszy ruch niż int)
    float speed;   // Prędkość spadania (o ile pikseli przesuwa się w dół w każdej klatce)
};

// Struktura reprezentująca koronę do zebrania (punkty u Bossa)
struct Crown {
    SDL_Rect rect; // Pozycja i wymiary korony
    bool active;   // Flaga: true = korona leży na mapie, false = została zebrana
};

// --- STRUKTURA DO MENU WYBORU KOLORU ---
// Przechowuje dane potrzebne do wyświetlenia przycisku wyboru koloru w menu
struct TractorOption {
    std::string filename;    // Ścieżka do pliku graficznego z teksturą traktora
    SDL_Color colorRGB;      // Kolor przycisku w menu (dla wizualizacji)
    SDL_Texture* texture;    // Wskaźnik na załadowaną teksturę (obrazek) w pamięci karty graficznej
};

// --- STRUKTURA DOPALACZY (BOOSTERÓW) ---
// Typ wyliczeniowy (enum) ułatwia rozróżnianie rodzajów bonusów w kodzie
enum BoosterType { SPEED_UP, INVINCIBLE, SLOW_ENEMY, FREEZE, NONE };

struct Booster {
    SDL_Rect rect;    // Obszar, który zajmuje booster na mapie
    BoosterType type; // Rodzaj efektu, jaki wywoła (np. zamrożenie wrogów)
    bool active;      // Czy booster jest widoczny i możliwy do zebrania
};

// --- DANE MAP (1 = Ściana/Trawa, 0 = Zboże do skoszenia) ---
// Trójwymiarowa tablica przechowująca układ wszystkich poziomów.
// [Numer Levelu] [Wiersz] [Kolumna]
int mazeLevels[LICZBA_LEVELI][MAP_ROWS][MAP_COLS] = {
    {   // POZIOM 1: Klasyczny labirynt
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,1,0,0,0,0,0,0,0,1,0,0,1},
        {1,0,1,0,1,0,1,1,0,1,1,0,1,0,1,1},
        {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1},
        {1,0,1,1,1,0,1,1,0,1,1,1,1,1,0,1},
        {1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,1},
        {1,1,1,1,1,1,0,1,1,1,1,1,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,0,1,1,0,1,1,1,1,0,1,1,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1},
        {1,0,0,0,1,1,1,1,0,1,1,1,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    {   // POZIOM 2: Bardziej otwarty układ
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
        {1,0,1,1,0,1,0,0,0,0,1,0,1,1,0,1},
        {1,0,1,1,0,1,1,0,0,1,1,0,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,0,1,1,1,0,1,1,0,1,1,1,0,1,1},
        {1,1,0,1,1,1,0,1,1,0,1,1,1,0,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,0,1,1,0,0,1,1,0,1,1,0,1},
        {1,0,1,1,0,1,0,0,0,0,1,0,1,1,0,1},
        {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    {   // POZIOM 3 (BOSS): Arena z przeszkodami
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1},
        {1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,1,1,0,0,0,0,0,0,0,1,1,0,1},
        {1,0,0,1,1,0,0,0,0,0,0,0,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    }
};

// Aktualnie załadowana mapa w pamięci.
// To na tej tablicy gra operuje (zmienia 0 na 2 po skoszeniu).
int maze[MAP_ROWS][MAP_COLS];

// Funkcja kopiuje wybrany poziom z tablicy stałej `mazeLevels` do tablicy roboczej `maze`.
// Przy okazji resetuje wszystkie boostery, aby były aktywne na nowym poziomie.
void ladujPoziom(int nr, std::vector<Booster>& bst) {
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            maze[r][c] = mazeLevels[nr][r][c];
        }
    }
    for (auto& b : bst) b.active = true;
}

// Funkcja przelicza, ile jeszcze zostało "nieskoszonego zboża" (pól o wartości 0).
// Jest używana do sprawdzenia warunku zwycięstwa.
int liczPunkty() {
    int p = 0;
    for (int r = 0; r < MAP_ROWS; r++)
        for (int c = 0; c < MAP_COLS; c++)
            if (maze[r][c] == 0) p++;
    return p;
}

// Pomocnicza funkcja do rysowania tekstu na ekranie.
// SDL_ttf wymaga stworzenia powierzchni (Surface), zamiany na teksturę i narysowania.
// Funkcja robi to wszystko i od razu czyści pamięć po tymczasowych obiektach.
void rysujTekst(SDL_Renderer* renderer, TTF_Font* font, std::string tekst, int x, int y, SDL_Color kolor) {
    if (tekst.empty()) return;
    SDL_Surface* surface = TTF_RenderText_Solid(font, tekst.c_str(), kolor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = { x, y, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);   // Ważne: zwalnianie pamięci RAM
        SDL_DestroyTexture(texture); // Ważne: zwalnianie pamięci VRAM
    }
}

// Funkcja centrująca tekst w poziomie (przydatna do menu i napisów końcowych).
// Oblicza szerokość tekstu i ustawia X tak, by był na środku ekranu.
void rysujTekstWycentrowany(SDL_Renderer* renderer, TTF_Font* font, std::string tekst, int y, SDL_Color kolor) {
    if (tekst.empty()) return;
    int w, h;
    TTF_SizeText(font, tekst.c_str(), &w, &h); // Zmierzenie wymiarów tekstu
    rysujTekst(renderer, font, tekst, (SCREEN_WIDTH - w) / 2, y, kolor);
}

// --- KLASA GRACZA (TRAKTOR) ---
class Player {
private:
    SDL_Rect rect;       // Prostokąt gracza (pozycja X, Y, szerokość, wysokość)
    int baseSpeed;       // Prędkość podstawowa (normalna)
    int currentSpeed;    // Aktualna prędkość (może być podwojona przez booster)
    bool invincible = false; // Flaga nieśmiertelności (po zebraniu tarczy)
    Uint32 invincibleTimer = 0; // Czas do końca nieśmiertelności
    Uint32 speedTimer = 0;      // Czas do końca przyspieszenia
    SDL_Texture* texture = nullptr; // Wskaźnik na grafikę gracza

public:
    Player(int x, int y, int size, int moveSpeed) {
        rect.x = x;
        rect.y = y;
        rect.w = size;
        rect.h = size;
        baseSpeed = moveSpeed;
        currentSpeed = moveSpeed;
    }

    // Ustawia pozycję gracza (np. przy restarcie poziomu)
    void setPos(int x, int y) { rect.x = x; rect.y = y; }

    // Przypisuje teksturę (wygląd) traktora
    void setTexture(SDL_Texture* tex) { texture = tex; }

    // Włącza działanie boostera i ustawia czas jego trwania
    void applyBooster(BoosterType type) {
        Uint32 now = SDL_GetTicks(); // Pobranie aktualnego czasu gry w ms
        if (type == SPEED_UP) {
            currentSpeed = baseSpeed * 2;
            speedTimer = now + 5000; // Efekt trwa 5000 ms (5 sekund)
        }
        else if (type == INVINCIBLE) {
            invincible = true;
            invincibleTimer = now + 5000; // Efekt trwa 5000 ms (5 sekund)
        }
    }

    // Główna funkcja sterowania. Sprawdza klawisze i obsługuje kolizje ze ścianami.
    void handleInput(SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            // Zapamiętujemy starą pozycję przed ruchem
            int oldX = rect.x;
            int oldY = rect.y;

            // Zmiana pozycji w zależności od wciśniętej strzałki
            switch (e.key.keysym.sym) {
            case SDLK_UP: rect.y -= currentSpeed; break;
            case SDLK_DOWN: rect.y += currentSpeed; break;
            case SDLK_LEFT: rect.x -= currentSpeed; break;
            case SDLK_RIGHT: rect.x += currentSpeed; break;
            }

            // Sprawdzanie kolizji ze ścianami mapy
            for (int r = 0; r < MAP_ROWS; r++) {
                for (int c = 0; c < MAP_COLS; c++) {
                    if (maze[r][c] == 1) { // Jeśli pole jest ścianą (wartość 1)
                        // Tworzymy prostokąt ściany
                        SDL_Rect wall = { c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE };

                        // Sprawdzamy czy prostokąt gracza nachodzi na prostokąt ściany
                        if (SDL_HasIntersection(&rect, &wall)) {
                            // Jeśli tak, cofamy ruch (anulujemy zmianę pozycji)
                            rect.x = oldX;
                            rect.y = oldY;
                        }
                    }
                }
            }
        }
    }

    // Aktualizacja stanu gracza w każdej klatce
    void update() {
        Uint32 now = SDL_GetTicks();

        // Sprawdzamy czy czas boosterów minął
        if (now > speedTimer) currentSpeed = baseSpeed;
        if (now > invincibleTimer) invincible = false;

        // Blokada wyjazdu poza ekran (żeby gracz nie zniknął)
        if (rect.x < 0) rect.x = 0;
        if (rect.y < 0) rect.y = 0;
        if (rect.x + rect.w > SCREEN_WIDTH) rect.x = SCREEN_WIDTH - rect.w;
        if (rect.y + rect.h > SCREEN_HEIGHT) rect.y = SCREEN_HEIGHT - rect.h;
    }

    // Rysowanie gracza na ekranie
    void draw(SDL_Renderer* renderer) {
        if (texture) {
            SDL_SetTextureColorMod(texture, 255, 255, 255); // Reset filtra koloru (rysowanie normalne)
            SDL_RenderCopy(renderer, texture, NULL, &rect);
        }
        else {
            // Zapasowe rysowanie kolorowego kwadratu, gdyby tekstura się nie wczytała
            if (invincible) SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Żółty jak nieśmiertelny
            else SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    // Funkcje pomocnicze (gettery) do pobierania danych o graczu
    float getInvincibleRemainingTime() {
        if (SDL_GetTicks() >= invincibleTimer) return 0;
        return(invincibleTimer - SDL_GetTicks()) / 1000.0f; // Zwraca czas w sekundach
    }
    float getSpeedRemainingTime() {
        if (SDL_GetTicks() >= speedTimer) return 0;
        return(speedTimer - SDL_GetTicks()) / 1000.0f;
    }
    int getX() { return rect.x; }
    int getY() { return rect.y; }
    SDL_Rect getRect() { return rect; }
    bool isInvincible() { return invincible; }
    bool isSpeedUp() { return currentSpeed > baseSpeed; }

    // Resetuje wszystkie moce (np. po śmierci lub zmianie poziomu)
    void resetBoosters() {
        currentSpeed = baseSpeed;
        invincible = false;
        invincibleTimer = 0;
        speedTimer = 0;
    }
};

// --- KLASA WROGA (PRZECIWNIKA) ---
class Enemy {
private:
    SDL_Rect rect;
    int baseSpeed;
    int currentSpeed;
    bool frozen = false;     // Czy wróg jest zamrożony
    bool slowed = false;     // Czy wróg jest spowolniony (błoto)
    Uint32 freezeTimer = 0;
    Uint32 slowTimer = 0;
    SDL_Texture* texture = nullptr;
    std::vector<SDL_Point> path; // Lista punktów ścieżki do gracza
    int pathTimer; // Licznik klatek do sterowania częstotliwością obliczeń

    // Algorytm BFS (Breadth-First Search) do znajdowania najkrótszej drogi.
    // Pozwala wrogowi inteligentnie omijać ściany i gonić gracza.
    void findPath(int startX, int startY, int targetX, int targetY) {
        path.clear();
        // Przeliczenie pikseli na współrzędne siatki (kolumny i wiersze)
        int sCol = startX / TILE_SIZE;
        int sRow = startY / TILE_SIZE;
        int tCol = targetX / TILE_SIZE;
        int tRow = targetY / TILE_SIZE;

        if (sCol == tCol && sRow == tRow) return; // Jeśli wróg jest na tym samym polu co gracz, nie szukaj

        std::queue<SDL_Point> q;
        q.push({ sCol, sRow });

        std::map<int, SDL_Point> parentMap; // Mapa rodziców do odtworzenia ścieżki
        bool visited[MAP_ROWS][MAP_COLS] = { false };
        visited[sRow][sCol] = true;

        bool found = false;
        // Pętla algorytmu "zalewania" mapy
        while (!q.empty()) {
            SDL_Point curr = q.front(); q.pop();
            if (curr.x == tCol && curr.y == tRow) { found = true; break; }

            // Sprawdzanie sąsiadów (dół, góra, prawo, lewo)
            int dx[] = { 0, 0, 1, -1 };
            int dy[] = { 1, -1, 0, 0 };
            for (int i = 0; i < 4; i++) {
                int nx = curr.x + dx[i], ny = curr.y + dy[i];
                // Wróg może chodzić tylko po polach 0 (zboże) i 2 (ściernisko), nie po 1 (ściana)
                if (nx >= 0 && nx < MAP_COLS && ny >= 0 && ny < MAP_ROWS && (maze[ny][nx] == 0 || maze[ny][nx] == 2) && !visited[ny][nx]) {
                    visited[ny][nx] = true;
                    parentMap[ny * MAP_COLS + nx] = curr;
                    q.push({ nx, ny });
                }
            }
        }

        // Odtwarzanie ścieżki od końca (od gracza do wroga)
        if (found) {
            SDL_Point curr = { tCol, tRow };
            while (curr.x != sCol || curr.y != sRow) {
                // Dodajemy offset +10, żeby wróg celował w środek kafelka, a nie w róg
                path.push_back({ curr.x * TILE_SIZE + 10, curr.y * TILE_SIZE + 10 });
                curr = parentMap[curr.y * MAP_COLS + curr.x];
            }
            std::reverse(path.begin(), path.end()); // Odwracamy ścieżkę, żeby była od wroga do gracza
        }
    }

public:
    Enemy(int x, int y, int size, int moveSpeed) {
        rect = { x, y, size, size };
        baseSpeed = moveSpeed;
        currentSpeed = moveSpeed;
        // Losowy start timera sprawia, że wrogowie nie ruszają się idealnie synchronicznie
        pathTimer = rand() % 30;
    }
    void setPos(int x, int y) { rect.x = x; rect.y = y; path.clear(); }
    void setTexture(SDL_Texture* tex) { texture = tex; }
    SDL_Rect getRect() { return rect; }

    // Funkcje aktywujące efekty na wrogu
    void freeze() {
        frozen = true;
        slowed = false;
        freezeTimer = SDL_GetTicks() + 5000;
    }

    void slowDown() {
        slowed = true;
        frozen = false;
        slowTimer = SDL_GetTicks() + 5000;
    }

    void setSpeed(int s) { baseSpeed = s; currentSpeed = s; }

    // Główna logika ruchu wroga (wywoływana w każdej klatce)
    void update(int playerX, int playerY) {
        Uint32 now = SDL_GetTicks();
        // Sprawdzanie czy efekty minęły
        if (frozen && now > freezeTimer) frozen = false;
        if (slowed && now > slowTimer) slowed = false;

        if (frozen) return; // Jak zamrożony, to przerywamy funkcję (stoi w miejscu)

        // Przelicz trasę co 30 klatek (optymalizacja wydajności, żeby nie liczyć co klatkę)
        pathTimer++;
        if (pathTimer % 30 == 0) {
            findPath(rect.x, rect.y, playerX, playerY);
        }

        // Logika spowolnienia (błoto): Wróg rusza się tylko w parzystych klatkach
        bool shouldMove = true;
        if (slowed) {
            if (pathTimer % 2 != 0) shouldMove = false;
        }

        // Wykonanie ruchu wzdłuż wyznaczonej ścieżki
        if (shouldMove && !path.empty()) {
            SDL_Point target = path[0];
            int speed = baseSpeed;

            if (rect.x < target.x) rect.x += speed;
            else if (rect.x > target.x) rect.x -= speed;
            if (rect.y < target.y) rect.y += speed;
            else if (rect.y > target.y) rect.y -= speed;

            // Jeśli wróg dotarł do punktu kontrolnego ścieżki, usuń go i idź do następnego
            if (abs(rect.x - target.x) < speed + 1 && abs(rect.y - target.y) < speed + 1) {
                path.erase(path.begin());
            }
        }
    }

    // Gettery czasu efektów (do wyświetlania w HUD)
    float getFreezeRemainingTime() {
        if (SDL_GetTicks() >= freezeTimer) return 0;
        return(freezeTimer - SDL_GetTicks()) / 1000.0f;
    }
    float getSlowRemainingTime() {
        if (SDL_GetTicks() >= slowTimer) return 0;
        return(slowTimer - SDL_GetTicks()) / 1000.0f;
    }

    void draw(SDL_Renderer* renderer) {
        if (texture) {
            SDL_SetTextureColorMod(texture, 255, 255, 255);
            SDL_RenderCopy(renderer, texture, NULL, &rect);
        }
        else {
            // Kolorowanie kwadratu w zależności od stanu (debug - gdy brak tekstury)
            if (frozen) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // Błękitny
            else if (slowed) SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // Brązowy
            else SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Czerwony
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    bool isFrozen() { return frozen; }
    bool isSlowed() { return slowed; }

    // Resetuje wroga do stanu początkowego
    void resetStatus() {
        frozen = false;
        slowed = false;
        freezeTimer = 0;
        slowTimer = 0;
        currentSpeed = baseSpeed;
        path.clear();
    }
};

// Funkcja zapisu postępu do pliku tekstowego (0 = zablokowany, 1 = odblokowany)
void zapiszPostep(bool odblokowane[], int rozmiar) {
    std::ofstream plik("postep.txt");
    if (plik.is_open()) {
        for (int i = 0; i < rozmiar; i++) {
            plik << odblokowane[i] << " ";
        }
        plik.close();
    }
}

// Funkcja odczytu postępu. Jeśli plik nie istnieje, tworzy domyślny stan.
void wczytajPostep(bool odblokowane[], int rozmiar) {
    std::ifstream plik("postep.txt");
    if (plik.is_open()) {
        for (int i = 0; i < rozmiar; i++) {
            plik >> odblokowane[i];
        }
        plik.close();
    }
    else {
        // Domyślnie odblokowany tylko 1 poziom, reszta zablokowana
        odblokowane[0] = true;
        for (int i = 1; i < rozmiar; i++) odblokowane[i] = false;
    }
}

// Funkcja ładująca obrazki BMP i zamieniająca je na tekstury SDL.
// Dodatkowo obsługuje "Color Key" - usuwa tło w kolorze Magenta (255, 0, 255).
SDL_Texture* wczytajTeksture(SDL_Renderer* renderer, const char* path, bool colorKey = false) {
    SDL_Surface* tempSurface = SDL_LoadBMP(path);
    if (!tempSurface) {
        std::cout << "Nie udalo sie wczytac " << path << "! Blad: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    if (colorKey) {
        // --- NAPRAWA PIKSELI TŁA (252,0,252 -> 255,0,255) ---
        // Niektóre programy graficzne zapisują Magentę z minimalnym błędem (np. 252 zamiast 255).
        // Ten kod ręcznie iteruje po pikselach i naprawia te kolory, aby przezroczystość działała.
        SDL_LockSurface(tempSurface);

        int bpp = tempSurface->format->BytesPerPixel;
        for (int y = 0; y < tempSurface->h; y++) {
            for (int x = 0; x < tempSurface->w; x++) {
                // Obliczanie adresu piksela w pamięci
                Uint8* p = (Uint8*)tempSurface->pixels + y * tempSurface->pitch + x * bpp;
                Uint32 pixelValue = 0;

                // Odczyt wartości piksela (dla formatów 24-bit i 32-bit)
                if (bpp == 3) {
                    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                        pixelValue = p[0] << 16 | p[1] << 8 | p[2];
                    else
                        pixelValue = p[0] | p[1] << 8 | p[2] << 16;
                }
                else if (bpp == 4) {
                    pixelValue = *(Uint32*)p;
                }

                Uint8 r, g, b;
                SDL_GetRGB(pixelValue, tempSurface->format, &r, &g, &b);

                // Jeśli piksel ma kolor "prawie magenta" (błędny), napraw go na "czystą magentę"
                if ((r == 252 && g == 0 && b == 252) || (r == 225 && g == 0 && b == 255)) {
                    Uint32 newPixel = SDL_MapRGB(tempSurface->format, 255, 0, 255);

                    if (bpp == 3) {
                        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                            p[0] = (newPixel >> 16) & 0xFF;
                            p[1] = (newPixel >> 8) & 0xFF;
                            p[2] = newPixel & 0xFF;
                        }
                        else {
                            p[0] = newPixel & 0xFF;
                            p[1] = (newPixel >> 8) & 0xFF;
                            p[2] = (newPixel >> 16) & 0xFF;
                        }
                    }
                    else if (bpp == 4) {
                        *(Uint32*)p = newPixel;
                    }
                }
            }
        }
        SDL_UnlockSurface(tempSurface);

        // Ustawienie klucza koloru: wszystko co jest Magentą (255,0,255) staje się przezroczyste
        SDL_SetColorKey(tempSurface, SDL_TRUE, SDL_MapRGB(tempSurface->format, 255, 0, 255));
    }

    // Konwersja z powierzchni (CPU) na teksturę (GPU)
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_FreeSurface(tempSurface); // Zwolnienie pamięci RAM
    return tex;
}

// --- FUNKCJA MAIN (Punkt startowy programu) ---
int main(int argc, char* argv[]) {
    // --- INICJALIZACJA SDL ---
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Tworzenie okna gry
    SDL_Window* window = SDL_CreateWindow("Traktorzysta", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    // Tworzenie renderera (narzędzia do rysowania, z włączonym VSync dla płynności)
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!window) {
        std::cout << "Window Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Inicjalizacja systemu czcionek
    if (TTF_Init() == -1) {
        printf("Błąd TTF_Init: %s\n", TTF_GetError());
        return 1;
    }

    // --- ŁADOWANIE ZASOBÓW (Grafiki i Czcionki) ---
    TTF_Font* fontUI = TTF_OpenFont("assets\\couree.fon", 20); // Czcionka mniejsza (interfejs)
    TTF_Font* fontTitle = TTF_OpenFont("assets\\couree.fon", 72); // Czcionka duża (tytuły)

    if (!fontUI || !fontTitle) {
        printf("Błąd ładowania czcionki: %s\n", TTF_GetError());
    }

    // Ładowanie grafik (boostery)
    SDL_Texture* shieldTexture = wczytajTeksture(renderer, "assets\\shield.bmp", true);
    SDL_Texture* speedTexture = wczytajTeksture(renderer, "assets\\speedF.bmp", true);
    SDL_Texture* slowTexture = wczytajTeksture(renderer, "assets\\slow.bmp", true);
    SDL_Texture* freezeTexture = wczytajTeksture(renderer, "assets\\freezeF.bmp", true);

    // Ładowanie grafik mapy (ściany, zboże, ściernisko)
    SDL_Texture* wallTexture = wczytajTeksture(renderer, "assets\\grass2.bmp");
    SDL_Texture* cropTexture = wczytajTeksture(renderer, "assets\\wheat2.bmp");
    SDL_Texture* stubbleTexture = wczytajTeksture(renderer, "assets\\wheatCut2.bmp");

    // Ładowanie grafik Bossa (siano, boss, korona)
    SDL_Texture* hayTexture = wczytajTeksture(renderer, "assets\\hay.bmp", true);
    SDL_Texture* bossTexture = wczytajTeksture(renderer, "assets\\soltys.bmp", true);
    SDL_Texture* crownTexture = wczytajTeksture(renderer, "assets\\crown.bmp", true);

    // --- KONFIGURACJA KOLORÓW TRAKTORA ---
    // Tworzymy listę dostępnych traktorów do wyboru w menu
    std::vector<TractorOption> tractorOptions;
    tractorOptions.push_back({ "assets\\tractor_blue.bmp",   {0, 0, 255, 255},     nullptr });
    tractorOptions.push_back({ "assets\\tractor_cyan.bmp",   {0, 255, 255, 255},   nullptr });
    tractorOptions.push_back({ "assets\\tractor_green.bmp",  {0, 255, 0, 255},     nullptr });
    tractorOptions.push_back({ "assets\\tractor_orange.bmp", {255, 165, 0, 255},   nullptr });
    tractorOptions.push_back({ "assets\\tractor_pink.bmp",   {255, 105, 180, 255}, nullptr });
    tractorOptions.push_back({ "assets\\tractor_purple.bmp", {128, 0, 128, 255},   nullptr });
    tractorOptions.push_back({ "assets\\tractor_red.bmp",    {255, 0, 0, 255},     nullptr }); // Domyślny
    tractorOptions.push_back({ "assets\\tractor_yellow.bmp", {255, 255, 0, 255},   nullptr });

    // Wczytujemy tekstury dla każdego wariantu kolorystycznego
    for (auto& opt : tractorOptions) {
        opt.texture = wczytajTeksture(renderer, opt.filename.c_str(), true);
    }
    int selectedColorIndex = 6; // Wybrany kolor (indeks 6 = czerwony)

    // Ładowanie grafik wrogów (różne zestawy dla różnych poziomów)
    SDL_Texture* enemyTextures[2][3];
    // Zestaw dla Level 1
    enemyTextures[0][0] = wczytajTeksture(renderer, "assets\\babes11.bmp", true);
    enemyTextures[0][1] = wczytajTeksture(renderer, "assets\\babes12.bmp", true);
    enemyTextures[0][2] = wczytajTeksture(renderer, "assets\\babes13.bmp", true);
    // Zestaw dla Level 2
    enemyTextures[1][0] = wczytajTeksture(renderer, "assets\\babes21.bmp", true);
    enemyTextures[1][1] = wczytajTeksture(renderer, "assets\\babes22.bmp", true);
    enemyTextures[1][2] = wczytajTeksture(renderer, "assets\\babes23.bmp", true);

    // Grafiki menu (przyciski poziomów)
    SDL_Texture* lvl1Tex = wczytajTeksture(renderer, "assets\\lvl1.bmp");
    SDL_Texture* lvl2Tex = wczytajTeksture(renderer, "assets\\lvl2.bmp");
    SDL_Texture* bossLvlTex = wczytajTeksture(renderer, "assets\\bosslvl.bmp");

    // Inicjalizacja obiektów gry
    Player player(80, 80, 50, 10); // Gracz na pozycji (80,80)
    std::vector<Enemy> enemies;    // Lista przeciwników
    std::vector<HayBale> hayBales; // Lista spadającego siana
    std::vector<Crown> crowns;     // Lista koron
    Uint32 haySpawnTimer = 0;      // Timer do generowania siana

    // Stałe pozycje boosterów na mapie (w rogach i na środku)
    SDL_Rect boosterPositions[4] = {
        {225, 85, 40, 40},
        {505, 225, 40, 40},
        {85, 715, 40, 40},
        {995, 85, 40, 40}
    };
    std::vector<Booster> boosters;
    // Wstępne dodanie boosterów do listy
    boosters.push_back({ {225, 85, 40, 40}, SPEED_UP, true });
    boosters.push_back({ {505, 225, 40, 40}, INVINCIBLE, true });
    boosters.push_back({ {85, 715, 40, 40}, SLOW_ENEMY, true });
    boosters.push_back({ {995, 85, 40, 40}, FREEZE, true });

    // Wczytanie postępu gry z pliku
    bool odblokowaneLevele[4];
    wczytajPostep(odblokowaneLevele, 4);

    // Obliczanie pozycji przycisków w menu (poziomy)
    SDL_Rect przyciskiMenu[3];
    int sqSize = 220;
    int spacing = (SCREEN_WIDTH - (3 * sqSize)) / 4;
    int menuOffsetY = 150;
    for (int i = 0; i < 3; i++) {
        przyciskiMenu[i] = { spacing + i * (sqSize + spacing), (SCREEN_HEIGHT - sqSize) / 2 + menuOffsetY, sqSize, sqSize };
    }

    // Obliczanie pozycji przycisków w menu (kolory traktora - siatka 2x4)
    SDL_Rect colorButtons[8];
    int colorBtnSize = 40;
    int colorBtnGap = 10;
    int gridStartX = SCREEN_WIDTH / 2 + 100;
    int gridStartY = 300;

    for (int i = 0; i < 8; i++) {
        int row = i / 4;
        int col = i % 4;
        colorButtons[i] = {
            gridStartX + col * (colorBtnSize + colorBtnGap),
            gridStartY + row * (colorBtnSize + colorBtnGap),
            colorBtnSize,
            colorBtnSize
        };
    }
    SDL_Rect previewRect = { SCREEN_WIDTH / 2 - 260, gridStartY, 100, 100 }; // Miejsce na podgląd wybranego traktora

    // Zmienne sterujące stanem gry
    int aktualnyLvl = 0;
    bool running = true;       // Czy gra działa
    bool gameOver = false;     // Czy przegrałeś
    bool menuActive = true;    // Czy jesteśmy w menu
    int aktualnePunkty = 0;    // Punkty zdobyte
    int pozostalePunkty = 0;   // Ile zboża/koron zostało do zebrania

    std::string playerName = "Gracz";
    SDL_StartTextInput(); // Włącza obsługę wprowadzania tekstu

    // Zmienne dla ekranów końcowych (zwycięstwo/przejście poziomu)
    bool levelCompleteScreen = false;
    bool gameWonScreen = false;
    bool creditsActive = false;
    float creditsScroll = 0; // Pozycja przewijania napisów końcowych

    // Treść napisów końcowych (Credits)
    std::vector<std::string> endCredits = {
        "--- TRAKTORZYSTA ---", "The Game", "GRATULACJE!", "Udało ci się pokonać baby i sołtysa!", "", "Dzielny Traktorzysta : ", playerName, "",
        "--- POKONANI ---", "Szalone Baby: 6", "Zly Soltys: 1", "",
        "--- OBSADA ---", "", "Traktor - Czerwona Strzala", "Koło gospodyń wiejskich", "Sołtys wsi", "Traktorzysta","",
        "--- PODZIEKOWANIA ---", "", "Dla kobiet za stworzenie koron", "Dla kazdego rolnika", "Dla tutoriali na youtubie","Dla wszystkich testerów gry","",
        "--- CIEKAWOSTKI ---", "", "Zadna baba nie ucierpiala", "podczas produkcji gry.", "",
        "Ilosc zuzytej colki zero: Zbyt duza, by policzyc", "Stworzone linie kodu: >1500", "",
        "--- NOTA PRAWNA ---", "","Wszelkie podobienstwo do prawdziwych","Soltysow jest czysto przypadkowe.","Traktor nie posiada waznego przegladu.","",
        "--- KADRA ---","Senior Developer: Lena Janik","Junior Developer/Marketing Manager: Bartosz Dumański","CEO/Art Director: Karolina Drost","",
        "--- PAMIECI BLEDOW ---", "","Ku pamieci buga, przez ktorego","zadna biblioteka nie dzialala","Bo nasza kochana CEO,"," chciala biblioteki w githubie","",
        "--- KONIEC ---", "", "Dziekujemy za gre!", "", "Wcisnij ESC aby wrocic", "", "", "", "", "", "", "", "", "", "", "", "Specjalne podziekowania dla Google Gemini"
    };
    for (int i = 0; i < 20; i++) endCredits.push_back(""); // Dodajemy puste linie na koniec, żeby tekst wyjechał poza ekran

    SDL_Event e;

    // --- GŁÓWNA PĘTLA GRY ---
    while (running) {

        // Obsługa zdarzeń (klawiatura, mysz, zamykanie okna)
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false; // Zamknięcie okna "X"

            if (menuActive) {
                // --- LOGIKA MENU ---

                // Wpisywanie imienia
                if (e.type == SDL_TEXTINPUT) {
                    if (playerName.length() < 15) {
                        playerName += e.text.text;
                    }
                }
                else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_BACKSPACE && playerName.length() > 0) {
                        playerName.pop_back();
                    }
                }

                // Kliknięcia myszką w menu
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                    SDL_Point mousePos = { e.button.x, e.button.y };

                    // Sprawdzenie czy kliknięto w wybór koloru
                    for (int i = 0; i < 8; i++) {
                        if (SDL_PointInRect(&mousePos, &colorButtons[i])) {
                            selectedColorIndex = i;
                        }
                    }

                    // Sprawdzenie czy kliknięto w przycisk poziomu (START GRY)
                    for (int i = 0; i < 3; i++) {
                        if (SDL_PointInRect(&mousePos, &przyciskiMenu[i]) && i < LICZBA_LEVELI && odblokowaneLevele[i]) {
                            aktualnyLvl = i;
                            menuActive = false; // Wyjście z menu, start gry

                            player.setTexture(tractorOptions[selectedColorIndex].texture);

                            // Reset stanu gry przed startem nowego poziomu
                            boosters.clear();
                            enemies.clear();
                            hayBales.clear();
                            crowns.clear();

                            // Konfiguracja poziomu w zależności od numeru
                            if (aktualnyLvl == 2) {
                                // --- KONFIGURACJA LEVELU Z BOSSEM ---
                                // 1. Czyścimy mapę (usuwamy stare dane)
                                for (int r = 1; r < MAP_ROWS - 1; r++) {
                                    for (int c = 1; c < MAP_COLS - 1; c++) {
                                        // Puste przebiegi
                                    }
                                }
                                ladujPoziom(aktualnyLvl, boosters);

                                // 2. Ustawiamy środek mapy na ściernisko (2), żeby była pusta arena
                                for (int r = 1; r < MAP_ROWS - 1; r++) {
                                    for (int c = 1; c < MAP_COLS - 1; c++) {
                                        if (maze[r][c] != 1) maze[r][c] = 2;
                                    }
                                }

                                // 3. Tworzenie bossa
                                Enemy boss(SCREEN_WIDTH / 2 - 30, SCREEN_HEIGHT / 2 - 30, 60, 1);
                                boss.setTexture(bossTexture);
                                boss.setSpeed(1);
                                enemies.push_back(boss);

                                // 4. Generowanie koron w losowych miejscach
                                int crownsToSpawn = 15;
                                while (crownsToSpawn > 0) {
                                    int rCol = (rand() % (MAP_COLS - 2)) + 1;
                                    int rRow = (rand() % (MAP_ROWS - 2)) + 1;

                                    if (maze[rRow][rCol] == 1) continue; // Pomiń, jeśli wylosowano ścianę

                                    int cx = rCol * TILE_SIZE + 10;
                                    int cy = rRow * TILE_SIZE + 10;
                                    SDL_Rect tempRect = { cx, cy, 50, 50 };
                                    SDL_Rect bossRect = boss.getRect();

                                    bool collision = false;
                                    if (SDL_HasIntersection(&tempRect, &bossRect)) collision = true;
                                    for (const auto& existingCrown : crowns) {
                                        if (SDL_HasIntersection(&tempRect, &existingCrown.rect)) {
                                            collision = true;
                                            break;
                                        }
                                    }

                                    if (!collision) {
                                        crowns.push_back({ tempRect, true });
                                        crownsToSpawn--;
                                    }
                                }
                                pozostalePunkty = 15; // Cel u Bossa: zebrać 15 koron
                            }
                            else {
                                // --- KONFIGURACJA ZWYKŁEGO LEVELU ---
                                // 1. Losowanie i ustawianie boosterów
                                std::vector<BoosterType> types = { SPEED_UP, INVINCIBLE, SLOW_ENEMY, FREEZE };
                                std::random_device rd;
                                std::mt19937 g(rd());
                                std::shuffle(types.begin(), types.end(), g);

                                for (int k = 0; k < 4; k++) {
                                    boosters.push_back({ boosterPositions[k], types[k], true });
                                }

                                // 2. Generowanie 3 wrogów w rogach mapy
                                int spawnX[] = { 990, 80, 990 };
                                int spawnY[] = { 80, 710, 710 };

                                for (int k = 0; k < 3; k++) {
                                    Enemy newEnemy(spawnX[k], spawnY[k], 50, 1);
                                    newEnemy.setTexture(enemyTextures[aktualnyLvl][k]);
                                    enemies.push_back(newEnemy);
                                }

                                ladujPoziom(aktualnyLvl, boosters);
                                pozostalePunkty = liczPunkty(); // Cel: skosić całe zboże
                            }

                            // Wspólny reset gracza
                            aktualnePunkty = 0;
                            player.setPos(80, 80);
                            gameOver = false;
                            player.resetBoosters();
                            SDL_StopTextInput();
                        }
                    }
                }
            }
            else {
                // --- OBSŁUGA GRY (NIE MENU) ---

                // Ekran przejścia poziomu (Level Complete)
                if (levelCompleteScreen) {
                    if (e.type == SDL_KEYDOWN) {
                        if (e.key.keysym.sym == SDLK_SPACE) {
                            // PRZEJŚCIE DO NASTĘPNEGO POZIOMU (RESET I KONFIGURACJA)
                            levelCompleteScreen = false;
                            aktualnyLvl++;

                            boosters.clear();
                            enemies.clear();
                            hayBales.clear();
                            crowns.clear();

                            // Konfiguracja nowego poziomu (kod analogiczny do tego w menu)
                            if (aktualnyLvl == 2) {
                                // Boss Level Setup
                                for (int r = 0; r < MAP_ROWS; r++) {
                                    for (int c = 0; c < MAP_COLS; c++) maze[r][c] = 0;
                                }
                                ladujPoziom(aktualnyLvl, boosters);
                                for (int r = 1; r < MAP_ROWS - 1; r++) {
                                    for (int c = 1; c < MAP_COLS - 1; c++) {
                                        if (maze[r][c] != 1) maze[r][c] = 2;
                                    }
                                }
                                Enemy boss(SCREEN_WIDTH / 2 - 30, SCREEN_HEIGHT / 2 - 30, 60, 1);
                                boss.setTexture(bossTexture);
                                boss.setSpeed(1);
                                enemies.push_back(boss);

                                int crownsToSpawn = 15;
                                while (crownsToSpawn > 0) {
                                    int rCol = (rand() % (MAP_COLS - 2)) + 1;
                                    int rRow = (rand() % (MAP_ROWS - 2)) + 1;
                                    if (maze[rRow][rCol] == 1) continue;
                                    int cx = rCol * TILE_SIZE + 10;
                                    int cy = rRow * TILE_SIZE + 10;
                                    SDL_Rect tempRect = { cx, cy, 50, 50 };
                                    SDL_Rect bossRect = boss.getRect();
                                    bool collision = false;
                                    if (SDL_HasIntersection(&tempRect, &bossRect)) collision = true;
                                    for (const auto& existingCrown : crowns) {
                                        if (SDL_HasIntersection(&tempRect, &existingCrown.rect)) {
                                            collision = true; break;
                                        }
                                    }
                                    if (!collision) {
                                        crowns.push_back({ tempRect, true });
                                        crownsToSpawn--;
                                    }
                                }
                                pozostalePunkty = 15;
                            }
                            else {
                                // Normal Level Setup
                                std::vector<BoosterType> types = { SPEED_UP, INVINCIBLE, SLOW_ENEMY, FREEZE };
                                std::random_device rd;
                                std::mt19937 g(rd());
                                std::shuffle(types.begin(), types.end(), g);
                                for (int k = 0; k < 4; k++) boosters.push_back({ boosterPositions[k], types[k], true });

                                int spawnX[] = { 990, 80, 990 };
                                int spawnY[] = { 80, 710, 710 };
                                for (int k = 0; k < 3; k++) {
                                    Enemy newEnemy(spawnX[k], spawnY[k], 50, 1);
                                    if (enemyTextures[aktualnyLvl][k]) newEnemy.setTexture(enemyTextures[aktualnyLvl][k]);
                                    enemies.push_back(newEnemy);
                                }
                                ladujPoziom(aktualnyLvl, boosters);
                                pozostalePunkty = liczPunkty();
                            }
                            aktualnePunkty = 0;
                            player.setPos(80, 80);
                            player.resetBoosters();
                        }
                        else if (e.key.keysym.sym == SDLK_ESCAPE) {
                            levelCompleteScreen = false;
                            menuActive = true;
                            SDL_StartTextInput();
                        }
                    }
                }
                // Ekran wygranej całej gry (po Bossie)
                else if (gameWonScreen) {
                    if (e.type == SDL_KEYDOWN) {
                        if (e.key.keysym.sym == SDLK_SPACE) {
                            endCredits[6] = playerName; // Wstawiamy imię gracza do napisów
                            gameWonScreen = false;
                            creditsActive = true;
                            creditsScroll = SCREEN_HEIGHT;
                        }
                        else if (e.key.keysym.sym == SDLK_ESCAPE) {
                            gameWonScreen = false;
                            menuActive = true;
                            SDL_StartTextInput();
                        }
                    }
                }
                // Ekran napisów końcowych
                else if (creditsActive) {
                    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                        creditsActive = false;
                        menuActive = true;
                        SDL_StartTextInput();
                    }
                }
                // Normalna rozgrywka (ruch gracza)
                else {
                    if (!gameOver && !levelCompleteScreen && !gameWonScreen) player.handleInput(e);

                    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                        menuActive = true;
                        SDL_StartTextInput();
                    }
                }
            }
        }

        // --- RYSOWANIE GRAFIKI (RENDEROWANIE) ---
        // Czyszczenie ekranu na czarno
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (menuActive) {
            // Rysowanie tła menu (kafelki trawy)
            if (wallTexture) {
                for (int y = 0; y < SCREEN_HEIGHT; y += 70) {
                    for (int x = 0; x < SCREEN_WIDTH; x += 70) {
                        SDL_Rect grassRect = { x, y, 70, 70 };
                        SDL_RenderCopy(renderer, wallTexture, NULL, &grassRect);
                    }
                }
            }
            // Rysowanie dużego traktora w tle menu
            if (tractorOptions.size() > 6 && tractorOptions[6].texture) {
                SDL_SetTextureColorMod(tractorOptions[6].texture, 255, 255, 255);
                int bigSize = 800;
                SDL_Rect bigTractorRect;
                bigTractorRect.w = bigSize;
                bigTractorRect.h = bigSize;
                bigTractorRect.x = (SCREEN_WIDTH - bigSize) / 2;
                bigTractorRect.y = (SCREEN_HEIGHT - bigSize) / 2;
                SDL_RenderCopy(renderer, tractorOptions[6].texture, NULL, &bigTractorRect);
            }

            // Przyciemnienie tła (półprzezroczysty czarny prostokąt)
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
            SDL_Rect fullscreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
            SDL_RenderFillRect(renderer, &fullscreen);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            // Rysowanie tytułu gry
            SDL_Color titleColor = { 255, 215, 0 };
            SDL_Surface* titleSurface = TTF_RenderText_Solid(fontTitle, "TRAKTORZYSTA", titleColor);
            if (titleSurface) {
                SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
                SDL_Rect titleRect = { 10, 0, SCREEN_WIDTH - 60, 140 };
                SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
                SDL_FreeSurface(titleSurface);
                SDL_DestroyTexture(titleTexture);
            }

            // Nagroda za przejście gry (korona w menu)
            if (odblokowaneLevele[0] && odblokowaneLevele[1] && odblokowaneLevele[2] && odblokowaneLevele[3]) {
                if (crownTexture) {
                    SDL_Rect awardRect = { 80, 150, 80, 80 };
                    SDL_RenderCopy(renderer, crownTexture, NULL, &awardRect);
                    rysujTekst(renderer, fontUI, "MISTRZ POLA!", 60, 240, { 255, 215, 0 });
                }
            }

            // Pola menu (imię, wybór koloru, poziomy)
            SDL_Color textColor = { 255, 255, 255 };
            rysujTekstWycentrowany(renderer, fontUI, "Wpisz imie traktorzysty:", 180, textColor);

            // Rysowanie pola wpisywania tekstu
            SDL_Rect inputRect = { (SCREEN_WIDTH - 300) / 2, 210, 300, 40 };
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &inputRect);
            rysujTekstWycentrowany(renderer, fontUI, playerName + (SDL_GetTicks() % 1000 < 500 ? "|" : ""), 220, textColor);

            // Podgląd wybranego traktora (lewa strona)
            rysujTekst(renderer, fontUI, "Wyglad:", previewRect.x, previewRect.y - 25, { 255, 255, 255 });
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &previewRect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &previewRect);
            if (tractorOptions[selectedColorIndex].texture) {
                SDL_SetTextureColorMod(tractorOptions[selectedColorIndex].texture, 255, 255, 255);
                SDL_RenderCopy(renderer, tractorOptions[selectedColorIndex].texture, NULL, &previewRect);
            }

            // Siatka kolorów do wyboru (prawa strona)
            rysujTekst(renderer, fontUI, "Wybierz kolor:", gridStartX, gridStartY - 25, { 255, 255, 255 });
            for (int i = 0; i < 8; i++) {
                SDL_SetRenderDrawColor(renderer, tractorOptions[i].colorRGB.r, tractorOptions[i].colorRGB.g, tractorOptions[i].colorRGB.b, 255);
                SDL_RenderFillRect(renderer, &colorButtons[i]);

                // Rysowanie ramki (biała dla wybranego, czarna dla reszty)
                if (i == selectedColorIndex) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_Rect border = colorButtons[i];
                    SDL_RenderDrawRect(renderer, &border);
                    border.x++; border.y++; border.w -= 2; border.h -= 2;
                    SDL_RenderDrawRect(renderer, &border);
                }
                else {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderDrawRect(renderer, &colorButtons[i]);
                }
            }

            // Rysowanie przycisków poziomów
            for (int i = 0; i < 3; i++) {
                SDL_Texture* currentLvlTex = nullptr;
                if (i == 0) currentLvlTex = lvl1Tex;
                else if (i == 1) currentLvlTex = lvl2Tex;
                else if (i == 2) currentLvlTex = bossLvlTex;

                if (currentLvlTex) {
                    // Przyciemnij przycisk, jeśli poziom jest zablokowany
                    if (odblokowaneLevele[i]) SDL_SetTextureColorMod(currentLvlTex, 255, 255, 255);
                    else SDL_SetTextureColorMod(currentLvlTex, 60, 60, 60);
                    SDL_RenderCopy(renderer, currentLvlTex, NULL, &przyciskiMenu[i]);
                }
                else {
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                    SDL_RenderFillRect(renderer, &przyciskiMenu[i]);
                }
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &przyciskiMenu[i]);
            }
        }
        else {
            // --- TRYB GRY (ROZGRYWKA) ---
            if (!gameOver && !levelCompleteScreen && !gameWonScreen && !creditsActive) {

                // Sprawdzanie zbierania punktów
                if (aktualnyLvl == 2) {
                    // Logika Bossa: Zbieranie koron
                    SDL_Rect pRect = player.getRect();
                    for (auto& crown : crowns) {
                        if (crown.active && SDL_HasIntersection(&pRect, &crown.rect)) {
                            crown.active = false;
                            aktualnePunkty += 50;
                            pozostalePunkty--;
                        }
                    }
                }
                else {
                    // Logika Zwykła: Koszenie zboża
                    int pCol = (player.getX() + 25) / TILE_SIZE;
                    int pRow = (player.getY() + 25) / TILE_SIZE;
                    if (maze[pRow][pCol] == 0) {
                        maze[pRow][pCol] = 2; // Zmiana na skoszone
                        aktualnePunkty += 10;
                        pozostalePunkty--;
                    }
                }

                // Sprawdzenie warunku zwycięstwa (czy zebrano wszystko)
                if (pozostalePunkty <= 0) {
                    // Odblokowanie kolejnego poziomu i zapis postępu
                    if (aktualnyLvl + 1 < LICZBA_LEVELI) {
                        if (!odblokowaneLevele[aktualnyLvl + 1]) {
                            odblokowaneLevele[aktualnyLvl + 1] = true;
                            zapiszPostep(odblokowaneLevele, 4);
                        }
                    }
                    else if (aktualnyLvl == 2) {
                        if (!odblokowaneLevele[3]) {
                            odblokowaneLevele[3] = true; // Zaznaczamy ukończenie całej gry
                            zapiszPostep(odblokowaneLevele, 4);
                        }
                    }

                    // Włączanie odpowiedniego ekranu końcowego
                    if (aktualnyLvl == 2) gameWonScreen = true;
                    else levelCompleteScreen = true;
                }

                // Aktualizacja gracza (ruch, boostery)
                player.update();

                // --- MECHANIKA BOSSA ---
                if (aktualnyLvl == 2) {
                    int currentSpawnRate = 1200; // Czas między spadającymi belami
                    int bossSpeed = 1;

                    // Zwiększanie trudności im mniej punktów zostało
                    if (!enemies.empty()) {
                        if (pozostalePunkty <= 5) { currentSpawnRate = 600; bossSpeed = 2; }
                        else if (pozostalePunkty <= 10) { currentSpawnRate = 900; bossSpeed = 1; }
                        else { currentSpawnRate = 1200; bossSpeed = 1; }
                        enemies[0].setSpeed(bossSpeed);
                    }

                    // Generowanie spadającego siana
                    if (SDL_GetTicks() > haySpawnTimer) {
                        HayBale bale;
                        bale.rect = { (rand() % (MAP_COLS - 2) + 1) * TILE_SIZE, -50, 50, 50 };
                        bale.y = -50;
                        int baseHaySpeed = (pozostalePunkty <= 5) ? 3 : 2;
                        bale.speed = (rand() % 3) + baseHaySpeed;
                        hayBales.push_back(bale);
                        haySpawnTimer = SDL_GetTicks() + currentSpawnRate;
                    }

                    // Obsługa ruchu siana i kolizji z graczem
                    for (size_t i = 0; i < hayBales.size(); ) {
                        hayBales[i].y += hayBales[i].speed;
                        hayBales[i].rect.y = (int)hayBales[i].y;

                        SDL_Rect pRect = player.getRect();
                        SDL_Rect hitBox = { pRect.x + 15, pRect.y + 15, pRect.w - 30, pRect.h - 30 }; // Mniejszy hitbox

                        if (SDL_HasIntersection(&hitBox, &hayBales[i].rect) && !player.isInvincible()) {
                            gameOver = true;
                        }

                        // Usuwanie siana, które wyleciało za ekran
                        if (hayBales[i].rect.y > SCREEN_HEIGHT) hayBales.erase(hayBales.begin() + i);
                        else i++;
                    }
                }

                // Ruch wrogów (AI)
                for (auto& enemy : enemies) {
                    enemy.update(player.getX(), player.getY());
                }

                SDL_Rect pRect = player.getRect();

                // Obsługa boosterów (tylko na zwykłych poziomach)
                if (aktualnyLvl != 2) {
                    for (auto& b : boosters) {
                        if (b.active && SDL_HasIntersection(&pRect, &b.rect)) {
                            // Aktywacja odpowiedniego efektu
                            if (b.type == FREEZE) { for (auto& enemy : enemies) enemy.freeze(); }
                            else if (b.type == SLOW_ENEMY) { for (auto& enemy : enemies) enemy.slowDown(); }
                            else player.applyBooster(b.type);
                            b.active = false; // Usunięcie boostera z mapy
                        }
                    }
                }

                // Kolizja gracza z wrogiem (Game Over)
                for (auto& enemy : enemies) {
                    SDL_Rect eRect = enemy.getRect();
                    SDL_Rect hitBox = { pRect.x + 10, pRect.y + 10, pRect.w - 20, pRect.h - 20 };
                    if (SDL_HasIntersection(&hitBox, &eRect) && !player.isInvincible()) {
                        gameOver = true;
                    }
                }
            }

            // Rysowanie tła gry (zielone)
            SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
            SDL_RenderClear(renderer);

            // Rysowanie mapy (ściany, zboże, ziemia)
            for (int r = 0; r < MAP_ROWS; r++) {
                for (int c = 0; c < MAP_COLS; c++) {
                    SDL_Rect tileRect = { c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                    if (maze[r][c] == 1) {
                        if (wallTexture) SDL_RenderCopy(renderer, wallTexture, NULL, &tileRect);
                        else { // Zapasowy kolor ściany
                            SDL_SetRenderDrawColor(renderer, 210, 180, 140, 255);
                            SDL_RenderFillRect(renderer, &tileRect);
                        }
                    }
                    else if (maze[r][c] == 0) {
                        if (cropTexture) SDL_RenderCopy(renderer, cropTexture, NULL, &tileRect);
                        else { // Zapasowa kropka (zboże)
                            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                            SDL_Rect dot = { c * TILE_SIZE + 32, r * TILE_SIZE + 32, 6, 6 };
                            SDL_RenderFillRect(renderer, &dot);
                        }
                    }
                    else if (maze[r][c] == 2) {
                        if (stubbleTexture) SDL_RenderCopy(renderer, stubbleTexture, NULL, &tileRect);
                        else { // Zapasowy kolor ziemi
                            SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
                            SDL_RenderFillRect(renderer, &tileRect);
                        }
                    }
                }
            }

            // Rysowanie elementów interaktywnych
            if (aktualnyLvl == 2) {
                // Boss level: Korony i Siano
                for (auto& c : crowns) if (c.active) SDL_RenderCopy(renderer, crownTexture, NULL, &c.rect);
                for (auto& h : hayBales) SDL_RenderCopy(renderer, hayTexture, NULL, &h.rect);
            }
            else {
                // Zwykły level: Boostery
                for (auto& b : boosters) {
                    if (b.active) {
                        if (b.type == INVINCIBLE && shieldTexture) SDL_RenderCopy(renderer, shieldTexture, NULL, &b.rect);
                        else if (b.type == SPEED_UP && speedTexture) SDL_RenderCopy(renderer, speedTexture, NULL, &b.rect);
                        else if (b.type == SLOW_ENEMY && slowTexture) SDL_RenderCopy(renderer, slowTexture, NULL, &b.rect);
                        else if (b.type == FREEZE && freezeTexture) SDL_RenderCopy(renderer, freezeTexture, NULL, &b.rect);
                        else {
                            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                            SDL_RenderFillRect(renderer, &b.rect);
                        }
                    }
                }
            }

            // Rysowanie postaci
            player.draw(renderer);
            for (auto& enemy : enemies) {
                enemy.draw(renderer);
            }

            // --- HUD (INTERFEJS UŻYTKOWNIKA) ---
            // Wyświetlanie imienia i punktów
            std::string celTxt = (aktualnyLvl == 2) ? " | KORONY: " : " | PKT: ";
            std::string celVal = (aktualnyLvl == 2) ? std::to_string(pozostalePunkty) : std::to_string(aktualnePunkty);
            std::string hudTop = playerName + celTxt + celVal;

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
            SDL_Rect bgRect = { 5, 5, (int)hudTop.length() * 12 + 20, 30 };
            SDL_RenderFillRect(renderer, &bgRect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            rysujTekst(renderer, fontUI, hudTop, 10, 10, { 255, 255, 255 });

            // Instrukcja w prawym górnym rogu
            std::string instrText;
            if (aktualnyLvl == 2) instrText = "Zbierz wszystkie korony i nie daj sie zlapac!";
            else instrText = "Zbierz cala pszenice i nie daj sie zlapac!";

            int iW, iH;
            TTF_SizeText(fontUI, instrText.c_str(), &iW, &iH);
            int instrX = SCREEN_WIDTH - iW - 15;

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
            SDL_Rect instrBgRect = { instrX - 5, 5, iW + 10, 30 };
            SDL_RenderFillRect(renderer, &instrBgRect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            rysujTekst(renderer, fontUI, instrText, instrX, 10, { 255, 255, 255 });

            // Wyświetlanie aktywnego boostera na dole ekranu
            std::string boosterText = "";
            SDL_Texture* activeIcon = nullptr;
            float timeRem = 0;

            if (player.isInvincible()) {
                activeIcon = shieldTexture;
                boosterText = "TARCZA";
                timeRem = player.getInvincibleRemainingTime();
            }
            else if (player.isSpeedUp()) {
                activeIcon = speedTexture;
                boosterText = "TURBO";
                timeRem = player.getSpeedRemainingTime();
            }
            else if (!enemies.empty() && enemies[0].isFrozen()) {
                activeIcon = freezeTexture;
                boosterText = "MROZ";
                timeRem = enemies[0].getFreezeRemainingTime();
            }
            else if (!enemies.empty() && enemies[0].isSlowed()) {
                activeIcon = slowTexture;
                boosterText = "BLOTO";
                timeRem = enemies[0].getSlowRemainingTime();
            }

            if (timeRem > 0) {
                if (activeIcon) {
                    SDL_Rect iconRect = { 20, SCREEN_HEIGHT - 60, 40, 40 };
                    SDL_RenderCopy(renderer, activeIcon, NULL, &iconRect);
                }
                std::stringstream ss;
                ss << boosterText << " " << std::fixed << std::setprecision(1) << timeRem << "s";
                rysujTekst(renderer, fontUI, ss.str(), 70, SCREEN_HEIGHT - 50, { 255, 255, 0 });
            }

            // Ekran Game Over (Czerwony)
            if (gameOver) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150);
                SDL_Rect fullScreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
                SDL_RenderFillRect(renderer, &fullScreen);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                rysujTekstWycentrowany(renderer, fontTitle, "KONIEC GRY", SCREEN_HEIGHT / 2 - 50, { 255, 255, 255 });
                rysujTekstWycentrowany(renderer, fontUI, "Nacisnij ESC aby wrocic do menu", SCREEN_HEIGHT / 2 + 20, { 255, 255, 255 });
            }

            // Ekran ukończenia poziomu (Przejściowy)
            if (levelCompleteScreen) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
                SDL_Rect fullscreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
                SDL_RenderFillRect(renderer, &fullscreen);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                rysujTekstWycentrowany(renderer, fontTitle, "POZIOM UKONCZONY!", 200, { 0, 255, 0 });
                rysujTekstWycentrowany(renderer, fontUI, "Brawo! Odblokowales kolejny etap.", 300, { 255, 255, 255 });

                if (SDL_GetTicks() % 1000 < 600) { // Efekt migania tekstu
                    rysujTekstWycentrowany(renderer, fontUI, "SPACJA - Jedziesz dalej", 500, { 255, 255, 0 });
                }
                rysujTekstWycentrowany(renderer, fontUI, "ESC - Powrot do Menu", 550, { 200, 200, 200 });
            }
            // Ekran zwycięstwa (Finał)
            else if (gameWonScreen) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
                SDL_Rect fullscreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
                SDL_RenderFillRect(renderer, &fullscreen);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                rysujTekstWycentrowany(renderer, fontTitle, "GRATULACJE!", 100, { 255, 215, 0 });
                rysujTekstWycentrowany(renderer, fontUI, "Pokonales baby i soltysa!", 250, { 255, 255, 255 });

                std::string msg = "Traktorzysta " + playerName + " moze bezpiecznie pracowac!";
                rysujTekstWycentrowany(renderer, fontUI, msg, 450, { 200, 0, 0 });

                rysujTekstWycentrowany(renderer, fontUI, "SPACJA - Zobacz Napisy Koncowe", 600, { 255, 255, 255 });
                rysujTekstWycentrowany(renderer, fontUI, "ESC - Wroc do Menu", 650, { 100, 100, 100 });

                if (bossTexture) {
                    SDL_Rect r = { SCREEN_WIDTH / 2 - 50, 300, 100, 100 };
                    SDL_RenderCopy(renderer, bossTexture, NULL, &r);
                }
            }
            // Ekran napisów końcowych (Credits)
            else if (creditsActive) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);

                creditsScroll -= 1.5; // Przesuwanie tekstu w górę

                // Powrót do menu po zakończeniu napisów
                if (creditsScroll < -((int)endCredits.size() * 50 + 400)) {
                    creditsActive = false;
                    menuActive = true;
                    SDL_StartTextInput();
                }

                // Rysowanie linijek tekstu
                for (size_t i = 0; i < endCredits.size(); i++) {
                    float y = creditsScroll + i * 50;
                    if (y > -50 && y < SCREEN_HEIGHT + 50) { // Optymalizacja: rysuj tylko widoczne
                        SDL_Color kol = { 255, 255, 255 };
                        if (endCredits[i].length() > 3 && endCredits[i].substr(0, 3) == "---") kol = { 255, 215, 0 }; // Nagłówki na żółto
                        rysujTekstWycentrowany(renderer, fontUI, endCredits[i], (int)y, kol);
                    }
                }

                // Ozdobne obrazki po bokach
                SDL_Rect leftImg = { 50, SCREEN_HEIGHT / 2 - 75, 150, 150 };
                SDL_Rect rightImg = { SCREEN_WIDTH - 200, SCREEN_HEIGHT / 2 - 75, 150, 150 };

                if (enemyTextures[0][0]) SDL_RenderCopy(renderer, enemyTextures[0][0], NULL, &leftImg);
                if (crownTexture) SDL_RenderCopy(renderer, crownTexture, NULL, &rightImg);
            }
        }
        SDL_RenderPresent(renderer); // Wyświetlenie narysowanej klatki
        SDL_Delay(16); // Opóźnienie dla stabilnych ~60 FPS
    }

    // --- SPRZĄTANIE PAMIĘCI (ZAMYKANIE PROGRAMU) ---
    // Należy zwolnić wszystkie zasoby, aby uniknąć wycieków pamięci
    SDL_DestroyTexture(shieldTexture);
    SDL_DestroyTexture(speedTexture);
    SDL_DestroyTexture(slowTexture);
    SDL_DestroyTexture(freezeTexture);

    SDL_DestroyTexture(wallTexture);
    SDL_DestroyTexture(cropTexture);
    SDL_DestroyTexture(stubbleTexture);

    SDL_DestroyTexture(hayTexture);
    SDL_DestroyTexture(bossTexture);
    SDL_DestroyTexture(crownTexture);

    for (auto& opt : tractorOptions) {
        if (opt.texture) SDL_DestroyTexture(opt.texture);
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            SDL_DestroyTexture(enemyTextures[i][j]);
        }
    }
    SDL_DestroyTexture(lvl1Tex);
    SDL_DestroyTexture(lvl2Tex);
    SDL_DestroyTexture(bossLvlTex);

    TTF_CloseFont(fontUI);
    TTF_CloseFont(fontTitle);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}