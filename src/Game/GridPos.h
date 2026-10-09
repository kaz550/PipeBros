#pragma once

struct GridPos
{
    int x;
    int y;

    bool operator==(const GridPos& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const GridPos& other) const
    {
        return !(*this == other);
    }
};