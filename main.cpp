#include "asteroids.h"
#include <memory>
#include <chrono>
#include <SFML/Audio.hpp>
#include <iostream>

bool isCollide(const Entity* a, const Entity* b) {
    return (b->x - a->x) * (b->x - a->x) +
           (b->y - a->y) * (b->y - a->y) <
           (a->R + b->R) * (a->R + b->R);
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    RenderWindow app(VideoMode(W, H), "Asteroids!");
    app.setFramerateLimit(60);

    // Stats tracking
    int asteroidsShotThisLife = 0;
    int maxAsteroidsShot = 0;

    sf::Music backgroundMusic;
    if (!backgroundMusic.openFromFile("doom.ogg")) {
        std::cerr << "Failed to load background music!" << std::endl;
    }
    else {
        backgroundMusic.setLoop(true);
        backgroundMusic.setVolume(50);
        backgroundMusic.play();
    }

    Texture t1, t2, t3, t4, t5, t6, t7;
    if (!t1.loadFromFile("spaceship.png") ||
        !t2.loadFromFile("background.jpg") ||
        !t3.loadFromFile("explosions/type_C.png") ||
        !t4.loadFromFile("rock.png") ||
        !t5.loadFromFile("fire_blue.png") ||
        !t6.loadFromFile("rock_small.png") ||
        !t7.loadFromFile("explosions/type_B.png")) {
        return EXIT_FAILURE;
    }

    t1.setSmooth(true);
    t2.setSmooth(true);

    Sprite background(t2);
    Animation sExplosion(t3, 0, 0, 256, 256, 48, 0.5);
    Animation sRock(t4, 0, 0, 64, 64, 16, 0.2);
    Animation sRock_small(t6, 0, 0, 64, 64, 16, 0.2);
    Animation sBullet(t5, 0, 0, 32, 64, 16, 0.8);
    Animation sPlayer(t1, 40, 0, 40, 40, 1, 0);
    Animation sPlayer_go(t1, 40, 40, 40, 40, 1, 0);
    Animation sExplosion_ship(t7, 0, 0, 192, 192, 64, 0.5);

    std::list<std::unique_ptr<Entity>> entities;
    for (int i = 0; i < 15; i++) {
        auto a = std::make_unique<Asteroid>();
        a->settings(sRock, rand() % W, rand() % H, rand() % 360, 25);
        entities.push_back(std::move(a));
    }
    auto p = std::make_unique<Player>();
    p->settings(sPlayer, W/2, H/2, 0, 20);
    Player* playerPtr = p.get();
    entities.push_back(std::move(p));

    // Cooldown variables
    float shootCooldown = 0.0f;
    const float shootCooldownTime = 0.175f;
    Clock clock;

    while (app.isOpen()) {
        float dt = clock.restart().asSeconds();
        shootCooldown -= dt;

        Event event;
        while (app.pollEvent(event)) {
            if (event.type == Event::Closed) {
                app.close();
            }

            if (event.type == Event::KeyPressed && event.key.code == Keyboard::Space) {
                if (shootCooldown <= 0) {
                    auto b = std::make_unique<Bullet>();
                    b->settings(sBullet, playerPtr->x, playerPtr->y, playerPtr->angle, 10);
                    entities.push_back(std::move(b));
                    shootCooldown = shootCooldownTime;
                }
            }
        }

        if (Keyboard::isKeyPressed(Keyboard::D)) playerPtr->angle += 3;
        if (Keyboard::isKeyPressed(Keyboard::A)) playerPtr->angle -= 3;
        playerPtr->thrust = Keyboard::isKeyPressed(Keyboard::W);

        for (auto itA = entities.begin(); itA != entities.end(); ++itA) {
            for (auto itB = std::next(itA); itB != entities.end(); ++itB) {
                Entity* a = itA->get();
                Entity* b = itB->get();

                if (a->name == "asteroid" && b->name == "bullet" && isCollide(a, b)) {
                    a->life = false;
                    b->life = false;
                    asteroidsShotThisLife++;  // Increment counter

                    auto explosion = std::make_unique<Explosion>();
                    explosion->settings(sExplosion, a->x, a->y);
                    entities.push_back(std::move(explosion));

                    if (a->R != 15) {
                        for (int i = 0; i < 2; i++) {
                            auto smallAsteroid = std::make_unique<Asteroid>();
                            smallAsteroid->settings(sRock_small, a->x, a->y, rand() % 360, 15);
                            entities.push_back(std::move(smallAsteroid));
                        }
                    }
                }

                if (a->name == "player" && b->name == "asteroid" && isCollide(a, b)) {
                    b->life = false;

                    // Update max asteroids shot if needed
                    if (asteroidsShotThisLife > maxAsteroidsShot) {
                        maxAsteroidsShot = asteroidsShotThisLife;
                    }

                    // Print stats to console
                    std::cout << "--- Ship Destroyed! ---\n";
                    std::cout << "Asteroids shot this life: " << asteroidsShotThisLife << "\n";
                    std::cout << "Max asteroids shot (all time): " << maxAsteroidsShot << "\n\n";

                    // Reset counter for new life
                    asteroidsShotThisLife = 0;

                    auto explosion = std::make_unique<Explosion>();
                    explosion->settings(sExplosion_ship, a->x, a->y);
                    entities.push_back(std::move(explosion));

                    playerPtr->settings(sPlayer, W/2, H/2, 0, 20);
                    playerPtr->dx = 0;
                    playerPtr->dy = 0;
                }
            }
        }

        playerPtr->anim = playerPtr->thrust ? sPlayer_go : sPlayer;
        entities.remove_if([](const std::unique_ptr<Entity>& e) {
            return (e->name == "explosion" && e->anim.isEnd()) || !e->life;
        });

        if (rand() % 150 == 0) {
            auto a = std::make_unique<Asteroid>();
            a->settings(sRock, 0, rand() % H, rand() % 360, 25);
            entities.push_back(std::move(a));
        }

        for (auto& e : entities) {
            e->update();
            e->anim.update();
        }

        app.clear();
        app.draw(background);
        for (auto& e : entities) {
            e->draw(app);
        }
        app.display();
    }

    return EXIT_SUCCESS;
}
