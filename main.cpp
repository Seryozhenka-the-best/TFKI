#include "Asteroids.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <iostream>
#include <sstream>

// Global animations
Animation sExplosion, sRock, sRock_small, sBullet, sHomingBullet;
Animation sPlayer, sPlayer_go, sExplosion_ship, sBossRock, sBossExplosion;

// Initialize global game state
std::list<std::unique_ptr<Entity>> entities;
bool bossSpawned = false;
int asteroidsShotDirectly = 0;
int asteroidsDestroyedInExplosions = 0;
int maxAsteroidsDestroyed = 0;
int activeBossCount = 0;

bool isCollide(const Entity* a, const Entity* b) {
    return (b->x - a->x) * (b->x - a->x) +
           (b->y - a->y) * (b->y - a->y) <
           (a->R + b->R) * (a->R + b->R);
}

void spawnInitialAsteroids() {
    for (int i = 0; i < INITIAL_ASTEROIDS; i++) {
        auto a = std::make_unique<Asteroid>();
        a->settings(sRock, rand() % W, rand() % H, rand() % 360, 25);
        entities.push_back(std::move(a));
    }
}

void spawnBossAsteroid() {
    auto boss = std::make_unique<BossAsteroid>();
    boss->settings(sBossRock, rand() % (W-200) + 100, rand() % (H-200) + 100, rand() % 360, 80);
    boss->dx = (rand() % 5 - 2) * 0.5f;
    boss->dy = (rand() % 5 - 2) * 0.5f;
    entities.push_back(std::move(boss));
    activeBossCount++;
    bossSpawned = true;
}

