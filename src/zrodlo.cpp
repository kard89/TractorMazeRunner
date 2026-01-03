#include <SDL.h>
#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>
#include <map>
#include <fstream>
#include <string>

const int SCREEN_WIDTH = 1120;
const int SCREEN_HEIGHT = 840;
const int TILE_SIZE = 70;
const int MAP_ROWS = 12;
const int MAP_COLS = 16;

// --- ZMIANA 1: Zastąpienie SHRINK nowym typem SLOW_ENEMY ---
enum BoosterType { SPEED_UP, INVINCIBLE, SLOW_ENEMY, FREEZE, NONE };

struct Booster {
    SDL_Rect rect;
    BoosterType type;
    bool active;
};

int maze[MAP_ROWS][MAP_COLS] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,1,0,0,0,0,0,0,0,1,0,0,1},
    {1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,1},
    {1,0,1,0,0,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,0,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,1},
    {1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,0,0,1,1,1,1,0,1,1,1,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

class Player {
private:
    SDL_Rect rect;
    int baseSpeed;
    int currentSpeed;
    bool invincible = false;
    Uint32 effectTimer = 0;
public:
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
            // --- ZMIANA 2: Usunięto case SHRINK ---
        default: break;
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
                    if (maze[r][c] == 1) {
                        SDL_Rect wall = { c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                        if (SDL_HasIntersection(&rect, &wall)) {
                            rect.x = oldX;
                            rect.y = oldY;
                        }
                    }
                }
            }
        }
    }

    void update() {
        if (SDL_GetTicks() > effectTimer) {
            currentSpeed = baseSpeed;
            invincible = false;
            // --- ZMIANA 3: Usunięto kod przywracający rozmiar gracza (rect.w/rect.h) ---
            // Nie musimy już sprawdzać if (rect.w != 50)
        }

        if (rect.x < 0) rect.x = 0;
        if (rect.y < 0) rect.y = 0;
        if (rect.x + rect.w > SCREEN_WIDTH) rect.x = SCREEN_WIDTH - rect.w;
        if (rect.y + rect.h > SCREEN_HEIGHT) rect.y = SCREEN_HEIGHT - rect.h;
    }

    void draw(SDL_Renderer* renderer) {
        if (invincible) SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        else SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &rect);
    }

    int getX() { return rect.x; }
    int getY() { return rect.y; }
    SDL_Rect getRect() { return rect; }
    bool isInvincible() { return invincible; }
};

class Enemy {
private:
    SDL_Rect rect;
    int baseSpeed; // Przechowujemy bazową prędkość
    int currentSpeed;
    bool frozen = false;
    bool slowed = false; // --- ZMIANA 4: Flaga spowolnienia ---
    Uint32 effectTimer = 0;
    std::vector<SDL_Point> path;

    void findPath(int startX, int startY, int targetX, int targetY) {
        path.clear();
        int sCol = startX / TILE_SIZE;
        int sRow = startY / TILE_SIZE;
        int tCol = targetX / TILE_SIZE;
        int tRow = targetY / TILE_SIZE;

        if (sCol == tCol && sRow == tRow) return;

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
        baseSpeed = moveSpeed;
        currentSpeed = moveSpeed;
    }
    SDL_Rect getRect() { return rect; }

    void freeze() {
        frozen = true;
        slowed = false; // Resetujemy inne stany
        effectTimer = SDL_GetTicks() + 5000;
    }

    // --- ZMIANA 5: Funkcja spowalniająca ---
    void slowDown() {
        slowed = true;
        frozen = false;
        effectTimer = SDL_GetTicks() + 5000;
        currentSpeed = baseSpeed / 2; // Połowa prędkości
        if (currentSpeed < 1) currentSpeed = 1; // Żeby się nie zatrzymał całkowicie
    }

