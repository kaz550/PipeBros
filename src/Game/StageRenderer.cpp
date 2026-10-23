#include "StageRenderer.h"
#include "StageData.h"
#include <algorithm>
#include "../ThirdParty/tinyxml2/tinyxml2.h"
#include "DxLib.h"
#include <charconv>
#include <cmath>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <system_error>

StageRenderer::~StageRenderer()
{
    Release_();
}

void StageRenderer::Release_()
{
    if (DxLib_IsInit() != 0)
    {
        if (m_powGraph != -1) DeleteGraph(m_powGraph);
        for (const Tileset& tileset : m_tilesets)
        {
            if (tileset.graph != -1) DeleteGraph(tileset.graph);
        }
    }
    m_powGraph = -1;
    m_pow = StageSolid{};
    m_tilesets.clear();
    m_tiles.clear();
    m_bumps.clear();
    m_mapPixelWidth = 0.0f;
}

bool StageRenderer::Fail_(const std::string& message)
{
    m_error = message;
    Release_();
    return false;
}

bool StageRenderer::Load(const char* filename)
{
    Release_();
    m_error.clear();
    tinyxml2::XMLDocument mapDocument;
    if (mapDocument.LoadFile(filename) != tinyxml2::XML_SUCCESS)
        return Fail_(mapDocument.ErrorStr());
    const tinyxml2::XMLElement* map = mapDocument.FirstChildElement("map");
    if (!map || !map->Attribute("orientation")
        || std::strcmp(map->Attribute("orientation"), "orthogonal") != 0
        || map->IntAttribute("infinite") != 0)
        return Fail_("StageRenderer needs a finite orthogonal TMX map.");
    m_tileWidth = map->IntAttribute("tilewidth");
    m_tileHeight = map->IntAttribute("tileheight");
    const int mapWidth = map->IntAttribute("width");
    const int mapHeight = map->IntAttribute("height");
    if (m_tileWidth <= 0 || m_tileHeight <= 0 || mapWidth <= 0 || mapHeight <= 0)
        return Fail_("Invalid map dimensions.");
    if (map->FirstChildElement("group") || map->FirstChildElement("imagelayer"))
        return Fail_("Use top-level tile layers; groups and image layers are not supported yet.");
    if (map->Attribute("renderorder")
        && std::strcmp(map->Attribute("renderorder"), "right-down") != 0)
        return Fail_("Use right-down tile render order.");
    m_mapPixelWidth = static_cast<float>(mapWidth) * m_tileWidth;
    const std::filesystem::path mapPath(filename);

    for (const tinyxml2::XMLElement* reference = map->FirstChildElement("tileset");
        reference; reference = reference->NextSiblingElement("tileset"))
    {
        const std::uint32_t firstGid = reference->UnsignedAttribute("firstgid");
        if (firstGid == 0 || (!m_tilesets.empty() && firstGid <= m_tilesets.back().firstGid))
            return Fail_("Tilesets must have increasing, nonzero firstgid values.");
        tinyxml2::XMLDocument tilesetDocument;
        const tinyxml2::XMLElement* element = reference;
        std::filesystem::path baseDirectory = mapPath.parent_path();
        if (const char* source = reference->Attribute("source"))
        {
            const std::filesystem::path tsxPath = (baseDirectory / source).lexically_normal();
            const std::string pathString = tsxPath.string();
            if (tilesetDocument.LoadFile(pathString.c_str()) != tinyxml2::XML_SUCCESS)
                return Fail_("Could not load TSX: " + pathString);
            element = tilesetDocument.FirstChildElement("tileset");
            baseDirectory = tsxPath.parent_path();
        }
        if (!element) return Fail_("Missing tileset element.");
        const tinyxml2::XMLElement* image = element->FirstChildElement("image");
        if (!image || !image->Attribute("source"))
            return Fail_("Use an image-atlas tileset, not an image collection.");
        if (image->Attribute("trans"))
            return Fail_("Use PNG alpha transparency instead of a tileset transparency color.");
        if (const tinyxml2::XMLElement* offset = element->FirstChildElement("tileoffset"))
        {
            if (offset->IntAttribute("x") != 0 || offset->IntAttribute("y") != 0)
                return Fail_("Tileset tileoffset must be zero.");
        }
        for (const tinyxml2::XMLElement* tile = element->FirstChildElement("tile");
            tile; tile = tile->NextSiblingElement("tile"))
        {
            if (tile->FirstChildElement("animation") || tile->FirstChildElement("image"))
                return Fail_("Animated and per-tile images are not supported yet.");
        }
        const char* fillMode = element->Attribute("fillmode");
        if (fillMode && std::strcmp(fillMode, "stretch") != 0)
            return Fail_("Use the stretch tileset fill mode.");
        Tileset tileset;
        tileset.firstGid = firstGid;
        const char* alignment = element->Attribute("objectalignment");
        tileset.objectsBottomLeft = !alignment || std::strcmp(alignment, "unspecified") == 0
            || std::strcmp(alignment, "bottomleft") == 0;
        tileset.tileWidth = element->IntAttribute("tilewidth");
        tileset.tileHeight = element->IntAttribute("tileheight");
        tileset.columns = element->IntAttribute("columns");
        tileset.tileCount = element->IntAttribute("tilecount");
        tileset.margin = element->IntAttribute("margin");
        tileset.spacing = element->IntAttribute("spacing");
        const char* renderSize = element->Attribute("tilerendersize");
        tileset.renderAtGridSize = renderSize && std::strcmp(renderSize, "grid") == 0;
        if (tileset.tileWidth <= 0 || tileset.tileHeight <= 0 || tileset.columns <= 0
            || tileset.tileCount <= 0 || tileset.margin < 0 || tileset.spacing < 0)
            return Fail_("Invalid tileset dimensions.");
        if (!tileset.renderAtGridSize
            && (tileset.tileWidth != m_tileWidth || tileset.tileHeight != m_tileHeight))
            return Fail_("Use tilerendersize=grid when source tiles differ from the map grid.");
        const std::string imagePath = (baseDirectory / image->Attribute("source")).lexically_normal().string();
        tileset.graph = LoadGraph(imagePath.c_str());
        if (tileset.graph == -1) return Fail_("Could not load tileset image: " + imagePath);
        m_tilesets.push_back(tileset);
        int imageWidth = 0, imageHeight = 0;
        if (GetGraphSize(tileset.graph, &imageWidth, &imageHeight) == -1)
            return Fail_("Could not read tileset image dimensions.");
        const int rows = (tileset.tileCount + tileset.columns - 1) / tileset.columns;
        if (tileset.margin + (tileset.columns - 1) * (tileset.tileWidth + tileset.spacing)
                + tileset.tileWidth > imageWidth
            || tileset.margin + (rows - 1) * (tileset.tileHeight + tileset.spacing)
                + tileset.tileHeight > imageHeight)
            return Fail_("Tileset rectangles extend past the atlas image.");
    }

    for (const tinyxml2::XMLElement* layer = map->FirstChildElement("layer");
        layer; layer = layer->NextSiblingElement("layer"))
    {
        if (!layer->BoolAttribute("visible", true)) continue;
        if (layer->FloatAttribute("opacity", 1.0f) != 1.0f || layer->Attribute("tintcolor")
            || layer->FloatAttribute("parallaxx", 1.0f) != 1.0f
            || layer->FloatAttribute("parallaxy", 1.0f) != 1.0f)
            return Fail_("Use opaque, untinted tile layers with parallax 1.");
        const tinyxml2::XMLElement* data = layer->FirstChildElement("data");
        if (!data || !data->Attribute("encoding")
            || std::strcmp(data->Attribute("encoding"), "csv") != 0
            || data->Attribute("compression"))
            return Fail_("Tile layer format must be uncompressed CSV.");
        const int width = layer->IntAttribute("width");
        const int height = layer->IntAttribute("height");
        if (width != mapWidth || height != mapHeight)
            return Fail_("Tile layers must match the map dimensions.");
        const float offsetX = layer->FloatAttribute("offsetx")
            + layer->IntAttribute("x") * m_tileWidth;
        const float offsetY = layer->FloatAttribute("offsety")
            + layer->IntAttribute("y") * m_tileHeight;
        if (!std::isfinite(offsetX) || !std::isfinite(offsetY))
            return Fail_("Invalid layer offset.");
        std::istringstream csv(data->GetText() ? data->GetText() : "");
        std::string token;
        int index = 0;
        while (std::getline(csv, token, ','))
        {
            const std::size_t begin = token.find_first_not_of(" \t\r\n");
            const std::size_t end = token.find_last_not_of(" \t\r\n");
            if (begin == std::string::npos || index >= width * height)
                return Fail_("Invalid CSV cell count.");
            const char* first = token.data() + begin;
            const char* last = token.data() + end + 1;
            std::uint32_t rawGid = 0;
            const auto parsed = std::from_chars(first, last, rawGid);
            if (parsed.ec != std::errc() || parsed.ptr != last)
                return Fail_("CSV cells must be unsigned 32-bit tile IDs.");
            const std::uint32_t gid = rawGid & 0x0fffffffu;
            if (gid != 0)
            {
                const char* layerName = layer->Attribute("name");
                const bool platform = layerName && std::strcmp(layerName, "Platforms") == 0;
                if (!AddTile_(rawGid, (index % width) * m_tileWidth + offsetX,
                    (index / width) * m_tileHeight + offsetY,
                    static_cast<float>(m_tileWidth), static_cast<float>(m_tileHeight), platform)) return false;
            }
            ++index;
        }
        if (index != width * height) return Fail_("CSV tile count does not match the layer size.");
    }
    // 画像オブジェクトは通常のタイルレイヤーの後に描画する。
    // 今回は回転なし・左下基準のタイルオブジェクトを扱う。
    for (const tinyxml2::XMLElement* group = map->FirstChildElement("objectgroup");
        group; group = group->NextSiblingElement("objectgroup"))
    {
        if (!group->BoolAttribute("visible", true)) continue;
        for (const tinyxml2::XMLElement* object = group->FirstChildElement("object");
            object; object = object->NextSiblingElement("object"))
        {
            if (!object->Attribute("gid") || !object->BoolAttribute("visible", true)) continue;
            if (group->FloatAttribute("opacity", 1.0f) != 1.0f || group->Attribute("tintcolor")
                || object->FloatAttribute("rotation") != 0.0f || object->Attribute("template"))
                return Fail_("Use unrotated tile objects without group tint/opacity or templates.");
            float x = 0.0f, bottom = 0.0f, width = 0.0f, height = 0.0f;
            std::uint32_t rawGid = 0;
            if (object->QueryUnsignedAttribute("gid", &rawGid) != tinyxml2::XML_SUCCESS
                || (rawGid & 0x0fffffffu) == 0
                || object->QueryFloatAttribute("x", &x) != tinyxml2::XML_SUCCESS
                || object->QueryFloatAttribute("y", &bottom) != tinyxml2::XML_SUCCESS
                || object->QueryFloatAttribute("width", &width) != tinyxml2::XML_SUCCESS
                || object->QueryFloatAttribute("height", &height) != tinyxml2::XML_SUCCESS
                || !std::isfinite(x) || !std::isfinite(bottom)
                || !std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f)
                return Fail_("Tile objects need a valid GID, x/y and positive width/height.");
            int setIndex = -1;
            for (int i = 0; i < static_cast<int>(m_tilesets.size()); ++i)
                if (m_tilesets[i].firstGid <= (rawGid & 0x0fffffffu)) setIndex = i;
            if (setIndex < 0 || !m_tilesets[setIndex].objectsBottomLeft)
                return Fail_("Tile objects require bottom-left tileset alignment.");
            x += group->FloatAttribute("offsetx") + group->FloatAttribute("x");
            bottom += group->FloatAttribute("offsety") + group->FloatAttribute("y");
            if (!std::isfinite(x) || !std::isfinite(bottom)) return Fail_("Invalid object group offset.");
            if (!AddTile_(rawGid, x, bottom - height, width, height, false)) return false;
        }
    }
    if (!ReadStagePow(map, m_pow, m_error)) return Fail_(m_error);
    if (m_pow.id != -1)
    {
        const std::string imagePath = (mapPath.parent_path() / "../images/pow.png").lexically_normal().string();
        m_powGraph = LoadGraph(imagePath.c_str());
        if (m_powGraph == -1) return Fail_("Could not load POW image: " + imagePath);
        int width = 0, height = 0;
        if (GetGraphSize(m_powGraph, &width, &height) == -1 || width != 1774 || height != 887)
            return Fail_("pow.png must be the supplied 1774 x 887 image.");
    }
    return true;
}

