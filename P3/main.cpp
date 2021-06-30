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

#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Entity.h"

#define PLATFORM_COUNT 5
#define WALL_COUNT 26

struct GameState {
    Entity *player;
    Entity *platforms;
    Entity *walls;
};

GameState state;

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint fontTextureID;

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
    displayWindow = SDL_CreateWindow("Lunar Lander!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
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
    state.player->position = glm::vec3(0, 3.5f, 0);
    state.player->movement = glm::vec3(0);
    state.player->acceleration = glm::vec3(0, -0.1f, 0);
    state.player->velocity = glm::vec3(0);
    state.player->speed = 1.5f;
    state.player->textureID = LoadTexture("george.png");
    
    state.player->animRight = new int[4] {3, 7, 11, 15};
    state.player->animLeft = new int[4] {1, 5, 9, 13};
    state.player->animUp = new int[4] {2, 6, 10, 14};
    state.player->animDown = new int[4] {0, 4, 8, 12};

    state.player->animIndices = state.player->animDown;
    state.player->animFrames = 4;
    state.player->animIndex = 0;
    state.player->animTime = 0;
    state.player->animCols = 4;
    state.player->animRows = 4;
    
    state.player->height = 0.8f;
    state.player->width = 0.8f;
    
    state.player->jumpPower = 5.0f;
    
    state.platforms = new Entity[PLATFORM_COUNT];
    
    GLuint platformTextureID = LoadTexture("platform.png");
    state.platforms[0].textureID = platformTextureID;
    state.platforms[0].position = glm::vec3(-1, -3.25f, 0);
    
    state.platforms[1].textureID = platformTextureID;
    state.platforms[1].position = glm::vec3(0, -3.25f, 0);
    
    state.platforms[2].textureID = platformTextureID;
    state.platforms[2].position = glm::vec3(1, -3.25f, 0);
    
    state.platforms[3].textureID = platformTextureID;
    state.platforms[3].position = glm::vec3(-3, -3.25f, 0);
    
    state.platforms[4].textureID = platformTextureID;
    state.platforms[4].position = glm::vec3(3, -3.25f, 0);
    
    for (int i = 0; i < PLATFORM_COUNT; i++){
        state.platforms[i].Update(0, NULL, 0, NULL, 0);
        state.platforms[i].entityType = EntityType::PLATFORMS;
    }
    
    state.walls = new Entity[WALL_COUNT];
    
    GLuint wallTextureID = LoadTexture("wall.png");
    state.walls[0].textureID = wallTextureID;
    state.walls[0].position = glm::vec3(-2, -3.25f, 0);
    
    state.walls[1].textureID = wallTextureID;
    state.walls[1].position = glm::vec3(-4, -3.25f, 0);
    
    state.walls[2].textureID = wallTextureID;
    state.walls[2].position = glm::vec3(2, -3.25f, 0);
    
    state.walls[3].textureID = wallTextureID;
    state.walls[3].position = glm::vec3(4, -3.25f, 0);
    
    state.walls[4].textureID = wallTextureID;
    state.walls[4].position = glm::vec3(-5, -3.25f, 0);
    
    state.walls[5].textureID = wallTextureID;
    state.walls[5].position = glm::vec3(-5, -2.25f, 0);
    
    state.walls[6].textureID = wallTextureID;
    state.walls[6].position = glm::vec3(-5, -1.25f, 0);
    
    state.walls[7].textureID = wallTextureID;
    state.walls[7].position = glm::vec3(-5, -0.25f, 0);
    
    state.walls[8].textureID = wallTextureID;
    state.walls[8].position = glm::vec3(-5, 0.25f, 0);
    
    state.walls[9].textureID = wallTextureID;
    state.walls[9].position = glm::vec3(-5, 1.25f, 0);
    
    state.walls[10].textureID = wallTextureID;
    state.walls[10].position = glm::vec3(-5, 2.25f, 0);
    
    state.walls[11].textureID = wallTextureID;
    state.walls[11].position = glm::vec3(-5, 3.25f, 0);
    
    state.walls[12].textureID = wallTextureID;
    state.walls[12].position = glm::vec3(5, -3.25f, 0);
    
    state.walls[13].textureID = wallTextureID;
    state.walls[13].position = glm::vec3(5, -2.25f, 0);
    
    state.walls[14].textureID = wallTextureID;
    state.walls[14].position = glm::vec3(5, -1.25f, 0);
    
    state.walls[15].textureID = wallTextureID;
    state.walls[15].position = glm::vec3(5, -0.25f, 0);
    
    state.walls[16].textureID = wallTextureID;
    state.walls[16].position = glm::vec3(5, 0.25f, 0);
    
    state.walls[17].textureID = wallTextureID;
    state.walls[17].position = glm::vec3(5, 1.25f, 0);
    
    state.walls[18].textureID = wallTextureID;
    state.walls[18].position = glm::vec3(5, 2.25f, 0);
    
    state.walls[19].textureID = wallTextureID;
    state.walls[19].position = glm::vec3(5, 3.25f, 0);
    
    state.walls[20].textureID = wallTextureID;
    state.walls[20].position = glm::vec3(4, 3.25f, 0);
    
    state.walls[21].textureID = wallTextureID;
    state.walls[21].position = glm::vec3(3, 3.25f, 0);
    
    state.walls[22].textureID = wallTextureID;
    state.walls[22].position = glm::vec3(2, 3.25f, 0);
    
    state.walls[23].textureID = wallTextureID;
    state.walls[23].position = glm::vec3(-2, 3.25f, 0);
    
    state.walls[24].textureID = wallTextureID;
    state.walls[24].position = glm::vec3(-3, 3.25f, 0);
    
    state.walls[25].textureID = wallTextureID;
    state.walls[25].position = glm::vec3(-4, 3.25f, 0);
    
    for (int i = 0; i < WALL_COUNT; i++){
        state.walls[i].Update(0, NULL, 0, NULL, 0);
        state.walls[i].entityType = EntityType::WALLS;
    }
    
    fontTextureID = LoadTexture("font1.png");
}

