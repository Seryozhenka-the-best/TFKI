#include "Asteroids.h"
#include <SFML/Audio.hpp>
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
GameState gameState = GameState::MAIN_MENU;

// Audio settings
bool soundEnabled = true;
float volume = 70.0f;
sf::Music backgroundMusic, pauseMusic;
sf::SoundBuffer shootBuffer;
sf::Sound shootSound;

// Player reference
Player* playerPtr = nullptr;

// Menu elements
Button* startButton = nullptr;
Button* exitButton = nullptr;

// Pause menu elements
sf::RectangleShape pauseResumeButton;
sf::Text pauseResumeText;
sf::RectangleShape pauseSoundButton;
sf::Text pauseSoundText;
sf::RectangleShape pauseBrightnessButton;
sf::Text pauseBrightnessText;
sf::RectangleShape pauseVolumeBackground;
sf::RectangleShape pauseVolumeSlider;
sf::Text pauseVolumeText;

// Slider control variables
bool pauseButtonHovered = false;
bool volumeSliderHovered = false;
bool volumeSliderDragging = false;
float volumeSliderMinX = 0;
float volumeSliderMaxX = 0;
float brightness = 1.0f;

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
    sf::Text bossHealthText;
    bossHealthText.setString("BOSS HP: " + std::to_string(boss->health));
    bossHealthText.setFont(font);
    bossHealthText.setCharacterSize(24);
    bossHealthText.setPosition(boss->x - 50, boss->y - 60);
    bossHealthText.setFillColor(sf::Color::Red);
    app.draw(bossHealthText);

    sf::RectangleShape healthBarBack(sf::Vector2f(100, 10));
    healthBarBack.setPosition(boss->x - 50, boss->y - 40);
    healthBarBack.setFillColor(sf::Color(50, 50, 50));
    app.draw(healthBarBack);

    float healthPercentage = static_cast<float>(boss->health) / BOSS_MAX_HEALTH;
    sf::RectangleShape healthBar(sf::Vector2f(100 * healthPercentage, 10));
    healthBar.setPosition(boss->x - 50, boss->y - 40);
    healthBar.setFillColor(sf::Color::Red);
    app.draw(healthBar);
}

void updateVolumeSlider(float mouseX) {
    // Calculate new slider position (clamped to bounds)
    float newSliderX = mouseX - pauseVolumeSlider.getSize().x / 2;
    newSliderX = std::max(volumeSliderMinX, std::min(newSliderX, volumeSliderMaxX));

    // Update slider position
    pauseVolumeSlider.setPosition(newSliderX, pauseVolumeSlider.getPosition().y);

    // Calculate volume (0-100) based on slider position
    volume = ((newSliderX - volumeSliderMinX) / (volumeSliderMaxX - volumeSliderMinX)) * 100.0f;
    volume = std::max(0.f, std::min(100.f, volume));

    // Update volume text
    pauseVolumeText.setString("VOLUME: " + std::to_string(int(volume)) + "%");

    // Update audio volumes
    if (soundEnabled) {
        shootSound.setVolume(volume);
        backgroundMusic.setVolume(volume * 0.7f);
        pauseMusic.setVolume(volume * 0.7f);
    }
}

