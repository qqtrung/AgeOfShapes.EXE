#include "VectorField.hpp"

#include "Components.hpp"
#include "Queue.hpp"

void VectorField::compute(int spacing, f32::Fix width, f32::Fix height, MemoryPool& pool, const std::vector<int>& idAgents)
{
    if (spacing <= 0 || width <= f32::Fix(0) || height <= f32::Fix(0))
        return;

    mSpacing = spacing;
    mWidth   = static_cast<int>(width) / mSpacing;
    mHeight  = static_cast<int>(height) / mSpacing;

    computeDirection(idAgents, pool);
}

void VectorField::computeDirection(const std::vector<int>& idAgents, MemoryPool& pool)
{
    const int w       = mWidth;
    const int h       = mHeight;
    const int spacing = mSpacing;

    mDirections.assign(w, h, UNVISITED);

    uint8_t* const   dirData  = mDirections.mData.data();
    const int* const gridData = mGrid.mData.data();

    struct BfsNode
    {
        uint16_t x;
        uint16_t y;
        uint32_t idx;
    };

    static Queue<BfsNode> queue;
    if (queue.q.capacity() < static_cast<size_t>(w * h))
    {
        queue.reserve(w * h);
    }
    queue.reset();

    for (const int idAgent : idAgents)
    {
        const auto* mask = pool.getMask(idAgent);
        if (!pool.hasComponent<CPosition>(mask))
        {
            continue;
        }

        const bool isDesignatedPoint = pool.hasComponent<CNavBeacon>(mask);
        if (pool.hasComponent<CRectDragDrop>(mask) && !isDesignatedPoint)
        {
            bool       seeded = false;
            const auto rect   = pool.getComponent<CRectDragDrop>(idAgent);

            const f32::Fix radiusPadding = f32::Fix(14);
            const int      minX = std::max(0, static_cast<int>((rect->pos.x - radiusPadding) / f32::Fix(spacing)));
            const int      minY = std::max(0, static_cast<int>((rect->pos.y - radiusPadding) / f32::Fix(spacing)));
            const int maxX = std::min(w - 1,
                                      static_cast<int>((rect->pos.x + rect->size.x + radiusPadding) / f32::Fix(spacing)) + 1);
            const int maxY = std::min(h - 1,
                                      static_cast<int>((rect->pos.y + rect->size.y + radiusPadding) / f32::Fix(spacing)) + 1);

            for (int x = minX; x <= maxX; ++x)
            {
                const int colOffset = x * h;
                for (int y = minY; y <= maxY; ++y)
                {
                    const int idx = colOffset + y;
                    if (gridData[idx] == idAgent)
                    {
                        if (dirData[idx] != SOURCE)
                        {
                            dirData[idx] = SOURCE;
                            queue.push({static_cast<uint16_t>(x), static_cast<uint16_t>(y), static_cast<uint32_t>(idx)});
                            seeded = true;
                        }
                    }
                }
            }

            if (!seeded)
            {
                const auto compPosition = pool.getComponent<CPosition>(idAgent);
                if (compPosition->pos.x >= f32::Fix(0) && compPosition->pos.y >= f32::Fix(0))
                {
                    const int cellX = static_cast<int>(compPosition->pos.x) / spacing;
                    const int cellY = static_cast<int>(compPosition->pos.y) / spacing;

                    if (static_cast<unsigned>(cellX) < static_cast<unsigned>(w) &&
                        static_cast<unsigned>(cellY) < static_cast<unsigned>(h))
                    {
                        const int idx = cellX * h + cellY;
                        if (dirData[idx] != SOURCE)
                        {
                            dirData[idx] = SOURCE;
                            queue.push(
                                {static_cast<uint16_t>(cellX), static_cast<uint16_t>(cellY), static_cast<uint32_t>(idx)});
                        }
                    }
                }
            }
        }
        else
        {
            const auto compPosition = pool.getComponent<CPosition>(idAgent);

            if (compPosition->pos.x < f32::Fix(0) || compPosition->pos.y < f32::Fix(0))
            {
                continue;
            }

            const int cellX = static_cast<int>(compPosition->pos.x) / spacing;
            const int cellY = static_cast<int>(compPosition->pos.y) / spacing;

            if (static_cast<unsigned>(cellX) >= static_cast<unsigned>(w) ||
                static_cast<unsigned>(cellY) >= static_cast<unsigned>(h))
            {
                continue;
            }

            const int idx = cellX * h + cellY;
            if (dirData[idx] == SOURCE)
            {
                continue;
            }

            dirData[idx] = SOURCE;
            queue.push({static_cast<uint16_t>(cellX), static_cast<uint16_t>(cellY), static_cast<uint32_t>(idx)});
        }
    }

    while (!queue.empty())
    {
        const BfsNode current = queue.front();
        queue.pop();

        if (current.y + 1 < h)
        {
            const uint32_t nIndex = current.idx + 1;
            if (dirData[nIndex] == UNVISITED && gridData[nIndex] == 0)
            {
                dirData[nIndex] = UP;
                queue.push({current.x, static_cast<uint16_t>(current.y + 1), nIndex});
            }
        }

        if (current.x > 0)
        {
            const uint32_t nIndex = current.idx - h;
            if (dirData[nIndex] == UNVISITED && gridData[nIndex] == 0)
            {
                dirData[nIndex] = RIGHT;
                queue.push({static_cast<uint16_t>(current.x - 1), current.y, nIndex});
            }
        }

        if (current.x + 1 < w)
        {
            const uint32_t nIndex = current.idx + h;
            if (dirData[nIndex] == UNVISITED && gridData[nIndex] == 0)
            {
                dirData[nIndex] = LEFT;
                queue.push({static_cast<uint16_t>(current.x + 1), current.y, nIndex});
            }
        }

        if (current.y > 0)
        {
            const uint32_t nIndex = current.idx - 1;
            if (dirData[nIndex] == UNVISITED && gridData[nIndex] == 0)
            {
                dirData[nIndex] = DOWN;
                queue.push({current.x, static_cast<uint16_t>(current.y - 1), nIndex});
            }
        }
    }
}

