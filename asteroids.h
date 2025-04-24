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

using namespace sf;

const int W = 1200;
const int H = 800;
const float DEGTORAD = 0.017453f;

// Forward declarations
class Entity;
extern std::list<std::unique_ptr<Entity>> entities;

class Animation {
public:
    float Frame, speed;
    Sprite sprite;
    std::vector<IntRect> frames;

    Animation() = default;

    Animation(Texture& t, int x, int y, int w, int h, int count, float Speed) {
        Frame = 0;
        speed = Speed;

        for (int i = 0; i < count; i++)
            frames.push_back(IntRect(x + i * w, y, w, h));

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
    virtual void draw(RenderWindow& app) {
        anim.sprite.setPosition(x, y);
        anim.sprite.setRotation(angle + 90);
        app.draw(anim.sprite);
    }
    virtual ~Entity() = default;
};

class Explosion : public Entity {
public:
    Explosion() {
        name = "explosion";
    }

    void update() override {}
};

class ExplosionEffect : public Entity {
public:
    float radius;
    float currentSize;
    float growthRate;
    int damageDealt;

    ExplosionEffect() : radius(100), currentSize(0), growthRate(150), damageDealt(0) {
        name = "explosionEffect";
    }

    void update() override {
        if (currentSize < radius) {
            currentSize += growthRate * 0.016f;

            // Check collisions with asteroids
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

    void draw(RenderWindow& app) override {
        CircleShape circle(currentSize);
        circle.setPosition(x - currentSize, y - currentSize);
        circle.setFillColor(Color(255, 50, 50, 100));
        circle.setOutlineColor(Color::Red);
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

        if (x > W) x = 0;
        if (x < 0) x = W;
        if (y > H) y = 0;
        if (y < 0) y = H;
    }
};

class Bullet : public Entity {
public:
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

    HomingBullet() {
        name = "homingbullet";
    }

    void update() override {
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
};

class Player : public Entity {
public:
    bool thrust;

    Player() : thrust(false) {
        name = "player";
    }

    void update() override {
        if (thrust) {
            dx += cos(angle * DEGTORAD) * 0.2;
            dy += sin(angle * DEGTORAD) * 0.2;
        } else {
            dx *= 0.99;
            dy *= 0.99;
        }

        int maxSpeed = 5;
        float speed = sqrt(dx * dx + dy * dy);
        if (speed > maxSpeed) {
            dx *= maxSpeed / speed;
            dy *= maxSpeed / speed;
        }

        x += dx;
        y += dy;

        if (x > W) x = 0;
        if (x < 0) x = W;
        if (y > H) y = 0;
        if (y < 0) y = H;
    }
};

bool isCollide(const Entity* a, const Entity* b);

#endif