bool StageRenderer::AddTile_(std::uint32_t rawGid, float x, float y,
    float width, float height, bool isPlatform)
{
    const std::uint32_t gid = rawGid & 0x0fffffffu;
    int setIndex = -1;
    for (int i = 0; i < static_cast<int>(m_tilesets.size()); ++i)
    {
        if (m_tilesets[i].firstGid <= gid) setIndex = i;
        else break;
    }
    if (setIndex == -1) return Fail_("Tile ID has no matching tileset.");
    const Tileset& set = m_tilesets[setIndex];
    const std::uint32_t local = gid - set.firstGid;
    if (local >= static_cast<std::uint32_t>(set.tileCount))
        return Fail_("Tile ID exceeds the tileset tile count.");
    Tile tile;
    tile.tileset = setIndex;
    tile.sourceX = set.margin + static_cast<int>(local % set.columns) * (set.tileWidth + set.spacing);
    tile.sourceY = set.margin + static_cast<int>(local / set.columns) * (set.tileHeight + set.spacing);
    tile.x = x; tile.y = y; tile.width = width; tile.height = height;
    tile.flipX = (rawGid & 0x80000000u) != 0;
    tile.flipY = (rawGid & 0x40000000u) != 0;
    tile.diagonal = (rawGid & 0x20000000u) != 0;
    tile.isPlatform = isPlatform;
    if (tile.diagonal && (width != height || set.tileWidth != set.tileHeight))
        return Fail_("Diagonal flips require square source and destination tiles.");
    m_tiles.push_back(tile);
    return true;
}

