#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include<vector>

#include "Entity.h"

#define PLATFORM_COUNT 15
#define ENEMY_COUNT 3

struct GameState {
    Entity *player;
    Entity *platforms;
    Entity *enemies;
};

GameState state;

enum GameStatus { WINNING, LOSING, SLEEPING, RUNNING };
GameStatus status = SLEEPING;

bool isRunning = false;

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint platformTextureID, enemy1TextureID, enemy2TextureID, enemy3TextureID, fontTextureID;

GLuint LoadTexture(const char* filePath) {
    int w, h, n;
    unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);
    
    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n" << std::endl;
        //assert(false);
    }
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    stbi_image_free(image);
    return textureID;
}

void DrawText(ShaderProgram *program, GLuint fontTextureID, std::string text, float size, float spacing, glm::vec3 position){
    float width = 1.0f / 16.0f;
    float height = 1.0f / 16.0f;
    
    std::vector<float> vertices;
    std::vector<float> texCoords;
    
    for(int i = 0; i < text.size(); i++) {
        int index = (int)text[i];
        float offset = (size + spacing) * i;
        
        float u = (float)(index % 16) / 16.0f;
        float v = (float)(index / 16) / 16.0f;
        
        vertices.insert(vertices.end(), {
            offset + (-0.5f * size), 0.5f * size,
            offset + (-0.5f * size), -0.5f * size,
            offset + (0.5f * size), 0.5f * size,
            offset + (0.5f * size), -0.5f * size,
            offset + (0.5f * size), 0.5f * size,
            offset + (-0.5f * size), -0.5f * size,
        });
        
        texCoords.insert(texCoords.end(), {
            u, v,
            u, v + height,
            u + width, v,
            u + width, v + height,
            u + width, v,
            u, v + height,
        });
        
    } // end of for loop
    
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    program->SetModelMatrix(modelMatrix);
    
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, fontTextureID);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("BATTLE!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(0, 0, 640, 480);
    
    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");
    
    viewMatrix = glm::mat4(1.0f);
    modelMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    
    glUseProgram(program.programID);
    
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
   
    // Initialize Game Objects
    
    // Initialize Player
    state.player = new Entity();
    state.player->entityType = PLAYER;
    state.player->position = glm::vec3(-4, -1, 0);
    state.player->movement = glm::vec3(0);
    state.player->acceleration = glm::vec3(0, -9.81f, 0);
    state.player->speed = 1.5f;
    state.player->textureID = LoadTexture("main.jpg");
    
    /*
    state.player->animRight = new int[4] {3, 7, 11, 15};
    state.player->animLeft = new int[4] {1, 5, 9, 13};
    state.player->animUp = new int[4] {2, 6, 10, 14};
    state.player->animDown = new int[4] {0, 4, 8, 12};

    state.player->animIndices = state.player->animRight;
    state.player->animFrames = 4;
    state.player->animIndex = 0;
    state.player->animTime = 0;
    state.player->animCols = 4;
    state.player->animRows = 4;
     */
    
    state.player->height = 0.8f;
    state.player->width = 0.8f;
    
    state.player->jumpPower = 5.0f;
    
    state.platforms = new Entity[PLATFORM_COUNT];
    GLuint platformTextureID = LoadTexture("stone.png");

    for(int i = 0; i < PLATFORM_COUNT - 4; i++){
        state.platforms[i].entityType = PLATFORM;
        state.platforms[i].textureID = platformTextureID;
        state.platforms[i].position = glm::vec3(-5 + i, -3.25f, 0);
    }
    
    state.platforms[11].entityType = PLATFORM;
    state.platforms[11].textureID = platformTextureID;
    state.platforms[11].position = glm::vec3(-1, 0.25f, 0);
    
    state.platforms[12].entityType = PLATFORM;
    state.platforms[12].textureID = platformTextureID;
    state.platforms[12].position = glm::vec3(0, 0.25f, 0);
    
    state.platforms[13].entityType = PLATFORM;
    state.platforms[13].textureID = platformTextureID;
    state.platforms[13].position = glm::vec3(1, 0.25f, 0);
    
    state.platforms[14].entityType = PLATFORM;
    state.platforms[14].textureID = platformTextureID;
    state.platforms[14].position = glm::vec3(2, 0.25f, 0);

    for (int i = 0; i<PLATFORM_COUNT; i++){
        state.platforms[i].Update(0, NULL, NULL, 0, NULL, 0);
    }
    
    state.enemies = new Entity[ENEMY_COUNT];
    GLuint enemy1TextureID = LoadTexture("side1.jpg");
    
    state.enemies[0].entityType = ENEMY;
    state.enemies[0].textureID = enemy1TextureID;
    state.enemies[0].position = glm::vec3(4, -2.45, 0);
    state.enemies[0].speed = 0.5;
    
    state.enemies[0].aiType = STABBER;
    state.enemies[0].aiState = WALKING;
    
    GLuint enemy2TextureID = LoadTexture("side2.jpg");
    
    state.enemies[1].entityType = ENEMY;
    state.enemies[1].textureID = enemy2TextureID;
    state.enemies[1].position = glm::vec3(2, -2.45, 0);
    state.enemies[1].speed = 0.5;

    state.enemies[1].aiType = SHOOTER;
    state.enemies[1].aiState = WALKING;
    
    GLuint enemy3TextureID = LoadTexture("side3.jpg");
    
    state.enemies[2].entityType = ENEMY;
    state.enemies[2].textureID = enemy3TextureID;
    state.enemies[2].position = glm::vec3(0, 1.10, 0);
    state.enemies[2].speed = 0.5;
    
    state.enemies[2].aiType = PUNCHER;
    state.enemies[2].aiState = WALKING;
    
    fontTextureID = LoadTexture("font1.png");
 
}

