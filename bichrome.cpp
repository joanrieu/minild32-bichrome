#define M_PI 3.14159265f

#include <random>
#include <functional>
#include <sstream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

static sf::Font font;

enum Color {
        BLUE,
        RED,
};

const sf::Color colors[] = {
        sf::Color(48, 192, 240),
        sf::Color(192, 48, 48),
};

sf::RectangleShape getRect(sf::Color color, float blockSize = 40) {
        sf::RectangleShape rect(sf::Vector2f(blockSize, blockSize));
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(color);
        rect.setOutlineThickness(1);
        return rect;
}

sf::RenderWindow* win;
std::function<float()> urand;

int screenBordersHit(sf::FloatRect rect) {

        sf::FloatRect inter;
        sf::FloatRect(0, 0, win->getSize().x, win->getSize().y).intersects(rect, inter);

        int result = 0;

        if (rect.width - inter.width > .5f)
                result |= 1;

        if (rect.height - inter.height > .5f)
                result |= 2;

        return result;

}

struct Game {
        virtual ~Game() {}
        virtual void update() {}
        virtual void draw() {}
        virtual void event(sf::Event&) {}
}* game;

struct Shmup: Game {
        Shmup();
        void update();
        void draw();
        void event(sf::Event&);
        void addOther();
        sf::RectangleShape base;
        sf::RectangleShape player;
        sf::RectangleShape shotRect;
        typedef std::pair<float, float> Shot;
        std::vector<Shot> shots;
        typedef std::tuple<Color, sf::FloatRect, sf::Vector2f> Other;
        std::vector<Other> others;
        int recoil;
        int spawner;
        int score;
        bool pause;
};

Shmup::Shmup() {

        base = getRect(colors[BLUE], 120);
        base.setOrigin(base.getSize() / 2.f);
        base.setPosition(sf::Vector2f(win->getSize().x, win->getSize().y) / 2.f);

        player = getRect(colors[BLUE], 60);
        player.setFillColor(player.getOutlineColor());
        player.setOrigin(player.getSize() / 2.f);
        player.setPosition(base.getPosition());

        shotRect = getRect(colors[BLUE]);
        shotRect.setPosition(player.getPosition());
        shotRect.setSize(sf::Vector2f(20.f, 1));

        recoil = 0;
        spawner = 0;

        score = 0;
        pause = false;

}

void Shmup::update() {

        if (pause)
                return;

        const float rot = player.getRotation() + .6f;
        player.setRotation(rot);

        if (not recoil--) {

                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                        shots.push_back(std::make_pair(rot - 90.f, 0.f));
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                        shots.push_back(std::make_pair(rot + 90.f, 0.f));
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
                        shots.push_back(std::make_pair(rot + 180.f, 0.f));
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                        shots.push_back(std::make_pair(rot, 0.f));

                recoil = 10;

        }

        if (not spawner--) {
                addOther();
                spawner = 100;
        }

        for (auto& shot: shots)
                shot = std::make_pair(shot.first, shot.second + 5.f);

        const sf::FloatRect sb(0, 0, win->getSize().x, win->getSize().y);

        for (auto& other: others) {
                std::get<1>(other).left += std::get<2>(other).x;
                std::get<1>(other).top += std::get<2>(other).y;
        }

        others.erase(std::remove_if(others.begin(), others.end(), [&](const Other& other) {
                sf::FloatRect inter;
                return not sb.intersects(std::get<1>(other), inter) or inter == sf::FloatRect();
        }), others.end());

        shots.erase(std::remove_if(shots.begin(), shots.end(), [&](const Shot& shot) {

                sf::Vector2f hit(cos(shot.first * M_PI / 180.f), sin(shot.first * M_PI / 180.f));
                hit *= shot.second;
                hit += player.getPosition();

                others.erase(std::remove_if(others.begin(), others.end(), [&](const Other& other) {
                        if (not std::get<1>(other).contains(hit))
                                return false;
                        if (std::get<0>(other) == RED)
                                score += 5;
                        else
                                score -= 10;
                        return true;
                }), others.end());

                return not sb.contains(hit);

        }), shots.end());

}

void Shmup::draw() {

        std::ostringstream ss;
        ss << "\tScore\t" << score;
        win->draw(sf::Text(ss.str(), font));

        win->draw(base);

        shotRect.setOrigin(-30.f, 0);

        shotRect.setRotation(player.getRotation() - 90.f);
        shotRect.setOutlineColor(colors[RED]);
        win->draw(shotRect);
        shotRect.setOutlineColor(colors[BLUE]);

        shotRect.rotate(90.f);
        win->draw(shotRect);

        shotRect.rotate(90.f);
        win->draw(shotRect);

        shotRect.rotate(90.f);
        win->draw(shotRect);

        win->draw(player);

        for (const auto& other: others) {
                const sf::FloatRect otherR = std::get<1>(other);
                sf::RectangleShape otherRect;
                otherRect.setPosition(sf::Vector2f(otherR.left, otherR.top));
                otherRect.setSize(sf::Vector2f(otherR.width, otherR.height));
                otherRect.setFillColor(colors[std::get<0>(other)]);
                win->draw(otherRect);
        }

        for (const auto& shot: shots) {
                shotRect.setOrigin(-shot.second, 0);
                shotRect.setRotation(shot.first);
                win->draw(shotRect);
        }

}

