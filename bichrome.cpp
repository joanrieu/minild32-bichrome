#define M_PI 3.14159265f

#include <random>
#include <functional>
#include <sstream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

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
        rect.SetFillColor(sf::Color::Transparent);
        rect.SetOutlineColor(color);
        rect.SetOutlineThickness(1);
        return rect;
}

sf::RenderWindow* win;
std::function<float()> urand;

int screenBordersHit(sf::FloatRect rect) {

        sf::FloatRect inter;
        sf::FloatRect(0, 0, win->GetWidth(), win->GetHeight()).Intersects(rect, inter);

        int result = 0;

        if (rect.Width - inter.Width > .5f)
                result |= 1;

        if (rect.Height - inter.Height > .5f)
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
        base.SetOrigin(base.GetSize() / 2.f);
        base.SetPosition(sf::Vector2f(win->GetWidth(), win->GetHeight()) / 2.f);

        player = getRect(colors[BLUE], 60);
        player.SetFillColor(player.GetOutlineColor());
        player.SetOrigin(player.GetSize() / 2.f);
        player.SetPosition(base.GetPosition());

        shotRect = getRect(colors[BLUE]);
        shotRect.SetPosition(player.GetPosition());
        shotRect.SetSize(sf::Vector2f(20.f, 1));

        recoil = 0;
        spawner = 0;

        score = 0;
        pause = false;

}

void Shmup::update() {

        if (pause)
                return;

        const float rot = player.GetRotation() + .6f;
        player.SetRotation(rot);

        if (not recoil--) {

                if (sf::Keyboard::IsKeyPressed(sf::Keyboard::Up))
                        shots.push_back(std::make_pair(rot - 90.f, 0.f));
                if (sf::Keyboard::IsKeyPressed(sf::Keyboard::Down))
                        shots.push_back(std::make_pair(rot + 90.f, 0.f));
                if (sf::Keyboard::IsKeyPressed(sf::Keyboard::Left))
                        shots.push_back(std::make_pair(rot + 180.f, 0.f));
                if (sf::Keyboard::IsKeyPressed(sf::Keyboard::Right))
                        shots.push_back(std::make_pair(rot, 0.f));

                recoil = 10;

        }

        if (not spawner--) {
                addOther();
                spawner = 100;
        }

        for (auto& shot: shots)
                shot = std::make_pair(shot.first, shot.second + 5.f);

        const sf::FloatRect sb(0, 0, win->GetWidth(), win->GetHeight());

        for (auto& other: others) {
                std::get<1>(other).Left += std::get<2>(other).x;
                std::get<1>(other).Top += std::get<2>(other).y;
        }

        others.erase(std::remove_if(others.begin(), others.end(), [&](const Other& other) {
                sf::FloatRect inter;
                return not sb.Intersects(std::get<1>(other), inter) or inter == sf::FloatRect();
        }), others.end());

        shots.erase(std::remove_if(shots.begin(), shots.end(), [&](const Shot& shot) {

                sf::Vector2f hit(cos(shot.first * M_PI / 180.f), sin(shot.first * M_PI / 180.f));
                hit *= shot.second;
                hit += player.GetPosition();

                others.erase(std::remove_if(others.begin(), others.end(), [&](const Other& other) {
                        if (not std::get<1>(other).Contains(hit))
                                return false;
                        if (std::get<0>(other) == RED)
                                score += 5;
                        else
                                score -= 10;
                        return true;
                }), others.end());

                return not sb.Contains(hit);

        }), shots.end());

}

void Shmup::draw() {

        std::ostringstream ss;
        ss << "\tScore\t" << score;
        win->Draw(sf::Text(ss.str()));

        win->Draw(base);

        shotRect.SetOrigin(-30.f, 0);

        shotRect.SetRotation(player.GetRotation() - 90.f);
        shotRect.SetOutlineColor(colors[RED]);
        win->Draw(shotRect);
        shotRect.SetOutlineColor(colors[BLUE]);

        shotRect.Rotate(90.f);
        win->Draw(shotRect);

        shotRect.Rotate(90.f);
        win->Draw(shotRect);

        shotRect.Rotate(90.f);
        win->Draw(shotRect);

        win->Draw(player);

        for (const auto& other: others) {
                const sf::FloatRect otherR = std::get<1>(other);
                sf::RectangleShape otherRect;
                otherRect.SetPosition(sf::Vector2f(otherR.Left, otherR.Top));
                otherRect.SetSize(sf::Vector2f(otherR.Width, otherR.Height));
                otherRect.SetFillColor(colors[std::get<0>(other)]);
                win->Draw(otherRect);
        }

        for (const auto& shot: shots) {
                shotRect.SetOrigin(-shot.second, 0);
                shotRect.SetRotation(shot.first);
                win->Draw(shotRect);
        }

}

