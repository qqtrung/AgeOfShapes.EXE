#ifndef F32_FPM_WRAPPER
#define F32_FPM_WRAPPER

#include "fixed.hpp"
#include "math.hpp"

#include <SFML/System/Vector2.hpp>

#include <string>

#include <cmath>

namespace f32
{
using raw_fixed = fpm::fixed<std::int64_t, __int128_t, 18>;

struct Fix : public raw_fixed
{
    using raw_fixed::raw_fixed;

    constexpr Fix() noexcept : raw_fixed()
    {
    }
    constexpr Fix(const raw_fixed& v) noexcept : raw_fixed(v)
    {
    }
    constexpr Fix(float v) noexcept : raw_fixed(v)
    {
    }
    constexpr Fix(double v) noexcept : raw_fixed(v)
    {
    }
    constexpr Fix(int v) noexcept : raw_fixed(v)
    {
    }
    constexpr Fix(unsigned int v) noexcept : raw_fixed(static_cast<std::int64_t>(v))
    {
    }
    constexpr Fix(long v) noexcept : raw_fixed(v)
    {
    }
    constexpr Fix(unsigned long v) noexcept : raw_fixed(static_cast<std::int64_t>(v))
    {
    }
    constexpr Fix(long long v) noexcept : raw_fixed(v)
    {
    }
    constexpr Fix(unsigned long long v) noexcept : raw_fixed(static_cast<std::int64_t>(v))
    {
    }

    constexpr operator float() const noexcept
    {
        return static_cast<float>(static_cast<const raw_fixed&>(*this));
    }

    constexpr explicit operator double() const noexcept
    {
        return static_cast<double>(static_cast<const raw_fixed&>(*this));
    }

