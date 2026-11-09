#ifndef VECTOR_FIELD_HPP
#define VECTOR_FIELD_HPP

#include "FloatFixed.hpp"
#include "Grid.hpp"
#include "MemoryPool.hpp"

#include <SFML/System/Vector2.hpp>

#include <array>
#include <vector>

#include <cstdint>

struct VectorField
{
    enum Direction : uint8_t
    {
        UNVISITED = 0,
        UP,
        LEFT,
        DOWN,
        RIGHT,
        UP_LEFT,
        UP_RIGHT,
        DOWN_LEFT,
        DOWN_RIGHT,
        SOURCE,
        BLOCKED,
    };

    struct Action
    {
        int dirX{};
        int dirY{};
        int cost{};
    };

    struct Cell
    {
        int x{};
        int y{};
    };

    static constexpr std::array<Action, 8> mAction = {
        {{.dirX = 0, .dirY = 1, .cost = 100},
         {.dirX = -1, .dirY = 0, .cost = 100},
         {.dirX = 1, .dirY = 0, .cost = 100},
         {.dirX = 0, .dirY = -1, .cost = 100},

         {.dirX = 1, .dirY = 1, .cost = 151},
         {.dirX = -1, .dirY = 1, .cost = 151},
         {.dirX = 1, .dirY = -1, .cost = 151},
         {.dirX = -1, .dirY = -1, .cost = 151}}};

    static constexpr auto&  action      = mAction;
    static constexpr size_t actionCount = 4;
    static constexpr int    maxAction   = 4;

    int mSpacing = 8;
    int mWidth   = 0;
    int mHeight  = 0;

    Grid<int>&    mGrid;
    Grid<uint8_t> mDirections;

    explicit VectorField(Grid<int>& grid) : mGrid(grid)
    {
    }

    void compute(int spacing, f32::Fix width, f32::Fix height, MemoryPool& pool, const std::vector<int>& idAgents);
    void computeDirection(const std::vector<int>& idAgents, MemoryPool& pool);

    [[nodiscard]] int           getGrid(int x, int y) const;
    [[nodiscard]] int           getDistance(int x, int y) const;
    [[nodiscard]] unsigned char getBit(int ox, int oy) const;
    [[nodiscard]] sf::Vector2i  getDirections(int x, int y) const;
    [[nodiscard]] sf::Vector2i  getValidDirectionNear(int x, int y) const;
    [[nodiscard]] sf::Vector2i  getDir(int x, int y) const
    {
        return getDirections(x, y);
    }
    [[nodiscard]] uint8_t getDirection(int x, int y) const;

    [[nodiscard]] bool isValidCell(int x, int y) const;
    [[nodiscard]] int  width() const
    {
        return mWidth;
    }
    [[nodiscard]] int height() const
    {
        return mHeight;
    }
    [[nodiscard]] int spacing() const
    {
        return mSpacing;
    }
};

#endif