void initMenu(sf::Font& font) {
    // Main menu buttons
    startButton = new Button("START GAME", font, 30,
                           sf::Vector2f(W/2 - 150, H/2 - 50),
                           sf::Vector2f(300, 60));
    startButton->setColors(
        sf::Color(0, 100, 200, 220),
        sf::Color(0, 150, 255, 220),
        sf::Color(0, 70, 150, 220)
    );

    exitButton = new Button("EXIT", font, 30,
                          sf::Vector2f(W/2 - 150, H/2 + 50),
                          sf::Vector2f(300, 60));
    exitButton->setColors(
        sf::Color(200, 50, 50, 220),
        sf::Color(255, 70, 70, 220),
        sf::Color(150, 30, 30, 220)
    );

    // Initialize pause menu elements
    pauseResumeButton.setSize(sf::Vector2f(300, 60));
    pauseResumeButton.setPosition(W/2 - 150, H/2 - 100);
    pauseResumeButton.setFillColor(sf::Color(70, 70, 70, 220));
    pauseResumeButton.setOutlineThickness(2);
    pauseResumeButton.setOutlineColor(sf::Color::White);

    pauseResumeText.setString("RESUME");
    pauseResumeText.setFont(font);
    pauseResumeText.setCharacterSize(30);
    pauseResumeText.setPosition(W/2 - pauseResumeText.getLocalBounds().width/2, H/2 - 90);
    pauseResumeText.setFillColor(sf::Color::White);

    pauseSoundButton.setSize(sf::Vector2f(300, 60));
    pauseSoundButton.setPosition(W/2 - 150, H/2);
    pauseSoundButton.setFillColor(sf::Color(70, 70, 70, 220));
    pauseSoundButton.setOutlineThickness(2);
    pauseSoundButton.setOutlineColor(sf::Color::White);

    pauseSoundText.setString("SOUND: ON");
    pauseSoundText.setFont(font);
    pauseSoundText.setCharacterSize(30);
    pauseSoundText.setPosition(W/2 - pauseSoundText.getLocalBounds().width/2, H/2 + 10);
    pauseSoundText.setFillColor(sf::Color::White);

    pauseBrightnessButton.setSize(sf::Vector2f(300, 60));
    pauseBrightnessButton.setPosition(W/2 - 150, H/2 + 80);
    pauseBrightnessButton.setFillColor(sf::Color(70, 70, 70, 220));
    pauseBrightnessButton.setOutlineThickness(2);
    pauseBrightnessButton.setOutlineColor(sf::Color::White);

    pauseBrightnessText.setString("BRIGHTNESS: 100%");
    pauseBrightnessText.setFont(font);
    pauseBrightnessText.setCharacterSize(30);
    pauseBrightnessText.setPosition(W/2 - pauseBrightnessText.getLocalBounds().width/2, H/2 + 90);
    pauseBrightnessText.setFillColor(sf::Color::White);

    // Volume slider setup
    pauseVolumeBackground.setSize(sf::Vector2f(300, 20));
    pauseVolumeBackground.setPosition(W/2 - 150, H/2 + 160);
    pauseVolumeBackground.setFillColor(sf::Color(50, 50, 50));

    pauseVolumeSlider.setSize(sf::Vector2f(20, 40));
    pauseVolumeSlider.setPosition(W/2 - 150 + (volume/100.f * 300), H/2 + 150);
    pauseVolumeSlider.setFillColor(sf::Color::White);

    // Set min/max X positions for the slider
    volumeSliderMinX = pauseVolumeBackground.getPosition().x;
    volumeSliderMaxX = volumeSliderMinX + pauseVolumeBackground.getSize().x - pauseVolumeSlider.getSize().x;

    pauseVolumeText.setString("VOLUME: " + std::to_string(int(volume)) + "%");
    pauseVolumeText.setFont(font);
    pauseVolumeText.setCharacterSize(24);
    pauseVolumeText.setPosition(W/2 - pauseVolumeText.getLocalBounds().width/2, H/2 + 190);
    pauseVolumeText.setFillColor(sf::Color::White);
}

void drawMenu(sf::RenderWindow& app, sf::Font& font) {
    // Draw title
    sf::Text title;
    title.setString("ASTEROIDS SURVIVAL");
    title.setFont(font);
    title.setCharacterSize(80);
    title.setPosition(W/2 - title.getLocalBounds().width/2, H/4);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    app.draw(title);

    // Draw version/subtitle
    sf::Text version;
    version.setString("Run and Fire");
    version.setFont(font);
    version.setCharacterSize(24);
    version.setPosition(W/2 - version.getLocalBounds().width/2, H/4 + 90);
    version.setFillColor(sf::Color(200, 200, 200));
    app.draw(version);

    // Draw buttons
    startButton->draw(app);
    exitButton->draw(app);

    // Draw controls info
    sf::Text controls;
    controls.setString("Controls: W/A/D to move, SPACE to shoot, ENTER for homing missiles");
    controls.setFont(font);
    controls.setCharacterSize(20);
    controls.setPosition(W/2 - controls.getLocalBounds().width/2, H - 50);
    controls.setFillColor(sf::Color(150, 150, 150));
    app.draw(controls);
}