int VectorField::getGrid(int x, int y) const
{
    if (!mGrid.isValidCell(x, y))
        return 0;
    return mGrid[x][y];
}

int VectorField::getDistance(int x, int y) const
{
    (void)x;
    (void)y;
    return 0;
}

unsigned char VectorField::getBit(int ox, int oy) const
{
    if (ox == 0 && oy == 1)
        return 1 << 0;

    if (ox == -1 && oy == 0)
        return 1 << 1;

    if (ox == 1 && oy == 0)
        return 1 << 2;

    if (ox == 0 && oy == -1)
        return 1 << 3;

    if (ox == 1 && oy == 1)
        return 1 << 4;

    if (ox == -1 && oy == 1)
        return 1 << 5;

    if (ox == 1 && oy == -1)
        return 1 << 6;

    if (ox == -1 && oy == -1)
        return 1 << 7;

    return 0;
}

bool VectorField::isValidCell(int x, int y) const
{
    return x >= 0 && y >= 0 && x < mWidth && y < mHeight && mGrid.isValidCell(x, y);
}

uint8_t VectorField::getDirection(int x, int y) const
{
    if (!isValidCell(x, y) || mDirections.mData.empty())
        return UNVISITED;

    return mDirections[x][y];
}

sf::Vector2i VectorField::getDirections(int x, int y) const
{
    if (!isValidCell(x, y) || mDirections.mData.empty())
        return {0, 0};

    const uint8_t direction = mDirections[x][y];

    switch (direction)
    {
        case UP:
            return {0, -1};

        case LEFT:
            return {-1, 0};

        case DOWN:
            return {0, 1};

        case RIGHT:
            return {1, 0};

        case UP_LEFT:
            return {-1, -1};

        case UP_RIGHT:
            return {1, -1};

        case DOWN_LEFT:
            return {-1, 1};

        case DOWN_RIGHT:
            return {1, 1};

        case SOURCE:
        case BLOCKED:
        case UNVISITED:
        default:
            return {0, 0};
    }
}

