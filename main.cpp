#include "Asteroids.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <iostream>
#include <sstream>

// Глобальные переменные
std::list<std::unique_ptr<Entity>> entities;
int asteroidsShotDirectly = 0;
int asteroidsDestroyedInExplosions = 0;
int maxAsteroidsDestroyed = 0;
bool bossSpawned = false;
int bossHitsTaken = 0;

bool isCollide(const Entity* a, const Entity* b) {
    return (b->x - a->x) * (b->x - a->x) +
           (b->y - a->y) * (b->y - a->y) <
           (a->R + b->R) * (a->R + b->R);
}

// Реализации методов классов
void ExplosionEffect::update() {
    if (currentSize < radius) {
        currentSize += growthRate * 0.016f;
        for (auto& e : entities) {
            if (e->name == "asteroid" && e->life) {
                float dist = sqrt((e->x - x) * (e->x - x) + (e->y - y) * (e->y - y));
                if (dist < currentSize + e->R) {
                    e->life = false;
                    damageDealt++;
                }
            }
        }
    } else {
        life = false;
    }
}

void ExplosionEffect::draw(sf::RenderWindow& app) {
    sf::CircleShape circle(currentSize);
    circle.setPosition(x - currentSize, y - currentSize);
    circle.setFillColor(sf::Color(255, 50, 50, 100));
    circle.setOutlineColor(sf::Color::Red);
    circle.setOutlineThickness(2);
    app.draw(circle);
}

void Asteroid::update() {
    x += dx;
    y += dy;
    if (x > W) x = 0;
    if (x < 0) x = W;
    if (y > H) y = 0;
    if (y < 0) y = H;
}

void Bullet::update() {
    dx = cos(angle * DEGTORAD) * 6;
    dy = sin(angle * DEGTORAD) * 6;
    x += dx;
    y += dy;
    if (x > W || x < 0 || y > H || y < 0) life = false;
}

void HomingBullet::update() {
    if (!target) {
        float minDist = std::numeric_limits<float>::max();
        for (auto& e : entities) {
            if (e->name == "asteroid" && e->life) {
                float dist = (e->x - x) * (e->x - x) + (e->y - y) * (e->y - y);
                if (dist < minDist) {
                    minDist = dist;
                    target = e.get();
                }
            }
        }
    }

    if (target && target->life) {
        float targetAngle = atan2(target->y - y, target->x - x) / DEGTORAD;
        angle = targetAngle;
    }

    dx = cos(angle * DEGTORAD) * 6;
    dy = sin(angle * DEGTORAD) * 6;
    x += dx;
    y += dy;
    if (x > W || x < 0 || y > H || y < 0) life = false;
}

void Player::update() {
    if (thrust) {
        dx += cos(angle * DEGTORAD) * 0.2;
        dy += sin(angle * DEGTORAD) * 0.2;
    } else {
        dx *= 0.99;
        dy *= 0.99;
    }

    float speed = sqrt(dx * dx + dy * dy);
    if (speed > 5) {
        dx *= 5 / speed;
        dy *= 5 / speed;
    }

    x += dx;
    y += dy;
    if (x > W) x = 0;
    if (x < 0) x = W;
    if (y > H) y = 0;
    if (y < 0) y = H;
}

