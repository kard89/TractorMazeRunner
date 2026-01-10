#include <SDL.h>
#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>
#include <map>
#include <fstream>  // Do obsługi pliku rekord.txt
#include <string>   // Do zamiany liczb na tekst w tytule okna
#include <iomanip>
#include <SDL_ttf.h>

const int SCREEN_WIDTH = 1120; //Szerokosc okna (16 * 70)
const int SCREEN_HEIGHT = 840; //Wysokosc okna (12 * 70)
//Klasa Gracza
const int TILE_SIZE = 70; // (Powiększone z 50)
const int MAP_ROWS = 12;
const int MAP_COLS = 16;
const int LICZBA_LEVELI = 2; // Liczba dostepnych map

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
        {1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,1}, // 3.
        {1,0,1,0,0,0,0,0,0,0,1,0,0,0,0,1}, // 4.
        {1,0,1,1,1,1,1,1,0,1,1,1,1,1,0,1}, // 5.
        {1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,1}, // 6. rodek
        {1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1}, // 7.
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // 8.
        {1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1}, // 9.
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
            case SDLK_UP: rect.y -= currentSpeed;
                break;
            case SDLK_DOWN: rect.y += currentSpeed;
                break;
            case SDLK_LEFT: rect.x -= currentSpeed;
                break;
            case SDLK_RIGHT: rect.x += currentSpeed;
                break;
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
        if (now > speedTimer) {
            currentSpeed = baseSpeed;
        }
        if (now > invincibleTimer) {
            invincible = false;
        }

        if (rect.x < 0) rect.x = 0;
        if (rect.y < 0) rect.y = 0;
        if (rect.x + rect.w > SCREEN_WIDTH) rect.x = SCREEN_WIDTH - rect.w;
        if (rect.y + rect.h > SCREEN_HEIGHT) rect.y = SCREEN_HEIGHT - rect.h;
    }
    void draw(SDL_Renderer* renderer) { //rysowanie gracza w oknie
        if (texture) {
            // Analogiczne zmiany kolorow dla tekstury (Color Mod)
            SDL_SetTextureColorMod(texture, 255, 255, 255); // Normalny
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
        currentSpeed = baseSpeed / 2; // Zwolnij o połowę
        if (currentSpeed < 1) currentSpeed = 1;
    }

    void update(int playerX, int playerY) {
        // Obsługa timerów efektów
        Uint32 now = SDL_GetTicks();
        if (frozen && now > freezeTimer) {
            frozen = false;
        }
        if (slowed && now > slowTimer) {
            slowed = false;
            currentSpeed = baseSpeed;
        }
        if (slowed && !frozen) {
            currentSpeed = baseSpeed / 2;
        }

        if (frozen) return;

        // Obliczaj now ciek co 30 klatek (eby nie obcia procesora)
        static int frameCounter = 0;
        if (frameCounter++ % 30 == 0) {
            findPath(rect.x, rect.y, playerX, playerY);
        }

        if (!path.empty()) {
            SDL_Point target = path[0];

            // Używamy currentSpeed
            if (rect.x < target.x) rect.x += currentSpeed;
            else if (rect.x > target.x) rect.x -= currentSpeed;

            if (rect.y < target.y) rect.y += currentSpeed;
            else if (rect.y > target.y) rect.y -= currentSpeed;

            // Jeli dotar do punktu kontrolnego cieki, usu go i id do nastpnego
            if (abs(rect.x - target.x) < currentSpeed + 1 && abs(rect.y - target.y) < currentSpeed + 1) {
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
            SDL_SetTextureColorMod(texture, 255, 255, 255); // Normalny
            SDL_RenderCopy(renderer, texture, NULL, &rect);
        }
        else {
            // Stary kod rysowania (backup)
            if (frozen) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // Niebieski jeśli zamrożony
            else if (slowed) SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // Brązowy jeśli spowolniony (Błoto)
            else SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    bool isFrozen() { return frozen; }
    bool isSlowed() { return slowed; }

    void resetStatus(){
        frozen = false;
        slowed = false;
        freezeTimer = 0;
        slowTimer = 0;
        currentSpeed = baseSpeed;
        path.clear();
    }
};

int wczytajRekord() {
    int rekord = 0;
    std::ifstream plik("rekord.txt");
    if (plik.is_open()) {
        plik >> rekord;
        plik.close();
    }
    return rekord;
}

void zapiszRekord(int punkty) {
    std::ofstream plik("rekord.txt");
    if (plik.is_open()) {
        plik << punkty;
        plik.close();
    }
}
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
        // 2. USTAWIANIE PRZEZROCZYSTOŚCI (Usuwamy Magentę: 255, 0, 255)
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
    SDL_Window* window = SDL_CreateWindow("Traktorem przez wies",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    //Renderowanie obrazkow
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
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
    // 24 to rozmiar czcionki
    TTF_Font* font = TTF_OpenFont("assets\\arial.ttf", 24);
    if (!font) {
        printf("Błąd ładowania czcionki: %s\n", TTF_GetError());
        // Nie returnujemy tutaj, żeby zobaczyć chociaż czarne okno, ale w normalnej grze to błąd krytyczny
    }
    // 3. PRZYGOTOWANIE TEKSTU (Tworzymy teksturę raz, przed pętlą)
    SDL_Color color = { 255, 0, 0 }; // Czerwony kolor (R, G, B)
    SDL_Surface* surface = TTF_RenderText_Solid(font, "SDL_ttf dziala!", color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    // Sprzątamy surface, bo już mamy teksturę w GPU
    SDL_FreeSurface(surface);
    int texW = 0;
    int texH = 0;
    SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
    SDL_Rect dstRect = { 50, 50, texW, texH }; // Pozycja x=50, y=50
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

    // Ładowanie tekstur POSTACI (Player i Enemy) z przezroczystością (true)
    SDL_Texture* playerTexture = wczytajTeksture(renderer, "assets\\tractor_red.bmp", true);
    SDL_Texture* enemyTexture = wczytajTeksture(renderer, "assets\\babes11.bmp", true);

    //Inicjalizacja obiektow
    Player player(80, 80, 50, 10);
    player.setTexture(playerTexture); // Przypisanie tekstury graczowi

    Enemy enemy(990, 710, 50, 2);
    enemy.setTexture(enemyTexture); // Przypisanie tekstury wrogowi

    //Inicjalizacja boosterow
    std::vector<Booster> boosters;
    boosters.push_back({ {225, 85, 40, 40}, SPEED_UP, true });
    boosters.push_back({ {505, 225, 40, 40}, INVINCIBLE, true });
    boosters.push_back({ {85, 715, 40, 40}, SLOW_ENEMY, true });
    boosters.push_back({ {995, 85, 40, 40}, FREEZE, true });
    //System Poziomow 
    bool odblokowaneLevele[3];
    wczytajPostep(odblokowaneLevele, 3); // WCZYTUJEMY STAN Z PLIKU

    SDL_Rect przyciskiMenu[3];
    int sqSize = 220;
    int spacing = (SCREEN_WIDTH - (3 * sqSize)) / 4;
    for (int i = 0; i < 3; i++) {
        przyciskiMenu[i] = { spacing + i * (sqSize + spacing), (SCREEN_HEIGHT - sqSize) / 2, sqSize, sqSize };
    }

    int aktualnyLvl = 0;
    bool running = true;
    bool gameOver = false;
    bool menuActive = true;
    int rekordZycia = wczytajRekord();
    int aktualnePunkty = 0;
   int pozostalePunkty = 0;
    SDL_Event e;
    while (running) {
        //Wejscie
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            //START GRY PO NACISNIECIU SPACJI
            if (menuActive && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                SDL_Point mousePos = { e.button.x, e.button.y };
                for (int i = 0; i < 3; i++) {
                    if (SDL_PointInRect(&mousePos, &przyciskiMenu[i]) && i < LICZBA_LEVELI && odblokowaneLevele[i]) {
                        aktualnyLvl = i; menuActive = false;
                        ladujPoziom(aktualnyLvl, boosters);
                        pozostalePunkty = liczPunkty();
                        aktualnePunkty = 0; player.setPos(80, 80); enemy.setPos(990, 710); gameOver = false;
                        player.resetBoosters();
                        enemy.resetStatus();
                    }
                }
            }
            if (!gameOver && !menuActive) player.handleInput(e);
        }

        if (menuActive) {
            std::string menuTxt = "TRAKTORZYSTA | REKORD: " + std::to_string(rekordZycia) + " | WYBIERZ POZIOM";
            SDL_SetWindowTitle(window, menuTxt.c_str());
            SDL_SetRenderDrawColor(renderer, 20, 100, 20, 255);
            SDL_RenderClear(renderer);

            for (int i = 0; i < 3; i++) {
                // Jeśli poziom jest odblokowany - kolor jasny, jeśli zablokowany - ciemnoszary
                if (odblokowaneLevele[i]) {
                    if (i == 0) SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Poziom 1 (Biały)
                    else if (i == 1) SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Poziom 2 (Żółty)
                    else SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Poziom 3 (Czerwony)
                }
                else {
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // ZABLOKOWANY (Szary)
                }
                SDL_RenderFillRect(renderer, &przyciskiMenu[i]);
            }
        }
        else {
            if (!gameOver) {
                int pCol = (player.getX() + 25) / TILE_SIZE;
                int pRow = (player.getY() + 25) / TILE_SIZE;

                if (maze[pRow][pCol] == 0) {
                    maze[pRow][pCol] = 2;
                    aktualnePunkty += 10;
                    pozostalePunkty--;

                    if (pozostalePunkty <= 0) {
                        // LOGIKA ODBLOKOWYWANIA
                        if (aktualnyLvl + 1 < 3) { // 3 to rozmiar Twojej tablicy odblokowaneLevele
                            if (!odblokowaneLevele[aktualnyLvl + 1]) {
                                odblokowaneLevele[aktualnyLvl + 1] = true;
                                zapiszPostep(odblokowaneLevele, 3);
                                std::cout << "Odblokowano poziom: " << aktualnyLvl + 2 << std::endl;
                            }
                        }

                        // Przejście do następnego poziomu lub powrót do menu
                        if (aktualnyLvl < LICZBA_LEVELI - 1) {
                            aktualnyLvl++;
                            ladujPoziom(aktualnyLvl, boosters);
                            pozostalePunkty = liczPunkty();
                            player.setPos(80, 80);
                            enemy.setPos(990, 710);
                            player.resetBoosters();
                            enemy.resetStatus();
                        }
                        else {
                            menuActive = true; // Koniec gry - powrót do menu
                        }
                    }
                }

                // 2. AKTUALIZACJA TYTUŁU OKNA
                std::string tytul = "Poziom " + std::to_string(aktualnyLvl + 1) + " | Punkty: " + std::to_string(aktualnePunkty) + " | Rekord: " + std::to_string(rekordZycia);
                if (player.isInvincible()) {
                    float t = player.getInvincibleRemainingTime();
                    if (t > 0) tytul += "|[TARCZA:" + std::to_string(t).substr(0, 3) + "s]";
                }
                if (player.isSpeedUp()) {
                    float t = player.getSpeedRemainingTime();
                    if (t > 0) tytul += "|[TURBO:" + std::to_string(t).substr(0, 3) + "s]";
                }
                if (enemy.isFrozen()) {
                    float t = enemy.getFreezeRemainingTime();
                    if (t > 0) tytul += "|[ZAMROZENIE:" + std::to_string(t).substr(0, 3) + "s]";
                }
                if (enemy.isSlowed()) {
                    float t = enemy.getSlowRemainingTime();
                    if (t > 0) tytul += "|[BLOTO:" + std::to_string(t).substr(0, 3) + "s]";
                }

                SDL_SetWindowTitle(window, tytul.c_str());
                player.update();
                enemy.update(player.getX(), player.getY());

                SDL_Rect pRect = player.getRect();
                for (auto& b : boosters) {
                    if (b.active && SDL_HasIntersection(&pRect, &b.rect)) {
                        if (b.type == FREEZE) enemy.freeze();
                        else if (b.type == SLOW_ENEMY) enemy.slowDown();
                        else player.applyBooster(b.type);
                        b.active = false;
                    }
                }

                // 3. KOLIZJA I ZAPIS REKORDU
                SDL_Rect eRect = enemy.getRect();
                if (SDL_HasIntersection(&pRect, &eRect) && !player.isInvincible()) {
                    gameOver = true;
                    if (aktualnePunkty > rekordZycia) {
                        rekordZycia = aktualnePunkty;
                        zapiszRekord(rekordZycia);
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

            // 4. RYSOWANIE BOOSTERÓW
            for (auto& b : boosters) {
                if (b.active) {
                    if (b.type == INVINCIBLE && shieldTexture) SDL_RenderCopy(renderer, shieldTexture, NULL, &b.rect);
                    else if (b.type == SPEED_UP && speedTexture) SDL_RenderCopy(renderer, speedTexture, NULL, &b.rect);
                    else if (b.type == SLOW_ENEMY && slowTexture) SDL_RenderCopy(renderer, slowTexture, NULL, &b.rect);
                    else if (b.type == FREEZE && freezeTexture) SDL_RenderCopy(renderer, freezeTexture, NULL, &b.rect);
                    else {
                        if (b.type == SPEED_UP) SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
                        else if (b.type == INVINCIBLE) SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                        else if (b.type == SLOW_ENEMY) SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
                        else if (b.type == FREEZE) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
                        SDL_RenderFillRect(renderer, &b.rect);
                    }
                }
            }

            player.draw(renderer);
            enemy.draw(renderer);
            // Rysowanie napisu testowego "SDL_ttf dziala!"
            if (texture) {
                SDL_RenderCopy(renderer, texture, NULL, &dstRect);
            }
            if (gameOver) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150);
                SDL_Rect fullScreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
                SDL_RenderFillRect(renderer, &fullScreen);
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Sprztanie
    SDL_DestroyTexture(shieldTexture);
    SDL_DestroyTexture(speedTexture);
    SDL_DestroyTexture(slowTexture);
    SDL_DestroyTexture(freezeTexture);

    // Sprzątanie nowych tekstur mapy
    SDL_DestroyTexture(wallTexture);
    SDL_DestroyTexture(cropTexture);
    SDL_DestroyTexture(stubbleTexture);

    // Sprzątanie tekstur postaci
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(texture); // Usuń teksturę napisu testowego
    TTF_CloseFont(font);         // Zamknij czcionkę
    TTF_Quit();                  // Wyłącz system czcionek
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    //KOMENTARZ
    //tak
    //test
    return 0;
}