void drawBossHealth(sf::RenderWindow& app, const BossAsteroid* boss, sf::Font& font) {
    // Health text
    sf::Text bossHealthText("BOSS HP: " + std::to_string(boss->health), font, 24);
    bossHealthText.setPosition(boss->x - 50, boss->y - 60);
    bossHealthText.setFillColor(sf::Color::Red);
    app.draw(bossHealthText);

    // Health bar background
    sf::RectangleShape healthBarBack(sf::Vector2f(100, 10));
    healthBarBack.setPosition(boss->x - 50, boss->y - 40);
    healthBarBack.setFillColor(sf::Color(50, 50, 50));
    app.draw(healthBarBack);

    // Health bar
    float healthPercentage = static_cast<float>(boss->health) / BOSS_MAX_HEALTH;
    sf::RectangleShape healthBar(sf::Vector2f(100 * healthPercentage, 10));
    healthBar.setPosition(boss->x - 50, boss->y - 40);
    healthBar.setFillColor(sf::Color::Red);
    app.draw(healthBar);
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    sf::RenderWindow app(sf::VideoMode(W, H), "Asteroids!");
    app.setFramerateLimit(60);

    // Load textures
    sf::Texture t1, t2, t3, t4, t5, t6, t7, t8;
    if (!t1.loadFromFile("spaceship.png") || !t2.loadFromFile("background.jpg") ||
        !t3.loadFromFile("explosions/type_C.png") || !t4.loadFromFile("rock.png") ||
        !t5.loadFromFile("fire_blue.png") || !t6.loadFromFile("rock_small.png") ||
        !t7.loadFromFile("explosions/type_B.png") || !t8.loadFromFile("fire_red.png")) {
        return EXIT_FAILURE;
    }

    t1.setSmooth(true);
    t2.setSmooth(true);

    // Initialize animations
    sExplosion = Animation(t3, 0, 0, 256, 256, 48, 0.5);
    sRock = Animation(t4, 0, 0, 64, 64, 16, 0.2);
    sRock_small = Animation(t6, 0, 0, 64, 64, 16, 0.2);
    sBullet = Animation(t5, 0, 0, 32, 64, 16, 0.8);
    sHomingBullet = Animation(t8, 0, 0, 32, 64, 16, 0.8);
    sPlayer = Animation(t1, 40, 0, 40, 40, 1, 0);
    sPlayer_go = Animation(t1, 40, 40, 40, 40, 1, 0);
    sExplosion_ship = Animation(t7, 0, 0, 192, 192, 64, 0.5);
    sBossRock = sRock;
    sBossRock.sprite.setScale(2.0f, 2.0f);
    sBossExplosion = sExplosion_ship;
    sBossExplosion.sprite.setScale(3.0f, 3.0f);

    // Game state
    bool gamePaused = false;
    bool pauseButtonHovered = false;
    float brightness = 1.0f;
    bool soundEnabled = true;
    float volume = 70.0f;
    bool volumeSliderDragging = false;

    // Load font
    sf::Font font;
    if (!font.loadFromFile("Roboto-Bold.ttf")) {
        std::cerr << "Failed to load font! Using default" << std::endl;
        font = sf::Font();
    }

    // UI elements
    sf::Text scoreText, recordText, pauseText, soundText, brightnessText, volumeText;
    scoreText.setFont(font);
    recordText.setFont(font);
    pauseText.setFont(font);
    soundText.setFont(font);
    brightnessText.setFont(font);
    volumeText.setFont(font);

    scoreText.setCharacterSize(24);
    recordText.setCharacterSize(24);
    pauseText.setCharacterSize(20);
    soundText.setCharacterSize(20);
    brightnessText.setCharacterSize(20);
    volumeText.setCharacterSize(20);

    scoreText.setFillColor(sf::Color::White);
    recordText.setFillColor(sf::Color::White);
    pauseText.setFillColor(sf::Color::White);
    soundText.setFillColor(sf::Color::White);
    brightnessText.setFillColor(sf::Color::White);
    volumeText.setFillColor(sf::Color::White);

    scoreText.setPosition(20, 20);
    recordText.setPosition(20, 50);
    pauseText.setPosition(W - 110, 25);
    soundText.setPosition(W/2 - 150, H/2 + 55);
    brightnessText.setPosition(W/2 + 20, H/2 + 55);
    volumeText.setPosition(W/2 - 150, H/2 + 105);

    pauseText.setString("PAUSE");
    soundText.setString(soundEnabled ? "SOUND: ON" : "SOUND: OFF");
    brightnessText.setString("BRIGHTNESS: " + std::to_string(int(brightness * 100)) + "%");
    volumeText.setString("VOLUME: " + std::to_string(int(volume)) + "%");

    // Volume slider
    sf::RectangleShape volumeSliderBackground(sf::Vector2f(200, 10));
    volumeSliderBackground.setPosition(W/2 - 100, H/2 + 130);
    volumeSliderBackground.setFillColor(sf::Color(100, 100, 100));

    sf::RectangleShape volumeSlider(sf::Vector2f(10, 20));
    volumeSlider.setPosition(W/2 - 100 + volume * 2, H/2 + 125);
    volumeSlider.setFillColor(sf::Color::White);

    // Buttons
    sf::RectangleShape pauseButton(sf::Vector2f(120, 50));
    pauseButton.setPosition(W - 130, 20);
    pauseButton.setFillColor(sf::Color(70, 70, 70, 220));
    pauseButton.setOutlineThickness(2);
    pauseButton.setOutlineColor(sf::Color::White);

    sf::RectangleShape soundButton(sf::Vector2f(130, 40));
    soundButton.setPosition(W/2 - 160, H/2 + 50);
    soundButton.setFillColor(sf::Color(50, 50, 50, 200));
    soundButton.setOutlineThickness(2);
    soundButton.setOutlineColor(sf::Color::White);

    sf::RectangleShape brightnessButton(sf::Vector2f(200, 40));
    brightnessButton.setPosition(W/2 + 10, H/2 + 50);
    brightnessButton.setFillColor(sf::Color(50, 50, 50, 200));
    brightnessButton.setOutlineThickness(2);
    brightnessButton.setOutlineColor(sf::Color::White);

    // Audio
    sf::Music backgroundMusic, pauseMusic;
    sf::SoundBuffer shootBuffer;
    sf::Sound shootSound;

    if (!backgroundMusic.openFromFile("doom.ogg")) {
        std::cerr << "Failed to load background music!" << std::endl;
    } else {
        backgroundMusic.setLoop(true);
        backgroundMusic.setVolume(volume * 0.7f);
        backgroundMusic.play();
    }

    if (!pauseMusic.openFromFile("pause.ogg")) {
        std::cerr << "Failed to load pause music!" << std::endl;
    } else {
        pauseMusic.setLoop(true);
        pauseMusic.setVolume(volume * 0.7f);
    }

    if (!shootBuffer.loadFromFile("blaster.ogg")) {
        std::cerr << "Failed to load shooting sound!" << std::endl;
    } else {
        shootSound.setBuffer(shootBuffer);
        shootSound.setVolume(volume);
    }

    // Initial entities
    spawnInitialAsteroids();

    auto p = std::make_unique<Player>();
    p->settings(sPlayer, W/2, H/2, 0, 20);
    Player* playerPtr = p.get();
    entities.push_back(std::move(p));

    // Game timing
    float shootCooldown = 0.0f;
    const float shootCooldownTime = 0.15f;
    float homingShootCooldown = 0.0f;
    const float homingShootCooldownTime = 1.5f;

    sf::Clock clock;

    // Main game loop
    while (app.isOpen()) {
        float dt = clock.restart().asSeconds();

        // Event handling
        sf::Event event;
        while (app.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                app.close();
            }

            // Pause button handling
            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2f mousePos = app.mapPixelToCoords(
                    sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

                if (pauseButton.getGlobalBounds().contains(mousePos)) {
                    gamePaused = !gamePaused;
                    pauseText.setString(gamePaused ? "RESUME" : "PAUSE");

                    if (gamePaused) {
                        backgroundMusic.pause();
                        if (soundEnabled) pauseMusic.play();
                    } else {
                        pauseMusic.stop();
                        if (soundEnabled) backgroundMusic.play();
                    }
                    continue;
                }

                if (gamePaused) {
                    if (volumeSlider.getGlobalBounds().contains(mousePos)) {
                        volumeSliderDragging = true;
                    }
                    else if (soundButton.getGlobalBounds().contains(mousePos)) {
                        soundEnabled = !soundEnabled;
                        soundText.setString(soundEnabled ? "SOUND: ON" : "SOUND: OFF");
                        backgroundMusic.setVolume(soundEnabled ? volume * 0.7f : 0);
                        pauseMusic.setVolume(soundEnabled ? volume * 0.7f : 0);
                        shootSound.setVolume(soundEnabled ? volume : 0);
                        if (gamePaused && soundEnabled) pauseMusic.play();
                    }
                    else if (brightnessButton.getGlobalBounds().contains(mousePos)) {
                        brightness = brightness >= 1.0f ? 0.5f : brightness + 0.1f;
                        if (brightness > 1.0f) brightness = 1.0f;
                        brightnessText.setString("BRIGHTNESS: " + std::to_string(int(brightness * 100)) + "%");
                    }
                }
            }
            else if (event.type == sf::Event::MouseButtonReleased) {
                volumeSliderDragging = false;
            }
            else if (event.type == sf::Event::MouseMoved) {
                sf::Vector2f mousePos = app.mapPixelToCoords(
                    sf::Vector2i(event.mouseMove.x, event.mouseMove.y));

                pauseButtonHovered = pauseButton.getGlobalBounds().contains(mousePos);
                pauseButton.setFillColor(pauseButtonHovered ?
                    sf::Color(90, 90, 90, 220) : sf::Color(70, 70, 70, 220));

                if (volumeSliderDragging) {
                    float newX = mousePos.x;
                    newX = std::max(W/2 - 100.f, std::min(newX, W/2 + 100.f));
                    volumeSlider.setPosition(newX, H/2 + 125);
                    volume = (newX - (W/2 - 100)) / 2.0f;
                    volume = std::max(0.f, std::min(100.f, volume));
                    volumeText.setString("VOLUME: " + std::to_string(int(volume)) + "%");

                    if (soundEnabled) {
                        shootSound.setVolume(volume);
                        backgroundMusic.setVolume(volume * 0.7f);
                        pauseMusic.setVolume(volume * 0.7f);
                    }
                }

                if (gamePaused) {
                    bool soundHovered = soundButton.getGlobalBounds().contains(mousePos);
                    soundButton.setFillColor(soundHovered ?
                        sf::Color(70, 70, 70, 200) : sf::Color(50, 50, 50, 200));

                    bool brightnessHovered = brightnessButton.getGlobalBounds().contains(mousePos);
                    brightnessButton.setFillColor(brightnessHovered ?
                        sf::Color(70, 70, 70, 200) : sf::Color(50, 50, 50, 200));
                }
            }

            if (event.type == sf::Event::KeyPressed && !gamePaused) {
                if (event.key.code == sf::Keyboard::Enter && homingShootCooldown <= 0) {
                    auto b = std::make_unique<HomingBullet>();
                    b->settings(sHomingBullet, playerPtr->x, playerPtr->y, playerPtr->angle, 10);
                    entities.push_back(std::move(b));
                    homingShootCooldown = homingShootCooldownTime;
                    if (soundEnabled) shootSound.play();
                }
            }
        }

        if (!gamePaused) {
            // Update cooldowns
            shootCooldown -= dt;
            homingShootCooldown -= dt;

            // Player controls
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) playerPtr->angle += 3;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) playerPtr->angle -= 3;
            playerPtr->thrust = sf::Keyboard::isKeyPressed(sf::Keyboard::W);

            // Continuous firing when space is held down
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && shootCooldown <= 0) {
                auto b = std::make_unique<Bullet>();
                b->settings(sBullet, playerPtr->x, playerPtr->y, playerPtr->angle, 10);
                entities.push_back(std::move(b));
                shootCooldown = shootCooldownTime;
                if (soundEnabled) shootSound.play();
            }

            // Collision detection
            for (auto itA = entities.begin(); itA != entities.end(); ++itA) {
                for (auto itB = std::next(itA); itB != entities.end(); ++itB) {
                    Entity* a = itA->get();
                    Entity* b = itB->get();

                    // Player vs Asteroid/Boss collision
                    if ((a->name == "player" && (b->name == "asteroid" || b->name == "boss")) ||
                        (b->name == "player" && (a->name == "asteroid" || a->name == "boss"))) {

                        Entity* player = (a->name == "player") ? a : b;
                        Entity* asteroid = (a->name == "asteroid" || a->name == "boss") ? a : b;

                        if (isCollide(player, asteroid)) {
                            asteroid->life = false;
                            if (asteroid->name == "boss") {
                                static_cast<BossAsteroid*>(asteroid)->spawnChildren = false;
                                activeBossCount--;
                            }

                            int totalDestroyed = asteroidsShotDirectly + asteroidsDestroyedInExplosions;
                            maxAsteroidsDestroyed = std::max(maxAsteroidsDestroyed, totalDestroyed);

                            // Reset only current score, keep max
                            asteroidsShotDirectly = 0;
                            asteroidsDestroyedInExplosions = 0;

                            // Clear all asteroids and bullets
                            entities.remove_if([](const std::unique_ptr<Entity>& e) {
                                return e->name == "asteroid" || e->name == "bullet" ||
                                       e->name == "homingbullet" || e->name == "boss";
                            });
                            activeBossCount = 0;

                            // Respawn initial asteroids
                            spawnInitialAsteroids();

                            // Create explosion effect
                            auto explosion = std::make_unique<Explosion>();
                            if (asteroid->name == "boss") {
                                explosion->settings(sBossExplosion, player->x, player->y);
                            } else {
                                explosion->settings(sExplosion_ship, player->x, player->y);
                            }
                            entities.push_back(std::move(explosion));

                            // Reset player
                            player->settings(sPlayer, W/2, H/2, 0, 20);
                            player->dx = 0;
                            player->dy = 0;
                        }
                    }
                    // Bullet vs Boss collision
                    else if ((a->name == "boss" && b->name == "bullet") ||
                            (b->name == "boss" && a->name == "bullet")) {

                        Entity* boss = (a->name == "boss") ? a : b;
                        Entity* bullet = (a->name == "bullet") ? a : b;

                        if (isCollide(boss, bullet)) {
                            bullet->life = false;
                            BossAsteroid* bossObj = static_cast<BossAsteroid*>(boss);
                            bossObj->health -= static_cast<Bullet*>(bullet)->damage;

                            auto hitEffect = std::make_unique<Explosion>();
                            hitEffect->settings(sExplosion_ship, bullet->x, bullet->y);
                            entities.push_back(std::move(hitEffect));

                            if (bossObj->health <= 0) {
                                boss->life = false;
                                activeBossCount--;
                                auto explosion = std::make_unique<Explosion>();
                                explosion->settings(sBossExplosion, boss->x, boss->y);
                                entities.push_back(std::move(explosion));

                                // Spawn 4 regular asteroids when boss is destroyed
                                if (bossObj->spawnChildren) {
                                    for (int i = 0; i < 4; i++) {
                                        auto a = std::make_unique<Asteroid>();
                                        a->settings(sRock_small, boss->x, boss->y, rand() % 360, 15);
                                        // Inherit some boss velocity
                                        a->dx = boss->dx * 0.5f + (rand() % 4 - 2);
                                        a->dy = boss->dy * 0.5f + (rand() % 4 - 2);
                                        entities.push_back(std::move(a));
                                    }
                                }

                                bossSpawned = activeBossCount > 0;
                                asteroidsShotDirectly += 10;
                            }
                        }
                    }
                    // HomingBullet vs Boss collision
                    else if ((a->name == "boss" && b->name == "homingbullet") ||
                            (b->name == "boss" && a->name == "homingbullet")) {

                        Entity* boss = (a->name == "boss") ? a : b;
                        Entity* bullet = (a->name == "homingbullet") ? a : b;

                        if (isCollide(boss, bullet)) {
                            bullet->life = false;
                            BossAsteroid* bossObj = static_cast<BossAsteroid*>(boss);
                            bossObj->health -= static_cast<HomingBullet*>(bullet)->damage;

                            auto hitEffect = std::make_unique<Explosion>();
                            hitEffect->settings(sExplosion, bullet->x, bullet->y);
                            entities.push_back(std::move(hitEffect));

                            if (bossObj->health <= 0) {
                                boss->life = false;
                                activeBossCount--;
                                auto explosion = std::make_unique<Explosion>();
                                explosion->settings(sBossExplosion, boss->x, boss->y);
                                entities.push_back(std::move(explosion));

                                // Spawn 4 regular asteroids when boss is destroyed
                                if (bossObj->spawnChildren) {
                                    for (int i = 0; i < 4; i++) {
                                        auto a = std::make_unique<Asteroid>();
                                        a->settings(sRock_small, boss->x, boss->y, rand() % 360, 15);
                                        // Inherit some boss velocity
                                        a->dx = boss->dx * 0.5f + (rand() % 4 - 2);
                                        a->dy = boss->dy * 0.5f + (rand() % 4 - 2);
                                        entities.push_back(std::move(a));
                                    }
                                }

                                bossSpawned = activeBossCount > 0;
                                asteroidsShotDirectly += 10;
                            }
                        }
                    }
                    // Bullet vs Asteroid collision
                    else if ((a->name == "asteroid" && b->name == "bullet") ||
                            (b->name == "asteroid" && a->name == "bullet")) {

                        Entity* asteroid = (a->name == "asteroid") ? a : b;
                        Entity* bullet = (a->name == "bullet") ? a : b;

                        if (isCollide(asteroid, bullet)) {
                            asteroid->life = false;
                            bullet->life = false;
                            asteroidsShotDirectly++;

                            auto explosion = std::make_unique<Explosion>();
                            explosion->settings(sExplosion, asteroid->x, asteroid->y);
                            entities.push_back(std::move(explosion));

                            if (asteroid->R != 15) {
                                for (int i = 0; i < 2; i++) {
                                    auto smallAsteroid = std::make_unique<Asteroid>();
                                    smallAsteroid->settings(sRock_small, asteroid->x, asteroid->y, rand() % 360, 15);
                                    entities.push_back(std::move(smallAsteroid));
                                }
                            }
                        }
                    }
                    // HomingBullet vs Asteroid collision
                    else if ((a->name == "asteroid" && b->name == "homingbullet") ||
                            (b->name == "asteroid" && a->name == "homingbullet")) {

                        Entity* asteroid = (a->name == "asteroid") ? a : b;
                        Entity* bullet = (a->name == "homingbullet") ? a : b;

                        if (isCollide(asteroid, bullet)) {
                            asteroid->life = false;
                            bullet->life = false;
                            asteroidsShotDirectly++;

                            auto explosion = std::make_unique<Explosion>();
                            explosion->settings(sExplosion, asteroid->x, asteroid->y);
                            entities.push_back(std::move(explosion));

                            auto explosionEffect = std::make_unique<ExplosionEffect>();
                            explosionEffect->x = asteroid->x;
                            explosionEffect->y = asteroid->y;
                            entities.push_back(std::move(explosionEffect));

                            if (asteroid->R != 15) {
                                for (int i = 0; i < 2; i++) {
                                    auto smallAsteroid = std::make_unique<Asteroid>();
                                    smallAsteroid->settings(sRock_small, asteroid->x, asteroid->y, rand() % 360, 15);
                                    entities.push_back(std::move(smallAsteroid));
                                }
                            }
                        }
                    }
                }
            }

            // Update animations and clean up
            playerPtr->anim = playerPtr->thrust ? sPlayer_go : sPlayer;
            entities.remove_if([](const std::unique_ptr<Entity>& e) {
                if (e->name == "explosionEffect" && !e->life) {
                    asteroidsDestroyedInExplosions += static_cast<ExplosionEffect*>(e.get())->damageDealt;
                }
                return (e->name == "explosion" && e->anim.isEnd()) ||
                       (e->name == "explosionEffect" && !e->life) ||
                       !e->life;
            });

            // Spawn logic
            int currentScore = asteroidsShotDirectly + asteroidsDestroyedInExplosions;
            if (currentScore >= BOSS_TRIGGER_SCORE &&
                activeBossCount < MAX_BOSS_ASTEROIDS &&
                rand() % 100 == 0) {
                spawnBossAsteroid();
            }
            else if (!bossSpawned && rand() % 150 == 0) {
                auto a = std::make_unique<Asteroid>();
                a->settings(sRock, 0, rand() % H, rand() % 360, 25);
                entities.push_back(std::move(a));
            }

            // Update all entities
            for (auto& e : entities) {
                e->update();
                if (e->name != "explosionEffect") {
                    e->anim.update();
                }
            }
        }

        // Update score display
        int currentScore = asteroidsShotDirectly + asteroidsDestroyedInExplosions;
        scoreText.setString("Current: " + std::to_string(currentScore));
        recordText.setString("Record: " + std::to_string(maxAsteroidsDestroyed));

        // Draw everything
        app.clear();

        // Apply brightness
        sf::Sprite background(t2);
        app.draw(background);
        if (brightness < 1.0f) {
            sf::RectangleShape dimmer(sf::Vector2f(W, H));
            dimmer.setFillColor(sf::Color(0, 0, 0, 255 * (1.0f - brightness)));
            app.draw(dimmer);
        }

        for (auto& e : entities) {
            e->draw(app);
            if (e->name == "boss") {
                drawBossHealth(app, static_cast<BossAsteroid*>(e.get()), font);
            }
        }

        // Draw UI
        app.draw(scoreText);
        app.draw(recordText);
        app.draw(pauseButton);
        app.draw(pauseText);

        // Pause overlay
        if (gamePaused) {
            sf::RectangleShape overlay(sf::Vector2f(W, H));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            app.draw(overlay);

            sf::Text pausedText("GAME PAUSED", font, 50);
            pausedText.setPosition(W/2 - pausedText.getLocalBounds().width/2, H/2 - 50);
            pausedText.setFillColor(sf::Color::White);
            app.draw(pausedText);

            app.draw(soundButton);
            app.draw(brightnessButton);
            app.draw(volumeSliderBackground);
            app.draw(volumeSlider);
            app.draw(soundText);
            app.draw(brightnessText);
            app.draw(volumeText);
        }

        app.display();
    }

    return EXIT_SUCCESS;
}
