#include "Asteroids.h"
#include <SFML/Audio.hpp>
#include <iostream>

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

    // Load and setup background music
    sf::Music backgroundMusic;
    if (!backgroundMusic.openFromFile("doom.ogg")) {
        std::cerr << "Failed to load background music!" << std::endl;
    } else {
        backgroundMusic.setLoop(true);
        backgroundMusic.setVolume(50);
        backgroundMusic.play();
    }

    // Load shooting sound only
    sf::SoundBuffer shootBuffer;
    sf::Sound shootSound;
    if (!shootBuffer.loadFromFile("blaster.ogg")) {
        std::cerr << "Failed to load shooting sound!" << std::endl;
    } else {
        shootSound.setBuffer(shootBuffer);
        shootSound.setVolume(70);
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
        shootCooldown -= dt;
        homingShootCooldown -= dt;

        // Event handling
        Event event;
        while (app.pollEvent(event)) {
            if (event.type == Event::Closed) {
                app.close();
            }

            if (event.type == Event::KeyPressed) {
                // Regular bullet
                if (event.key.code == Keyboard::Space && shootCooldown <= 0) {
                    auto b = std::make_unique<Bullet>();
                    b->settings(sBullet, playerPtr->x, playerPtr->y, playerPtr->angle, 10);
                    entities.push_back(std::move(b));
                    shootCooldown = shootCooldownTime;
                    shootSound.play();
                }
                // Homing bullet
                else if (event.key.code == Keyboard::Enter && homingShootCooldown <= 0) {
                    auto b = std::make_unique<HomingBullet>();
                    b->settings(sHomingBullet, playerPtr->x, playerPtr->y, playerPtr->angle, 10);
                    entities.push_back(std::move(b));
                    homingShootCooldown = homingShootCooldownTime;
                    shootSound.play();
                }
            }
        }

        // Player controls
        if (Keyboard::isKeyPressed(Keyboard::D)) playerPtr->angle += 3;
        if (Keyboard::isKeyPressed(Keyboard::A)) playerPtr->angle -= 3;
        playerPtr->thrust = Keyboard::isKeyPressed(Keyboard::W);

        // Collision detection
        for (auto itA = entities.begin(); itA != entities.end(); ++itA) {
            for (auto itB = std::next(itA); itB != entities.end(); ++itB) {
                Entity* a = itA->get();
                Entity* b = itB->get();

                // Regular bullet collision
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
                // Homing bullet collision
                else if ((a->name == "asteroid" && b->name == "homingbullet") ||
                         (b->name == "asteroid" && a->name == "homingbullet")) {

                    Entity* asteroid = (a->name == "asteroid") ? a : b;
                    Entity* bullet = (a->name == "homingbullet") ? a : b;

                    if (isCollide(asteroid, bullet)) {
                        asteroid->life = false;
                        bullet->life = false;
                        asteroidsShotDirectly++;

                        // Create visual explosion
                        auto explosion = std::make_unique<Explosion>();
                        explosion->settings(sExplosion, asteroid->x, asteroid->y);
                        entities.push_back(std::move(explosion));

                        // Create explosion effect
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
                // Player collision
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

                        std::cout << "--- Ship Destroyed! ---\n";
                        std::cout << "Asteroids shot directly: " << asteroidsShotDirectly << "\n";
                        std::cout << "Asteroids destroyed in explosions: " << asteroidsDestroyedInExplosions << "\n";
                        std::cout << "Total this life: " << totalDestroyed << "\n";
                        std::cout << "Record: " << maxAsteroidsDestroyed << "\n\n";

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

        // Draw everything
        app.clear();
        app.draw(background);
        for (auto& e : entities) {
            e->draw(app);
        }
        app.display();
    }

    return EXIT_SUCCESS;
}