void ProcessInput() {
    
    state.player->movement = glm::vec3(0);
    state.player->acceleration.x = 0.0f;
    
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
                        break;
                }
                break; // SDL_KEYDOWN
        }
    }
    
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT]) {
//        state.player->movement.x = -1.0f;
        state.player->animIndices = state.player->animLeft;
        if(state.player->collidedBottom == false || state.player->collidedLeft == false || state.player->collidedRight == false){
            state.player->acceleration.x = -1.0f;
        }
    }
    else if (keys[SDL_SCANCODE_RIGHT]) {
//        state.player->movement.x = 1.0f;
        state.player->animIndices = state.player->animRight;
        if(state.player->collidedBottom == false || state.player->collidedLeft == false || state.player->collidedRight == false){
           state.player->acceleration.x = 1.0f;
        }
    }
    

    if (glm::length(state.player->movement) > 1.0f) {
        state.player->movement = glm::normalize(state.player->movement);
    }

}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;

void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;
    
    deltaTime += accumulator;
    if (deltaTime < FIXED_TIMESTEP) {
        accumulator = deltaTime;
        return;
    }
    
    while (deltaTime >= FIXED_TIMESTEP) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
        state.player->Update(FIXED_TIMESTEP, state.platforms, PLATFORM_COUNT, state.walls, WALL_COUNT);
        
        deltaTime -= FIXED_TIMESTEP;
    }
    
    accumulator = deltaTime;
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < PLATFORM_COUNT; i++){
        state.platforms[i].Render(&program);
    }
    
    for (int i = 0; i < WALL_COUNT; i++){
        state.walls[i].Render(&program);
    }
    
    state.player->Render(&program);
    
    if (state.player->isDead) {
        DrawText(&program, fontTextureID, "Mission Failed", 0.5f, -0.25f,
            glm::vec3(-2.0, 0, 0));
    }
    else if (state.player->hasWon) {
        DrawText(&program, fontTextureID, "Mission Successful", 0.5f, -0.25f,
            glm::vec3(-2.0, 0, 0));
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