void StageRenderer::Update(float dt)
{
    if (!std::isfinite(dt) || dt <= 0.0f) return;
    for (BumpEffect& bump : m_bumps) bump.age += dt;
    m_bumps.erase(std::remove_if(m_bumps.begin(), m_bumps.end(),
        [](const BumpEffect& bump) { return bump.age >= bump.duration; }),
        m_bumps.end());
}

void StageRenderer::Bump(const StageSolid& solid, float hitX)
{
    if (!solid.bumpable || m_mapPixelWidth <= 0.0f || !std::isfinite(hitX)) return;
    // 画面端のループで、元の足場の範囲へ打撃位置を戻す。
    const float centerX = solid.x + solid.width * 0.5f;
    hitX += std::round((centerX - hitX) / m_mapPixelWidth) * m_mapPixelWidth;
    if (hitX < solid.x) hitX = solid.x;
    if (hitX > solid.x + solid.width) hitX = solid.x + solid.width;

    BumpEffect effect;
    effect.solidId = solid.id;
    effect.x = solid.x;
    effect.y = solid.y;
    effect.width = solid.width;
    effect.height = solid.height;
    effect.hitX = hitX;
    // 同じ足場を再度叩いたら、新しい打撃位置で演出を開始し直す。
    for (BumpEffect& bump : m_bumps)
    {
        if (bump.solidId == solid.id)
        {
            bump = effect;
            return;
        }
    }
    m_bumps.push_back(effect);
}

