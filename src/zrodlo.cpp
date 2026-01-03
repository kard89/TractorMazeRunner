#include <SDL.h>
#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>
#include <map>
#include <fstream>  // Do obsługi pliku rekord.txt
#include <string>   // Do zamiany liczb na tekst w tytule okna

const int SCREEN_WIDTH = 800; //Szerokosc okna
const int SCREEN_HEIGHT = 600; //Wysokosc okna
//Klasa Gracza
const int TILE_SIZE = 50;
const int MAP_ROWS = 12;
const int MAP_COLS = 16;

//Struktura Boosterow
enum BoosterType { SPEED_UP, INVINCIBLE, SHRINK, FREEZE, NONE };
struct Booster {
    SDL_Rect rect;
    BoosterType type;
    bool active;
};

int maze[MAP_ROWS][MAP_COLS] = {
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
};

class Player {
private:
    SDL_Rect rect; //struktura do przechowywania polozenia
    int baseSpeed;
    int currentSpeed;
    bool invincible = false;
    Uint32 effectTimer = 0;
public://Ustawianie pozycji startowej i predkosci
    Player(int x, int y, int size, int moveSpeed) {
        rect.x = x;
        rect.y = y;
        rect.w = size;
        rect.h = size;
        baseSpeed = moveSpeed;
        currentSpeed = moveSpeed;
    }
    void applyBooster(BoosterType type) {
        effectTimer = SDL_GetTicks() + 5000; // Efekt trwa 5 sekund
        switch (type) {
        case SPEED_UP:   currentSpeed = baseSpeed * 2; break;
        case INVINCIBLE: invincible = true; break;
        case SHRINK:     rect.w = 15; rect.h = 15; break;
        default: break;
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
        if (SDL_GetTicks() > effectTimer) {
            currentSpeed = baseSpeed;
            invincible = false;
            if (rect.w != 35) { rect.w = 35; rect.h = 35; }
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
    //Funkcja umozliwaja poznanie antagnoiscie pozycje gracza
    int getX() { return rect.x; }
    int getY() { return rect.y; }
    SDL_Rect getRect() { return rect; }
    bool isInvincible() { return invincible; }
};

class Enemy {
private:
    SDL_Rect rect;
    int speed;
    bool frozen = false;
    Uint32 freezeTimer = 0;
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
                if (nx >= 0 && nx < MAP_COLS && ny >= 0 && ny < MAP_ROWS && maze[ny][nx] == 0 && !visited[ny][nx]) {
                    visited[ny][nx] = true;
                    parentMap[ny * MAP_COLS + nx] = curr;
                    q.push({ nx, ny });
                }
            }
        }

        if (found) {
            SDL_Point curr = { tCol, tRow };
            while (curr.x != sCol || curr.y != sRow) {
                path.push_back({ curr.x * TILE_SIZE + 10, curr.y * TILE_SIZE + 10 });
                curr = parentMap[curr.y * MAP_COLS + curr.x];
            }
            std::reverse(path.begin(), path.end());
        }
    }

public:
    Enemy(int x, int y, int size, int moveSpeed) {
        rect = { x, y, size, size };
        speed = moveSpeed;
    }
    SDL_Rect getRect() { return rect; }
    void freeze() {
        frozen = true;
        freezeTimer = SDL_GetTicks() + 5000;
    }

    void update(int playerX, int playerY) {
        if (frozen) {
            if (SDL_GetTicks() > freezeTimer) frozen = false;
            else return;
        }

        // Obliczaj now ciek co 30 klatek (eby nie obcia procesora)
        static int frameCounter = 0;
        if (frameCounter++ % 30 == 0) {
            findPath(rect.x, rect.y, playerX, playerY);
        }

        if (!path.empty()) {
            SDL_Point target = path[0];
            if (rect.x < target.x) rect.x += speed;
            else if (rect.x > target.x) rect.x -= speed;

            if (rect.y < target.y) rect.y += speed;
            else if (rect.y > target.y) rect.y -= speed;

            // Jeli dotar do punktu kontrolnego cieki, usu go i id do nastpnego
            if (abs(rect.x - target.x) < speed && abs(rect.y - target.y) < speed) {
                path.erase(path.begin());
            }
        }
    }

    void draw(SDL_Renderer* renderer) {
        if (frozen) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // Niebieski jeśli zamrożony
        else SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect);
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
    //Inicjalizacja obiektow
    Player player(60, 60, 35, 10);
    Enemy enemy(700, 500, 30, 2);
    //Inicjalizacja boosterow
    std::vector<Booster> boosters;
    boosters.push_back({ {150, 65, 25, 25}, SPEED_UP, true });
    boosters.push_back({ {350, 165, 25, 25}, INVINCIBLE, true });
    boosters.push_back({ {65, 515, 25, 25}, SHRINK, true });
    boosters.push_back({ {715, 65, 25, 25}, FREEZE, true });
    //zmienna sterujaca gra
    bool running = true;
    bool gameOver = false;
    SDL_Event e;
    int rekordZycia = wczytajRekord(); // Ładujemy stary rekord na start
    Uint32 startTime = SDL_GetTicks(); // Zapamiętujemy moment startu
    int aktualnePunkty = 0;
    while (running) {
        //Wejscie
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (!gameOver) player.handleInput(e);
        }

        if (!gameOver) {
            // 1. OBLICZANIE PUNKTÓW (1000ms = 1 sekunda, razy 10 punktów)
            Uint32 czasTrwania = SDL_GetTicks() - startTime;
            aktualnePunkty = (czasTrwania / 1000) * 10;

            // 2. AKTUALIZACJA TYTUŁU OKNA
            // Sklejamy tekst: Punkty + Rekord
            std::string tytul = "Traktorzysta | Punkty: " + std::to_string(aktualnePunkty) + " | Rekord: " + std::to_string(rekordZycia);
            SDL_SetWindowTitle(window, tytul.c_str());

            player.update();
            enemy.update(player.getX(), player.getY());

            // Logika zbierania boosterów
            SDL_Rect pRect = player.getRect();
            for (auto& b : boosters) {
                if (b.active && SDL_HasIntersection(&pRect, &b.rect)) {
                    if (b.type == FREEZE) enemy.freeze();
                    else player.applyBooster(b.type);
                    b.active = false;
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
        // Rysowanie cian labiryntu
        for (int r = 0; r < MAP_ROWS; r++) {
            for (int c = 0; c < MAP_COLS; c++) {
                if (maze[r][c] == 1) {
                    SDL_Rect wall = { c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                    SDL_SetRenderDrawColor(renderer, 210, 180, 140, 255); // Kolor somiany
                    SDL_RenderFillRect(renderer, &wall);
                }
            }
        }
        //rysowanie boosterow
        for (auto& b : boosters) {
            if (b.active) {
                if (b.type == SPEED_UP) SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
                else if (b.type == INVINCIBLE) SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                else if (b.type == SHRINK) SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
                else if (b.type == FREEZE) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
                SDL_RenderFillRect(renderer, &b.rect);
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
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Sprztanie
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    //KOMENTARZ
    //tak
    return 0;
}