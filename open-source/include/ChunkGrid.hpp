#ifndef CHUNKGRID_HPP
#define CHUNKGRID_HPP

#include "Debug.hpp"
#include "Grid.hpp"

#include <vector>

struct Chunk
{
    std::vector<int> vecIdEntity;

    void remove(int entityID)
    {
        for (int i = 0; i < (int)vecIdEntity.size(); i++)
        {
            if (vecIdEntity[i] == entityID)
            {
                vecIdEntity[i] = vecIdEntity.back();
                vecIdEntity.pop_back();
                return;
            }
        }

#ifndef NDEBUG
        logout() << "remove : entityId : " << entityID << " not found in chunk\n";
#endif
    }

    void insert(int entityID)
    {

#ifndef NDEBUG
        for (const int id : vecIdEntity)
        {
            if (id == entityID)
            {
                logout() << "insert : entityId : " << entityID << "already exists in chunk\n";
                return;
            }
        }
#endif

        vecIdEntity.push_back(entityID);
    }

    [[nodiscard]] const std::vector<int>& getEntityInCell() const
    {
        return vecIdEntity;
    }
};

struct ChunkGrid
{
    Grid<Chunk> mGrid;

    void create(int width, int height)
    {
        mGrid = Grid<Chunk>(width, height, Chunk{});
    }

    [[nodiscard]] bool isValid(int row, int col) const
    {
        return col >= 0 && col < mGrid.width() && row >= 0 && row < mGrid.height();
    }

    [[nodiscard]] Chunk& get(int row, int col)
    {
        return mGrid[col][row];
    }

    [[nodiscard]] const Chunk& get(int row, int col) const
    {
        return mGrid[col][row];
    }

    void insert(int row, int col, int entityID)
    {
        if (!isValid(row, col))
            return;
        get(row, col).insert(entityID);
    }

    void remove(int row, int col, int entityID)
    {
        if (!isValid(row, col))
            return;
        get(row, col).remove(entityID);
    }

    [[nodiscard]] int width() const
    {
        return mGrid.width();
    }
    [[nodiscard]] int height() const
    {
        return mGrid.height();
    }
};

#endif
