#ifndef GRID_HPP
#define GRID_HPP

#include <algorithm>
#include <vector>

template <class T>
struct Grid
{
    int            mWidth  = 0;
    int            mHeight = 0;
    std::vector<T> mData;

    Grid() = default;

    Grid(int width, int height, T val) : mWidth(width), mHeight(height), mData(width * height, val)
    {
    }

    void assign(int width, int height, T val)
    {
        mWidth             = width;
        mHeight            = height;
        const size_t total = static_cast<size_t>(width) * height;
        if (mData.size() != total)
        {
            mData.assign(total, val);
        }
        else
        {
            std::fill(mData.begin(), mData.end(), val);
        }
    }

    struct ColProxy
    {
        T* ptr;
        T& operator[](int row)
        {
            return ptr[row];
        }
        const T& operator[](int row) const
        {
            return ptr[row];
        }
    };

    ColProxy operator[](int col)
    {
        return ColProxy{mData.data() + col * mHeight};
    }

    struct ConstColProxy
    {
        const T* ptr;
        const T& operator[](int row) const
        {
            return ptr[row];
        }
    };

    ConstColProxy operator[](int col) const
    {
        return ConstColProxy{mData.data() + col * mHeight};
    }

    [[nodiscard]] int width() const
    {
        return mWidth;
    }
    [[nodiscard]] int height() const
    {
        return mHeight;
    }

    [[nodiscard]] bool isValidCell(int col, int row) const
    {
        return col >= 0 && col < mWidth && row >= 0 && row < mHeight;
    }
};

#endif
