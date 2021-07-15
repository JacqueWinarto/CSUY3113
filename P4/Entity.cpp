#include "Entity.h"

Entity::Entity()
{
    position = glm::vec3(0);
    movement = glm::vec3(0);
    acceleration = glm::vec3(0);
    velocity = glm::vec3(0);
    speed = 0;
    
    modelMatrix = glm::mat4(1.0f);
}

bool Entity::CheckCollision(Entity *other){
    if(isActive == false || other->isActive == false) return false;
    
    float xdist = fabs(position.x - other->position.x) - ((width + other->width) / 2.0f);
    float ydist = fabs(position.y - other->position.y) - ((height + other->height) / 2.0f);
    
    if (xdist < 0 && ydist < 0) return true;
    return false;
}

void Entity::CheckCollisionsY(Entity *objects, int objectCount){
    for (int i = 0; i < objectCount; i++)
    {
        Entity *object = &objects[i];
        
        if (CheckCollision(object)){
            float ydist = fabs(position.y - object->position.y);
            float penetrationY = fabs(ydist - (height / 2.0f) - (object->height / 2.0f));
            if (velocity.y > 0) {
                position.y -= penetrationY;
                velocity.y = 0;
                collidedTop = true;
            }
            else if (velocity.y < 0) {
                position.y += penetrationY;
                velocity.y = 0;
                collidedBottom = true;
            }
        }
    }
}

void Entity::CheckCollisionsX(Entity *objects, int objectCount){
    for (int i = 0; i < objectCount; i++){
        Entity *object = &objects[i];
        if (CheckCollision(object)){
            float xdist = fabs(position.x - object->position.x);
            float penetrationX = fabs(xdist - (width / 2.0f) - (object->width / 2.0f));
            if (velocity.x > 0) {
                position.x -= penetrationX;
                velocity.x = 0;
                collidedRight = true;
            }
            else if (velocity.x < 0) {
                position.x += penetrationX;
                velocity.x = 0;
                collidedLeft = true;
            }
        }
    }
}

void Entity:: JumpEnemy(Entity* enemies, int enemyCount){
    for (int i = 0; i < enemyCount; i++) {
        collidedBottom = false;

        Entity* Enemy = &enemies[i];

        if (CheckCollision(Enemy) && Enemy->isActive) {
            if (velocity.y < 0 && position.y > Enemy->position.y) {
                collidedBottom = true;
                Enemy->isActive = false;
            }

            else {
                isDead = true;
            }
        }

    }
}

void Entity::AIStabber(Entity* player){
    movement = glm::vec3(-1, 0, 0);
    
    if(collidedRight){
        movement = glm::vec3(-1, 0, 0);
    }
    else if (collidedLeft){
        movement = glm::vec3(1, 0, 0);
    }
}

void Entity::AIShooter(Entity *player){
    movement = glm::vec3(1, 0, 0);
    
    if(collidedRight){
        movement = glm::vec3(-1, 0, 0);
    }
    else if (collidedLeft){
        movement = glm::vec3(1, 0, 0);
    }
}

void Entity::AIPuncher(Entity *player){
    switch(aiState){

        case WALKING:
//            if(player->position.x < position.x){
//                movement = glm::vec3(-1, 0, 0);
//            }
//            else{
//                movement = glm::vec3(1, 0, 0);
//            }
            
            movement = glm::vec3(-1, 0, 0);
            
            if(collidedRight){
                movement = glm::vec3(-1, 0, 0);
            }
            else if (collidedLeft){
                movement = glm::vec3(1, 0, 0);
            }
            
            if(glm::distance(position, player->position) < 0.5f){
                aiState = ATTACKING;
            }
            break;

        case ATTACKING:
            player->isDead = true;
            break;

    }
}

void Entity::AI(Entity* player){
    switch (aiType){
            
        case STABBER:
            AIStabber(player);
            break;
            
        case SHOOTER:
            AIShooter(player);
            break;
            
        case PUNCHER:
            AIPuncher(player);
            break;
    }
}



void Entity::Update(float deltaTime, Entity *player, Entity *platforms, int platformCount, Entity *enemies, int enemiesCount){
    
    if(isActive == false) return;
    
    collidedTop = false;
    collidedBottom = false;
    collidedLeft = false;
    collidedRight = false;
    
    if (entityType == ENEMY){
        AI(player);
    }
    
    if (animIndices != NULL) {
        if (glm::length(movement) != 0) {
            animTime += deltaTime;

            if (animTime >= 0.25f)
            {
                animTime = 0.0f;
                animIndex++;
                if (animIndex >= animFrames)
                {
                    animIndex = 0;
                }
            }
        } else {
            animIndex = 0;
        }
    }
    
//    for (int i = 0; i < platformCount; i++){
//        if(CheckCollision(&platforms[i])) return; //if collide, return
//    }
    
    if (entityType == PLAYER){
        if(jump){
            jump = false;
        
            velocity.y += jumpPower;
        }
    
        velocity.x = movement.x * speed;
        velocity += acceleration * deltaTime;
//      position += velocity * deltaTime;
    
        position.y += velocity.y * deltaTime; // Move on Y
        CheckCollisionsY(platforms, platformCount);// Fix if needed
    
        position.x += velocity.x * deltaTime; // Move on X
        CheckCollisionsX(platforms, platformCount);// Fix if needed
        
        JumpEnemy(enemies, enemiesCount);
    }
    
//      for (int i = 0; i < platformCount; i++){
//          Entity *platform = &platforms[i];
//
//          if (CheckCollision(platform)){
//              float ydist = fabs(position.y - platform->position.y);
//              float penetrationY = fabs(ydist - (height / 2.0f) - (platform->height / 2.0f));
//              if (velocity.y > 0) {
//                  position.y -= penetrationY;
//                  velocity.y = 0;
//              }
//              else if (velocity.y < 0) {
//                  position.y += penetrationY;
//                  velocity.y = 0;
//              }
//          }
//      }
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
}

void Entity::DrawSpriteFromTextureAtlas(ShaderProgram *program, GLuint textureID, int index)
{
    float u = (float)(index % animCols) / (float)animCols;
    float v = (float)(index / animCols) / (float)animRows;
    
    float width = 1.0f / (float)animCols;
    float height = 1.0f / (float)animRows;
    
    float texCoords[] = { u, v + height, u + width, v + height, u + width, v,
        u, v + height, u + width, v, u, v};
    
    float vertices[]  = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void Entity::Render(ShaderProgram *program) {
    
    if(isActive == false) return;
    
    program->SetModelMatrix(modelMatrix);
    
    if (animIndices != NULL) {
        DrawSpriteFromTextureAtlas(program, textureID, animIndices[animIndex]);
        return;
    }
    
    float vertices[]  = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}
