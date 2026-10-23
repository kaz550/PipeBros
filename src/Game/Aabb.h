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