void Shmup::event(sf::Event& event) {

        if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::P)
                pause = not pause;

}

void Shmup::addOther() {

        Other other;

        std::get<0>(other) = urand() > .5f ? BLUE : RED;

        do {
                std::get<1>(other) = sf::FloatRect(urand() * win->getSize().x, urand() * win->getSize().y,
                        urand() * base.getSize().x, urand() * base.getSize().y);
        } while (std::get<1>(other).intersects(sf::FloatRect(base.getPosition() - base.getOrigin(), base.getSize())));

        std::get<2>(other) = sf::Vector2f(urand(), urand());

        others.push_back(other);

}

struct Precision: Game {
        Precision();
        void update();
        void draw();
        sf::RectangleShape player, enemy;
        float speed;
        sf::Vector2f eSpeed;
        bool lost;
        sf::Clock clock;
        float score;
};

Precision::Precision() {

        player = getRect(colors[BLUE]);
        enemy = getRect(colors[RED], 200);
        eSpeed = sf::Vector2f(1., 1.) * 1.3f;
        lost = 0;

        player.move(static_cast<int>(urand() * (win->getSize().x - player.getSize().x)),
                static_cast<int>(urand() * (win->getSize().y - player.getSize().y)));
        enemy.move(static_cast<int>(urand() * (win->getSize().x - enemy.getSize().x)),
                static_cast<int>(urand() * (win->getSize().y - enemy.getSize().y)));

}

void Precision::update() {

        sf::FloatRect pb(player.getPosition(), player.getSize());
        sf::FloatRect eb(enemy.getPosition(), enemy.getSize());

        lost = lost or pb.intersects(eb);

        enemy.move(eSpeed);
        eb.left += eSpeed.x;
        eb.top += eSpeed.y;

        int borders = screenBordersHit(eb);

        if (borders & 1)
                eSpeed.x = -eSpeed.x;
        if (borders & 2)
                eSpeed.y = -eSpeed.y;

        if (lost)
                return;

        sf::Vector2i mouse = sf::Mouse::getPosition(*win);

        sf::Vector2f move(mouse.x, mouse.y);
        move -= player.getPosition() + player.getSize() / 2.f;

        if (move.x)
                move.x = move.x > 0 ? 1 : -1;
        if (move.y)
                move.y = move.y > 0 ? 1 : -1;

        player.move(move);
        pb.left += move.x;
        pb.top += move.y;

        borders = screenBordersHit(pb);
        move.x = borders & 1 ? -move.x : 0;
        move.y = borders & 2 ? -move.y : 0;
        player.move(move);

        score = clock.getElapsedTime().asSeconds();

}

void Precision::draw() {

        std::ostringstream ss;
        ss << "\tTime\t" << score;
        win->draw(sf::Text(ss.str(), font));

        if (lost == 1) {
                lost = 2;
                player.setOutlineColor(enemy.getOutlineColor());
        }

        win->draw(player);
        win->draw(enemy);

}

struct Startup: Game {
        void draw();
        void event(sf::Event&);
};

void Startup::draw() {

        auto rect = getRect(colors[RED], 300);
        rect.setPosition(200, 100);
        win->draw(rect);

        rect = getRect(colors[BLUE], 300);
        rect.setPosition(300, 150);
        win->draw(rect);
        rect.move(15, 15);
        win->draw(rect);
        rect.move(15, 15);
        win->draw(rect);

        sf::Text text("UP\tshmup :P", font);
        text.setPosition(220, 50);
        win->draw(text);

        text.setString("DOWN\tprecision :o");
        text.setPosition(350, 500);
        win->draw(text);

}

void Startup::event(sf::Event& event) {

        if (event.type != sf::Event::KeyPressed)
                return;

        if (event.key.code == sf::Keyboard::Up)
                game = new Shmup;
        else if (event.key.code == sf::Keyboard::Down)
                game = new Precision;
        else return;

        delete this;

}

int main() {

        sf::Clock clock;

        win = new sf::RenderWindow(sf::VideoMode(800, 600), "Bichrome by Fififox", sf::Style::Default &~ sf::Style::Resize);
        win->setFramerateLimit(60);
        game = new Startup;

        font.loadFromFile("Tuffy.ttf");

        std::mt19937 twister(clock.getElapsedTime().asMilliseconds());
        std::uniform_real_distribution<float> distrib(0, 1);
        urand = std::bind(distrib, twister);

        sf::Music music;
        music.openFromFile("solace.ogg");
        music.setLoop(true);
        music.play();

        float time;
        while (win->isOpen()) {

                clock.restart();

                win->display();
                win->clear(sf::Color(48, 48, 48));
                game->draw();

                sf::Event event;
                while (win->pollEvent(event)) {
                        if ((event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::Escape)
                                or event.type == sf::Event::Closed)
                                return 0;
                        else
                                game->event(event);
                }

                time += clock.getElapsedTime().asSeconds();
                while (time > 0.f) {
                        time -= .005f;
                        game->update();
                }

        }

        return 0;

}
