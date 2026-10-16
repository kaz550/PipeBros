#include "StageRenderer.h"
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
        for (const Tileset& tileset : m_tilesets)
        {
            if (tileset.graph != -1) DeleteGraph(tileset.graph);
        }
    }
    m_tilesets.clear();
    m_tiles.clear();
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
                tile.x = (index % width) * m_tileWidth + offsetX;
                tile.y = (index / width) * m_tileHeight + offsetY;
                tile.flipX = (rawGid & 0x80000000u) != 0;
                tile.flipY = (rawGid & 0x40000000u) != 0;
                tile.diagonal = (rawGid & 0x20000000u) != 0;
                if (tile.diagonal && (m_tileWidth != m_tileHeight || set.tileWidth != set.tileHeight))
                    return Fail_("Diagonal flips currently require square source and map tiles.");
                m_tiles.push_back(tile);
            }
            ++index;
        }
        if (index != width * height) return Fail_("CSV tile count does not match the layer size.");
    }
    return true;
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
            static_cast<int>(std::lround(tile.x + m_tileWidth * 0.5f)),
            static_cast<int>(std::lround(tile.y + m_tileHeight * 0.5f)),
            tile.sourceX, tile.sourceY, set.tileWidth, set.tileHeight,
            set.tileWidth / 2, set.tileHeight / 2,
            static_cast<double>(m_tileWidth) / set.tileWidth,
            static_cast<double>(m_tileHeight) / set.tileHeight,
            angle, set.graph, TRUE, flipX ? TRUE : FALSE, flipY ? TRUE : FALSE);
    }
}
