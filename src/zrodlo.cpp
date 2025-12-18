#include <SDL.h>
#include <iostream>
const int SCREEN_WIDTH = 800; //Szerokosc okna
const int SCREEN_HEIGHT = 600; //Wysokosc okna
//Klasa Gracza
class Player {
private:
    SDL_Rect rect; //struktura do przechowywania polozenia
    int speed = 10; // predkosc poruszania sie
public://Ustawianie pozycji startowej i predkosci
    Player(int x, int y, int size, int moveSpeed) {
        rect.x = x;
        rect.y = y;
        rect.w = size;
        rect.h = size;
        speed = moveSpeed;
    }

    void handleInput(SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
            case SDLK_UP: rect.y -= speed;
                break;
            case SDLK_DOWN: rect.y += speed;
                break;
            case SDLK_LEFT: rect.x -= speed;
                break;
            case SDLK_RIGHT: rect.x += speed;
                break;
            }
        }
    }
    void update() { //Punkt kontrolny czy nasz ,,traktor" nie wyjezdza poza wymiar ekranu
        if (rect.x < 0) rect.x = 0;
        if (rect.y < 0) rect.y = 0;
        if (rect.x + rect.w > SCREEN_WIDTH) rect.x = SCREEN_WIDTH - rect.w;
        if (rect.y + rect.h > SCREEN_HEIGHT) rect.y = SCREEN_HEIGHT - rect.h;
    }
    void draw(SDL_Renderer* renderer) { //rysowanie gracza w oknie
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);//ustawienie koloru na bialy
        SDL_RenderFillRect(renderer, &rect);
    }
};

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
    Player player(100, 100, 50, 15);
    //zmienna sterujaca gra
    bool running = true;
    SDL_Event e;
    while (running) {
        //Wejscie
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            player.handleInput(e);
        }
        player.update();

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        player.draw(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    // Pauza ¿eby zobaczyæ okno (3 sekundy)
   // SDL_Delay(3000);
    
    // Sprz¹tanie
SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    //KOMENTARZ
    //tak
    return 0;
}