float StageRenderer::GetBumpOffset_(const Tile& tile) const
{
    if (!tile.isPlatform) return 0.0f;
    const float centerX = tile.x + m_tileWidth * 0.5f;
    const float centerY = tile.y + m_tileHeight * 0.5f;
    float offset = 0.0f;
    for (const BumpEffect& bump : m_bumps)
    {
        // 打撃対象の足場の中にあるタイルだけを持ち上げる。
        if (centerX < bump.x || centerX >= bump.x + bump.width
            || centerY < bump.y || centerY >= bump.y + bump.height) continue;
        const float distance = std::fabs(centerX - bump.hitX);
        if (distance >= bump.radius) continue;
        const float strength = std::cos(distance / bump.radius * 1.5707963267948966f);
        float lift = 0.0f;
        if (bump.age < bump.peakTime)
        {
            lift = std::sin(bump.age / bump.peakTime * 1.5707963267948966f);
        }
        else
        {
            lift = std::cos((bump.age - bump.peakTime)
                / (bump.duration - bump.peakTime) * 1.5707963267948966f);
        }
        const float displacement = -bump.amplitude * strength * lift;
        if (displacement < offset) offset = displacement;
    }
    return offset;
}

void StageRenderer::Draw() const
{
    for (const Tile& tile : m_tiles)
    {
        const Tileset& set = m_tilesets[tile.tileset];
        // Tiledの対角反転は先にX/Yを入れ替え、その後に左右・上下反転する。
        const bool flipX = tile.diagonal ? tile.flipY : tile.flipX;
        const bool flipY = tile.diagonal ? !tile.flipX : tile.flipY;
        const double angle = tile.diagonal ? 1.5707963267948966 : 0.0;
        DrawRectRotaGraph3(
            static_cast<int>(std::lround(tile.x + tile.width * 0.5f)),
            static_cast<int>(std::lround(tile.y + tile.height * 0.5f + GetBumpOffset_(tile))),
            tile.sourceX, tile.sourceY, set.tileWidth, set.tileHeight,
            set.tileWidth / 2, set.tileHeight / 2,
            static_cast<double>(tile.width) / set.tileWidth,
            static_cast<double>(tile.height) / set.tileHeight,
            angle, set.graph, TRUE, flipX ? TRUE : FALSE, flipY ? TRUE : FALSE);
    }
    if (m_powGraph != -1)
    {
        // 配布画像の透明な余白を除いた矩形を、TMXの矩形へ合わせる。
        constexpr int sourceX = 12, sourceY = 65, sourceWidth = 1751, sourceHeight = 756;
        for (int copy = -1; copy <= 1; ++copy)
        {
            const float x = m_pow.x + copy * m_mapPixelWidth;
            if (x + m_pow.width <= 0.0f || x >= m_mapPixelWidth) continue;
            DrawRectRotaGraph3(
                static_cast<int>(std::lround(x + m_pow.width * 0.5f)),
                static_cast<int>(std::lround(m_pow.y + m_pow.height * 0.5f)),
                sourceX, sourceY, sourceWidth, sourceHeight,
                sourceWidth / 2, sourceHeight / 2,
                static_cast<double>(m_pow.width) / sourceWidth,
                static_cast<double>(m_pow.height) / sourceHeight,
                0.0, m_powGraph, TRUE, FALSE, FALSE);
        }
    }
}
