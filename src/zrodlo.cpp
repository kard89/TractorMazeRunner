#include <SDL.h>
#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>
#include <map>
#include <fstream>  // Do obsługi pliku rekord.txt
#include <string>   // Do zamiany liczb na tekst w tytule okna (teraz używane w HUD)
#include <iomanip>
#include <sstream>
#include <SDL_ttf.h>
#include <random> // Potrzebne do tasowania boosterów

const int SCREEN_WIDTH = 1120; //Szerokosc okna (16 * 70)
const int SCREEN_HEIGHT = 840; //Wysokosc okna (12 * 70)
const int TILE_SIZE = 70; // (Powiększone z 50)
const int MAP_ROWS = 12;
const int MAP_COLS = 16;
const int LICZBA_LEVELI = 3; // Liczba dostepnych map

// --- STRUKTURY DLA BOSS LEVELU (DODANE) ---
struct HayBale {
    SDL_Rect rect;
    float y;
    float speed;
};
struct Crown {
    SDL_Rect rect;
    bool active;
};
// -------------------------------------------

// --- NOWA STRUKTURA DO WYBORU KOLORU (DODANE DLA MENU) ---
struct TractorOption {
    std::string filename;
    SDL_Color colorRGB; // Kolor przycisku w menu
    SDL_Texture* texture;
};
// ---------------------------------------------------------

//Struktura Boosterow
enum BoosterType { SPEED_UP, INVINCIBLE, SLOW_ENEMY, FREEZE, NONE };
struct Booster {
    SDL_Rect rect;
    BoosterType type;
    bool active;
};

// Tablica przechowująca wzory labiryntów
int mazeLevels[LICZBA_LEVELI][MAP_ROWS][MAP_COLS] = {
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, // 1. Gra (same ciany)
        {1,0,0,0,1,0,0,0,0,0,0,0,1,0,0,1}, // 2. Przejcie
        {1,0,1,0,1,0,1,1,0,1,1,0,1,0,1,1}, // 3.
        {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1}, // 4.
        {1,0,1,1,1,0,1,1,0,1,1,1,1,1,0,1}, // 5.
        {1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,1}, // 6. rodek
        {1,1,1,1,1,1,0,1,1,1,1,1,0,0,0,1}, // 7.
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // 8.
        {1,0,1,0,1,1,0,1,1,1,1,0,1,1,0,1}, // 9.
        {1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1}, // 10.
        {1,0,0,0,1,1,1,1,0,1,1,1,0,0,0,1}, // 11.
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}  // 12. D (same ciany)
    },
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, // Nowy labirynt (Poziom 2)
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
{    // POZIOM 3 (BOSS) - Arena z przeszkodami
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, // Ściana górna
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // Puste
        {1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1}, // Przeszkoda LEWA GÓRA
        {1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1}, // Przeszkoda LEWA GÓRA
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // Puste
        {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1}, // Przeszkoda ŚRODEK (Twoja)
        {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1}, // Przeszkoda ŚRODEK (Twoja)
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // Puste
        {1,0,0,1,1,0,0,0,0,0,0,0,1,1,0,1}, // Przeszkoda PRAWA DÓŁ
        {1,0,0,1,1,0,0,0,0,0,0,0,1,1,0,1}, // Przeszkoda PRAWA DÓŁ
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // Puste
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}  // Ściana dolna
    }
};

int maze[MAP_ROWS][MAP_COLS]; // Aktualna mapa w grze

// Funkcja do ladowania konkretnego poziomu
void ladujPoziom(int nr, std::vector<Booster>& bst) {
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            maze[r][c] = mazeLevels[nr][r][c];
        }
    }
    for (auto& b : bst) b.active = true;
}

// Funkcja liczaca kropki na mapie
int liczPunkty() {
    int p = 0;
    for (int r = 0; r < MAP_ROWS; r++)
        for (int c = 0; c < MAP_COLS; c++)
            if (maze[r][c] == 0) p++;
    return p;
}

