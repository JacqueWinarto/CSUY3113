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

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

glm::mat4 ballMatrix, wallOneMatrix, wallTwoMatrix;

// Start at 0, 0, 0
glm::vec3 ball_position = glm::vec3(0, 0, 0);
glm::vec3 wallOne_position = glm::vec3(0, 0, 0);
glm::vec3 wallTwo_position = glm::vec3(0, 0, 0);

// Don’t go anywhere (yet).
glm::vec3 ball_movement = glm::vec3(0, 0, 0);
glm::vec3 wallOne_movement = glm::vec3(0, 0, 0);
glm::vec3 wallTwo_movement = glm::vec3(0, 0, 0);

float ball_height = 0.5f;
float ball_width = 0.5f;
float wall_height = 2.0f;
float wall_width = 0.5f;

float ball_speed = 1.0f;
float wall_speed = 1.0f;

GLuint ballTextureID, wallOneTextureID, wallTwoTextureID;

bool start, end = false;

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

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Pong!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(0, 0, 640, 480);
    
    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");
    
    viewMatrix = glm::mat4(1.0f);
    modelMatrix = glm::mat4(1.0f);
    ballMatrix = glm::mat4(1.0f);
    wallOneMatrix = glm::mat4(1.0f);
    wallTwoMatrix = glm::mat4(1.0f);
    
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    //program.SetColor(1.0f, 0.0f, 0.0f, 1.0f);
    
    glUseProgram(program.programID);
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    glEnable(GL_BLEND);
    
    // Good setting for transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    ballTextureID = LoadTexture("ball.png");
    wallOneTextureID = LoadTexture("wall.jpg");
    wallTwoTextureID = LoadTexture("wall.jpg");
    
    wallOne_position.x = -5;
    wallOne_position.y = 0;
    
    wallTwo_position.x = 5;
    wallTwo_position.y = 0;
    
    ball_position = glm::vec3(0, 0, 0);
    ball_movement = glm::vec3(0, 0, 0);
    
}

void startOver(){
    wallOne_position.x = -5;
    wallOne_position.y = 0;
    
    wallTwo_position.x = 5;
    wallTwo_position.y = 0;
    
    ball_position = glm::vec3(0, 0, 0);
    ball_movement = glm::vec3(0, 0, 0);
    
}

void ProcessInput() {
    
    wallOne_movement = glm::vec3(0, 0, 0);
    wallTwo_movement = glm::vec3(0, 0, 0);
    
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
                        // Some sort of action
                        start = true;
                        
                }
                break; // SDL_KEYDOWN
        }
    }
    
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_W]) {
        wallOne_movement.y = 1.0f;
    }
    else if (keys[SDL_SCANCODE_S]) {
        wallOne_movement.y = -1.0f;
    }
    
    if (keys[SDL_SCANCODE_UP]) {
        wallTwo_movement.y = 1.0f;
    }
    else if (keys[SDL_SCANCODE_DOWN]) {
        wallTwo_movement.y = -1.0f;
    }
    
    if (glm::length(wallOne_movement) > 1.0f) {
        wallOne_movement = glm::normalize(wallOne_movement);
    }
    
    if (glm::length(wallTwo_movement) > 1.0f) {
        wallTwo_movement = glm::normalize(wallTwo_movement);
    }
    
}

void moveBall(){
    int val = rand() % 4;
    switch (val) {
    case 0:
            ball_movement = normalize(glm::vec3(1, 1, 0));
            break;
    case 1:
            ball_movement = normalize(glm::vec3(1, -1, 0));
            break;
    case 2:
            ball_movement = normalize(glm::vec3(-1, 1, 0));
            break;
    case 3:
            ball_movement = normalize(glm::vec3(-1, -1, 0));
            break;
    }
    start = false;
}

void wallCollision(){
    float xdist1 = fabs((float)ball_position.x - (float)wallOne_position.x) - ((ball_width + wall_width) / 2.0f);
    float ydist1 = fabs((float)ball_position.y - (float)wallOne_position.y) - ((ball_height + wall_height) / 2.0f);

    float xdist2 = fabs((float)ball_position.x - (float)wallTwo_position.x) - ((ball_width + wall_width) / 2.0f);
    float ydist2 = fabs((float)ball_position.y - (float)wallTwo_position.y) - ((ball_height + wall_height) / 2.0f);

    if (xdist1 < 0 && ydist1 < 0){
        ball_movement.x = 1;
    }

    if (xdist2 < 0 && ydist2 < 0){
        ball_movement.x = -1;
    }

}

void worldCollision(){
    if(ball_position.x + ball_width / 2.0f >= 5.0f){
        end = true;
        startOver();
    }
    
    else if(ball_position.x - ball_width / 2.0f <= -5.0f){
        end = true;
        startOver();
    }

}



float lastTicks;

void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;
    
    if (start){
        moveBall();
    }
    
    wallCollision();
    worldCollision();
    
    if(!end){
        if(ball_position.y + ball_height / 2.0f >= 3.75f){
            ball_movement.y = -1;
        }
        
        if(ball_position.y - ball_height / 2.0f <= -3.75f){
            ball_movement.y = 1;
        }
        
        // Add (direction * units per second * elapsed time)
        ball_position += ball_movement * ball_speed * deltaTime;
        
        if(wallOne_position.y + wall_height / 2.0f >= 3.75){
            wallOne_position.y = 3.72f - wall_height / 2.0f;
        }
        
        else if (wallOne_position.y - wall_height / 2.0f <= -3.75){
            wallOne_position.y = 3.72f + wall_height / 2.0f;
        }
        
        wallOne_position += wallOne_movement * wall_speed * deltaTime;
        
        if(wallTwo_position.y + wall_height / 2.0f >= 3.75){
            wallTwo_position.y = 3.72f - wall_height / 2.0f;
        }
        
        else if (wallTwo_position.y - wall_height / 2.0f <= -3.75){
            wallTwo_position.y = 3.72f + wall_height / 2.0f;
        }
        
        wallTwo_position += wallTwo_movement * wall_speed * deltaTime;
        
    }
   
    ballMatrix = glm::mat4(1.0f);
    wallOneMatrix = glm::mat4(1.0f);
    wallTwoMatrix = glm::mat4(1.0f);
    
    ballMatrix = glm::translate(ballMatrix, ball_position);
    wallOneMatrix = glm::translate(wallOneMatrix, wallOne_position);
    wallTwoMatrix = glm::translate(wallTwoMatrix, wallTwo_position);

}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
    
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);
    
    program.SetModelMatrix(ballMatrix);
    glBindTexture(GL_TEXTURE_2D, ballTextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    program.SetModelMatrix(wallOneMatrix);
    glBindTexture(GL_TEXTURE_2D, wallOneTextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    program.SetModelMatrix(wallTwoMatrix);
    glBindTexture(GL_TEXTURE_2D, wallTwoTextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    
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
