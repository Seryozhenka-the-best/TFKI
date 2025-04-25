#include "Asteroids.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <iostream>
#include <sstream>

std::list<std::unique_ptr<Entity>> entities;
int asteroidsShotDirectly = 0;
int asteroidsDestroyedInExplosions = 0;
int maxAsteroidsDestroyed = 0;

bool isCollide(const Entity* a, const Entity* b) {
    return (b->x - a->x) * (b->x - a->x) +
           (b->y - a->y) * (b->y - a->y) <
           (a->R + b->R) * (a->R + b->R);
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    RenderWindow app(VideoMode(W, H), "Asteroids!");
    app.setFramerateLimit(60);

    // Game state
    bool gamePaused = false;
    bool pauseButtonHovered = false;
    float brightness = 1.0f; // 0.0-1.0
    bool soundEnabled = true;

    // Load font for UI
    sf::Font font;
    if (!font.loadFromFile("Roboto-Bold.ttf")) {
        std::cerr << "Failed to load font! Using default." << std::endl;
    }

    // Setup score display (top-left)
    sf::Text scoreText;
    sf::Text recordText;
    if (font.getInfo().family != "") {
        scoreText.setFont(font);
        recordText.setFont(font);
        scoreText.setCharacterSize(24);
        recordText.setCharacterSize(24);
        scoreText.setFillColor(sf::Color::White);
        recordText.setFillColor(sf::Color::White);
        scoreText.setPosition(20, 20);
        recordText.setPosition(20, 50);
    }

    // Setup pause button (top-right)
    sf::RectangleShape pauseButton(sf::Vector2f(100, 40));
    pauseButton.setPosition(W - 120, 20);
    pauseButton.setFillColor(sf::Color(50, 50, 50, 200));
    pauseButton.setOutlineThickness(2);
    pauseButton.setOutlineColor(sf::Color::White);

    sf::Text pauseText;
    if (font.getInfo().family != "") {
        pauseText.setFont(font);
        pauseText.setString("PAUSE");
        pauseText.setCharacterSize(20);
        pauseText.setFillColor(sf::Color::White);
        pauseText.setPosition(W - 110, 25);
    }

    // Setup sound and brightness buttons (only visible when paused)
    sf::RectangleShape soundButton(sf::Vector2f(150, 40));
    soundButton.setPosition(W/2 - 160, H/2 + 50);
    soundButton.setFillColor(sf::Color(50, 50, 50, 200));
    soundButton.setOutlineThickness(2);
    soundButton.setOutlineColor(sf::Color::White);

    sf::RectangleShape brightnessButton(sf::Vector2f(150, 40));
    brightnessButton.setPosition(W/2 + 10, H/2 + 50);
    brightnessButton.setFillColor(sf::Color(50, 50, 50, 200));
    brightnessButton.setOutlineThickness(2);
    brightnessButton.setOutlineColor(sf::Color::White);

    sf::Text soundText;
    sf::Text brightnessText;
    if (font.getInfo().family != "") {
        soundText.setFont(font);
        soundText.setString(soundEnabled ? "SOUND: ON" : "SOUND: OFF");
        soundText.setCharacterSize(20);
        soundText.setFillColor(sf::Color::White);
        soundText.setPosition(W/2 - 150, H/2 + 55);

        brightnessText.setFont(font);
        brightnessText.setString("BRIGHTNESS: " + std::to_string(int(brightness * 100)) + "%");
        brightnessText.setCharacterSize(20);
        brightnessText.setFillColor(sf::Color::White);
        brightnessText.setPosition(W/2 + 20, H/2 + 55);
    }

    // Load background music
    sf::Music backgroundMusic;
    if (!backgroundMusic.openFromFile("doom.ogg")) {
        std::cerr << "Failed to load background music!" << std::endl;
    } else {
        backgroundMusic.setLoop(true);
        backgroundMusic.setVolume(soundEnabled ? 50 : 0);
        backgroundMusic.play();
    }

    // Load pause music
    sf::Music pauseMusic;
    if (!pauseMusic.openFromFile("pause.ogg")) {
        std::cerr << "Failed to load pause music!" << std::endl;
    } else {
        pauseMusic.setLoop(true);
        pauseMusic.setVolume(soundEnabled ? 50 : 0);
    }

    // Load shooting sound
    sf::SoundBuffer shootBuffer;
    sf::Sound shootSound;
    if (!shootBuffer.loadFromFile("blaster.ogg")) {
        std::cerr << "Failed to load shooting sound!" << std::endl;
    } else {
        shootSound.setBuffer(shootBuffer);
        shootSound.setVolume(soundEnabled ? 70 : 0);
    }

    // Load textures
    Texture t1, t2, t3, t4, t5, t6, t7, t8;
    if (!t1.loadFromFile("spaceship.png") ||
        !t2.loadFromFile("background.jpg") ||
        !t3.loadFromFile("explosions/type_C.png") ||
        !t4.loadFromFile("rock.png") ||
        !t5.loadFromFile("fire_blue.png") ||
        !t6.loadFromFile("rock_small.png") ||
        !t7.loadFromFile("explosions/type_B.png") ||
        !t8.loadFromFile("fire_red.png")) {
        return EXIT_FAILURE;
    }

    t1.setSmooth(true);
    t2.setSmooth(true);

    // Create animations
    Sprite background(t2);
    Animation sExplosion(t3, 0, 0, 256, 256, 48, 0.5);
    Animation sRock(t4, 0, 0, 64, 64, 16, 0.2);
    Animation sRock_small(t6, 0, 0, 64, 64, 16, 0.2);
    Animation sBullet(t5, 0, 0, 32, 64, 16, 0.8);
    Animation sHomingBullet(t8, 0, 0, 32, 64, 16, 0.8);
    Animation sPlayer(t1, 40, 0, 40, 40, 1, 0);
    Animation sPlayer_go(t1, 40, 40, 40, 40, 1, 0);
    Animation sExplosion_ship(t7, 0, 0, 192, 192, 64, 0.5);

    // Initialize asteroids
    for (int i = 0; i < 15; i++) {
        auto a = std::make_unique<Asteroid>();
        a->settings(sRock, rand() % W, rand() % H, rand() % 360, 25);
        entities.push_back(std::move(a));
    }

    // Create player
    auto p = std::make_unique<Player>();
    p->settings(sPlayer, W/2, H/2, 0, 20);
    Player* playerPtr = p.get();
    entities.push_back(std::move(p));

    // Game timing
    float shootCooldown = 0.0f;
    const float shootCooldownTime = 0.175f;
    float homingShootCooldown = 0.0f;
    const float homingShootCooldownTime = 2.0f;
    Clock clock;

    while (app.isOpen()) {
        float dt = clock.restart().asSeconds();

        if (!gamePaused) {
            shootCooldown -= dt;
            homingShootCooldown -= dt;
        }

        // Event handling
        Event event;
        while (app.pollEvent(event)) {
            if (event.type == Event::Closed) {
                app.close();
            }

            if (event.type == Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mousePos = app.mapPixelToCoords(
                        sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

                    // Pause button
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
                    }

                    // Sound and brightness buttons (only when paused)
                    if (gamePaused) {
                        if (soundButton.getGlobalBounds().contains(mousePos)) {
                            soundEnabled = !soundEnabled;
                            soundText.setString(soundEnabled ? "SOUND: ON" : "SOUND: OFF");

                            // Update all sounds
                            backgroundMusic.setVolume(soundEnabled ? 50 : 0);
                            pauseMusic.setVolume(soundEnabled ? 50 : 0);
                            shootSound.setVolume(soundEnabled ? 70 : 0);

                            if (gamePaused && soundEnabled) {
                                pauseMusic.play();
                            }
                        }

                        if (brightnessButton.getGlobalBounds().contains(mousePos)) {
                            brightness = brightness >= 1.0f ? 0.5f : brightness + 0.1f;
                            if (brightness > 1.0f) brightness = 1.0f;
                            brightnessText.setString("BRIGHTNESS: " + std::to_string(int(brightness * 100)) + "%");
                        }
                    }
                }
            }
            else if (event.type == Event::MouseMoved) {
                sf::Vector2f mousePos = app.mapPixelToCoords(
                    sf::Vector2i(event.mouseMove.x, event.mouseMove.y));

                // Button hover effects
                pauseButtonHovered = pauseButton.getGlobalBounds().contains(mousePos);
                pauseButton.setFillColor(pauseButtonHovered ?
                    sf::Color(70, 70, 70, 200) : sf::Color(50, 50, 50, 200));

                if (gamePaused) {
                    bool soundHovered = soundButton.getGlobalBounds().contains(mousePos);
                    soundButton.setFillColor(soundHovered ?
                        sf::Color(70, 70, 70, 200) : sf::Color(50, 50, 50, 200));

                    bool brightnessHovered = brightnessButton.getGlobalBounds().contains(mousePos);
                    brightnessButton.setFillColor(brightnessHovered ?
                        sf::Color(70, 70, 70, 200) : sf::Color(50, 50, 50, 200));
                }
            }

            if (!gamePaused && event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Space && shootCooldown <= 0) {
                    auto b = std::make_unique<Bullet>();
                    b->settings(sBullet, playerPtr->x, playerPtr->y, playerPtr->angle, 10);
                    entities.push_back(std::move(b));
                    shootCooldown = shootCooldownTime;
                    if (soundEnabled) shootSound.play();
                }
                else if (event.key.code == Keyboard::Enter && homingShootCooldown <= 0) {
                    auto b = std::make_unique<HomingBullet>();
                    b->settings(sHomingBullet, playerPtr->x, playerPtr->y, playerPtr->angle, 10);
                    entities.push_back(std::move(b));
                    homingShootCooldown = homingShootCooldownTime;
                    if (soundEnabled) shootSound.play();
                }
            }
        }

        // Game logic (only when not paused)
        if (!gamePaused) {
            // Player controls
            if (Keyboard::isKeyPressed(Keyboard::D)) playerPtr->angle += 3;
            if (Keyboard::isKeyPressed(Keyboard::A)) playerPtr->angle -= 3;
            playerPtr->thrust = Keyboard::isKeyPressed(Keyboard::W);

            // Collision detection
            for (auto itA = entities.begin(); itA != entities.end(); ++itA) {
                for (auto itB = std::next(itA); itB != entities.end(); ++itB) {
                    Entity* a = itA->get();
                    Entity* b = itB->get();

                    if ((a->name == "asteroid" && b->name == "bullet") ||
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
                    else if ((a->name == "player" && b->name == "asteroid") ||
                             (a->name == "asteroid" && b->name == "player")) {

                        Entity* player = (a->name == "player") ? a : b;
                        Entity* asteroid = (a->name == "asteroid") ? a : b;

                        if (isCollide(player, asteroid)) {
                            asteroid->life = false;

                            int totalDestroyed = asteroidsShotDirectly + asteroidsDestroyedInExplosions;
                            if (totalDestroyed > maxAsteroidsDestroyed) {
                                maxAsteroidsDestroyed = totalDestroyed;
                            }

                            asteroidsShotDirectly = 0;
                            asteroidsDestroyedInExplosions = 0;

                            auto explosion = std::make_unique<Explosion>();
                            explosion->settings(sExplosion_ship, player->x, player->y);
                            entities.push_back(std::move(explosion));

                            player->settings(sPlayer, W/2, H/2, 0, 20);
                            player->dx = 0;
                            player->dy = 0;
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

            // Spawn new asteroids
            if (rand() % 150 == 0) {
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
        if (brightness < 1.0f) {
            sf::RectangleShape dimmer(sf::Vector2f(W, H));
            dimmer.setFillColor(sf::Color(0, 0, 0, 255 * (1.0f - brightness)));
            app.draw(background);
            app.draw(dimmer);
        } else {
            app.draw(background);
        }

        for (auto& e : entities) {
            e->draw(app);
        }

        // Draw UI elements
        app.draw(scoreText);
        app.draw(recordText);
        app.draw(pauseButton);
        app.draw(pauseText);

        // Draw pause overlay and buttons if game is paused
        if (gamePaused) {
            sf::RectangleShape overlay(sf::Vector2f(W, H));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            app.draw(overlay);

            if (font.getInfo().family != "") {
                sf::Text pausedText("GAME PAUSED", font, 50);
                pausedText.setPosition(W/2 - pausedText.getLocalBounds().width/2, H/2 - 50);
                pausedText.setFillColor(sf::Color::White);
                app.draw(pausedText);

                // Draw settings buttons
                app.draw(soundButton);
                app.draw(brightnessButton);
                app.draw(soundText);
                app.draw(brightnessText);
            }
        }

        app.display();
    }

    return EXIT_SUCCESS;
}