void Shmup::event(sf::Event& event) {

        if (event.Type == sf::Event::KeyPressed and event.Key.Code == sf::Keyboard::P)
                pause = not pause;

}

void Shmup::addOther() {

        Other other;

        std::get<0>(other) = urand() > .5f ? BLUE : RED;

        do {
                std::get<1>(other) = sf::FloatRect(urand() * win->GetWidth(), urand() * win->GetHeight(),
                        urand() * base.GetSize().x, urand() * base.GetSize().y);
        } while (std::get<1>(other).Intersects(sf::FloatRect(base.GetPosition() - base.GetOrigin(), base.GetSize())));

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

        player.Move(static_cast<int>(urand() * (win->GetWidth() - player.GetSize().x)),
                static_cast<int>(urand() * (win->GetHeight() - player.GetSize().y)));
        enemy.Move(static_cast<int>(urand() * (win->GetWidth() - enemy.GetSize().x)),
                static_cast<int>(urand() * (win->GetHeight() - enemy.GetSize().y)));

}

void Precision::update() {

        sf::FloatRect pb(player.GetPosition(), player.GetSize());
        sf::FloatRect eb(enemy.GetPosition(), enemy.GetSize());

        lost = lost or pb.Intersects(eb);

        enemy.Move(eSpeed);
        eb.Left += eSpeed.x;
        eb.Top += eSpeed.y;

        int borders = screenBordersHit(eb);

        if (borders & 1)
                eSpeed.x = -eSpeed.x;
        if (borders & 2)
                eSpeed.y = -eSpeed.y;

        if (lost)
                return;

        sf::Vector2i mouse = sf::Mouse::GetPosition(*win);

        sf::Vector2f move(mouse.x, mouse.y);
        move -= player.GetPosition() + player.GetSize() / 2.f;

        if (move.x)
                move.x = move.x > 0 ? 1 : -1;
        if (move.y)
                move.y = move.y > 0 ? 1 : -1;

        player.Move(move);
        pb.Left += move.x;
        pb.Top += move.y;

        borders = screenBordersHit(pb);
        move.x = borders & 1 ? -move.x : 0;
        move.y = borders & 2 ? -move.y : 0;
        player.Move(move);

        score = clock.GetElapsedTime().AsSeconds();

}

void Precision::draw() {

        std::ostringstream ss;
        ss << "\tTime\t" << score;
        win->Draw(sf::Text(ss.str()));

        if (lost == 1) {
                lost = 2;
                player.SetOutlineColor(enemy.GetOutlineColor());
        }

        win->Draw(player);
        win->Draw(enemy);

}

struct Startup: Game {
        void draw();
        void event(sf::Event&);
};

void Startup::draw() {

        auto rect = getRect(colors[RED], 300);
        rect.SetPosition(200, 100);
        win->Draw(rect);

        rect = getRect(colors[BLUE], 300);
        rect.SetPosition(300, 150);
        win->Draw(rect);
        rect.Move(15, 15);
        win->Draw(rect);
        rect.Move(15, 15);
        win->Draw(rect);

        sf::Text text(L"↑\tshmup :P");
        text.SetPosition(220, 50);
        win->Draw(text);

        text.SetString(L"↓\tprecision :o");
        text.SetPosition(350, 500);
        win->Draw(text);

}

void Startup::event(sf::Event& event) {

        if (event.Type != sf::Event::KeyPressed)
                return;

        if (event.Key.Code == sf::Keyboard::Up)
                game = new Shmup;
        else if (event.Key.Code == sf::Keyboard::Down)
                game = new Precision;
        else return;

        delete this;

}

int main() {

        sf::Clock clock;

        win = new sf::RenderWindow(sf::VideoMode(800, 600), "Bichrome by Fififox", sf::Style::Default &~ sf::Style::Resize);
        win->SetFramerateLimit(60);
        game = new Startup;

        std::mt19937 twister(clock.GetElapsedTime().AsMilliseconds());
        std::uniform_real_distribution<float> distrib(0, 1);
        urand = std::bind(distrib, twister);

        sf::Music music;
        music.OpenFromFile("solace.ogg");
        music.SetLoop(true);
        music.Play();

        float time;
        while (win->IsOpen()) {

                clock.Restart();

                win->Display();
                win->Clear(sf::Color(48, 48, 48));
                game->draw();

                sf::Event event;
                while (win->PollEvent(event)) {
                        if ((event.Type == sf::Event::KeyPressed and event.Key.Code == sf::Keyboard::Escape)
                                or event.Type == sf::Event::Closed)
                                if (dynamic_cast<Startup*>(game))
                                        return 0;
                                else {
                                        delete game;
                                        game = new Startup;
                                }
                        else
                                game->event(event);
                }

                time += clock.GetElapsedTime().AsSeconds();
                while (time > 0.f) {
                        time -= .005f;
                        game->update();
                }

        }

        return 0;

}
