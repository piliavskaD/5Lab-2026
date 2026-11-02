#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

constexpr float PI = 3.14159265f;
constexpr float G = 9.81f;
constexpr float PX_PER_M = 100.f;

constexpr float WINDOW_W = 900.f;
constexpr float WINDOW_H = 650.f;
constexpr float GROUND_Y = 560.f;
constexpr float BASE_X = 820.f;
constexpr float PLANE_LEN = 500.f;
constexpr float BOX_W = 50.f;
constexpr float BOX_H = 30.f;
constexpr float START_OFFSET = 40.f;

constexpr float S_MAX = (PLANE_LEN - START_OFFSET - BOX_W / 2.f) / PX_PER_M;

struct Model
{
    float alphaDeg = 30.f;
    float mu = 0.30f;

    float s = 0.f;
    float v = 0.f;
    float a = 0.f;
    float t = 0.f;

    bool running = false;
    bool finished = false;
    bool sliding = false;
};

float toRad(float deg) { return deg * PI / 180.f; }

void resetModel(Model& m)
{
    m.s = 0.f;
    m.v = 0.f;
    m.a = 0.f;
    m.t = 0.f;
    m.running = false;
    m.finished = false;
    m.sliding = false;
}

void updatePhysics(Model& m, float dt)
{
    if (!m.running || m.finished)
        return;

    const float alpha = toRad(m.alphaDeg);
    const float sinA = std::sin(alpha);
    const float cosA = std::cos(alpha);

    const bool canSlide = sinA > m.mu * cosA;

    if (m.v <= 0.f && !canSlide)
    {
        m.v = 0.f;
        m.a = 0.f;
        m.sliding = false;
        return;
    }

    m.sliding = true;

    m.a = G * (sinA - m.mu * cosA);

    m.v += m.a * dt;

    if (m.v < 0.f)
    {
        m.v = 0.f;
        m.a = 0.f;
        m.sliding = false;
    }

    m.s += m.v * dt;
    m.t += dt;

    if (m.s >= S_MAX)
    {
        m.s = S_MAX;
        m.finished = true;
        m.sliding = false;
    }
}

void handleInput(sf::RenderWindow& window, Model& m)
{
    while (const std::optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
        }

        if (const auto* key = event->getIf<sf::Event::KeyPressed>())
        {
            switch (key->code)
            {
            case sf::Keyboard::Key::Up:      m.alphaDeg += 1.f;   break;
            case sf::Keyboard::Key::Down:    m.alphaDeg -= 1.f;   break;
            case sf::Keyboard::Key::Right:   m.mu += 0.05f;       break;
            case sf::Keyboard::Key::Left:    m.mu -= 0.05f;       break;

            case sf::Keyboard::Key::Space:
                if (!m.finished)
                    m.running = !m.running;
                break;

            case sf::Keyboard::Key::R:
                resetModel(m);
                break;

            case sf::Keyboard::Key::Escape:
                window.close();
                break;

            default: break;
            }

            m.alphaDeg = std::clamp(m.alphaDeg, 0.f, 60.f);
            m.mu = std::clamp(m.mu, 0.f, 1.f);
        }
    }
}

sf::String utf8(const std::string& s)
{
    return sf::String::fromUtf8(s.begin(), s.end());
}

std::string fmt(float value, int precision)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

bool loadFont(sf::Font& font)
{
    const char* paths[] = {
        "../resources/fonts/arial.ttf",
        "resources/fonts/arial.ttf",
        "../../resources/fonts/arial.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };
    for (const char* p : paths)
    {
        if (font.openFromFile(p))
            return true;
    }
    return false;
}

std::string statusText(const Model& m)
{
    if (m.finished)
        return "Тіло досягло кінця площини";
    if (!m.running && m.s == 0.f && m.t == 0.f)
        return "Очікування (Space - старт)";
    if (!m.running)
        return "Пауза";
    if (m.sliding)
        return "Тіло ковзає";
    return "Тіло стоїть (tg α <= μ)";
}

void drawScene(sf::RenderWindow& window, const Model& m, const sf::Font& font)
{
    window.clear(sf::Color(25, 25, 35));

    const float alpha = toRad(m.alphaDeg);
    const float cosA = std::cos(alpha);
    const float sinA = std::sin(alpha);

    sf::RectangleShape ground({ WINDOW_W, WINDOW_H - GROUND_Y });
    ground.setFillColor(sf::Color(60, 60, 60));
    ground.setPosition({ 0.f, GROUND_Y });
    window.draw(ground);

    const sf::Vector2f B(BASE_X, GROUND_Y);
    const sf::Vector2f T(BASE_X - PLANE_LEN * cosA, GROUND_Y - PLANE_LEN * sinA);
    const sf::Vector2f C(T.x, GROUND_Y);

    sf::ConvexShape plane;
    plane.setPointCount(3);
    plane.setPoint(0, T);
    plane.setPoint(1, B);
    plane.setPoint(2, C);
    plane.setFillColor(sf::Color(110, 110, 110));
    window.draw(plane);

    const float distPx = START_OFFSET + m.s * PX_PER_M;
    const sf::Vector2f pos = T + sf::Vector2f(cosA, sinA) * distPx;

    sf::RectangleShape box({ BOX_W, BOX_H });
    box.setOrigin({ BOX_W / 2.f, BOX_H });
    box.setPosition(pos);
    box.setRotation(sf::degrees(m.alphaDeg));

    if (m.finished)
        box.setFillColor(sf::Color(200, 80, 80));
    else if (m.sliding)
        box.setFillColor(sf::Color(80, 200, 100));
    else
        box.setFillColor(sf::Color(240, 180, 60));
    window.draw(box);

    const float alphaCrit = std::atan(m.mu) * 180.f / PI;
    const float aTheory = std::max(0.f, G * (sinA - m.mu * cosA));

    std::string info =
        "Кут нахилу α: " + fmt(m.alphaDeg, 1) + "°\n"
        "Коефіцієнт тертя μ: " + fmt(m.mu, 2) + "\n"
        "Критичний кут arctg(μ): " + fmt(alphaCrit, 1) + "°\n"
        "Теор. прискорення: " + fmt(aTheory, 3) + " м/с²\n"
        "Прискорення a: " + fmt(m.a, 3) + " м/с²\n"
        "Швидкість v: " + fmt(m.v, 3) + " м/с\n"
        "Шлях s: " + fmt(m.s, 3) + " м\n"
        "Час t: " + fmt(m.t, 3) + " с\n"
        "Стан: " + statusText(m);

    sf::Text infoText(font, utf8(info), 18);
    infoText.setFillColor(sf::Color::White);
    infoText.setPosition({ 15.f, 10.f });
    window.draw(infoText);

    sf::Text help(font,
        utf8("↑/↓ - кут α | ←/→ - тертя μ | Space - старт/пауза | R - перезапуск | Esc - вихід"),
        15);
    help.setFillColor(sf::Color(220, 220, 220));
    help.setPosition({ 15.f, GROUND_Y + 25.f });
    window.draw(help);

    window.display();
}

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({ static_cast<unsigned>(WINDOW_W), static_cast<unsigned>(WINDOW_H) }),
        "SFML Physics Demo - Inclined plane");
    window.setFramerateLimit(120);

    sf::Font font;
    if (!loadFont(font))
        return 1;

    Model model;
    sf::Clock clock;

    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();
        dt = std::min(dt, 0.02f);

        handleInput(window, model);
        updatePhysics(model, dt);
        drawScene(window, model, font);
    }

    return 0;
}