#include <random>
#include <ctime>
#include <iostream>

#include <SDL/SDL.h>
#include <glm/gtx/rotate_vector.hpp>

#include "Player.h"


Player::Player() { }

Player::~Player() {
	// Delete the projectiles
	for (unsigned int i = 0; i < projectiles.size(); i++) {
		delete projectiles[i];
	}
}

void Player::init(glm::fvec2 pos, GEngine::InputManager* inputManager, GEngine::Camera2D* camera, GEngine::SoundManager* soundManager) {
	m_textureID = GEngine::ResourceManager::getTexture("../assets/Textures/gizmo_right.png").id;
	textureID2 = GEngine::ResourceManager::getTexture("../assets/Textures/gizmo_right.png").id;
	textureID3 = GEngine::ResourceManager::getTexture("../assets/Textures/gizmo_left.png").id;
	textureID4 = GEngine::ResourceManager::getTexture("../assets/Textures/gizmo_right_reverse.png").id;
	textureID5 = GEngine::ResourceManager::getTexture("../assets/Textures/gizmo_left_reverse.png").id;

    width = 40.0f;
    height = 40.0f;

	m_speed = glm::fvec2(0.0f, 0.0f);
    m_position = pos;

	playerStartPos = pos;

    m_inputManager = inputManager;
    m_camera = camera;
	m_color = GEngine::ColorRGBA8(255, 255, 255, 255);

	m_jumpSound = soundManager->loadSoundEffect("../assets/sounds/jump_sound.ogg");
	m_doubleJumpSound = soundManager->loadSoundEffect("../assets/sounds/double_jump_sound.ogg");
	m_normalGravitySound = soundManager->loadSoundEffect("../assets/sounds/normal_gravity_sound.ogg");
	m_reverseGravitySound = soundManager->loadSoundEffect("../assets/sounds/reverse_gravity_sound.ogg");
	m_shootBubbleSound = soundManager->loadSoundEffect("../assets/sounds/shoot_sound.ogg");
	m_bubbleHitSound = soundManager->loadSoundEffect("../assets/sounds/shoot_hit_sound.ogg");
	
	flickerTimer.start();
}

void Player::draw(GEngine::SpriteBatch& spriteBatch) {
	const glm::vec4 uvRect(0.0f, 0.0f, 1.0f, 1.0f);

	glm::vec4 destRect;
	destRect.x = m_position.x;
	destRect.y = m_position.y;
	destRect.z = width;
	destRect.w = height;

	if (upsideDown && direction == "right") {
		m_textureID = textureID4;
	}
	else if (!upsideDown && direction == "right"){
		m_textureID = textureID2;
	}
	else if (upsideDown && direction == "left") {
		m_textureID = textureID5;
	}
	else if (!upsideDown && direction == "left"){
		m_textureID = textureID3;
	}

	spriteBatch.draw(destRect, uvRect, m_textureID, 0.0f, m_color);
}

