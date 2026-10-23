// 土管兄弟 ソース解説
// TMX/TSXのCSVタイルと画像を読み、固定画面の地形・土管・POWを描く。
// 床叩きの持ち上げは描画上の演出だけ。StageDataの判定矩形は動かさない。

#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "StageData.h"

// 有限・直交TMX、非圧縮CSVタイルレイヤー、画像アトラスTSXの描画。
// 当たり判定は、これまでのStageDataが担当する。
class StageRenderer
{
public:
    StageRenderer() = default;
    ~StageRenderer();
    StageRenderer(const StageRenderer&) = delete;
    StageRenderer& operator=(const StageRenderer&) = delete;
    bool Load(const char* filename);
    void Update(float dt);
    void Bump(const StageSolid& solid, float hitX);
    void Draw() const;
    void SetPowState(const StageSolid& pow, int remainingUses);
    const std::string& GetError() const { return m_error; }

private:
    // TSXの画像ハンドルと切り出し規則。TMXのGIDはfirstGidでローカル番号へ直す。
    struct Tileset
    {
        std::uint32_t firstGid = 0;
        int tileWidth = 0;
        int tileHeight = 0;
        int columns = 0;
        int tileCount = 0;
        int margin = 0;
        int spacing = 0;
        int graph = -1;
        bool renderAtGridSize = false;
        bool objectsBottomLeft = true;
    };
    struct Tile
    {
        int tileset = 0;
        int sourceX = 0;
        int sourceY = 0;
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        bool flipX = false;
        bool flipY = false;
        bool diagonal = false;
        bool isPlatform = false;
    };
    // 床叩きの見た目だけの状態。秒とpxで、寿命・頂点時間・高さ・半径を調整する。
    struct BumpEffect
    {
        int solidId = -1;
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float hitX = 0.0f;
        float age = 0.0f;
        float duration = 0.24f;
        float peakTime = 0.06f;
        float amplitude = 10.0f;
        float radius = 80.0f;
    };
    bool AddTile_(std::uint32_t rawGid, float x, float y,
        float width, float height, bool isPlatform);
    float GetBumpOffset_(const Tile& tile) const;
    void Release_();
    bool Fail_(const std::string& message);
    std::vector<Tileset> m_tilesets;
    std::vector<Tile> m_tiles;
    std::vector<BumpEffect> m_bumps;
    StageSolid m_pow;
    int m_powGraph = -1;
    std::string m_error;
    int m_tileWidth = 0;
    int m_tileHeight = 0;
    float m_mapPixelWidth = 0.0f;
};
