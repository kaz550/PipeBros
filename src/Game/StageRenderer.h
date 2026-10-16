#pragma once
#include <cstdint>
#include <string>
#include <vector>

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
    void Draw() const;
    const std::string& GetError() const { return m_error; }

private:
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
    };
    struct Tile
    {
        int tileset = 0;
        int sourceX = 0;
        int sourceY = 0;
        float x = 0.0f;
        float y = 0.0f;
        bool flipX = false;
        bool flipY = false;
        bool diagonal = false;
    };
    void Release_();
    bool Fail_(const std::string& message);
    std::vector<Tileset> m_tilesets;
    std::vector<Tile> m_tiles;
    std::string m_error;
    int m_tileWidth = 0;
    int m_tileHeight = 0;
};