void handleMenuEvents(sf::RenderWindow& app, sf::Event& event) {
    sf::Vector2f mousePos = app.mapPixelToCoords(
        sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

    startButton->update(mousePos);
    exitButton->update(mousePos);

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        if (startButton->isClicked(mousePos, sf::Mouse::Left)) {
            gameState = GameState::PLAYING;
            // Reset game state
            entities.clear();
            spawnInitialAsteroids();
            auto p = std::make_unique<Player>();
            p->settings(sPlayer, W/2, H/2, 0, 20);
            playerPtr = p.get();
            entities.push_back(std::move(p));

            // Start game music
            if (soundEnabled) {
                backgroundMusic.play();
            }
        }
        else if (exitButton->isClicked(mousePos, sf::Mouse::Left)) {
            app.close();
        }
    }
}

void resetGame() {
    entities.clear();
    asteroidsShotDirectly = 0;
    asteroidsDestroyedInExplosions = 0;
    activeBossCount = 0;
    bossSpawned = false;
    spawnInitialAsteroids();

    auto p = std::make_unique<Player>();
    p->settings(sPlayer, W/2, H/2, 0, 20);
    playerPtr = p.get();
    entities.push_back(std::move(p));
}

void drawPauseMenu(sf::RenderWindow& app) {
    // Dark overlay
    sf::RectangleShape overlay(sf::Vector2f(W, H));
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    app.draw(overlay);

    // "GAME PAUSED" text
    sf::Text pausedText;
    pausedText.setString("GAME PAUSED");
    pausedText.setFont(*pauseResumeText.getFont());
    pausedText.setCharacterSize(50);
    pausedText.setPosition(W/2 - pausedText.getLocalBounds().width/2, H/4);
    pausedText.setFillColor(sf::Color::White);
    app.draw(pausedText);

    // Update button colors based on hover state
    pauseResumeButton.setFillColor(pauseButtonHovered ?
        sf::Color(90, 90, 90, 220) : sf::Color(70, 70, 70, 220));

    // Draw volume slider with hover effect
    pauseVolumeSlider.setFillColor(volumeSliderHovered || volumeSliderDragging ?
        sf::Color(200, 200, 255) : sf::Color::White);

    // Draw all pause menu elements
    app.draw(pauseResumeButton);
    app.draw(pauseResumeText);
    app.draw(pauseSoundButton);
    app.draw(pauseSoundText);
    app.draw(pauseBrightnessButton);
    app.draw(pauseBrightnessText);
    app.draw(pauseVolumeBackground);
    app.draw(pauseVolumeSlider);
    app.draw(pauseVolumeText);
}