    void update(int playerX, int playerY) {
        // Obsługa timerów efektów
        if (frozen || slowed) {
            if (SDL_GetTicks() > effectTimer) {
                frozen = false;
                slowed = false;
                currentSpeed = baseSpeed;
            }
        }

        if (frozen) return; // Jak zamrożony, to nie wykonuje ruchu

        // Obliczaj ścieżkę co 30 klatek
        static int frameCounter = 0;
        if (frameCounter++ % 30 == 0) {
            findPath(rect.x, rect.y, playerX, playerY);
        }

        if (!path.empty()) {
            SDL_Point target = path[0];

            // Używamy currentSpeed zamiast speed
            if (rect.x < target.x) rect.x += currentSpeed;
            else if (rect.x > target.x) rect.x -= currentSpeed;

            if (rect.y < target.y) rect.y += currentSpeed;
            else if (rect.y > target.y) rect.y -= currentSpeed;

            // Sprawdzamy dotarcie do punktu z uwzględnieniem aktualnej prędkości
            if (abs(rect.x - target.x) < currentSpeed + 1 && abs(rect.y - target.y) < currentSpeed + 1) {
                path.erase(path.begin());
            }
        }
    }

    void draw(SDL_Renderer* renderer) {
        if (frozen) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // Niebieski (Lód)
        else if (slowed) SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // --- ZMIANA 6: Brązowy (Błoto) ---
        else SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Czerwony (Normalny)
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
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Traktorem przez wies",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window) return -1;

    Player player(80, 80, 50, 10);
    Enemy enemy(990, 710, 50, 2);

    std::vector<Booster> boosters;
    boosters.push_back({ {225, 85, 40, 40}, SPEED_UP, true });
    boosters.push_back({ {505, 225, 40, 40}, INVINCIBLE, true });
    // --- ZMIANA 7: Podmiana SHRINK na SLOW_ENEMY w wektorze ---
    boosters.push_back({ {85, 715, 40, 40}, SLOW_ENEMY, true });
    boosters.push_back({ {995, 85, 40, 40}, FREEZE, true });

    bool running = true;
    bool gameOver = false;
    SDL_Event e;
    int rekordZycia = wczytajRekord();
    Uint32 startTime = SDL_GetTicks();
    int aktualnePunkty = 0;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (!gameOver) player.handleInput(e);
        }

        if (!gameOver) {
            Uint32 czasTrwania = SDL_GetTicks() - startTime;
            aktualnePunkty = (czasTrwania / 1000) * 10;

            std::string tytul = "Traktorzysta | Punkty: " + std::to_string(aktualnePunkty) + " | Rekord: " + std::to_string(rekordZycia);
            SDL_SetWindowTitle(window, tytul.c_str());

            player.update();
            enemy.update(player.getX(), player.getY());

            SDL_Rect pRect = player.getRect();
            for (auto& b : boosters) {
                if (b.active && SDL_HasIntersection(&pRect, &b.rect)) {
                    // --- ZMIANA 8: Obsługa nowego boostera ---
                    if (b.type == FREEZE) enemy.freeze();
                    else if (b.type == SLOW_ENEMY) enemy.slowDown();
                    else player.applyBooster(b.type);

                    b.active = false;
                }
            }

            SDL_Rect eRect = enemy.getRect();
            if (SDL_HasIntersection(&pRect, &eRect) && !player.isInvincible()) {
                gameOver = true;
                if (aktualnePunkty > rekordZycia) {
                    zapiszRekord(aktualnePunkty);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
        SDL_RenderClear(renderer);

        for (int r = 0; r < MAP_ROWS; r++) {
            for (int c = 0; c < MAP_COLS; c++) {
                if (maze[r][c] == 1) {
                    SDL_Rect wall = { c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                    SDL_SetRenderDrawColor(renderer, 210, 180, 140, 255);
                    SDL_RenderFillRect(renderer, &wall);
                }
            }
        }

        for (auto& b : boosters) {
            if (b.active) {
                if (b.type == SPEED_UP) SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
                else if (b.type == INVINCIBLE) SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                // --- ZMIANA 9: Kolor dla SLOW_ENEMY (Brązowy - Błoto) ---
                else if (b.type == SLOW_ENEMY) SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
                else if (b.type == FREEZE) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
                SDL_RenderFillRect(renderer, &b.rect);
            }
        }

        player.draw(renderer);
        enemy.draw(renderer);

        if (gameOver) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150);
            SDL_Rect fullScreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
            SDL_RenderFillRect(renderer, &fullScreen);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}