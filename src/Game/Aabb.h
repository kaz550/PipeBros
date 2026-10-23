// 土管兄弟 ソース解説
// 軸に平行な矩形をl/t/r/bで表す。PlayerとEnemyManagerもこの型を利用する。
// 末尾のIsHitAabbは旧ObstacleManagerのみが使用する。矩形型自体は現行でも必須。

#pragma once

struct Aabb
{
    float l = 0.0f;
    float t = 0.0f;
    float r = 0.0f;
    float b = 0.0f;
};

// 旧ObstacleManagerだけが呼ぶ補助関数。辺だけが触れてもtrueで、Collision.h版とは境界条件が異なる。
inline bool IsHitAabb(const Aabb& a, const Aabb& b)
{
    if (a.r < b.l) return false;
    if (a.l > b.r) return false;
    if (a.b < b.t) return false;
    if (a.t > b.b) return false;

    return true;
}