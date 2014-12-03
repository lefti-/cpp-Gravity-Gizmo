#include <SDL/SDL.h>

#include "Player.h"

#include <iostream>

Player::Player() {
    // Empty
}

Player::~Player() {
    // Empty
}

void Player::init(glm::fvec2 pos, GEngine::InputManager* inputManager, GEngine::Camera2D* camera) {

    textureID = GEngine::ResourceManager::getTexture("Textures/glass.png").id;

    width = 64.0f;
    height = 64.0f;

    runningSpeed = 15.0f;

    _speed.x = 0.0f;
    _speed.y = 0.0f;

    _position = pos;
    _inputManager = inputManager;
    _camera = camera;

    _color.r = 255;
    _color.g = 255;
    _color.b = 255;
    _color.a = 255;
}

void Player::update(std::vector<Tile*> tiles, float deltaTime) {


    // If player has not jumped and presses W.
    if (jumped == false && _inputManager->isKeyPressed(SDLK_w) == true) {
        jump();
    }

    // Player is in air, apply gravity.
    if (inAir) {
        jumped = true;
        _speed.y -= 0.80f * deltaTime;
    }

    // Move left.
    if (_inputManager->isKeyDown(SDLK_a)) {
        _speed.x = -runningSpeed;
    }
    // Move right.
    else if (_inputManager->isKeyDown(SDLK_d)) {
        _speed.x = runningSpeed;
    }
    else {
        _speed.x = 0.0f;
    }

    // Move on Y-axis.
    _position.y += _speed.y * deltaTime;

    // Assume player is in air. This makes player fall off platform ledges.
    inAir = true;

    // Check collision on Y-axis.
    collide(glm::fvec2(0.0f, _speed.y), tiles);

    // Move on X-axis.
    _position.x += _speed.x * deltaTime;

    // Check collision on X-axis.
    collide(glm::fvec2(_speed.x, 0.0f), tiles);
}

void Player::jump() {
    _speed.y = 30.0f;
    inAir = true;
    jumped = true;
}

// Collisions.
void Player::collide(glm::fvec2(speed), std::vector<Tile*> tiles) {

    // Collide with level tiles.
    for (int i = 0; i < tiles.size(); i++) {
        if (collideWithTile(width, height, tiles[i])) {
            // Collide from left.
            if (speed.x > 0) {
                _position.x = tiles[i]->getPosition().x - width;
            }
            // Collide from right.
            else if (speed.x < 0) {
                _position.x = tiles[i]->getPosition().x + tiles[i]->width;
            }

            // Collide from below.
            if (speed.y > 0) {
                _speed.y = 0;
                _position.y = tiles[i]->getPosition().y - height;
                inAir = true;
            }
            // Collide from above.
            else if (speed.y < 0) {
                _speed.y = 0;
                _position.y = tiles[i]->getPosition().y + tiles[i]->height;
                inAir = false;
                jumped = false;
            }
        }
    }
}