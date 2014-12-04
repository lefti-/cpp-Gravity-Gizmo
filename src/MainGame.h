#pragma once

#include <GEngine/Window.h>
#include <GEngine/GLSLProgram.h>
#include <GEngine/Camera2D.h>
#include <GEngine/InputManager.h>
#include <GEngine/SpriteBatch.h>

#include "Player.h"
#include "Tile.h"
#include "Level.h"
#include "Enemy.h"


enum class GameState {
    PLAY,
    EXIT
};

class MainGame {
public:
    MainGame();
    ~MainGame();

    // Runs the game
    void run();

private:
    // Initializes the core systems
    void initSystems();

    // Initializes the level and sets up everything
    void initLevel();

    // Initializes the shaders
    void initShaders();

    // Main game loop for the program
    void gameLoop();

    // Handles input processing
    void processInput();

    // Renders the game
    void drawGame();

    // Member Variables
    GEngine::Window _window; // The game window

    GEngine::GLSLProgram _textureProgram; // The shader program

    GEngine::InputManager _inputManager; // Handles input

    GEngine::Camera2D _camera; // Main camera

    GEngine::SpriteBatch _mainSpriteBatch;

    std::vector<Level*> _levels; // Vector of all levels

	GameState _gameState;

    Player* _player;

	std::vector<Enemy*> _enemies; // Vector of all enemies

    int _screenWidth, _screenHeight;
    float _fps;
    int _currentLevel;
    int _frameCounter = 0;
};