void BossAsteroid::update() {
    x += dx * 0.3f;
    y += dy * 0.3f;
    if (x > W) x = 0;
    if (x < 0) x = W;
    if (y > H) y = 0;
    if (y < 0) y = H;
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    sf::RenderWindow app(sf::VideoMode(W, H), "Asteroids!");
    app.setFramerateLimit(60);

    // Game state
    bool gamePaused = false;
    bool pauseButtonHovered = false;
    float brightness = 1.0f;
    bool soundEnabled = true;
    bool spacePressed = false; // Для обработки одиночного нажатия Space
    bool enterPressed = false; // Для обработки одиночного нажатия Enter

    // Load font
    sf::Font font;
    if (!font.loadFromFile("Roboto-Bold.ttf")) {
        std::cerr << "Failed to load font!" << std::endl;
    }

    // UI elements
    sf::Text scoreText, recordText, pauseText, soundText, brightnessText;
    if (font.getInfo().family != "") {
        scoreText.setFont(font);
        recordText.setFont(font);
        pauseText.setFont(font);
        soundText.setFont(font);
        brightnessText.setFont(font);

        scoreText.setCharacterSize(24);
        recordText.setCharacterSize(24);
        pauseText.setCharacterSize(20);
        soundText.setCharacterSize(20);
        brightnessText.setCharacterSize(20);

        scoreText.setFillColor(sf::Color::White);
        recordText.setFillColor(sf::Color::White);
        pauseText.setFillColor(sf::Color::White);
        soundText.setFillColor(sf::Color::White);
        brightnessText.setFillColor(sf::Color::White);

        scoreText.setPosition(20, 20);
        recordText.setPosition(20, 50);
        pauseText.setPosition(W - 110, 25);
        soundText.setPosition(W/2 - 150, H/2 + 55);
        brightnessText.setPosition(W/2 + 20, H/2 + 55);

        pauseText.setString("PAUSE");
        soundText.setString(soundEnabled ? "SOUND: ON" : "SOUND: OFF");
        brightnessText.setString("BRIGHTNESS: " + std::to_string(int(brightness * 100)) + "%");
    }

    // Buttons
    sf::RectangleShape pauseButton(sf::Vector2f(100, 40));
    pauseButton.setPosition(W - 120, 20);
    pauseButton.setFillColor(sf::Color(50, 50, 50, 200));
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

    // Sounds
    sf::Music backgroundMusic, pauseMusic;
    sf::SoundBuffer shootBuffer;
    sf::Sound shootSound;

    if (!backgroundMusic.openFromFile("doom.ogg")) {
        std::cerr << "Failed to load background music!" << std::endl;
    } else {
        backgroundMusic.setLoop(true);
        backgroundMusic.setVolume(soundEnabled ? 50 : 0);
        backgroundMusic.play();
    }

    if (!pauseMusic.openFromFile("pause.ogg")) {
        std::cerr << "Failed to load pause music!" << std::endl;
    } else {
        pauseMusic.setLoop(true);
        pauseMusic.setVolume(soundEnabled ? 50 : 0);
    }

    if (!shootBuffer.loadFromFile("blaster.ogg")) {
        std::cerr << "Failed to load shooting sound!" << std::endl;
    } else {
        shootSound.setBuffer(shootBuffer);
        shootSound.setVolume(soundEnabled ? 70 : 0);
    }

    // Textures and animations
    sf::Texture t1, t2, t3, t4, t5, t6, t7, t8;
    if (!t1.loadFromFile("spaceship.png") || !t2.loadFromFile("background.jpg") ||
        !t3.loadFromFile("explosions/type_C.png") || !t4.loadFromFile("rock.png") ||
        !t5.loadFromFile("fire_blue.png") || !t6.loadFromFile("rock_small.png") ||
        !t7.loadFromFile("explosions/type_B.png") || !t8.loadFromFile("fire_red.png")) {
        return EXIT_FAILURE;
    }

    t1.setSmooth(true);
    t2.setSmooth(true);

    // Создаем увеличенную анимацию для босса
    Animation sBossRock = Animation(t4, 0, 0, 64, 64, 16, 0.2);
    sBossRock.sprite.setScale(2.0f, 2.0f); // Увеличиваем спрайт в 2 раза

    Animation sExplosion(t3, 0, 0, 256, 256, 48, 0.5);
    Animation sRock(t4, 0, 0, 64, 64, 16, 0.2);
    Animation sRock_small(t6, 0, 0, 64, 64, 16, 0.2);
    Animation sBullet(t5, 0, 0, 32, 64, 16, 0.8);
    Animation sHomingBullet(t8, 0, 0, 32, 64, 16, 0.8);
    Animation sPlayer(t1, 40, 0, 40, 40, 1, 0);
    Animation sPlayer_go(t1, 40, 40, 40, 40, 1, 0);
    Animation sExplosion_ship(t7, 0, 0, 192, 192, 64, 0.5);
    Animation sBossExplosion = sExplosion_ship;
    sBossExplosion.sprite.setScale(3.0f, 3.0f); // Увеличенный взрыв для босса

    // Create initial entities
    for (int i = 0; i < 15; i++) {
        auto a = std::make_unique<Asteroid>();
        a->settings(sRock, rand() % W, rand() % H, rand() % 360, 25);
        entities.push_back(std::move(a));
    }

    auto p = std::make_unique<Player>();
    p->settings(sPlayer, W/2, H/2, 0, 20);
    Player* playerPtr = p.get();
    entities.push_back(std::move(p));

    // Game timing
    float shootCooldown = 0.0f;
    const float shootCooldownTime = 0.175f;
    float homingShootCooldown = 0.0f;
    const float homingShootCooldownTime = 2.0f;
    sf::Clock clock;

    while (app.isOpen()) {
        float dt = clock.restart().asSeconds();

        if (!gamePaused) {
            shootCooldown -= dt;
            homingShootCooldown -= dt;
        }

        // Event handling
        sf::Event event;
        while (app.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                app.close();
            }

            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {

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
                }

                if (gamePaused) {
                    if (soundButton.getGlobalBounds().contains(mousePos)) {
                        soundEnabled = !soundEnabled;
                        soundText.setString(soundEnabled ? "SOUND: ON" : "SOUND: OFF");
                        backgroundMusic.setVolume(soundEnabled ? 50 : 0);
                        pauseMusic.setVolume(soundEnabled ? 50 : 0);
                        shootSound.setVolume(soundEnabled ? 70 : 0);
                        if (gamePaused && soundEnabled) pauseMusic.play();
                    }

                    if (brightnessButton.getGlobalBounds().contains(mousePos)) {
                        brightness = brightness >= 1.0f ? 0.5f : brightness + 0.1f;
                        if (brightness > 1.0f) brightness = 1.0f;
                        brightnessText.setString("BRIGHTNESS: " + std::to_string(int(brightness * 100)) + "%");
                    }
                }
            }
            else if (event.type == sf::Event::MouseMoved) {
                sf::Vector2f mousePos = app.mapPixelToCoords(
                    sf::Vector2i(event.mouseMove.x, event.mouseMove.y));

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

            // Обработка клавиш для стрельбы (только при нажатии, а не удержании)
            if (event.type == sf::Event::KeyPressed && !gamePaused) {
                if (event.key.code == sf::Keyboard::Space && !spacePressed && shootCooldown <= 0) {
                    spacePressed = true;
                    auto b = std::make_unique<Bullet>();
                    b->settings(sBullet, playerPtr->x, playerPtr->y, playerPtr->angle, 10);
                    entities.push_back(std::move(b));
                    shootCooldown = shootCooldownTime;
                    if (soundEnabled) shootSound.play();
                }
                else if (event.key.code == sf::Keyboard::Enter && !enterPressed && homingShootCooldown <= 0) {
                    enterPressed = true;
                    auto b = std::make_unique<HomingBullet>();
                    b->settings(sHomingBullet, playerPtr->x, playerPtr->y, playerPtr->angle, 10);
                    entities.push_back(std::move(b));
                    homingShootCooldown = homingShootCooldownTime;
                    if (soundEnabled) shootSound.play();
                }
            }

            // Сброс флагов при отпускании клавиш
            if (event.type == sf::Event::KeyReleased) {
                if (event.key.code == sf::Keyboard::Space) spacePressed = false;
                if (event.key.code == sf::Keyboard::Enter) enterPressed = false;
            }
        }

        // Game logic
        if (!gamePaused) {
            // Player controls
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) playerPtr->angle += 3;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) playerPtr->angle -= 3;
            playerPtr->thrust = sf::Keyboard::isKeyPressed(sf::Keyboard::W);

            // Collision detection
            for (auto itA = entities.begin(); itA != entities.end(); ++itA) {
                for (auto itB = std::next(itA); itB != entities.end(); ++itB) {
                    Entity* a = itA->get();
                    Entity* b = itB->get();

                    if ((a->name == "asteroid" && b->name == "bullet") ||
                        (b->name == "asteroid" && a->name == "bullet")) {

                        if (isCollide(a, b)) {
                            a->life = false;
                            b->life = false;
                            asteroidsShotDirectly++;

                            auto explosion = std::make_unique<Explosion>();
                            explosion->settings(sExplosion, a->name == "asteroid" ? a->x : b->x,
                                                      a->name == "asteroid" ? a->y : b->y);
                            entities.push_back(std::move(explosion));

                            if (a->name == "asteroid" && a->R != 15) {
                                for (int i = 0; i < 2; i++) {
                                    auto smallAsteroid = std::make_unique<Asteroid>();
                                    smallAsteroid->settings(sRock_small, a->x, a->y, rand() % 360, 15);
                                    entities.push_back(std::move(smallAsteroid));
                                }
                            }
                            else if (b->name == "asteroid" && b->R != 15) {
                                for (int i = 0; i < 2; i++) {
                                    auto smallAsteroid = std::make_unique<Asteroid>();
                                    smallAsteroid->settings(sRock_small, b->x, b->y, rand() % 360, 15);
                                    entities.push_back(std::move(smallAsteroid));
                                }
                            }
                        }
                    }
                    else if ((a->name == "boss" && (b->name == "bullet" || b->name == "homingbullet")) ||
                            (b->name == "boss" && (a->name == "bullet" || a->name == "homingbullet"))) {

                        if (isCollide(a, b)) {
                            Entity* boss = a->name == "boss" ? a : b;
                            Entity* bullet = a->name == "bullet" || a->name == "homingbullet" ? a : b;

                            bullet->life = false;
                            bossHitsTaken++;

                            auto hitEffect = std::make_unique<Explosion>();
                            hitEffect->settings(sExplosion_ship, bullet->x, bullet->y);
                            entities.push_back(std::move(hitEffect));

                            if (bossHitsTaken >= BOSS_HITS_TO_DEFEAT) {
                                boss->life = false;
                                auto explosion = std::make_unique<Explosion>();
                                explosion->settings(sBossExplosion, boss->x, boss->y);
                                entities.push_back(std::move(explosion));

                                bossSpawned = false;
                                asteroidsShotDirectly += 10;
                            }
                        }
                    }
                    else if ((a->name == "asteroid" && b->name == "homingbullet") ||
                            (b->name == "asteroid" && a->name == "homingbullet")) {

                        if (isCollide(a, b)) {
                            a->life = false;
                            b->life = false;
                            asteroidsShotDirectly++;

                            auto explosion = std::make_unique<Explosion>();
                            explosion->settings(sExplosion, a->name == "asteroid" ? a->x : b->x,
                                                      a->name == "asteroid" ? a->y : b->y);
                            entities.push_back(std::move(explosion));

                            auto explosionEffect = std::make_unique<ExplosionEffect>();
                            explosionEffect->x = a->name == "asteroid" ? a->x : b->x;
                            explosionEffect->y = a->name == "asteroid" ? a->y : b->y;
                            entities.push_back(std::move(explosionEffect));

                            if (a->name == "asteroid" && a->R != 15) {
                                for (int i = 0; i < 2; i++) {
                                    auto smallAsteroid = std::make_unique<Asteroid>();
                                    smallAsteroid->settings(sRock_small, a->x, a->y, rand() % 360, 15);
                                    entities.push_back(std::move(smallAsteroid));
                                }
                            }
                            else if (b->name == "asteroid" && b->R != 15) {
                                for (int i = 0; i < 2; i++) {
                                    auto smallAsteroid = std::make_unique<Asteroid>();
                                    smallAsteroid->settings(sRock_small, b->x, b->y, rand() % 360, 15);
                                    entities.push_back(std::move(smallAsteroid));
                                }
                            }
                        }
                    }
                    else if ((a->name == "player" && b->name == "asteroid") ||
                            (a->name == "asteroid" && b->name == "player")) {

                        if (isCollide(a, b)) {
                            Entity* player = a->name == "player" ? a : b;
                            Entity* asteroid = a->name == "asteroid" ? a : b;

                            asteroid->life = false;
                            int totalDestroyed = asteroidsShotDirectly + asteroidsDestroyedInExplosions;
                            maxAsteroidsDestroyed = std::max(maxAsteroidsDestroyed, totalDestroyed);
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

            // Spawn logic
            if (!bossSpawned && (asteroidsShotDirectly + asteroidsDestroyedInExplosions) >= BOSS_TRIGGER_SCORE) {
                entities.remove_if([](const auto& e) { return e->name == "asteroid"; });

                auto boss = std::make_unique<BossAsteroid>();
                boss->settings(sBossRock, W/2, 50, 45, 80); // Используем увеличенный спрайт и радиус 80
                boss->dx = 1.5f;
                boss->dy = 1.5f;
                entities.push_back(std::move(boss));

                bossSpawned = true;
                bossHitsTaken = 0;
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

            if (font.getInfo().family != "") {
                sf::Text pausedText("GAME PAUSED", font, 50);
                pausedText.setPosition(W/2 - pausedText.getLocalBounds().width/2, H/2 - 50);
                pausedText.setFillColor(sf::Color::White);
                app.draw(pausedText);

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