void ProcessInput() {
    
    state.player->movement = glm::vec3(0);
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                gameIsRunning = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT:
                        // Move the player left
                        break;
                        
                    case SDLK_RIGHT:
                        // Move the player right
                        break;
                        
                    case SDLK_SPACE:
                        //if(state.player->collidedBottom){
                        state.player->jump = true;
                        //}
                        break;
                    case SDLK_b:
                        switch (status) {
                            case WINNING:
                                break;
                                        
                            case LOSING:
                                break;
                    
                            case SLEEPING:
                                status = RUNNING;
                                isRunning = true;
                                break;
                                        
                            case RUNNING:
                                break;
                                    
                        }        
                }
                break; // SDL_KEYDOWN

        }
    }
    
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT]) {
        state.player->movement.x = -1.0f;
        state.player->animIndices = state.player->animLeft;
    }
    else if (keys[SDL_SCANCODE_RIGHT]) {
        state.player->movement.x = 1.0f;
        state.player->animIndices = state.player->animRight;
    }

    if (glm::length(state.player->movement) > 1.0f) {
        state.player->movement = glm::normalize(state.player->movement);
    }

}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;

void Update() {
    
    if (isRunning == true) {
        float ticks = (float)SDL_GetTicks() / 1000.0f;
        float deltaTime = ticks - lastTicks;
        lastTicks = ticks;
        
        int numEnemiesAlive = 0;
        
        deltaTime += accumulator;
        if (deltaTime < FIXED_TIMESTEP) {
            accumulator = deltaTime;
            return;
        }
        
        while (deltaTime >= FIXED_TIMESTEP) {
            // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
            state.player->Update(FIXED_TIMESTEP, state.player, state.platforms, PLATFORM_COUNT, state.enemies, ENEMY_COUNT);
            
            for (int i = 0; i < ENEMY_COUNT; i++){
                state.enemies[i].Update(FIXED_TIMESTEP, state.player, state.platforms, PLATFORM_COUNT, state.enemies, ENEMY_COUNT);
            }
            
            deltaTime -= FIXED_TIMESTEP;
            
            for (int i=0; i < ENEMY_COUNT; i++){
                if (state.enemies[i].isActive == true){
                    numEnemiesAlive ++;
                }
            }
            
            if (numEnemiesAlive == 0){
                isRunning = false;
                status = WINNING;
            }
            
            if (state.player->isDead){
                isRunning = false;
                status = LOSING;
            }
        }
        
        accumulator = deltaTime;
        
    }
    
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i<PLATFORM_COUNT; i++){
        state.platforms[i].Render(&program);
    }
    
    for (int i = 0; i<ENEMY_COUNT; i++){
        state.enemies[i].Render(&program);
    }
    
    state.player->Render(&program);
    
    switch(status){
        case WINNING:
            DrawText(&program, fontTextureID, "Congrats! You won the battle!", 0.4f, -0.25f, glm::vec3(-2.25, 0, 0));
            break;
            
        case LOSING:
            DrawText(&program, fontTextureID, "Oh no! You loss the battle!", 0.4f, -0.25f, glm::vec3(-2.25, 0, 0));
            break;
            
        case SLEEPING:
            DrawText(&program, fontTextureID, "Defeat your opponents! Good luck!", 0.4f, -0.25f, glm::vec3(-2.25, 0, 0));
            DrawText(&program, fontTextureID, "Press B to begin battle", 0.4f, -0.25f, glm::vec3(-1.25, -1, 0));
            break;
        
        case RUNNING:
            break;
    }

    
    SDL_GL_SwapWindow(displayWindow);
    
}


void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();
    
    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }
    
    Shutdown();
    return 0;
}