void Player::update(GEngine::ParticleBatch2D* smokeParticleBatch, GEngine::ParticleBatch2D* bubbleParticleBatch, std::vector<Tile*> tiles, std::vector<Enemy*> enemies, float deltaTime) {
	if (timeSinceFlickerStarted < 0.70f) {
		applyDeathFlicker();
	}
	// Shoot projectile
	if (m_inputManager->isKeyPressed(SDLK_LCTRL)) {
		shootProjectile(bubbleParticleBatch);
	}
	// Apply bend gravity, if player presses W or UP arrow and the gravity is normal
	if ((m_inputManager->isKeyPressed(SDLK_w) || m_inputManager->isKeyPressed(SDLK_UP)) && normalGravity) {
		applyGravityBend(smokeParticleBatch);
	}
	// Apply bend gravity, if player presses S or DOWN arrow and the gravity is reversed
	if ((m_inputManager->isKeyPressed(SDLK_s) || m_inputManager->isKeyPressed(SDLK_DOWN)) && !normalGravity) {
		applyGravityBend(smokeParticleBatch);
	}
    // Space jumps
    if(m_inputManager->isKeyPressed(SDLK_SPACE)) {
        if(jumped) {
            // Double jumping
			applyDoubleJump(smokeParticleBatch);
        }
    }
	// Space jumps
    if(m_inputManager->isKeyDown(SDLK_SPACE)) {
		if (!jumped) {
			// Normal jumping
			applyJump(smokeParticleBatch);
		}
    }

	if (timeSinceFlickerStarted >= 0.5f) {
		// Player is in air, apply gravity
		if (inAir) {
			jumped = true;
			m_speed.y -= gravityAcceleration * deltaTime;
		}

		// Player movement left or right
		updateHorizontalMovement(smokeParticleBatch);

		// Move on Y-axis
		m_position.y += m_speed.y * deltaTime;
	}

    // Assume player is in air, this makes player fall off platform ledges
    inAir = true;

    // Check collisions on Y-axis
    applyCollisions(glm::fvec2(0.0f, m_speed.y), tiles, enemies);

    // Move on X-axis
    m_position.x += m_speed.x * deltaTime;

    // Check collisions on X-axis
	applyCollisions(glm::fvec2(m_speed.x, 0.0f), tiles, enemies);

	// Update projectiles
	for (unsigned int i = 0; i < projectiles.size(); i++) {
		projectiles[i]->update(tiles, enemies, deltaTime);
		if (direction == "right") {
			addBubbles(2.0f, bubbleParticleBatch, glm::fvec2(projectiles.back()->getPosition().x - 15.0f, projectiles.back()->getPosition().y + 12.5f), 1, glm::vec2(0.0f, 1.5f));
		}
		else if (direction == "left") {
			addBubbles(2.0f, bubbleParticleBatch, glm::fvec2(projectiles.back()->getPosition().x + 40.0f, projectiles.back()->getPosition().y + 12.5f), 1, glm::vec2(0.0f, 1.5f));
		}
		// Projectile hits 
		if (projectiles[i]->destroyed || dead) {
			m_bubbleHitSound.play();
			projectiles.clear();
		}
		if (projectiles[i]->getPosition().x < projectiles[i]->startPosition.x - PROJECTILE_REACH ||
			projectiles[i]->getPosition().x > projectiles[i]->startPosition.x + PROJECTILE_REACH) {
			m_bubbleHitSound.play();
			projectiles.clear();
		}
	}
	if (dead) {
		flickerTimer.start();
	}

	timeSinceFlickerStarted = flickerTimer.getTicks() / 1000.f;
}

void Player::addSmoke(float lifeTime, GEngine::ParticleBatch2D* smokeParticleBatch, const glm::vec2& position, int numParticles, glm::vec2 vel) {
	static std::mt19937 randEngine(time(nullptr));
	static std::uniform_real_distribution<float> randAngle(0.0f, 360.0f);

	GEngine::ColorRGBA8 col(255, 255, 255, 255);

	for (int i = 0; i < numParticles; i++) {
		smokeParticleBatch->addParticle(lifeTime, glm::fvec2((position.x + 4) + i * 0.6f, position.y), glm::rotate(vel, randAngle(randEngine)), col, 5.0f);
	}
}

void Player::addBubbles(float lifeTime, GEngine::ParticleBatch2D* bubbleParticleBatch, const glm::vec2& position, int numParticles, glm::vec2 vel) {
	static std::mt19937 randEngine(time(nullptr));
	static std::uniform_real_distribution<float> randAngle(0.0f, 360.0f);

	GEngine::ColorRGBA8 col(255, 255, 255, 255);

	for (int i = 0; i < numParticles; i++) {
		bubbleParticleBatch->addParticle(lifeTime, glm::fvec2(position.x, position.y), glm::rotate(vel, randAngle(randEngine)), col, 12.0f);
	}
}

void Player::shootProjectile(GEngine::ParticleBatch2D* bubbleParticleBatch) {
	if (projectiles.size() == 0) {
		m_shootBubbleSound.play();

		if (!normalGravity) {
			if (direction == "right") {
				projectiles.push_back(new Projectile(glm::fvec2(11.0f, 0.0f), glm::vec2(m_position.x + 42, m_position.y)));
			}
			else if (direction == "left") {
				projectiles.push_back(new Projectile(glm::fvec2(-11.0f, 0.0f), glm::vec2(m_position.x - 42, m_position.y)));
			}
		}
		else {
			if (direction == "right") {
				projectiles.push_back(new Projectile(glm::fvec2(11.0f, 0.0f), glm::vec2(m_position.x + 42, m_position.y)));

			}
			else if (direction == "left") {
				projectiles.push_back(new Projectile(glm::fvec2(-11.0f, 0.0f), glm::vec2(m_position.x - 42, m_position.y)));
				addBubbles(2.0f, bubbleParticleBatch, glm::fvec2(projectiles.back()->getPosition().x + 30.0f, projectiles.back()->getPosition().y), 1, glm::vec2(0.3f, 0.3f));
			}
		}
	}
}