sf::Vector2i VectorField::getValidDirectionNear(int x, int y) const
{
    if (getDirection(x, y) == SOURCE)
    {
        return {0, 0};
    }

    static constexpr std::array<std::pair<int, int>, 168> offsets = {
        {{0, -1},  {0, 1},  {-1, 0}, {1, 0}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1},

         {0, -2},  {0, 2},  {-2, 0}, {2, 0}, {-1, -2}, {1, -2}, {-1, 2}, {1, 2}, {-2, -1}, {2, -1}, {-2, 1}, {2, 1},
         {-2, -2}, {2, -2}, {-2, 2}, {2, 2},

         {0, -3},  {0, 3},  {-3, 0}, {3, 0}, {-1, -3}, {1, -3}, {-1, 3}, {1, 3}, {-2, -3}, {2, -3}, {-2, 3}, {2, 3},
         {-3, -1}, {3, -1}, {-3, 1}, {3, 1}, {-3, -2}, {3, -2}, {-3, 2}, {3, 2}, {-3, -3}, {3, -3}, {-3, 3}, {3, 3},

         {0, -4},  {0, 4},  {-4, 0}, {4, 0}, {-1, -4}, {1, -4}, {-1, 4}, {1, 4}, {-2, -4}, {2, -4}, {-2, 4}, {2, 4},
         {-3, -4}, {3, -4}, {-3, 4}, {3, 4}, {-4, -1}, {4, -1}, {-4, 1}, {4, 1}, {-4, -2}, {4, -2}, {-4, 2}, {4, 2},
         {-4, -3}, {4, -3}, {-4, 3}, {4, 3}, {-4, -4}, {4, -4}, {-4, 4}, {4, 4},

         {0, -5},  {0, 5},  {-5, 0}, {5, 0}, {-1, -5}, {1, -5}, {-1, 5}, {1, 5}, {-2, -5}, {2, -5}, {-2, 5}, {2, 5},
         {-3, -5}, {3, -5}, {-3, 5}, {3, 5}, {-4, -5}, {4, -5}, {-4, 5}, {4, 5}, {-5, -1}, {5, -1}, {-5, 1}, {5, 1},
         {-5, -2}, {5, -2}, {-5, 2}, {5, 2}, {-5, -3}, {5, -3}, {-5, 3}, {5, 3}, {-5, -4}, {5, -4}, {-5, 4}, {5, 4},
         {-5, -5}, {5, -5}, {-5, 5}, {5, 5},

         {0, -6},  {0, 6},  {-6, 0}, {6, 0}, {-1, -6}, {1, -6}, {-1, 6}, {1, 6}, {-2, -6}, {2, -6}, {-2, 6}, {2, 6},
         {-3, -6}, {3, -6}, {-3, 6}, {3, 6}, {-4, -6}, {4, -6}, {-4, 6}, {4, 6}, {-5, -6}, {5, -6}, {-5, 6}, {5, 6},
         {-6, -1}, {6, -1}, {-6, 1}, {6, 1}, {-6, -2}, {6, -2}, {-6, 2}, {6, 2}, {-6, -3}, {6, -3}, {-6, 3}, {6, 3},
         {-6, -4}, {6, -4}, {-6, 4}, {6, 4}, {-6, -5}, {6, -5}, {-6, 5}, {6, 5}, {-6, -6}, {6, -6}, {-6, 6}, {6, 6}}};

    const int            w        = mWidth;
    const int            h        = mHeight;
    const int* const     gridData = mGrid.mData.data();
    const uint8_t* const dirData  = mDirections.mData.data();

    for (const auto& [dx, dy] : offsets)
    {
        const int nx = x + dx;
        const int ny = y + dy;

        if (static_cast<unsigned>(nx) < static_cast<unsigned>(w) && static_cast<unsigned>(ny) < static_cast<unsigned>(h))
        {
            const int idx = nx * h + ny;
            if (gridData[idx] == 0 && dirData[idx] != UNVISITED)
            {
                return {dx, dy};
            }
        }
    }

    return {0, 0};
}
