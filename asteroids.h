#ifndef ASTEROIDS_HPP
#define ASTEROIDS_HPP

#include <SFML/Graphics.hpp>
#include <list>
#include <vector>
#include <string>
#include <cmath>
#include <ctime>
#include <limits>
#include <memory>

// Game constants
const int W = 1200;
const int H = 800;
const float DEGTORAD = 0.017453f;
const int BOSS_TRIGGER_SCORE = 25;
const int MAX_BOSS_ASTEROIDS = 3;
const int INITIAL_ASTEROIDS = 15;

// Damage constants
const int REGULAR_BULLET_DAMAGE = 1;
const int HOMING_BULLET_DAMAGE = 5;
const int BOSS_MAX_HEALTH = 15;

// Global game state
extern std::list<std::unique_ptr<class Entity>> entities;
extern int asteroidsShotDirectly;
extern int asteroidsDestroyedInExplosions;
extern int maxAsteroidsDestroyed;
extern int activeBossCount;
extern bool bossSpawned;

class Animation {
public:
    float Frame, speed;
    sf::Sprite sprite;
    std::vector<sf::IntRect> frames;

    Animation() = default;

    Animation(sf::Texture& t, int x, int y, int w, int h, int count, float Speed) {
        Frame = 0;
        speed = Speed;
        for (int i = 0; i < count; i++)
            frames.push_back(sf::IntRect(x + i * w, y, w, h));
        sprite.setTexture(t);
        sprite.setOrigin(w / 2, h / 2);
        sprite.setTextureRect(frames[0]);
    }

    void update() {
        Frame += speed;
        int n = frames.size();
        if (Frame >= n) Frame -= n;
        if (n > 0) sprite.setTextureRect(frames[int(Frame)]);
    }

    bool isEnd() const {
        return Frame + speed >= frames.size();
    }
};

class Entity {
public:
    float x, y, dx, dy, R, angle;
    bool life;
    std::string name;
    Animation anim;

    Entity() : x(0), y(0), dx(0), dy(0), R(1), angle(0), life(true) {}

    void settings(Animation& a, int X, int Y, float Angle = 0, int radius = 1) {
        anim = a;
        x = X;
        y = Y;
        angle = Angle;
        R = radius;
    }

    virtual void update() = 0;

    virtual void draw(sf::RenderWindow& app) {
        anim.sprite.setPosition(x, y);
        anim.sprite.setRotation(angle + 90);
        app.draw(anim.sprite);
    }

    virtual ~Entity() = default;
};

class Explosion : public Entity {
public:
    Explosion() { name = "explosion"; }
    void update() override {
        if (anim.isEnd()) life = false;
    }
};

class ExplosionEffect : public Entity {
public:
    float radius = 100;
    float currentSize = 0;
    float growthRate = 150;
    int damageDealt = 0;

    ExplosionEffect() { name = "explosionEffect"; }

    void update() override {
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

    void draw(sf::RenderWindow& app) override {
        sf::CircleShape circle(currentSize);
        circle.setPosition(x - currentSize, y - currentSize);
        circle.setFillColor(sf::Color(255, 50, 50, 100));
        circle.setOutlineColor(sf::Color::Red);
        circle.setOutlineThickness(2);
        app.draw(circle);
    }
};

class Asteroid : public Entity {
public:
    Asteroid() {
        dx = rand() % 8 - 4;
        dy = rand() % 8 - 4;
        name = "asteroid";
    }

    void update() override {
        x += dx;
        y += dy;
        if (x > W) x = 0; if (x < 0) x = W;
        if (y > H) y = 0; if (y < 0) y = H;
    }
};

class BossAsteroid : public Entity {
public:
    int health = BOSS_MAX_HEALTH;

    BossAsteroid() {
        name = "boss";
        R = 80;
        dx = (rand() % 5 - 2) * 0.5f;
        dy = (rand() % 5 - 2) * 0.5f;
    }

    void update() override {
        x += dx * 0.3f;
        y += dy * 0.3f;
        if (x > W) x = 0; if (x < 0) x = W;
        if (y > H) y = 0; if (y < 0) y = H;
    }
};

class Bullet : public Entity {
public:
    int damage = REGULAR_BULLET_DAMAGE;

    Bullet() {
        name = "bullet";
    }

    void update() override {
        dx = cos(angle * DEGTORAD) * 6;
        dy = sin(angle * DEGTORAD) * 6;
        x += dx;
        y += dy;
        if (x > W || x < 0 || y > H || y < 0) life = false;
    }
};

class HomingBullet : public Entity {
public:
    Entity* target = nullptr;
    int damage = HOMING_BULLET_DAMAGE;

    HomingBullet() {
        name = "homingbullet";
    }

    void update() override {
        // Find closest target (asteroid or boss)
        float minDist = std::numeric_limits<float>::max();
        target = nullptr;

        for (auto& e : entities) {
            if ((e->name == "asteroid" || e->name == "boss") && e->life) {
                float dist = (e->x - x) * (e->x - x) + (e->y - y) * (e->y - y);
                if (dist < minDist) {
                    minDist = dist;
                    target = e.get();
                }
            }
        }

        // If target found, adjust angle
        if (target && target->life) {
            float targetAngle = atan2(target->y - y, target->x - x) / DEGTORAD;
            float angleDiff = targetAngle - angle;

            // Normalize angle difference
            while (angleDiff > 180) angleDiff -= 360;
            while (angleDiff < -180) angleDiff += 360;

            angle += angleDiff * 0.1f; // Gradual turn
        }

        dx = cos(angle * DEGTORAD) * 6;
        dy = sin(angle * DEGTORAD) * 6;
        x += dx;
        y += dy;

        if (x > W || x < 0 || y > H || y < 0) life = false;
    }
};

class Player : public Entity {
public:
    bool thrust = false;

    Player() { name = "player"; }

    void update() override {
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
        if (x > W) x = 0; if (x < 0) x = W;
        if (y > H) y = 0; if (y < 0) y = H;
    }
};

#endif // ASTEROIDS_HPP