void Player::applyDeathFlicker() {
	if (timeSinceFlickerStarted <= 0.05f) {
		m_color.a = 255;
	}
	else if (timeSinceFlickerStarted > 0.05f && timeSinceFlickerStarted <= 0.1f) {
		m_color.a = 128;
	}
	else if (timeSinceFlickerStarted > 0.1f && timeSinceFlickerStarted <= 0.15f) {
		m_color.a = 0;
	}
	else if (timeSinceFlickerStarted > 0.15f && timeSinceFlickerStarted <= 0.2f) {
		m_color.a = 128;
	}
	else if (timeSinceFlickerStarted > 0.2f && timeSinceFlickerStarted <= 0.25f) {
		m_color.a = 255;
	}
	else if (timeSinceFlickerStarted > 0.25f && timeSinceFlickerStarted <= 0.3f) {
		m_color.a = 128;
	}
	else if (timeSinceFlickerStarted > 0.3f && timeSinceFlickerStarted <= 0.35f) {
		m_color.a = 0;
	}
	else if (timeSinceFlickerStarted > 0.35f && timeSinceFlickerStarted <= 0.4f) {
		m_color.a = 128;
	}
	else if (timeSinceFlickerStarted > 0.4f && timeSinceFlickerStarted <= 0.45f) {
		m_color.a = 255;
	}
	else if (timeSinceFlickerStarted > 0.45f && timeSinceFlickerStarted <= 0.5f) {
		m_color.a = 128;
	}
	else if (timeSinceFlickerStarted > 0.5f && timeSinceFlickerStarted <= 0.55f) {
		m_color.a = 0;
	}
	else if (timeSinceFlickerStarted > 0.55f && timeSinceFlickerStarted <= 0.6f) {
		m_color.a = 128;
	}
	else if (timeSinceFlickerStarted > 0.6f) {
		m_color.a = 255;
	}
}

// TODO: Add freeze for some time, so player doesn't accidentally move fatally when respawning
void Player::respawnAt(glm::vec2 respawnPos) {
	deathFlickerFrames = 60;
	m_speed.x = 0;
	m_speed.y = 0;
	setPosition(respawnPos);
	if (!normalGravity) {
		upsideDown = false;
		gravityAcceleration *= -1;
		normalGravity = true;
	}
}

void Player::updateHorizontalMovement(GEngine::ParticleBatch2D* smokeParticleBatch) {
	// Move left
	if (m_inputManager->isKeyDown(SDLK_a) || m_inputManager->isKeyDown(SDLK_LEFT)) {
		if (!inAir) {
			if (normalGravity) {
				addSmoke(0.2f, smokeParticleBatch, glm::fvec2(getPosition().x + 30.0f, getPosition().y), 5, glm::vec2(0.3f, 0.8f));
			}
			else {
				addSmoke(0.2f, smokeParticleBatch, glm::fvec2(getPosition().x + 30.0f, getPosition().y + 35), 5, glm::vec2(0.3f, 0.8f));
			}
		}
		direction = "left";
		// Apply acceleration
		m_speed.x -= ACCELERATION;
		if (m_speed.x < -MAX_MOVE_SPEED) {
			m_speed.x = -MAX_MOVE_SPEED;
		}
	}
	// Move right
	else if (m_inputManager->isKeyDown(SDLK_d) || m_inputManager->isKeyDown(SDLK_RIGHT)) {
		if (!inAir) {
			if (normalGravity) {
				addSmoke(0.2f, smokeParticleBatch, glm::fvec2(getPosition().x, getPosition().y), 5, glm::vec2(-0.3f, 0.8f));
			}
			else {
				addSmoke(0.2f, smokeParticleBatch, glm::fvec2(getPosition().x, getPosition().y + 35), 5, glm::vec2(0.3f, 0.8f));
			}
		}
		direction = "right";
		// Apply acceleration
		m_speed.x += ACCELERATION;
		if (m_speed.x > MAX_MOVE_SPEED) {
			m_speed.x = MAX_MOVE_SPEED;
		}
	}
	else {
		// Apply de-acceleration
		if (m_speed.x < 0) {
			m_speed.x += ACCELERATION * 2;
			if (m_speed.x >= 0) {
				m_speed.x = 0.0f;
			}
		}
		// Apply de-acceleration
		else {
			m_speed.x -= ACCELERATION * 2;
			if (m_speed.x <= 0) {
				m_speed.x = 0.0f;
			}
		}
	}
}