void handlePauseMenuEvents(sf::RenderWindow& app, sf::Event& event) {
    sf::Vector2f mousePos = app.mapPixelToCoords(
        sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

    // Update hover states
    pauseButtonHovered = pauseResumeButton.getGlobalBounds().contains(mousePos);
    volumeSliderHovered = pauseVolumeSlider.getGlobalBounds().contains(mousePos) ||
                         pauseVolumeBackground.getGlobalBounds().contains(mousePos);

    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            // Resume button
            if (pauseResumeButton.getGlobalBounds().contains(mousePos)) {
                gameState = GameState::PLAYING;
                pauseMusic.stop();
                if (soundEnabled) backgroundMusic.play();
            }
            // Sound toggle
            else if (pauseSoundButton.getGlobalBounds().contains(mousePos)) {
                soundEnabled = !soundEnabled;
                pauseSoundText.setString(soundEnabled ? "SOUND: ON" : "SOUND: OFF");
                backgroundMusic.setVolume(soundEnabled ? volume * 0.7f : 0);
                pauseMusic.setVolume(soundEnabled ? volume * 0.7f : 0);
                shootSound.setVolume(soundEnabled ? volume : 0);
            }
            // Brightness adjustment
            else if (pauseBrightnessButton.getGlobalBounds().contains(mousePos)) {
                brightness = brightness >= 1.0f ? 0.5f : brightness + 0.1f;
                if (brightness > 1.0f) brightness = 1.0f;
                pauseBrightnessText.setString("BRIGHTNESS: " + std::to_string(int(brightness * 100)) + "%");
            }
            // Volume slider - check if clicked on slider or background
            else if (volumeSliderHovered) {
                volumeSliderDragging = true;
                updateVolumeSlider(mousePos.x);
            }
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            volumeSliderDragging = false;
        }
    }
    else if (event.type == sf::Event::MouseMoved) {
        if (volumeSliderDragging) {
            updateVolumeSlider(mousePos.x);
        }
    }
    // Allow ESC to unpause
    else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        gameState = GameState::PLAYING;
        pauseMusic.stop();
        if (soundEnabled) backgroundMusic.play();
    }
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

    // Load font
    sf::Font font;
    if (!font.loadFromFile("Roboto-Bold.ttf")) {
        std::cerr << "Failed to load font! Using default" << std::endl;
        font = sf::Font();
    }

    // Initialize menus
    initMenu(font);

    // Audio initialization
    if (!backgroundMusic.openFromFile("doom.ogg")) {
        std::cerr << "Failed to load background music!" << std::endl;
    } else {
        backgroundMusic.setLoop(true);
        backgroundMusic.setVolume(volume * 0.7f);
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

    // Game timing
    float shootCooldown = 0.0f;
    const float shootCooldownTime = 0.15f;
    float homingShootCooldown = 0.0f;
    const float homingShootCooldownTime = 1.5f;

    sf::Clock clock;

    // Main game loop
    while (app.isOpen()) {
        float dt = clock.restart().asSeconds();
        int currentScore = 0;

        // Event handling
        sf::Event event;
        while (app.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                app.close();
            }

            switch (gameState) {
                case GameState::MAIN_MENU:
                    handleMenuEvents(app, event);
                    break;

                case GameState::PLAYING:
                    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                        gameState = GameState::PAUSED;
                        backgroundMusic.pause();
                        if (soundEnabled) pauseMusic.play();
                        continue;
                    }

                    // Pause button handling
                    if (event.type == sf::Event::MouseButtonPressed &&
                        event.mouseButton.button == sf::Mouse::Left) {
                        sf::Vector2f mousePos = app.mapPixelToCoords(
                            sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

                        // Check if pause button was clicked
                        sf::FloatRect pauseBtnBounds(W - 140, 20, 120, 50);
                        if (pauseBtnBounds.contains(mousePos)) {
                            gameState = GameState::PAUSED;
                            backgroundMusic.pause();
                            if (soundEnabled) pauseMusic.play();
                            continue;
                        }
                    }

                    if (gameState == GameState::PLAYING) {
                        if (event.type == sf::Event::KeyPressed) {
                            if (event.key.code == sf::Keyboard::Enter && homingShootCooldown <= 0) {
                                auto b = std::make_unique<HomingBullet>();
                                b->settings(sHomingBullet, playerPtr->x, playerPtr->y, playerPtr->angle, 10);
                                entities.push_back(std::move(b));
                                homingShootCooldown = homingShootCooldownTime;
                                if (soundEnabled) shootSound.play();
                            }
                        }
                    }
                    break;

                case GameState::PAUSED:
                    handlePauseMenuEvents(app, event);
                    break;
            }
        }

        // Update based on game state
        switch (gameState) {
            case GameState::MAIN_MENU:
                // Menu doesn't need updates beyond what's handled in events
                break;

            case GameState::PLAYING: {
                // Update cooldowns
                shootCooldown -= dt;
                homingShootCooldown -= dt;

                // Player controls
                if (playerPtr) {
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
                }

                // Collision detection
                for (auto itA = entities.begin(); itA != entities.end(); ++itA) {
                    for (auto itB = std::next(itA); itB != entities.end(); ++itB) {
                        Entity* a = itA->get();
                        Entity* b = itB->get();

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

                                    // Spawn 8 regular asteroids when boss is destroyed
                                    if (bossObj->spawnChildren) {
                                        for (int i = 0; i < 8; i++) {
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

                                    // Spawn 8 regular asteroids when boss is destroyed
                                    if (bossObj->spawnChildren) {
                                        for (int i = 0; i < 8; i++) {
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
                if (playerPtr) {
                    playerPtr->anim = playerPtr->thrust ? sPlayer_go : sPlayer;
                }
                entities.remove_if([](const std::unique_ptr<Entity>& e) {
                    if (e->name == "explosionEffect" && !e->life) {
                        asteroidsDestroyedInExplosions += static_cast<ExplosionEffect*>(e.get())->damageDealt;
                    }
                    return (e->name == "explosion" && e->anim.isEnd()) ||
                           (e->name == "explosionEffect" && !e->life) ||
                           !e->life;
                });

                // Spawn logic
                currentScore = asteroidsShotDirectly + asteroidsDestroyedInExplosions;
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
                break;
            }

            case GameState::PAUSED:
                // Pause state doesn't need updates
                break;
        }

        // Draw everything
        app.clear();

        // Draw background (for all states)
        sf::Sprite background(t2);
        app.draw(background);

        // Apply brightness
        if (brightness < 1.0f) {
            sf::RectangleShape dimmer(sf::Vector2f(W, H));
            dimmer.setFillColor(sf::Color(0, 0, 0, 255 * (1.0f - brightness)));
            app.draw(dimmer);
        }

        // Draw game entities (when not in main menu)
        if (gameState != GameState::MAIN_MENU) {
            for (auto& e : entities) {
                e->draw(app);
                if (e->name == "boss") {
                    drawBossHealth(app, static_cast<BossAsteroid*>(e.get()), font);
                }
            }

            // Draw score
            currentScore = asteroidsShotDirectly + asteroidsDestroyedInExplosions;
            sf::Text scoreText;
            scoreText.setString("Score: " + std::to_string(currentScore));
            scoreText.setFont(font);
            scoreText.setCharacterSize(24);
            scoreText.setPosition(20, 20);
            scoreText.setFillColor(sf::Color::White);
            app.draw(scoreText);

            // Draw high score
            sf::Text highScoreText;
            highScoreText.setString("High Score: " + std::to_string(maxAsteroidsDestroyed));
            highScoreText.setFont(font);
            highScoreText.setCharacterSize(24);
            highScoreText.setPosition(20, 50);
            highScoreText.setFillColor(sf::Color::White);
            app.draw(highScoreText);
        }

        // State-specific UI
        switch (gameState) {
            case GameState::MAIN_MENU:
                drawMenu(app, font);
                break;

            case GameState::PLAYING: {
                // Draw pause button
                sf::RectangleShape pauseBtn(sf::Vector2f(120, 50));
                pauseBtn.setPosition(W - 140, 20);
                pauseBtn.setFillColor(sf::Color(70, 70, 70, 220));
                pauseBtn.setOutlineThickness(2);
                pauseBtn.setOutlineColor(sf::Color::White);
                app.draw(pauseBtn);

                sf::Text pauseTxt;
                pauseTxt.setString("PAUSE");
                pauseTxt.setFont(font);
                pauseTxt.setCharacterSize(24);
                pauseTxt.setPosition(W - 120, 30);
                pauseTxt.setFillColor(sf::Color::White);
                app.draw(pauseTxt);
                break;
            }

            case GameState::PAUSED:
                drawPauseMenu(app);
                break;
        }

        app.display();
    }

    // Clean up
    delete startButton;
    delete exitButton;

    return EXIT_SUCCESS;
}