// Funkcja pomocnicza do rysowania tekstu
void rysujTekst(SDL_Renderer* renderer, TTF_Font* font, std::string tekst, int x, int y, SDL_Color kolor) {
    if (tekst.empty()) return;
    SDL_Surface* surface = TTF_RenderText_Solid(font, tekst.c_str(), kolor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = { x, y, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

// Funkcja centrująca tekst w poziomie
void rysujTekstWycentrowany(SDL_Renderer* renderer, TTF_Font* font, std::string tekst, int y, SDL_Color kolor) {
    if (tekst.empty()) return;
    int w, h;
    TTF_SizeText(font, tekst.c_str(), &w, &h);
    rysujTekst(renderer, font, tekst, (SCREEN_WIDTH - w) / 2, y, kolor);
}

class Player {
private:
    SDL_Rect rect; //struktura do przechowywania polozenia
    int baseSpeed;
    int currentSpeed;
    bool invincible = false;
    Uint32 invincibleTimer = 0;//rozdzielone timery dla boosterow
    Uint32 speedTimer = 0;
    SDL_Texture* texture = nullptr; // Dodana tekstura gracza
public://Ustawianie pozycji startowej i predkosci
    Player(int x, int y, int size, int moveSpeed) {
        rect.x = x;
        rect.y = y;
        rect.w = size;
        rect.h = size;
        baseSpeed = moveSpeed;
        currentSpeed = moveSpeed;
    }
    void setPos(int x, int y) { rect.x = x; rect.y = y; }
    void setTexture(SDL_Texture* tex) { texture = tex; } // Setter tekstury
    void applyBooster(BoosterType type) {
        Uint32 now = SDL_GetTicks();
        if (type == SPEED_UP) {
            currentSpeed = baseSpeed * 2;
            speedTimer = now + 5000;
        }
        else if (type == INVINCIBLE) {
            //void setPos(int x, int y) { rect.x = x; rect.y = y; }
            invincible = true;
            invincibleTimer = now + 5000;
        }
    }
    void handleInput(SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            int oldX = rect.x;
            int oldY = rect.y;
            switch (e.key.keysym.sym) {
            case SDLK_UP: rect.y -= currentSpeed; break;
            case SDLK_DOWN: rect.y += currentSpeed; break;
            case SDLK_LEFT: rect.x -= currentSpeed; break;
            case SDLK_RIGHT: rect.x += currentSpeed; break;
            }
            for (int r = 0; r < MAP_ROWS; r++) {
                for (int c = 0; c < MAP_COLS; c++) {
                    if (maze[r][c] == 1) { // Jeli to pole jest cian
                        SDL_Rect wall = { c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE };

                        // Funkcja SDL_HasIntersection sprawdza, czy dwa kwadraty na siebie nachodz
                        if (SDL_HasIntersection(&rect, &wall)) {
                            // Jeli nastpia kolizja, cofnij gracza do starej pozycji
                            rect.x = oldX;
                            rect.y = oldY;
                        }
                    }
                }
            }
        }
    }
    void update() { //Punkt kontrolny czy nasz ,,traktor" nie wyjezdza poza wymiar ekranu
        Uint32 now = SDL_GetTicks();
        if (now > speedTimer) currentSpeed = baseSpeed;
        if (now > invincibleTimer) invincible = false;

        if (rect.x < 0) rect.x = 0;
        if (rect.y < 0) rect.y = 0;
        if (rect.x + rect.w > SCREEN_WIDTH) rect.x = SCREEN_WIDTH - rect.w;
        if (rect.y + rect.h > SCREEN_HEIGHT) rect.y = SCREEN_HEIGHT - rect.h;
    }
    void draw(SDL_Renderer* renderer) { //rysowanie gracza w oknie
        if (texture) {
            // Analogiczne zmiany kolorow dla tekstury (Color Mod)
            // USUNIĘTE ZMIANY KOLORÓW DLA BOOSTERA (zgodnie z prośbą)
            // if(invincible) SDL_SetTextureColorMod(texture, 255, 255, 100); 
            // else if(currentSpeed > baseSpeed) SDL_SetTextureColorMod(texture, 255, 150, 150); 
            // else SDL_SetTextureColorMod(texture, 255, 255, 255); // Normalny

            // Upewniamy się, że kolor jest standardowy (brak tintu)
            SDL_SetTextureColorMod(texture, 255, 255, 255);

            SDL_RenderCopy(renderer, texture, NULL, &rect);
        }
        else {
            // Stary kod rysowania kwadratu (backup)
            if (invincible) SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Kolor żółty jeśli nieśmiertelny
            else SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);//ustawienie koloru na bialy
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    //Osobne funkcje czasow dla boosterow
    float getInvincibleRemainingTime() {
        if (SDL_GetTicks() >= invincibleTimer) return 0;
        return(invincibleTimer - SDL_GetTicks()) / 1000.0f;
    }
    float getSpeedRemainingTime() {
        if (SDL_GetTicks() >= speedTimer) return 0;
        return(speedTimer - SDL_GetTicks()) / 1000.0f;
    }
    //Funkcja umozliwaja poznanie antagnoiscie pozycje gracza
    int getX() { return rect.x; }
    int getY() { return rect.y; }
    SDL_Rect getRect() { return rect; }
    bool isInvincible() { return invincible; }
    bool isSpeedUp() { return currentSpeed > baseSpeed; }

    void resetBoosters() {
        currentSpeed = baseSpeed;
        invincible = false;
        invincibleTimer = 0;
        speedTimer = 0;
    }
};

class Enemy {
private:
    SDL_Rect rect;
    int baseSpeed; // Przechowujemy bazową prędkość
    int currentSpeed;
    bool frozen = false;
    bool slowed = false; // Flaga spowolnienia (Błoto)
    Uint32 freezeTimer = 0;
    Uint32 slowTimer = 0;
    SDL_Texture* texture = nullptr; // Dodana tekstura wroga
    std::vector<SDL_Point> path; // Przechowuje list punktw do przejcia

    // Licznik dla pathfindingu i teraz również dla spowalniania ruchu
    int pathTimer;

    // Funkcja obliczajca drog (Uproszczony A*)
    void findPath(int startX, int startY, int targetX, int targetY) {
        path.clear();
        int sCol = startX / TILE_SIZE;
        int sRow = startY / TILE_SIZE;
        int tCol = targetX / TILE_SIZE;
        int tRow = targetY / TILE_SIZE;

        if (sCol == tCol && sRow == tRow) return;

        // Prosty algorytm zalewowy (BFS), ktry wyznaczy tras w labiryncie
        std::queue<SDL_Point> q;
        q.push({ sCol, sRow });

        std::map<int, SDL_Point> parentMap;
        bool visited[MAP_ROWS][MAP_COLS] = { false };
        visited[sRow][sCol] = true;

        bool found = false;
        while (!q.empty()) {
            SDL_Point curr = q.front(); q.pop();
            if (curr.x == tCol && curr.y == tRow) { found = true; break; }

            int dx[] = { 0, 0, 1, -1 };
            int dy[] = { 1, -1, 0, 0 };
            for (int i = 0; i < 4; i++) {
                int nx = curr.x + dx[i], ny = curr.y + dy[i];
                // Wrog moze chodzic po nieskoszonym (0) i skoszonym (2)
                if (nx >= 0 && nx < MAP_COLS && ny >= 0 && ny < MAP_ROWS && (maze[ny][nx] == 0 || maze[ny][nx] == 2) && !visited[ny][nx]) {
                    visited[ny][nx] = true;
                    parentMap[ny * MAP_COLS + nx] = curr;
                    q.push({ nx, ny });
                }
            }
        }

        if (found) {
            SDL_Point curr = { tCol, tRow };
            while (curr.x != sCol || curr.y != sRow) {
                // (70 - 50) / 2 = 10, offset jest nadal poprawny
                path.push_back({ curr.x * TILE_SIZE + 10, curr.y * TILE_SIZE + 10 });
                curr = parentMap[curr.y * MAP_COLS + curr.x];
            }
            std::reverse(path.begin(), path.end());
        }
    }

public:
    Enemy(int x, int y, int size, int moveSpeed) {
        rect = { x, y, size, size };
        baseSpeed = moveSpeed;
        currentSpeed = moveSpeed;
        // Inicjalizujemy licznik losową wartością
        pathTimer = rand() % 30;
    }
    void setPos(int x, int y) { rect.x = x; rect.y = y; path.clear(); }
    void setTexture(SDL_Texture* tex) { texture = tex; } // Setter tekstury
    SDL_Rect getRect() { return rect; }

    void freeze() {
        frozen = true;
        slowed = false;
        freezeTimer = SDL_GetTicks() + 5000;
    }

    // Nowa funkcja do spowalniania (Błoto)
    void slowDown() {
        slowed = true;
        frozen = false;
        slowTimer = SDL_GetTicks() + 5000;
        // USUNIĘTO: currentSpeed = baseSpeed / 2; (To powodowało 0)
    }

    // Dodana metoda do zmiany prędkości (dla bossa)
    void setSpeed(int s) { baseSpeed = s; currentSpeed = s; }

    void update(int playerX, int playerY) {
        // Obsługa timerów efektów
        Uint32 now = SDL_GetTicks();
        if (frozen && now > freezeTimer) frozen = false;
        if (slowed && now > slowTimer) {
            slowed = false;
        }

        if (frozen) return;

        // Obliczaj now ciek co 30 klatek
        pathTimer++;
        if (pathTimer % 30 == 0) {
            findPath(rect.x, rect.y, playerX, playerY);
        }

        // --- NAPRAWA LOGIKI RUCHU ---
        bool shouldMove = true;

        if (slowed) {
            // Jeśli wróg jest w błocie, ruszamy się tylko w parzystych klatkach
            // Dzięki temu przy prędkości 1, efektywna prędkość to 0.5
            if (pathTimer % 2 != 0) shouldMove = false;
        }

        if (shouldMove && !path.empty()) {
            SDL_Point target = path[0];

            // Używamy baseSpeed (które wynosi 1), ale dzięki 'shouldMove'
            // ruch wykonuje się rzadziej, dając efekt spowolnienia.
            int speed = baseSpeed;

            if (rect.x < target.x) rect.x += speed;
            else if (rect.x > target.x) rect.x -= speed;
            if (rect.y < target.y) rect.y += speed;
            else if (rect.y > target.y) rect.y -= speed;

            // Jeli dotar do punktu kontrolnego cieki, usu go i id do nastpnego
            if (abs(rect.x - target.x) < speed + 1 && abs(rect.y - target.y) < speed + 1) {
                path.erase(path.begin());
            }
        }
    }
    //Osobne funkcje czasu dla boosterow
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
            // Zawsze normalny kolor
            SDL_SetTextureColorMod(texture, 255, 255, 255);
            SDL_RenderCopy(renderer, texture, NULL, &rect);
        }
        else {
            // Stary kod rysowania (backup)
            if (frozen) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
            else if (slowed) SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
            else SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    bool isFrozen() { return frozen; }
    bool isSlowed() { return slowed; }
    void resetStatus() {
        frozen = false;
        slowed = false;
        freezeTimer = 0;
        slowTimer = 0;
        currentSpeed = baseSpeed;
        path.clear();
    }
};


void zapiszPostep(bool odblokowane[], int rozmiar) {
    std::ofstream plik("postep.txt");
    if (plik.is_open()) {
        for (int i = 0; i < rozmiar; i++) {
            plik << odblokowane[i] << " ";
        }
        plik.close();
    }
}

void wczytajPostep(bool odblokowane[], int rozmiar) {
    std::ifstream plik("postep.txt");
    if (plik.is_open()) {
        for (int i = 0; i < rozmiar; i++) {
            plik >> odblokowane[i];
        }
        plik.close();
    }
    else {
        // Jeśli plik nie istnieje, ustaw domyślnie tylko 1. poziom
        odblokowane[0] = true;
        for (int i = 1; i < rozmiar; i++) odblokowane[i] = false;
    }
}

// Funkcja pomocnicza do ładowania tekstur (zachowująca czystość kodu w main)
SDL_Texture* wczytajTeksture(SDL_Renderer* renderer, const char* path, bool colorKey = false) {
    SDL_Surface* tempSurface = SDL_LoadBMP(path);
    if (!tempSurface) {
        std::cout << "Nie udalo sie wczytac " << path << "! Blad: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    if (colorKey) {
        // --- NAPRAWA GRAFIKI 252,0,252 -> 255,0,255 ---
        // Zanim ustawimy klucz koloru, zamieniamy wszystkie piksele (252,0,252) na (255,0,255)
        // Musimy zablokować powierzchnię, aby dostać się do pikseli
        SDL_LockSurface(tempSurface);

        int bpp = tempSurface->format->BytesPerPixel;
        for (int y = 0; y < tempSurface->h; y++) {
            for (int x = 0; x < tempSurface->w; x++) {
                // Wskaźnik do konkretnego piksela
                Uint8* p = (Uint8*)tempSurface->pixels + y * tempSurface->pitch + x * bpp;
                Uint32 pixelValue = 0;

                // Pobranie wartości piksela w zależności od formatu (24 lub 32 bity)
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

                // JEŚLI KOLOR TO TEN BŁĘDNY (252, 0, 252)
                if ((r == 252 && g == 0 && b == 252) || (r == 225 && g == 0 && b == 255)) {
                    // Zamień go na idealną Magentę (255, 0, 255)
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

        // 2. USTAWIANIE PRZEZROCZYSTOŚCI (Usuwamy Magentę: 255, 0, 255)
        // Teraz, gdy usunęliśmy 252,0,252, ten klucz usunie oba kolory (bo oba są teraz 255,0,255)
        SDL_SetColorKey(tempSurface, SDL_TRUE, SDL_MapRGB(tempSurface->format, 255, 0, 255));
    }
    // 3. TWORZENIE TEKSTURY
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_FreeSurface(tempSurface);
    return tex;
}

int main(int argc, char* argv[]) {
    // Inicjalizacja
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Tworzenie okna
    SDL_Window* window = SDL_CreateWindow("Traktorzysta", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    //Renderowanie obrazkow
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // VSync dla płynności

    if (!window) {
        std::cout << "Window Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 1. INICJALIZACJA (przed pętlą while)
    if (TTF_Init() == -1) {
        printf("Błąd TTF_Init: %s\n", TTF_GetError());
        return 1;
    }

    // 2. ŁADOWANIE CZCIONKI
    // Upewnij się, że plik "arial.ttf" jest w folderze z plikiem .exe!
    // 24 to rozmiar czcionki (tu zmienione na couree.fon)
    TTF_Font* fontUI = TTF_OpenFont("assets\\couree.fon", 20); // Mniejsza dla UI
    TTF_Font* fontTitle = TTF_OpenFont("assets\\couree.fon", 72); // Duża dla tytułu

    if (!fontUI || !fontTitle) {
        printf("Błąd ładowania czcionki: %s\n", TTF_GetError());
    }
    // 3. PRZYGOTOWANIE TEKSTU (Tworzymy teksturę raz, przed pętlą) - stara instrukcja, teraz używamy funkcji pomocniczych
    // Sprzątamy surface, bo już mamy teksturę w GPU (robione w funkcji rysujTekst)

    // ==========================================
    // 1. ŁADOWANIE GRAFIKI TARCZY (i innych boosterów)
    // ==========================================
    SDL_Texture* shieldTexture = wczytajTeksture(renderer, "assets\\shield.bmp", true);
    SDL_Texture* speedTexture = wczytajTeksture(renderer, "assets\\speedF.bmp", true);
    SDL_Texture* slowTexture = wczytajTeksture(renderer, "assets\\slow.bmp", true);
    SDL_Texture* freezeTexture = wczytajTeksture(renderer, "assets\\freezeF.bmp", true);

    // Ładowanie tekstur mapy
    SDL_Texture* wallTexture = wczytajTeksture(renderer, "assets\\grass2.bmp");    // Sciana
    SDL_Texture* cropTexture = wczytajTeksture(renderer, "assets\\wheat2.bmp");    // Nieskoszone (punkty)
    SDL_Texture* stubbleTexture = wczytajTeksture(renderer, "assets\\wheatCut2.bmp");// Skoszone

    // NOWOSC: TEKSTURY DO BOSS LEVELU
    SDL_Texture* hayTexture = wczytajTeksture(renderer, "assets\\hay.bmp", true);
    SDL_Texture* bossTexture = wczytajTeksture(renderer, "assets\\soltys.bmp", true);
    SDL_Texture* crownTexture = wczytajTeksture(renderer, "assets\\crown.bmp", true);

    // --- NOWY SYSTEM WYBORU KOLORU TRAKTORA ---
    std::vector<TractorOption> tractorOptions;
    // Definicja 8 kolorów (Blue, Cyan, Green, Orange, Pink, Purple, Red, Yellow)
    tractorOptions.push_back({ "assets\\tractor_blue.bmp",   {0, 0, 255, 255},    nullptr });
    tractorOptions.push_back({ "assets\\tractor_cyan.bmp",   {0, 255, 255, 255},  nullptr });
    tractorOptions.push_back({ "assets\\tractor_green.bmp",  {0, 255, 0, 255},    nullptr });
    tractorOptions.push_back({ "assets\\tractor_orange.bmp", {255, 165, 0, 255},  nullptr });
    tractorOptions.push_back({ "assets\\tractor_pink.bmp",   {255, 105, 180, 255},nullptr });
    tractorOptions.push_back({ "assets\\tractor_purple.bmp", {128, 0, 128, 255},  nullptr });
    tractorOptions.push_back({ "assets\\tractor_red.bmp",    {255, 0, 0, 255},    nullptr }); // Domyślny
    tractorOptions.push_back({ "assets\\tractor_yellow.bmp", {255, 255, 0, 255},  nullptr });

    // Wczytanie tekstur dla wszystkich kolorów
    for (auto& opt : tractorOptions) {
        opt.texture = wczytajTeksture(renderer, opt.filename.c_str(), true);
    }
    int selectedColorIndex = 6; // Domyślnie Red (indeks 6 w wektorze)

    // Ładowanie tekstur POSTACI (Player i Enemy) z przezroczystością (true)
    //SDL_Texture* playerTexture = wczytajTeksture(renderer, "assets\\tractor_red.bmp", true);
    // ^ ZASTĄPIONE PRZEZ WYBÓR Z tractorOptions

    //SDL_Texture* enemyTexture = wczytajTeksture(renderer, "assets\\babes11.bmp", true);
    // Level 0 (Gra 1): babes11, babes12, babes13
    // Level 1 (Gra 2): babes21, babes22, babes23
    // Level 2 (Boss):  Domyślnie użyjemy tych z poziomu 2 (lub stwórz pliki babes31...)
    SDL_Texture* enemyTextures[3][3];

    // Level 1
    enemyTextures[0][0] = wczytajTeksture(renderer, "assets\\babes11.bmp", true);
    enemyTextures[0][1] = wczytajTeksture(renderer, "assets\\babes12.bmp", true);
    enemyTextures[0][2] = wczytajTeksture(renderer, "assets\\babes13.bmp", true);

    // Level 2
    enemyTextures[1][0] = wczytajTeksture(renderer, "assets\\babes21.bmp", true);
    enemyTextures[1][1] = wczytajTeksture(renderer, "assets\\babes22.bmp", true);
    enemyTextures[1][2] = wczytajTeksture(renderer, "assets\\babes23.bmp", true);

    // Level 3 (Boss) - kopiujemy te z levelu 2 (chyba że masz pliki babes31.bmp itd.)
    enemyTextures[2][0] = enemyTextures[1][0];
    enemyTextures[2][1] = enemyTextures[1][1];
    enemyTextures[2][2] = enemyTextures[1][2];
    // Ładowanie grafik do menu wyboru poziomów
    SDL_Texture* lvl1Tex = wczytajTeksture(renderer, "assets\\lvl1.bmp");
    SDL_Texture* lvl2Tex = wczytajTeksture(renderer, "assets\\lvl2.bmp");
    SDL_Texture* bossLvlTex = wczytajTeksture(renderer, "assets\\bosslvl.bmp");

    //Inicjalizacja obiektow
    Player player(80, 80, 50, 10);
    // player.setTexture(playerTexture); // Przypisanie tekstury graczowi - TERAZ ROBIMY TO PRZY STARCIE GRY

    //Enemy enemy(990, 710, 50, 2);
    //enemy.setTexture(enemyTexture); // Przypisanie tekstury wrogowi

    // ZMIANA: Lista wrogów zamiast jednego
    std::vector<Enemy> enemies;

    // NOWOSC: Kontener na bele siana i korony
    std::vector<HayBale> hayBales;
    std::vector<Crown> crowns;
    Uint32 haySpawnTimer = 0; // Timer do generowania siana

    // Definiujemy STAŁE pozycje boosterów
    SDL_Rect boosterPositions[4] = {
        {225, 85, 40, 40},
        {505, 225, 40, 40},
        {85, 715, 40, 40},
        {995, 85, 40, 40}
    };
    //Inicjalizacja boosterow
    std::vector<Booster> boosters;
    boosters.push_back({ {225, 85, 40, 40}, SPEED_UP, true });
    boosters.push_back({ {505, 225, 40, 40}, INVINCIBLE, true });
    boosters.push_back({ {85, 715, 40, 40}, SLOW_ENEMY, true });
    boosters.push_back({ {995, 85, 40, 40}, FREEZE, true });

    //System Poziomow 
    bool odblokowaneLevele[4];
    // WCZYTUJEMY STAN Z PLIKU
    wczytajPostep(odblokowaneLevele, 4);

    SDL_Rect przyciskiMenu[3];
    int sqSize = 220;
    int spacing = (SCREEN_WIDTH - (3 * sqSize)) / 4;
    int menuOffsetY = 150;
    for (int i = 0; i < 3; i++) {
        przyciskiMenu[i] = { spacing + i * (sqSize + spacing), (SCREEN_HEIGHT - sqSize) / 2 + menuOffsetY, sqSize, sqSize };
    }

    // --- PRZYCISKI WYBORU KOLORU (SIATKA 2x4) ---
    SDL_Rect colorButtons[8];
    int colorBtnSize = 40;
    int colorBtnGap = 10;
    // Obliczanie pozycji siatki (prawa strona)
    int gridStartX = SCREEN_WIDTH / 2 + 100;
    int gridStartY = 300; // Pod polem wpisywania imienia

    for (int i = 0; i < 8; i++) {
        int row = i / 4; // 0 lub 1
        int col = i % 4; // 0, 1, 2, 3
        colorButtons[i] = {
            gridStartX + col * (colorBtnSize + colorBtnGap),
            gridStartY + row * (colorBtnSize + colorBtnGap),
            colorBtnSize,
            colorBtnSize
        };
    }
    // Pozycja podglądu (lewa strona)
    SDL_Rect previewRect = { SCREEN_WIDTH / 2 - 260, gridStartY, 100, 100 };

    int aktualnyLvl = 0;
    bool running = true;
    bool gameOver = false;
    bool menuActive = true;
    int aktualnePunkty = 0;
    int pozostalePunkty = 0;

    // --- 1. DEKLARACJA GRACZA PRZED UŻYCIEM W CREDITS ---
    std::string playerName = "Gracz";
    SDL_StartTextInput();

    // --- NOWE ZMIENNE DO STEROWANIA EKRANAMI ---
    bool levelCompleteScreen = false; // Ekran między poziomami
    bool gameWonScreen = false;       // Ekran po pokonaniu bossa
    bool creditsActive = false;       // Napisy końcowe
    float creditsScroll = 0;          // Do przesuwania napisów

    // Tekst napisów końcowych 
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
    // Dodajemy puste linie żeby napisy wyjechały poza ekran
    for (int i = 0; i < 20; i++) endCredits.push_back("");

    SDL_Event e;
    while (running) {
        //Wejscie
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            if (menuActive) {
                if (e.type == SDL_TEXTINPUT) {
                    if (playerName.length() < 15) {
                        playerName += e.text.text;
                    }
                }
                else if (e.type == SDL_KEYDOWN) {
                    // !!! POPRAWKA: SDLK_BACKSPACE !!!
                    if (e.key.keysym.sym == SDLK_BACKSPACE && playerName.length() > 0) {
                        playerName.pop_back();
                    }
                }

                //START GRY PO NACISNIECIU SPACJI (Teraz myszki)
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                    SDL_Point mousePos = { e.button.x, e.button.y };

                    // 1. SPRAWDZENIE KLIKNIĘCIA W KOLOR
                    for (int i = 0; i < 8; i++) {
                        if (SDL_PointInRect(&mousePos, &colorButtons[i])) {
                            selectedColorIndex = i; // Zmiana wybranego koloru
                        }
                    }

                    for (int i = 0; i < 3; i++) {
                        if (SDL_PointInRect(&mousePos, &przyciskiMenu[i]) && i < LICZBA_LEVELI && odblokowaneLevele[i]) {
                            aktualnyLvl = i;
                            menuActive = false;

                            // !!! PRZYPISANIE WYBRANEJ TEKSTURY GRACZOWI !!!
                            player.setTexture(tractorOptions[selectedColorIndex].texture);

                            // 1. LOSOWANIE BOOSTERÓW
                            boosters.clear();
                            // Reset wrogów i elementów bossa
                            enemies.clear();
                            hayBales.clear();
                            crowns.clear();

                            // Konfiguracja levelu
                            if (aktualnyLvl == 2) {
                                // --- KONFIGURACJA BOSS LEVELU ---
                                // 2. Mapa - ustawiamy wszystko na "skoszone" (2), żeby nie było zboża
                                for (int r = 1; r < MAP_ROWS - 1; r++) {
                                    for (int c = 1; c < MAP_COLS - 1; c++) {
                                        // Ustawiamy w pamieci, ze pola sa puste/skoszone
                                    }
                                }
                                ladujPoziom(aktualnyLvl, boosters);
                                // Nadpisanie mapy na skoszone:
                                for (int r = 1; r < MAP_ROWS - 1; r++) {
                                    for (int c = 1; c < MAP_COLS - 1; c++) {
                                        if (maze[r][c] != 1) { // Jeżeli to nie jest ściana...
                                            maze[r][c] = 2;    // ...to zrób skoszone pole (tło)
                                        }
                                    }
                                }
                                // 3. Boss Sołtys
                                Enemy boss(SCREEN_WIDTH / 2 - 30, SCREEN_HEIGHT / 2 - 30, 60, 1);
                                boss.setTexture(bossTexture);
                                boss.setSpeed(1); // Bardzo wolny
                                enemies.push_back(boss);

                                // 4. Generowanie koron
                                int crownsToSpawn = 15;
                                while (crownsToSpawn > 0) {
                                    // 1. Losujemy współrzędne w siatce mapy (indeksy tablicy), a nie od razu piksele
                                    // Pomijamy krawędzie mapy (indeksy od 1 do MAX-2)
                                    int rCol = (rand() % (MAP_COLS - 2)) + 1;
                                    int rRow = (rand() % (MAP_ROWS - 2)) + 1;

                                    // 2. SPRAWDZENIE: Czy w tym miejscu jest ściana (trawa)?
                                    // Jeśli tak (wartość 1), to przerywamy tę pętlę i losujemy od nowa
                                    if (maze[rRow][rCol] == 1) {
                                        continue;
                                    }

                                    // 3. Jeśli miejsce jest wolne, przeliczamy na piksele
                                    int cx = rCol * TILE_SIZE + 10;
                                    int cy = rRow * TILE_SIZE + 10;

                                    SDL_Rect tempRect = { cx, cy, 50, 50 };
                                    SDL_Rect bossRect = boss.getRect();

                                    // 4. Sprawdzenie czy nie koliduje z Bossem
                                    bool collision = false;
                                    if (SDL_HasIntersection(&tempRect, &bossRect)) collision = true;

                                    // (Opcjonalnie) Sprawdzenie czy korona nie wchodzi na inną koronę
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
                                pozostalePunkty = 15; // Celem są korony
                            }
                            else {
                                // Setup Zwykły (skopiuj ze swojego starego kodu)
                                // --- NOWY KOD: GENEROWANIE BOOSTERÓW DLA NOWEGO POZIOMU ---
                                std::vector<BoosterType> types = { SPEED_UP, INVINCIBLE, SLOW_ENEMY, FREEZE };
                                // Tasowanie typów
                                std::random_device rd;
                                std::mt19937 g(rd());
                                std::shuffle(types.begin(), types.end(), g);

                                // Przypisanie przetasowanych typów do stałych pozycji
                                for (int k = 0; k < 4; k++) {
                                    boosters.push_back({ boosterPositions[k], types[k], true });
                                }

                                // --- NOWY KOD: GENEROWANIE 3 WROGÓW ---
                                int spawnX[] = { 990, 80, 990 };
                                int spawnY[] = { 80, 710, 710 };

                                for (int k = 0; k < 3; k++) {
                                    // ZMIANA: Prędkość ustawiona na 1 (wolniej), bo jest ich trzech
                                    Enemy newEnemy(spawnX[k], spawnY[k], 50, 1);
                                    newEnemy.setTexture(enemyTextures[aktualnyLvl][k]); // Unikalna tekstura dla każdego
                                    enemies.push_back(newEnemy);
                                }

                                ladujPoziom(aktualnyLvl, boosters);
                                // ... generowanie wrogów i boosterów ...
                                pozostalePunkty = liczPunkty();
                            }

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
                // A. EKRAN PRZEJŚCIA (BRAWO)
                if (levelCompleteScreen) {
                    if (e.type == SDL_KEYDOWN) {
                        if (e.key.keysym.sym == SDLK_SPACE) {
                            // !!! TU NASTĘPUJE FAKTYCZNE PRZEJŚCIE DALEJ !!!
                            levelCompleteScreen = false;
                            aktualnyLvl++;

                            // 1. OBOWIĄZKOWE CZYSZCZENIE WEKTORÓW (To naprawia brak generowania)
                            boosters.clear();
                            enemies.clear();
                            hayBales.clear();
                            crowns.clear();

                            // 2. KONFIGURACJA POZIOMU
                            if (aktualnyLvl == 2) {
                                // --- SETUP DLA BOSSA (LEVEL 3) ---
                                // Reset mapy w pamięci na 0 (puste)
                                for (int r = 0; r < MAP_ROWS; r++) {
                                    for (int c = 0; c < MAP_COLS; c++) {
                                        maze[r][c] = 0;
                                    }
                                }

                                ladujPoziom(aktualnyLvl, boosters); // Ładuje ściany

                                // Nadpisanie pustych pól na "skoszone" (tło 2)
                                for (int r = 1; r < MAP_ROWS - 1; r++) {
                                    for (int c = 1; c < MAP_COLS - 1; c++) {
                                        if (maze[r][c] != 1) maze[r][c] = 2;
                                    }
                                }

                                // Tworzenie Bossa
                                Enemy boss(SCREEN_WIDTH / 2 - 30, SCREEN_HEIGHT / 2 - 30, 60, 1);
                                boss.setTexture(bossTexture);
                                boss.setSpeed(1);
                                enemies.push_back(boss);

                                // Generowanie koron (Skopiowane z Twojego kodu)
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
                                            collision = true;
                                            break;
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
                                // --- SETUP DLA ZWYKŁEGO POZIOMU (NP. LEVEL 2) ---

                                // A. Generowanie Boosterów
                                std::vector<BoosterType> types = { SPEED_UP, INVINCIBLE, SLOW_ENEMY, FREEZE };
                                std::random_device rd;
                                std::mt19937 g(rd());
                                std::shuffle(types.begin(), types.end(), g);

                                for (int k = 0; k < 4; k++) {
                                    boosters.push_back({ boosterPositions[k], types[k], true });
                                }

                                // B. Generowanie 3 Wrogów
                                int spawnX[] = { 990, 80, 990 };
                                int spawnY[] = { 80, 710, 710 };

                                for (int k = 0; k < 3; k++) {
                                    Enemy newEnemy(spawnX[k], spawnY[k], 50, 1);
                                    // Upewniamy się, że tekstura istnieje dla tego levelu
                                    if (enemyTextures[aktualnyLvl][k]) {
                                        newEnemy.setTexture(enemyTextures[aktualnyLvl][k]);
                                    }
                                    enemies.push_back(newEnemy);
                                }

                                // C. Ładowanie mapy i punktów
                                ladujPoziom(aktualnyLvl, boosters);
                                pozostalePunkty = liczPunkty();
                            }

                            // 3. WSPÓLNY RESET GRACZA
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
                // B. EKRAN WYGRANEJ (PO BOSSIE)
                else if (gameWonScreen) {
                    if (e.type == SDL_KEYDOWN) {
                        if (e.key.keysym.sym == SDLK_SPACE) {
                            // Aktualizujemy imię w wektorze napisów (indeks 6 to miejsce, gdzie wstawiłeś playerName)
                            endCredits[6] = playerName;
                            gameWonScreen = false;
                            creditsActive = true; // Odpalamy napisy!
                            creditsScroll = SCREEN_HEIGHT; // Reset pozycji
                        }
                        else if (e.key.keysym.sym == SDLK_ESCAPE) {
                            gameWonScreen = false;
                            menuActive = true;
                            SDL_StartTextInput();
                        }
                    }
                }
                // C. NAPISY KOŃCOWE
                else if (creditsActive) {
                    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                        creditsActive = false;
                        menuActive = true;
                        SDL_StartTextInput();
                    }
                }
                // D. NORMALNA ROZGRYWKA
                else {
                    // ZMIANA: Blokada ruchu gracza, jeśli wyświetlamy ekrany końcowe
                    if (!gameOver && !levelCompleteScreen && !gameWonScreen) player.handleInput(e);

                    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                        menuActive = true;
                        SDL_StartTextInput();
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (menuActive) {
            if (wallTexture) {
                for (int y = 0; y < SCREEN_HEIGHT; y += 70) {
                    for (int x = 0; x < SCREEN_WIDTH; x += 70) {
                        SDL_Rect grassRect = { x, y, 70, 70 };
                        SDL_RenderCopy(renderer, wallTexture, NULL, &grassRect);
                    }
                }
            }
            if (tractorOptions.size() > 6 && tractorOptions[6].texture) {
                // Upewniamy się, że kolor jest oryginalny (brak modyfikacji koloru)
                SDL_SetTextureColorMod(tractorOptions[6].texture, 255, 255, 255);

                // Ustawiamy bardzo duży rozmiar (np. 800x800)
                int bigSize = 800;
                SDL_Rect bigTractorRect;
                bigTractorRect.w = bigSize;
                bigTractorRect.h = bigSize;

                // Centrujemy go na ekranie
                bigTractorRect.x = (SCREEN_WIDTH - bigSize) / 2;
                bigTractorRect.y = (SCREEN_HEIGHT - bigSize) / 2;

                // Rysujemy zawsze czerwony traktor
                SDL_RenderCopy(renderer, tractorOptions[6].texture, NULL, &bigTractorRect);
            }
            // --- TŁO MENU (PRZYCIEMNIENIE) ---
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
            SDL_Rect fullscreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
            SDL_RenderFillRect(renderer, &fullscreen);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            SDL_Color titleColor = { 255, 215, 0 };
            //rysujTekstWycentrowany(renderer, fontTitle, "TRAKTORZYSTA", 50, titleColor);

            // --- KOD ROZCIĄGAJĄCY TYTUŁ ---
            SDL_Surface* titleSurface = TTF_RenderText_Solid(fontTitle, "TRAKTORZYSTA", titleColor);
            if (titleSurface) {
                SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
                // Ustawiamy prostokąt: X=10, Y=0 (sama góra),
                // Szerokość = Szerokość ekranu - 20 (marginesy), 
                // Wysokość = 180 (wszystko co dostępne nad napisem 'Wpisz imie')
                SDL_Rect titleRect = { 10, 0, SCREEN_WIDTH - 60, 140 };
                SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);

                SDL_FreeSurface(titleSurface);
                SDL_DestroyTexture(titleTexture);
            }
            // -------------------------------
            // SPRAWDZENIE NAGRODY ZA PRZEJŚCIE CAŁEJ GRY
            // Sprawdzamy czy wczytane z pliku postep.txt dane to same jedynki
            if (odblokowaneLevele[0] && odblokowaneLevele[1] && odblokowaneLevele[2] && odblokowaneLevele[3]) {// Jeśli tak, rysujemy koronę Bossa w menu
                if (crownTexture) {
                    SDL_Rect awardRect = { 80, 150, 80, 80 }; // Po lewej stronie

                    // !!! ZMIANA: Usunięto rotację Ex, używamy zwykłego Copy !!!
                    SDL_RenderCopy(renderer, crownTexture, NULL, &awardRect);

                    // I dopisek
                    rysujTekst(renderer, fontUI, "MISTRZ POLA!", 60, 240, { 255, 215, 0 });
                }
            }
            SDL_Color textColor = { 255, 255, 255 };
            rysujTekstWycentrowany(renderer, fontUI, "Wpisz imie traktorzysty:", 180, textColor);

            SDL_Rect inputRect = { (SCREEN_WIDTH - 300) / 2, 210, 300, 40 };
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &inputRect);

            rysujTekstWycentrowany(renderer, fontUI, playerName + (SDL_GetTicks() % 1000 < 500 ? "|" : ""), 220, textColor);

            // --- NOWOŚĆ: RYSOWANIE WYBORU KOLORU ---

            // 1. RYSOWANIE PODGLĄDU (PO LEWEJ)
            rysujTekst(renderer, fontUI, "Wyglad:", previewRect.x, previewRect.y - 25, { 255, 255, 255 });

            // Tło pod podglądem
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &previewRect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &previewRect);

            // Rysowanie wybranego traktora
            if (tractorOptions[selectedColorIndex].texture) {
                // Ustawiamy kolor na normalny
                SDL_SetTextureColorMod(tractorOptions[selectedColorIndex].texture, 255, 255, 255);
                SDL_RenderCopy(renderer, tractorOptions[selectedColorIndex].texture, NULL, &previewRect);
            }

            // 2. RYSOWANIE SIATKI (PO PRAWEJ)
            rysujTekst(renderer, fontUI, "Wybierz kolor:", gridStartX, gridStartY - 25, { 255, 255, 255 });

            for (int i = 0; i < 8; i++) {
                // Wypełnienie kolorem
                SDL_SetRenderDrawColor(renderer,
                    tractorOptions[i].colorRGB.r,
                    tractorOptions[i].colorRGB.g,
                    tractorOptions[i].colorRGB.b,
                    255);
                SDL_RenderFillRect(renderer, &colorButtons[i]);

                // Ramka
                if (i == selectedColorIndex) {
                    // Wybrany: Gruba biała ramka
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_Rect border = colorButtons[i];
                    SDL_RenderDrawRect(renderer, &border);
                    border.x++; border.y++; border.w -= 2; border.h -= 2; // Pogrubienie
                    SDL_RenderDrawRect(renderer, &border);
                }
                else {
                    // Niewybrany: Cienka czarna ramka
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderDrawRect(renderer, &colorButtons[i]);
                }
            }
            // ----------------------------------------

            for (int i = 0; i < 3; i++) {
                // 1. Wybór odpowiedniej tekstury
                SDL_Texture* currentLvlTex = nullptr;
                if (i == 0) currentLvlTex = lvl1Tex;
                else if (i == 1) currentLvlTex = lvl2Tex;
                else if (i == 2) currentLvlTex = bossLvlTex;

                if (currentLvlTex) {
                    if (odblokowaneLevele[i]) {
                        // ODBLOKOWANY: Resetujemy kolor do pełnej jasności (255, 255, 255)
                        SDL_SetTextureColorMod(currentLvlTex, 255, 255, 255);
                    }
                    else {
                        // ZABLOKOWANY: Przyciemniamy grafikę (np. 60, 60, 60)
                        // Im mniejsza liczba, tym ciemniejsza grafika
                        SDL_SetTextureColorMod(currentLvlTex, 60, 60, 60);
                    }

                    // Rysujemy grafikę (będzie jasna lub ciemna w zależności od ustawienia wyżej)
                    SDL_RenderCopy(renderer, currentLvlTex, NULL, &przyciskiMenu[i]);
                }
                else {
                    // Zabezpieczenie na wypadek braku grafiki (rysowanie kwadratu)
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                    SDL_RenderFillRect(renderer, &przyciskiMenu[i]);
                }

                // Rysowanie czarnej ramki dookoła przycisku
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &przyciskiMenu[i]);
            }
        }
        else {
            if (!gameOver && !levelCompleteScreen && !gameWonScreen && !creditsActive) {

                // LOGIKA PUNKTÓW DLA BOSSA
                if (aktualnyLvl == 2) {
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
                    int pCol = (player.getX() + 25) / TILE_SIZE;
                    int pRow = (player.getY() + 25) / TILE_SIZE;
                    if (maze[pRow][pCol] == 0) {
                        maze[pRow][pCol] = 2;
                        aktualnePunkty += 10;
                        pozostalePunkty--;
                    }
                }

                if (pozostalePunkty <= 0) {
                    // 1. ZAPIS POSTĘPU
                    if (aktualnyLvl + 1 < LICZBA_LEVELI) {
                        // Odblokowanie kolejnego poziomu (np. z 1 na 2, z 2 na 3)
                        if (!odblokowaneLevele[aktualnyLvl + 1]) {
                            odblokowaneLevele[aktualnyLvl + 1] = true;
                            zapiszPostep(odblokowaneLevele, 4); // ZMIANA: rozmiar 4
                        }
                    }
                    else if (aktualnyLvl == 2) {
                        // --- NOWOŚĆ: JEŚLI POKONANO BOSSA (LEVEL 3 / INDEKS 2) ---
                        // Ustawiamy czwartą wartość (indeks 3) na true
                        if (!odblokowaneLevele[3]) {
                            odblokowaneLevele[3] = true;
                            zapiszPostep(odblokowaneLevele, 4); // Zapisujemy komplet 1 1 1 1
                        }
                    }

                    // 2. STEROWANIE EKRANAMI (Zamiast natychmiastowego skoku)
                    if (aktualnyLvl == 2) {
                        // To był Boss (ostatni level) -> Ekran Wygranej
                        gameWonScreen = true;
                    }
                    else {
                        // To był zwykły level -> Ekran Przejścia
                        levelCompleteScreen = true;
                    }

                    // UWAGA: Nie resetujemy tu gry! Czekamy na spację.
                }

                // 2. AKTUALIZACJA
                player.update();

                // BOSS LEVEL LOGIKA - ZBALANSOWANA (Bez zmiany kolorów)
                if (aktualnyLvl == 2) {
                    // --- USTAWIENIA TRUDNOŚCI ---
                    int currentSpawnRate = 1200; // Start: Siano co 1.2 sekundy (wolno)
                    int bossSpeed = 1;           // Start: Boss wolny

                    if (!enemies.empty()) {
                        if (pozostalePunkty <= 5) {
                            // KOŃCÓWKA: Trochę szybciej, ale bez przesady
                            currentSpawnRate = 600; // Co 0.6 sekundy (było 0.3s)
                            bossSpeed = 2;          // Boss przyspiesza do 2 (było 3)
                        }
                        else if (pozostalePunkty <= 10) {
                            // ŚRODEK: Lekkie przyspieszenie siana
                            currentSpawnRate = 900; // Co 0.9 sekundy
                            bossSpeed = 1;          // Boss nadal wolny
                        }
                        else {
                            // POCZĄTEK
                            currentSpawnRate = 1200;
                            bossSpeed = 1;
                        }

                        // Aplikujemy prędkość bossa
                        enemies[0].setSpeed(bossSpeed);
                    }

                    // --- GENEROWANIE SIANA ---
                    if (SDL_GetTicks() > haySpawnTimer) {
                        HayBale bale;
                        // Losowa pozycja X (z marginesem od krawędzi)
                        bale.rect = { (rand() % (MAP_COLS - 2) + 1) * TILE_SIZE, -50, 50, 50 };
                        bale.y = -50;

                        // Prędkość spadania siana:
                        // Na końcu (<=5 pkt) losuje od 3 do 5
                        // Normalnie losuje od 2 do 4
                        int baseHaySpeed = (pozostalePunkty <= 5) ? 3 : 2;
                        bale.speed = (rand() % 3) + baseHaySpeed;

                        hayBales.push_back(bale);

                        // Ustawienie timera na następną belę
                        haySpawnTimer = SDL_GetTicks() + currentSpawnRate;
                    }

                    // --- OBSŁUGA RUCHU I KOLIZJI SIANA ---
                    for (size_t i = 0; i < hayBales.size(); ) {
                        hayBales[i].y += hayBales[i].speed;
                        hayBales[i].rect.y = (int)hayBales[i].y;

                        SDL_Rect pRect = player.getRect();
                        // Mniejszy hitbox dla gracza (łatwiej unikać)
                        SDL_Rect hitBox = { pRect.x + 15, pRect.y + 15, pRect.w - 30, pRect.h - 30 };

                        // Sprawdzenie kolizji
                        if (SDL_HasIntersection(&hitBox, &hayBales[i].rect) && !player.isInvincible()) {
                            gameOver = true;
                        }

                        // Usuwanie siana, które wyleciało za ekran
                        if (hayBales[i].rect.y > SCREEN_HEIGHT) {
                            hayBales.erase(hayBales.begin() + i);
                        }
                        else {
                            i++;
                        }
                    }
                }

                // Aktualizacja wszystkich wrogów
                for (auto& enemy : enemies) {
                    enemy.update(player.getX(), player.getY());
                }

                SDL_Rect pRect = player.getRect();
                // Booster logika tylko dla zwyklych leveli
                if (aktualnyLvl != 2) {
                    for (auto& b : boosters) {
                        if (b.active && SDL_HasIntersection(&pRect, &b.rect)) {
                            if (b.type == FREEZE) {
                                // Zamroź WSZYSTKICH wrogów
                                for (auto& enemy : enemies) enemy.freeze();
                            }
                            else if (b.type == SLOW_ENEMY) {
                                // Spowolnij WSZYSTKICH wrogów
                                for (auto& enemy : enemies) enemy.slowDown();
                            }
                            else player.applyBooster(b.type);
                            b.active = false;
                        }
                    }
                }

                // 3. KOLIZJA 
                // Sprawdzamy kolizję z KAŻDYM wrogiem
                for (auto& enemy : enemies) {
                    SDL_Rect eRect = enemy.getRect();

                    // !!! POPRAWKA: "Zmniejszanie" hitboxa gracza dla bardziej precyzyjnej kolizji !!!
                    // Dzięki temu gracz nie ginie, gdy tylko "muśnie" piksel wroga.
                    // Zmniejszamy prostokąt kolizji o 10 pikseli z każdej strony.
                    SDL_Rect hitBox = { pRect.x + 10, pRect.y + 10, pRect.w - 20, pRect.h - 20 };

                    if (SDL_HasIntersection(&hitBox, &eRect) && !player.isInvincible()) {
                        gameOver = true;
                    }
                }
            }

            SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
            SDL_RenderClear(renderer);

            // Rysowanie cian labiryntu i punktow
            for (int r = 0; r < MAP_ROWS; r++) {
                for (int c = 0; c < MAP_COLS; c++) {
                    SDL_Rect tileRect = { c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                    if (maze[r][c] == 1) {
                        // Jeli to pole jest cian (tekstura0)
                        if (wallTexture) SDL_RenderCopy(renderer, wallTexture, NULL, &tileRect);
                        else {
                            SDL_SetRenderDrawColor(renderer, 210, 180, 140, 255); // Kolor somiany
                            SDL_RenderFillRect(renderer, &tileRect);
                        }
                    }
                    else if (maze[r][c] == 0) {
                        // Rysowanie punktu do zebrania (tekstura1 - nieskoszone)
                        if (cropTexture) SDL_RenderCopy(renderer, cropTexture, NULL, &tileRect);
                        else {
                            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                            SDL_Rect dot = { c * TILE_SIZE + 32, r * TILE_SIZE + 32, 6, 6 };
                            SDL_RenderFillRect(renderer, &dot);
                        }
                    }
                    else if (maze[r][c] == 2) {
                        // Skoszone pole (tekstura2)
                        if (stubbleTexture) SDL_RenderCopy(renderer, stubbleTexture, NULL, &tileRect);
                        else {
                            SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
                            SDL_RenderFillRect(renderer, &tileRect);
                        }
                    }
                }
            }

            if (aktualnyLvl == 2) {
                // Rysowanie Boss elementow
                for (auto& c : crowns) if (c.active) SDL_RenderCopy(renderer, crownTexture, NULL, &c.rect);
                for (auto& h : hayBales) SDL_RenderCopy(renderer, hayTexture, NULL, &h.rect);
            }
            else {
                // 4. RYSOWANIE BOOSTERÓW
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

            player.draw(renderer);
            for (auto& enemy : enemies) {
                enemy.draw(renderer);
            }

            // --- UI W GRZE (HUD) ---

            // 1. Nazwa i Punkty (Góra-Lewo)
            std::string celTxt = (aktualnyLvl == 2) ? " | KORONY: " : " | PKT: ";
            std::string celVal = (aktualnyLvl == 2) ? std::to_string(pozostalePunkty) : std::to_string(aktualnePunkty);
            std::string hudTop = playerName + celTxt + celVal;

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
            SDL_Rect bgRect = { 5, 5, (int)hudTop.length() * 12 + 20, 30 };
            SDL_RenderFillRect(renderer, &bgRect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            rysujTekst(renderer, fontUI, hudTop, 10, 10, { 255, 255, 255 });

            // --- NOWOŚĆ: INSTRUKCJA PRAWA GÓRA ---
            std::string instrText;
            if (aktualnyLvl == 2) {
                instrText = "Zbierz wszystkie korony i nie daj sie zlapac!";
            }
            else {
                instrText = "Zbierz cala pszenice i nie daj sie zlapac!";
            }

            // Obliczamy szerokość tekstu, aby wyrównać do prawej
            int iW, iH;
            TTF_SizeText(fontUI, instrText.c_str(), &iW, &iH);
            int instrX = SCREEN_WIDTH - iW - 15; // 15px marginesu od prawej

            // Tło pod instrukcją (szare, półprzezroczyste)
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
            SDL_Rect instrBgRect = { instrX - 5, 5, iW + 10, 30 };
            SDL_RenderFillRect(renderer, &instrBgRect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            // Rysowanie tekstu
            rysujTekst(renderer, fontUI, instrText, instrX, 10, { 255, 255, 255 }); // Lekko szary tekst

            // 2. Aktywny Booster (Dół-Lewo)
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
            // Sprawdzamy stan pierwszego wroga (bo wszyscy dostają efekt naraz)
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

            if (gameOver) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150);
                SDL_Rect fullScreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
                SDL_RenderFillRect(renderer, &fullScreen);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                rysujTekstWycentrowany(renderer, fontTitle, "KONIEC GRY", SCREEN_HEIGHT / 2 - 50, { 255, 255, 255 });
                rysujTekstWycentrowany(renderer, fontUI, "Nacisnij ESC aby wrocic do menu", SCREEN_HEIGHT / 2 + 20, { 255, 255, 255 });
            }
            // --- NAKŁADKI GRAFICZNE (EKRANY KOŃCOWE) ---

            if (levelCompleteScreen) {
                // Tło półprzezroczyste (przyciemnienie gry)
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
                SDL_Rect fullscreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
                SDL_RenderFillRect(renderer, &fullscreen);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                // Teksty
                rysujTekstWycentrowany(renderer, fontTitle, "POZIOM UKONCZONY!", 200, { 0, 255, 0 });
                rysujTekstWycentrowany(renderer, fontUI, "Brawo! Odblokowales kolejny etap.", 300, { 255, 255, 255 });

                // Migające instrukcje
                if (SDL_GetTicks() % 1000 < 600) {
                    rysujTekstWycentrowany(renderer, fontUI, "SPACJA - Jedziesz dalej", 500, { 255, 255, 0 });
                }
                rysujTekstWycentrowany(renderer, fontUI, "ESC - Powrot do Menu", 550, { 200, 200, 200 });
            }
            else if (gameWonScreen) {
                // !!! ZMIANA: TŁO TAKIE SAMO JAK W LEVEL COMPLETE (CZARNE PÓŁPRZEZROCZYSTE) !!!
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
                SDL_Rect fullscreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
                SDL_RenderFillRect(renderer, &fullscreen);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                // Nagłówek
                rysujTekstWycentrowany(renderer, fontTitle, "GRATULACJE!", 100, { 255, 215, 0 }); // Złoty napis zamiast tła
                rysujTekstWycentrowany(renderer, fontUI, "Pokonales baby i soltysa!", 250, { 255, 255, 255 });

                // Imię gracza
                std::string msg = "Traktorzysta " + playerName + " moze bezpiecznie pracowac!";
                rysujTekstWycentrowany(renderer, fontUI, msg, 450, { 200, 0, 0 });

                // Instrukcje
                rysujTekstWycentrowany(renderer, fontUI, "SPACJA - Zobacz Napisy Koncowe", 600, { 255, 255, 255 });
                rysujTekstWycentrowany(renderer, fontUI, "ESC - Wroc do Menu", 650, { 100, 100, 100 });

                // Grafika Bossa na środku (jeśli wczytana)
                if (bossTexture) {
                    SDL_Rect r = { SCREEN_WIDTH / 2 - 50, 300, 100, 100 };
                    SDL_RenderCopy(renderer, bossTexture, NULL, &r);
                }
            }
            else if (creditsActive) {
                // Czarne tło
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);

                // Przesuwanie tekstu w górę
                creditsScroll -= 1.5;

                // Reset do menu jak napisy się skończą
                if (creditsScroll < -((int)endCredits.size() * 50 + 400)) {
                    creditsActive = false;
                    menuActive = true;
                    SDL_StartTextInput();
                }

                // Pętla rysująca tekst linijka po linijce
                for (size_t i = 0; i < endCredits.size(); i++) {
                    float y = creditsScroll + i * 50;
                    // Rysuj tylko to co widać na ekranie (optymalizacja)
                    if (y > -50 && y < SCREEN_HEIGHT + 50) {
                        SDL_Color kol = { 255, 255, 255 };
                        // Jeśli linia zaczyna się od "---", zrób ją żółtą
                        if (endCredits[i].length() > 3 && endCredits[i].substr(0, 3) == "---") kol = { 255, 215, 0 };

                        rysujTekstWycentrowany(renderer, fontUI, endCredits[i], (int)y, kol);
                    }
                }

                // Obrazki po bokach (statyczne)
                SDL_Rect leftImg = { 50, SCREEN_HEIGHT / 2 - 75, 150, 150 };
                SDL_Rect rightImg = { SCREEN_WIDTH - 200, SCREEN_HEIGHT / 2 - 75, 150, 150 };

                if (enemyTextures[0][0]) SDL_RenderCopy(renderer, enemyTextures[0][0], NULL, &leftImg); // Baba po lewej
                if (crownTexture) SDL_RenderCopy(renderer, crownTexture, NULL, &rightImg);     // Korona po prawej

                
            } // Koniec bloku else if (creditsActive)
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Sprzątanie
    SDL_DestroyTexture(shieldTexture);
    SDL_DestroyTexture(speedTexture);
    SDL_DestroyTexture(slowTexture);
    SDL_DestroyTexture(freezeTexture);

    // Sprzątanie nowych tekstur mapy
    SDL_DestroyTexture(wallTexture);
    SDL_DestroyTexture(cropTexture);
    SDL_DestroyTexture(stubbleTexture);

    // Boss textures
    SDL_DestroyTexture(hayTexture);
    SDL_DestroyTexture(bossTexture);
    SDL_DestroyTexture(crownTexture);

    // Czyszczenie tekstur kolorów traktorów
    for (auto& opt : tractorOptions) {
        if (opt.texture) SDL_DestroyTexture(opt.texture);
    }

    // Sprzątanie tekstur postaci
    // SDL_DestroyTexture(playerTexture); // playerTexture nie istnieje jako osobna zmienna, jest teraz w wektorze
    //SDL_DestroyTexture(enemyTexture);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            // Sprawdzenie czy nie usuwamy 2 razy tego samego (dla bossa)
            if (i == 2) continue;
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
    //KOMENTARZ
    //tak
    //test
    return 0;
}