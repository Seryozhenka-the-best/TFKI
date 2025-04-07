#ifndef ASTEROIDS_HPP
#define ASTEROIDS_HPP

#include <SFML/Graphics.hpp>
#include <list>
#include <vector>
#include <string>
#include <cmath>
#include <ctime>

using namespace sf;

const int W = 1200;
const int H = 800;
const float DEGTORAD = 0.017453f;

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

    Entity() : life(true) {}

    void settings(Animation& a, int X, int Y, float Angle = 0, int radius = 1) {
        anim = a;
        x = X;
        y = Y;
        angle = Angle;
        R = radius;
    }

    virtual void update() = 0;
    virtual ~Entity() = default;

    void draw(RenderWindow& app) {
        anim.sprite.setPosition(x, y);
        anim.sprite.setRotation(angle + 90);
        app.draw(anim.sprite);
    }
};

class Explosion : public Entity {
public:
    Explosion() {
        name = "explosion";
    }

    void update() override {
        // Explosions don't move
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

        int maxSpeed = 15;
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
