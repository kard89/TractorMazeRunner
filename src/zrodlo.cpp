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
    void applyBooster(BoosterType type) {
        Uint32 now = SDL_GetTicks();
        if (type == SPEED_UP) {
            currentSpeed = baseSpeed * 2;
            speedTimer = now + 5000;
        }
        else if (type == INVINCIBLE) {
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
        if (invincible) SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Kolor żółty jeśli nieśmiertelny
        else SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);//ustawienie koloru na bialy
        SDL_RenderFillRect(renderer, &rect);
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
        if (frozen) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // Niebieski jeśli zamrożony
        else if (slowed) SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // Brązowy jeśli spowolniony (Błoto)
        else SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect);
    }


    bool isFrozen() { return frozen; }
    bool isSlowed() { return slowed; }
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

    // ==========================================
    // 1. ŁADOWANIE GRAFIKI TARCZY (Wklejone poprawnie w sekcji init)
    // ==========================================
    SDL_Surface* tempSurface = SDL_LoadBMP("assets\\shield.bmp"); // Wczytujemy plik z assets
    if (!tempSurface) {
        std::cout << "Nie udalo sie wczytac shield.bmp! Blad: " << SDL_GetError() << std::endl;
        // Kontynuujemy mimo błedu, żeby gra się nie wywaliła - ale tekstura będzie pusta
    }

    SDL_Texture* shieldTexture = nullptr;
    if (tempSurface) {
        // 2. USTAWIANIE PRZEZROCZYSTOŚCI (Usuwamy Magentę: 255, 0, 255)
        SDL_SetColorKey(tempSurface, SDL_TRUE, SDL_MapRGB(tempSurface->format, 255, 0, 255));

        // 3. TWORZENIE TEKSTURY
        shieldTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
        SDL_FreeSurface(tempSurface);
    }

    //Inicjalizacja obiektow
    // (Zaktualizowane pozycje startowe na srodek kafelkow 70px)
    Player player(80, 80, 50, 10);
    Enemy enemy(990, 710, 50, 2);
    //Inicjalizacja boosterow
    std::vector<Booster> boosters;
    // (Zaktualizowane pozycje boosterow dla nowej skali i rozmiar 40x40)
    boosters.push_back({ {225, 85, 40, 40}, SPEED_UP, true });
    boosters.push_back({ {505, 225, 40, 40}, INVINCIBLE, true });
    // --- ZMIANA: Tu był SHRINK, teraz jest SLOW_ENEMY (błoto) ---
    boosters.push_back({ {85, 715, 40, 40}, SLOW_ENEMY, true });
    boosters.push_back({ {995, 85, 40, 40}, FREEZE, true });

    int aktualnyLvl = 0;
    ladujPoziom(aktualnyLvl, boosters);

    //zmienna sterujaca gra
    bool running = true;
    bool gameOver = false;
    bool menuActive = true;
    SDL_Event e;
    int rekordZycia = wczytajRekord(); // Ładujemy stary rekord na start
    int aktualnePunkty = 0;
    int pozostalePunkty = liczPunkty();

    while (running) {
        //Wejscie
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            //START GRY PO NACISNIECIU SPACJI
            if (menuActive && e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
                menuActive = false;
            }
            //OBSLUGA RUCHU TYLKO GDY UZYTKOWNIK GRA
            if (!gameOver && !menuActive) player.handleInput(e);
        }
        if (menuActive) {
            std::string menuTxt = "TRAKTORZYSTA | REKORD: " + std::to_string(rekordZycia) + " | ABY ROZPOCZAC GRE WCISNIJ SPACJE";
            SDL_SetWindowTitle(window, menuTxt.c_str());
            SDL_SetRenderDrawColor(renderer, 20, 100, 20, 255);
            SDL_RenderClear(renderer);
        }
        else {


            if (!gameOver) {
                // 1. LOGIKA ZBIERANIA PUNKTÓW
                int pCol = (player.getX() + 25) / TILE_SIZE; // Srodek gracza (50/2 = 25)
                int pRow = (player.getY() + 25) / TILE_SIZE;
                if (maze[pRow][pCol] == 0) {
                    maze[pRow][pCol] = 2; // Oznaczenie jako zebrane
                    aktualnePunkty += 10;
                    pozostalePunkty--;

                    // Sprawdzenie czy wszystkie punkty zebrane
                    if (pozostalePunkty <= 0) {
                        aktualnyLvl = (aktualnyLvl + 1) % LICZBA_LEVELI;
                        ladujPoziom(aktualnyLvl, boosters);
                        pozostalePunkty = liczPunkty();
                        player.setPos(80, 80);
                        enemy.setPos(990, 710);
                    }
                }

                // 2. AKTUALIZACJA TYTUŁU OKNA
                // Sklejamy tekst: Punkty + Rekord
                std::string tytul = "Poziom " + std::to_string(aktualnyLvl + 1) + " | Punkty: " + std::to_string(aktualnePunkty) + " | Rekord: " + std::to_string(rekordZycia);
                if (player.isInvincible()) {//Dodanie czasu trwania boosterow do napisu zwiazanego z punktami i rekordem
                    float t = player.getInvincibleRemainingTime();
                    if (t > 0)tytul += "|[TARCZA:" + std::to_string(t).substr(0, 3) + "s]";
                }
                if (player.isSpeedUp()) {
                    float t = player.getSpeedRemainingTime();
                    if (t > 0)tytul += "|[TURBO:" + std::to_string(t).substr(0, 3) + "s]";
                }
                if (enemy.isFrozen()) {
                    float t = enemy.getFreezeRemainingTime();
                    if (t > 0)tytul += "|[ZAMROZENIE:" + std::to_string(t).substr(0, 3) + "s]";
                }
                if (enemy.isSlowed()) {
                    float t = enemy.getSlowRemainingTime();
                    if (t > 0)tytul += "|[BLOTO:" + std::to_string(t).substr(0, 3) + "s]";
                }


                SDL_SetWindowTitle(window, tytul.c_str());

                player.update();
                enemy.update(player.getX(), player.getY());

                // Logika zbierania boosterów (TYLKO LOGIKA, bez rysowania!)
                SDL_Rect pRect = player.getRect();
                for (auto& b : boosters) {
                    if (b.active && SDL_HasIntersection(&pRect, &b.rect)) {
                        // Tutaj decydujemy co robi booster
                        if (b.type == FREEZE) enemy.freeze();
                        else if (b.type == SLOW_ENEMY) enemy.slowDown();
                        else player.applyBooster(b.type); // Tutaj włączamy nieśmiertelność

                        b.active = false; // Usuwamy booster z mapy
                    }
                }

                // 3. KOLIZJA I ZAPIS REKORDU
                // Pobieramy prostokąt przeciwnika do sprawdzenia kolizji
                SDL_Rect eRect = enemy.getRect();

                // Sprawdzamy kolizję gracza z wrogiem (tylko jeśli gracz nie jest nieśmiertelny)
                if (SDL_HasIntersection(&pRect, &eRect) && !player.isInvincible()) {
                    gameOver = true;
                    if (aktualnePunkty > rekordZycia) {
                        zapiszRekord(aktualnePunkty);
                    }
                }
            }
            SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
            SDL_RenderClear(renderer);
            // Rysowanie cian labiryntu i punktow
            for (int r = 0; r < MAP_ROWS; r++) {
                for (int c = 0; c < MAP_COLS; c++) {
                    if (maze[r][c] == 1) {
                        SDL_Rect wall = { c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                        SDL_SetRenderDrawColor(renderer, 210, 180, 140, 255); // Kolor somiany
                        SDL_RenderFillRect(renderer, &wall);
                    }
                    else if (maze[r][c] == 0) { // Rysowanie punktu do zebrania
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        SDL_Rect dot = { c * TILE_SIZE + 32, r * TILE_SIZE + 32, 6, 6 };
                        SDL_RenderFillRect(renderer, &dot);
                    }
                }
            }

            // ==========================================
            // 4. RYSOWANIE BOOSTERÓW (Tutaj faktycznie rysujemy)
            // ==========================================
            for (auto& b : boosters) {
                if (b.active) {
                    // JEŚLI TO TARCZA I MAMY TEKSTURE -> RYSUJEMY OBRAZEK
                    if (b.type == INVINCIBLE && shieldTexture != nullptr) {
                        SDL_RenderCopy(renderer, shieldTexture, NULL, &b.rect);
                    }
                    // JEŚLI TO INNE BOOSTERY -> RYSUJEMY KWADRATY
                    else {
                        if (b.type == SPEED_UP) SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
                        else if (b.type == INVINCIBLE) SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Fallback jeśli tekstura nie działa
                        else if (b.type == SLOW_ENEMY) SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // Brązowy (Błoto)
                        else if (b.type == FREEZE) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);

                        SDL_RenderFillRect(renderer, &b.rect);
                    }
                }
            }

            player.draw(renderer);
            enemy.draw(renderer);
            if (gameOver) {
                // Rysujemy pprzezroczysty czerwony nakad na ekran
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150); // Czerwony z przezroczystoci
                SDL_Rect fullScreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
                SDL_RenderFillRect(renderer, &fullScreen);

                // Tutaj gra stoi w miejscu, bo update'y s zablokowane przez if(!gameOver)
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Sprztanie
    SDL_DestroyTexture(shieldTexture); // Ważne: usuwamy teksturę
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    //KOMENTARZ
    //tak
    //test
    return 0;
}