    template <typename T>
        requires std::is_integral_v<T>
    constexpr explicit operator T() const noexcept
    {
        return static_cast<T>(static_cast<const raw_fixed&>(*this));
    }
};

using fixed = Fix;

constexpr Fix pi = Fix(raw_fixed::pi());

inline constexpr Fix operator+(Fix a, Fix b) noexcept
{
    return Fix(static_cast<const raw_fixed&>(a) + static_cast<const raw_fixed&>(b));
}
inline constexpr Fix operator-(Fix a, Fix b) noexcept
{
    return Fix(static_cast<const raw_fixed&>(a) - static_cast<const raw_fixed&>(b));
}
inline constexpr Fix operator*(Fix a, Fix b) noexcept
{
    return Fix(static_cast<const raw_fixed&>(a) * static_cast<const raw_fixed&>(b));
}
inline constexpr Fix operator/(Fix a, Fix b) noexcept
{
    return Fix(static_cast<const raw_fixed&>(a) / static_cast<const raw_fixed&>(b));
}
inline constexpr Fix operator-(Fix a) noexcept
{
    return Fix(-static_cast<const raw_fixed&>(a));
}

inline constexpr Fix& operator+=(Fix& a, Fix b) noexcept
{
    a = a + b;
    return a;
}
inline constexpr Fix& operator-=(Fix& a, Fix b) noexcept
{
    a = a - b;
    return a;
}
inline constexpr Fix& operator*=(Fix& a, Fix b) noexcept
{
    a = a * b;
    return a;
}
inline constexpr Fix& operator/=(Fix& a, Fix b) noexcept
{
    a = a / b;
    return a;
}

inline constexpr bool operator==(Fix a, Fix b) noexcept
{
    return static_cast<const raw_fixed&>(a) == static_cast<const raw_fixed&>(b);
}
inline constexpr bool operator!=(Fix a, Fix b) noexcept
{
    return static_cast<const raw_fixed&>(a) != static_cast<const raw_fixed&>(b);
}
inline constexpr bool operator<(Fix a, Fix b) noexcept
{
    return static_cast<const raw_fixed&>(a) < static_cast<const raw_fixed&>(b);
}
inline constexpr bool operator<=(Fix a, Fix b) noexcept
{
    return static_cast<const raw_fixed&>(a) <= static_cast<const raw_fixed&>(b);
}
inline constexpr bool operator>(Fix a, Fix b) noexcept
{
    return static_cast<const raw_fixed&>(a) > static_cast<const raw_fixed&>(b);
}
inline constexpr bool operator>=(Fix a, Fix b) noexcept
{
    return static_cast<const raw_fixed&>(a) >= static_cast<const raw_fixed&>(b);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix operator+(Fix a, T b) noexcept
{
    return a + Fix(b);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix operator+(T a, Fix b) noexcept
{
    return Fix(a) + b;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix operator-(Fix a, T b) noexcept
{
    return a - Fix(b);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix operator-(T a, Fix b) noexcept
{
    return Fix(a) - b;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix operator*(Fix a, T b) noexcept
{
    return a * Fix(b);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix operator*(T a, Fix b) noexcept
{
    return Fix(a) * b;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix operator/(Fix a, T b) noexcept
{
    return a / Fix(b);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix operator/(T a, Fix b) noexcept
{
    return Fix(a) / b;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix& operator+=(Fix& a, T b) noexcept
{
    a = a + Fix(b);
    return a;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix& operator-=(Fix& a, T b) noexcept
{
    a = a - Fix(b);
    return a;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix& operator*=(Fix& a, T b) noexcept
{
    a = a * Fix(b);
    return a;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Fix& operator/=(Fix& a, T b) noexcept
{
    a = a / Fix(b);
    return a;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator==(Fix a, T b) noexcept
{
    return a == Fix(b);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator==(T a, Fix b) noexcept
{
    return Fix(a) == b;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator!=(Fix a, T b) noexcept
{
    return a != Fix(b);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator!=(T a, Fix b) noexcept
{
    return Fix(a) != b;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator<(Fix a, T b) noexcept
{
    return a < Fix(b);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator<(T a, Fix b) noexcept
{
    return Fix(a) < b;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator<=(Fix a, T b) noexcept
{
    return a <= Fix(b);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator<=(T a, Fix b) noexcept
{
    return Fix(a) <= b;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator>(Fix a, T b) noexcept
{
    return a > Fix(b);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator>(T a, Fix b) noexcept
{
    return Fix(a) > b;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator>=(Fix a, T b) noexcept
{
    return a >= Fix(b);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr bool operator>=(T a, Fix b) noexcept
{
    return Fix(a) >= b;
}

inline std::string to_string(Fix x)
{
    std::int64_t raw = x.raw_value();
    if (raw == 0)
        return "0.000000";

    bool          neg   = false;
    std::uint64_t u_raw = 0;
    if (raw < 0)
    {
        neg   = true;
        u_raw = static_cast<std::uint64_t>(0 - raw);
    }
    else
    {
        u_raw = static_cast<std::uint64_t>(raw);
    }

    constexpr std::uint64_t FRAC     = 1ULL << 18;
    std::uint64_t           intPart  = u_raw / FRAC;
    std::uint64_t           fracPart = u_raw % FRAC;
    std::uint64_t           fracDec  = (fracPart * 1000000ULL) / FRAC;

    std::string fracStr = std::to_string(fracDec);
    while (fracStr.length() < 6)
        fracStr = "0" + fracStr;

    std::string res;
    if (neg)
        res += "-";
    res += std::to_string(intPart) + "." + fracStr;
    return res;
}

} // namespace f32

namespace std
{
inline string to_string(f32::Fix x)
{
    return f32::to_string(x);
}
} // namespace std

namespace f32
{

inline constexpr Fix toFixed(float x)
{
    return Fix(x);
}

inline constexpr Fix toFixed(double x)
{
    return Fix(x);
}

inline constexpr float toFloat(Fix x)
{
    return static_cast<float>(x);
}

inline constexpr double toDouble(Fix x)
{
    return static_cast<double>(static_cast<const raw_fixed&>(x));
}

inline Fix sin(Fix x)
{
    return Fix(fpm::sin(static_cast<const raw_fixed&>(x)));
}
inline Fix cos(Fix x)
{
    return Fix(fpm::cos(static_cast<const raw_fixed&>(x)));
}
inline Fix tan(Fix x)
{
    return Fix(fpm::tan(static_cast<const raw_fixed&>(x)));
}
inline Fix asin(Fix x)
{
    return Fix(fpm::asin(static_cast<const raw_fixed&>(x)));
}
inline Fix acos(Fix x)
{
    return Fix(fpm::acos(static_cast<const raw_fixed&>(x)));
}
inline Fix atan(Fix x)
{
    return Fix(fpm::atan(static_cast<const raw_fixed&>(x)));
}
inline Fix atan2(Fix y, Fix x)
{
    return Fix(fpm::atan2(static_cast<const raw_fixed&>(y), static_cast<const raw_fixed&>(x)));
}
inline Fix sqrt(Fix x)
{
    return Fix(fpm::sqrt(static_cast<const raw_fixed&>(x)));
}
inline Fix cbrt(Fix x)
{
    return Fix(fpm::cbrt(static_cast<const raw_fixed&>(x)));
}
inline Fix log(Fix x)
{
    return Fix(fpm::log(static_cast<const raw_fixed&>(x)));
}
inline Fix log10(Fix x)
{
    return Fix(fpm::log10(static_cast<const raw_fixed&>(x)));
}
inline Fix exp(Fix x)
{
    return Fix(fpm::exp(static_cast<const raw_fixed&>(x)));
}
inline Fix pow(Fix x, Fix y)
{
    return Fix(fpm::pow(static_cast<const raw_fixed&>(x), static_cast<const raw_fixed&>(y)));
}
inline Fix abs(Fix x)
{
    return Fix(fpm::abs(static_cast<const raw_fixed&>(x)));
}
inline Fix floor(Fix x)
{
    return Fix(fpm::floor(static_cast<const raw_fixed&>(x)));
}
inline Fix ceil(Fix x)
{
    return Fix(fpm::ceil(static_cast<const raw_fixed&>(x)));
}
inline Fix round(Fix x)
{
    return Fix(fpm::round(static_cast<const raw_fixed&>(x)));
}
inline Fix fmod(Fix x, Fix y)
{
    return Fix(fpm::fmod(static_cast<const raw_fixed&>(x), static_cast<const raw_fixed&>(y)));
}
inline Fix hypot(Fix x, Fix y)
{
    if (x == Fix(0) && y == Fix(0))
        return Fix(0);
    return Fix(fpm::hypot(static_cast<const raw_fixed&>(x), static_cast<const raw_fixed&>(y)));
}

inline float sin(float x)
{
    return std::sin(x);
}
inline float cos(float x)
{
    return std::cos(x);
}
inline float tan(float x)
{
    return std::tan(x);
}
inline float asin(float x)
{
    return std::asin(x);
}
inline float acos(float x)
{
    return std::acos(x);
}
inline float atan(float x)
{
    return std::atan(x);
}
inline float atan2(float y, float x)
{
    return std::atan2(y, x);
}
inline float sqrt(float x)
{
    return std::sqrt(x);
}
inline float cbrt(float x)
{
    return std::cbrt(x);
}
inline float log(float x)
{
    return std::log(x);
}
inline float log10(float x)
{
    return std::log10(x);
}
inline float exp(float x)
{
    return std::exp(x);
}
inline float pow(float x, float y)
{
    return std::pow(x, y);
}
inline float abs(float x)
{
    return std::abs(x);
}
inline float floor(float x)
{
    return std::floor(x);
}
inline float ceil(float x)
{
    return std::ceil(x);
}
inline float round(float x)
{
    return std::round(x);
}
inline float fmod(float x, float y)
{
    return std::fmod(x, y);
}
inline float hypot(float x, float y)
{
    return std::hypot(x, y);
}

inline double sin(double x)
{
    return std::sin(x);
}
inline double cos(double x)
{
    return std::cos(x);
}
inline double tan(double x)
{
    return std::tan(x);
}
inline double asin(double x)
{
    return std::asin(x);
}
inline double acos(double x)
{
    return std::acos(x);
}
inline double atan(double x)
{
    return std::atan(x);
}
inline double atan2(double y, double x)
{
    return std::atan2(y, x);
}
inline double sqrt(double x)
{
    return std::sqrt(x);
}
inline double cbrt(double x)
{
    return std::cbrt(x);
}
inline double log(double x)
{
    return std::log(x);
}
inline double log10(double x)
{
    return std::log10(x);
}
inline double exp(double x)
{
    return std::exp(x);
}
inline double pow(double x, double y)
{
    return std::pow(x, y);
}
inline double abs(double x)
{
    return std::abs(x);
}
inline double floor(double x)
{
    return std::floor(x);
}
inline double ceil(double x)
{
    return std::ceil(x);
}
inline double round(double x)
{
    return std::round(x);
}
inline double fmod(double x, double y)
{
    return std::fmod(x, y);
}
inline double hypot(double x, double y)
{
    return std::hypot(x, y);
}

struct Vec2 : public sf::Vector2<Fix>
{
    using sf::Vector2<Fix>::Vector2;

    constexpr Vec2() noexcept : sf::Vector2<Fix>(Fix(0), Fix(0))
    {
    }
    constexpr Vec2(Fix x_, Fix y_) noexcept : sf::Vector2<Fix>(x_, y_)
    {
    }
    constexpr Vec2(const sf::Vector2<Fix>& v) noexcept : sf::Vector2<Fix>(v.x, v.y)
    {
    }
    constexpr Vec2(const sf::Vector2f& v) noexcept : sf::Vector2<Fix>(Fix(v.x), Fix(v.y))
    {
    }
    constexpr Vec2(const sf::Vector2<double>& v) noexcept : sf::Vector2<Fix>(Fix(v.x), Fix(v.y))
    {
    }

    constexpr operator sf::Vector2f() const noexcept
    {
        return sf::Vector2f(static_cast<float>(x), static_cast<float>(y));
    }

    constexpr explicit operator sf::Vector2<double>() const noexcept
    {
        return sf::Vector2<double>(static_cast<double>(x), static_cast<double>(y));
    }

    Vec2 normalized() const noexcept;
    Fix  length() const noexcept;
};

inline constexpr Vec2 operator+(const Vec2& a, const Vec2& b) noexcept
{
    return Vec2(a.x + b.x, a.y + b.y);
}
inline constexpr Vec2 operator-(const Vec2& a, const Vec2& b) noexcept
{
    return Vec2(a.x - b.x, a.y - b.y);
}
inline constexpr Vec2 operator*(const Vec2& a, Fix s) noexcept
{
    return Vec2(a.x * s, a.y * s);
}
inline constexpr Vec2 operator*(Fix s, const Vec2& a) noexcept
{
    return Vec2(a.x * s, a.y * s);
}
inline constexpr Vec2 operator/(const Vec2& a, Fix s) noexcept
{
    return Vec2(a.x / s, a.y / s);
}
inline constexpr Vec2 operator-(const Vec2& a) noexcept
{
    return Vec2(-a.x, -a.y);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Vec2 operator*(const Vec2& a, T s) noexcept
{
    return Vec2(a.x * Fix(s), a.y * Fix(s));
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Vec2 operator*(T s, const Vec2& a) noexcept
{
    return Vec2(Fix(s) * a.x, Fix(s) * a.y);
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Vec2 operator/(const Vec2& a, T s) noexcept
{
    return Vec2(a.x / Fix(s), a.y / Fix(s));
}

inline constexpr Vec2& operator+=(Vec2& a, const Vec2& b) noexcept
{
    a.x += b.x;
    a.y += b.y;
    return a;
}
inline constexpr Vec2& operator-=(Vec2& a, const Vec2& b) noexcept
{
    a.x -= b.x;
    a.y -= b.y;
    return a;
}
inline constexpr Vec2& operator*=(Vec2& a, Fix s) noexcept
{
    a.x *= s;
    a.y *= s;
    return a;
}
inline constexpr Vec2& operator/=(Vec2& a, Fix s) noexcept
{
    a.x /= s;
    a.y /= s;
    return a;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Vec2& operator*=(Vec2& a, T s) noexcept
{
    a.x *= Fix(s);
    a.y *= Fix(s);
    return a;
}

template <typename T>
    requires std::is_arithmetic_v<T>
inline constexpr Vec2& operator/=(Vec2& a, T s) noexcept
{
    a.x /= Fix(s);
    a.y /= Fix(s);
    return a;
}

inline constexpr bool operator==(const Vec2& a, const Vec2& b) noexcept
{
    return a.x == b.x && a.y == b.y;
}
inline constexpr bool operator!=(const Vec2& a, const Vec2& b) noexcept
{
    return a.x != b.x || a.y != b.y;
}

inline Fix length(const Vec2& v)
{
    return hypot(v.x, v.y);
}

inline Vec2 normalized(const Vec2& v)
{
    Fix len = length(v);
    if (len > Fix(1) / 10000)
    {
        return v / len;
    }
    return Vec2(Fix(0), Fix(0));
}

inline Vec2 Vec2::normalized() const noexcept
{
    return f32::normalized(*this);
}

inline Fix Vec2::length() const noexcept
{
    return f32::length(*this);
}

struct Rect
{
    Vec2 pos;
    Vec2 size;

    constexpr Rect() noexcept : pos(), size()
    {
    }
    constexpr Rect(Fix x, Fix y, Fix w, Fix h) noexcept : pos(x, y), size(w, h)
    {
    }
    constexpr Rect(const Vec2& pos_, const Vec2& size_) noexcept : pos(pos_), size(size_)
    {
    }

    [[nodiscard]] constexpr bool contains(const Vec2& point) const noexcept
    {
        return point.x >= pos.x && point.x <= pos.x + size.x && point.y >= pos.y && point.y <= pos.y + size.y;
    }

    [[nodiscard]] constexpr bool intersects(const Rect& other) const noexcept
    {
        return pos.x < other.pos.x + other.size.x && pos.x + size.x > other.pos.x &&
               pos.y < other.pos.y + other.size.y && pos.y + size.y > other.pos.y;
    }
};

inline float length(const sf::Vector2f& v)
{
    return hypot(v.x, v.y);
}

inline sf::Vector2f normalized(const sf::Vector2f& v)
{
    float len = length(v);
    if (len > 0.0001f)
    {
        const double invLen = 1.0 / static_cast<double>(len);
        return sf::Vector2f{static_cast<float>(v.x * invLen), static_cast<float>(v.y * invLen)};
    }
    return sf::Vector2f{0.f, 0.f};
}

} // namespace f32

#endif