void Player::applyJump(GEngine::ParticleBatch2D* smokeParticleBatch) {
	m_jumpSound.play();

	inAir = true;
	jumped = true;
	canDoubleJump = true;

	if (normalGravity) {
		addSmoke(1.0f, smokeParticleBatch, glm::fvec2(getPosition().x, getPosition().y + 5), 50, glm::vec2(0.2f, 0.0f));
		m_speed.y = JUMP_SPEED;
	}
	else {
		addSmoke(1.0f, smokeParticleBatch, glm::fvec2(getPosition().x, getPosition().y + 30), 50, glm::vec2(0.2f, 0.0f));
		m_speed.y = -JUMP_SPEED;
	}
}

void Player::applyDoubleJump(GEngine::ParticleBatch2D* smokeParticleBatch) {
    if(canDoubleJump) {
		m_doubleJumpSound.play();

        canDoubleJump = false;
        if(normalGravity) {
			addSmoke(1.0f, smokeParticleBatch, glm::fvec2(getPosition().x, getPosition().y + 5), 50, glm::vec2(0.2f, 0.0f));
            m_speed.y = JUMP_SPEED + 1;
        }
        else {
			addSmoke(1.0f, smokeParticleBatch, glm::fvec2(getPosition().x, getPosition().y + 30), 50, glm::vec2(0.2f, 0.0f));
            m_speed.y = -JUMP_SPEED - 1;
        }
    }

}

void Player::applyGravityBend(GEngine::ParticleBatch2D* smokeParticleBatch) {
	if (normalGravity) {
		if (!jumped && gravityAcceleration > 0) {
			m_reverseGravitySound.play();
			addSmoke(1.0f, smokeParticleBatch, glm::fvec2(getPosition().x, getPosition().y + 5), 50, glm::vec2(0.2f, 0.0f));

			upsideDown = true;
			gravityAcceleration *= -1;
			normalGravity = false;
		}
	}
	else {
		if (!jumped && gravityAcceleration < 0) {
			m_normalGravitySound.play();
			addSmoke(1.0f, smokeParticleBatch, glm::fvec2(getPosition().x, getPosition().y + 30), 50, glm::vec2(0.2f, 0.0f));

			upsideDown = false;
			gravityAcceleration *= -1;
			normalGravity = true;
		}
	}
}

// Collisions
void Player::applyCollisions(glm::fvec2(speed), std::vector<Tile*> tiles, std::vector<Enemy*> enemies) {

	// Collide with level tiles
	for (unsigned int i = 0; i < tiles.size(); i++) {
		if (collideWithTile((int)width, (int)height, tiles[i])) {
			if (tiles[i]->type == SOLID || DISAPPEARING) {
				// Collide from left
				if (speed.x > 0) {
					m_position.x = tiles[i]->getPosition().x - width;
				}
				// Collide from right
				else if (speed.x < 0) {
					m_position.x = tiles[i]->getPosition().x + tiles[i]->width;
				}

				if (normalGravity) {
					// Collide from below
					if (speed.y > 0) {
						m_speed.y = 0;
						m_position.y = tiles[i]->getPosition().y - height;
						inAir = true;
					}
					// Collide from above
					else if (speed.y < 0) {
						m_speed.y = 0;
						m_position.y = tiles[i]->getPosition().y + tiles[i]->height;
						inAir = false;
						jumped = false;
						canDoubleJump = false;
					}
				}
				else {
					// Collide from below
					if (speed.y > 0) {
						m_speed.y = 0;
						m_position.y = tiles[i]->getPosition().y - height;
						inAir = false;
						jumped = false;
						canDoubleJump = false;
					}
					// Collide from above
					else if (speed.y < 0) {
						m_speed.y = 0;
						m_position.y = tiles[i]->getPosition().y + tiles[i]->height;
						inAir = true;
					}
				}
			}
			if (tiles[i]->type == KILL || tiles[i]->type == KILLREVERSE) {
				dead = true;
				tiles[i]->playerDiedOnThisTile = true;
			}
			if (tiles[i]->type == EXIT) {
				finishedLevel = true;
			}
		}
	}
	// Collide with enemies
	for (unsigned int i = 0; i < enemies.size(); i++) {
		if (collideWithEntity((int)width, (int)height, enemies[i])) {
			if (enemies[i]->bubbled) {
				// Destroy the enemy
				enemies[i]->destroyed = true;
				std::cout << "destroy ze enemy" << std::endl;
			}
			else {
				dead = true;
			}
		}
	}
}