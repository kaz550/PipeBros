#include "EnemyManager.h"
#include "StageData.h"
#include "../Core/Config.h"
#include "DxLib.h"
#include <cmath>

EnemyManager::~EnemyManager()
{
    if (m_spriteSheet != -1 && DxLib_IsInit() != 0)
        DeleteGraph(m_spriteSheet);
}

bool EnemyManager::LoadSprite_()
{
    if (m_spriteSheet != -1) return true;
    // ユーザー指定のasset/imagesを優先。既存構成のassets/imagesも扱う。
    m_spriteSheet = LoadGraph("asset/images/enemy_turtle.png");
    if (m_spriteSheet == -1)
        m_spriteSheet = LoadGraph("assets/images/enemy_turtle.png");
    if (m_spriteSheet == -1)
    {
        m_error = "Could not load asset/images/enemy_turtle.png (or assets/images/).";
        return false;
    }
    int width = 0, height = 0;
    if (GetGraphSize(m_spriteSheet, &width, &height) == -1
        || width != 1774 || height != 887)
    {
        m_error = "enemy_turtle.png must be 1774 x 887. Actual: "
            + std::to_string(width) + " x " + std::to_string(height);
        DeleteGraph(m_spriteSheet);
        m_spriteSheet = -1;
        return false;
    }
    return true;
}

void EnemyManager::Reset(const StageData& stage)
{
    m_turtles.clear();
    m_defeatedCount = 0;
    m_error.clear();
    if (!stage.IsLoaded())
    {
        m_error = "Stage data is not loaded.";
        return;
    }
    if (!LoadSprite_()) return;
    for (int i = 0; i < static_cast<int>(stage.GetEnemySpawns().size()); ++i)
    {
        Turtle turtle;
        RespawnTurtle_(turtle, stage, i);
        m_turtles.push_back(turtle);
    }
    if (m_turtles.empty()) m_error = "No EnemySpawn points in the Spawns layer.";
}

void EnemyManager::RespawnTurtle_(Turtle& turtle, const StageData& stage, int spawnIndex)
{
    const auto& spawns = stage.GetEnemySpawns();
    if (spawns.empty()) return;
    if (spawnIndex < 0 || spawnIndex >= static_cast<int>(spawns.size())) spawnIndex = 0;
    const StageEnemySpawn& spawn = spawns[spawnIndex];
    turtle = Turtle{};
    turtle.spawnIndex = spawnIndex;
    turtle.x = spawn.x - turtle.width * 0.5f;
    turtle.y = spawn.footY - turtle.height;
    turtle.direction = spawn.direction;
}

bool EnemyManager::TryExit_(Turtle& turtle, const StageData& stage, float dt)
{
    if (turtle.state != State::Walking || !turtle.grounded) return false;
    for (const StageEnemyExit& exit : stage.GetEnemyExits())
    {
        if (turtle.direction != exit.direction
            || std::fabs(turtle.y + turtle.height - exit.footY) > 0.5f) continue;
        const float advance = 85.0f * dt;
        const bool reached = exit.direction < 0
            ? turtle.x - advance <= exit.x
            : turtle.x + turtle.width + advance >= exit.x;
        if (!reached) continue;
        // 同じ側の上段出現ポイントを選ぶ。敵の個体数・撃破数は増やさない。
        const auto& spawns = stage.GetEnemySpawns();
        if (spawns.empty()) return false;
        int target = 0;
        float nearest = std::fabs(spawns[0].x - exit.x);
        for (int i = 1; i < static_cast<int>(spawns.size()); ++i)
        {
            const float distance = std::fabs(spawns[i].x - exit.x);
            if (distance < nearest) { nearest = distance; target = i; }
        }
        turtle.spawnIndex = target;
        turtle.state = State::Respawning;
        turtle.stateTime = 2.0f;
        turtle.vx = turtle.vy = 0.0f;
        turtle.grounded = false;
        return true;
    }
    return false;
}

void EnemyManager::UpdateTurtle_(Turtle& turtle, float dt, const StageData& stage)
{
    if (!turtle.active) return;
    if (turtle.state == State::Respawning)
    {
        turtle.stateTime -= dt;
        if (turtle.stateTime <= 0.0f)
            RespawnTurtle_(turtle, stage, turtle.spawnIndex);
        return;
    }
    turtle.animationTime += dt;
    if (turtle.state == State::Defeated)
    {
        // 蹴られた後は足場と衝突せず、画面の外へ落ちる。
        turtle.vy += 1800.0f * dt;
        turtle.x += turtle.vx * dt;
        turtle.y += turtle.vy * dt;
        if (turtle.y > SCREEN_H + 100.0f || turtle.x < -150.0f || turtle.x > SCREEN_W + 150.0f)
            turtle.active = false;
        return;
    }
    if (TryExit_(turtle, stage, dt)) return;
    if (turtle.state == State::Overturned)
    {
        turtle.stateTime -= dt;
        if (turtle.stateTime <= 0.0f)
        {
            turtle.state = State::Recovering;
            turtle.stateTime = 0.35f;
            turtle.animationTime = 0.0f;
        }
    }
    else if (turtle.state == State::Recovering)
    {
        turtle.stateTime -= dt;
        if (turtle.stateTime <= 0.0f)
        {
            turtle.state = State::Walking;
            turtle.animationTime = 0.0f;
        }
    }
    turtle.vx = turtle.state == State::Walking ? turtle.direction * 85.0f : 0.0f;
    if (!turtle.grounded) turtle.vy += 1800.0f * dt;
    int ignoredBumpId = -1;
    stage.Move(turtle.x, turtle.y, turtle.vx, turtle.vy,
        turtle.width, turtle.height, dt, static_cast<float>(SCREEN_W),
        turtle.grounded, ignoredBumpId);
    if (TryExit_(turtle, stage, 0.0f)) return;
    if (turtle.state == State::Walking && turtle.vx == 0.0f)
        turtle.direction = -turtle.direction;
}

void EnemyManager::ResolveEnemyContacts_(float dt)
{
    // 本体の間に8pxの余白を取り、次の移動で接触する場合も先に反転する。
    const float gap = 8.0f;
    const float speed = 85.0f;
    for (std::size_t i = 0; i < m_turtles.size(); ++i)
    {
        Turtle& a = m_turtles[i];
        if (!a.active || a.state == State::Defeated || a.state == State::Respawning) continue;
        for (std::size_t j = i + 1; j < m_turtles.size(); ++j)
        {
            Turtle& b = m_turtles[j];
            if (!b.active || b.state == State::Defeated || b.state == State::Respawning) continue;
            // 別の段の敵や、上下に接するだけの敵は対象外。
            if (a.y + a.height <= b.y + 4.0f || b.y + b.height <= a.y + 4.0f) continue;

            float distance = (b.x + b.width * 0.5f) - (a.x + a.width * 0.5f);
            distance -= std::round(distance / SCREEN_W) * SCREEN_W;
            const int side = distance >= 0.0f ? 1 : -1;
            const bool walkingA = a.state == State::Walking;
            const bool walkingB = b.state == State::Walking;
            const float velocityA = walkingA ? a.direction * speed : 0.0f;
            const float velocityB = walkingB ? b.direction * speed : 0.0f;
            const float closingSpeed = (velocityA - velocityB) * side;
            // 離れ始めた組み合わせは反転しない。連続フレームでの向きの振動を防ぐ。
            if (closingSpeed <= 0.0f) continue;
            const float contactDistance = (a.width + b.width) * 0.5f + gap;
            if (std::fabs(distance) > contactDistance + closingSpeed * dt) continue;

            if (walkingA && a.direction == side)
            {
                a.direction = -side;
                a.vx = a.direction * speed;
            }
            if (walkingB && b.direction == -side)
            {
                b.direction = side;
                b.vx = b.direction * speed;
            }
        }
    }
}

void EnemyManager::ApplyBump_(const StageData& stage, int solidId, float hitX)
{
    if (solidId == -1) return;
    for (const StageSolid& solid : stage.GetSolids())
    {
        if (solid.id != solidId || !solid.bumpable) continue;
        const float screenWidth = static_cast<float>(SCREEN_W);
        const float solidCenter = solid.x + solid.width * 0.5f;
        hitX += std::round((solidCenter - hitX) / screenWidth) * screenWidth;
        if (hitX < solid.x) hitX = solid.x;
        if (hitX > solid.x + solid.width) hitX = solid.x + solid.width;
        for (Turtle& turtle : m_turtles)
        {
            if (!turtle.active || turtle.state == State::Defeated || turtle.state == State::Respawning || !turtle.grounded) continue;
            if (std::fabs(turtle.y + turtle.height - solid.y) > 0.5f) continue;
            float centerX = turtle.x + turtle.width * 0.5f;
            centerX += std::round((solidCenter - centerX) / screenWidth) * screenWidth;
            if (centerX + turtle.width * 0.5f <= solid.x
                || centerX - turtle.width * 0.5f >= solid.x + solid.width
                || std::fabs(centerX - hitX) > 80.0f) continue;
            // 通常時は転倒、転倒・復帰中ならもう一度叩くと通常状態へ戻す。
            if (turtle.state == State::Walking)
            {
                turtle.state = State::Overturned;
                turtle.stateTime = 6.0f;
            }
            else
            {
                turtle.state = State::Walking;
                turtle.stateTime = 0.0f;
            }
            turtle.animationTime = 0.0f;
            turtle.vx = 0.0f;
            turtle.vy = -220.0f;
            turtle.grounded = false;
        }
        break;
    }
}

bool EnemyManager::TouchesPlayer_(const Turtle& turtle, const Aabb& player) const
{
    // 画面端にまたがっている場合も反対側のプレイヤーと接触できる。
    for (int copy = -1; copy <= 1; ++copy)
    {
        const float x = turtle.x + copy * static_cast<float>(SCREEN_W);
        const float left = x + 3.0f, right = x + turtle.width - 3.0f;
        const float top = turtle.y + 3.0f, bottom = turtle.y + turtle.height - 2.0f;
        if (player.r > left && player.l < right && player.b > top && player.t < bottom)
            return true;
    }
    return false;
}

void EnemyManager::Kick_(Turtle& turtle, int direction)
{
    turtle.state = State::Defeated;
    turtle.direction = direction;
    turtle.vx = direction * 260.0f;
    turtle.vy = -420.0f;
    turtle.grounded = false;
    turtle.animationTime = 0.0f;
    ++m_defeatedCount;
}

bool EnemyManager::Update(float dt, const StageData& stage, int bumpedSolidId,
    float hitX, const Aabb& player, bool canHurtPlayer)
{
    if (!stage.IsLoaded() || !std::isfinite(dt) || dt <= 0.0f) return false;
    ResolveEnemyContacts_(dt);
    for (Turtle& turtle : m_turtles) UpdateTurtle_(turtle, dt, stage);
    ApplyBump_(stage, bumpedSolidId, hitX);
    // 着地・復帰・転倒によって、このフレームで近づいた場合も処理する。
    ResolveEnemyContacts_(0.0f);
    bool playerHit = false;
    for (Turtle& turtle : m_turtles)
    {
        if (!turtle.active || turtle.state == State::Defeated || turtle.state == State::Respawning || !TouchesPlayer_(turtle, player)) continue;
        if (turtle.state == State::Overturned || turtle.state == State::Recovering)
        {
            const float playerCenter = (player.l + player.r) * 0.5f;
            float enemyCenter = turtle.x + turtle.width * 0.5f;
            enemyCenter += std::round((playerCenter - enemyCenter) / SCREEN_W) * SCREEN_W;
            Kick_(turtle, playerCenter <= enemyCenter ? 1 : -1);
        }
        else if (canHurtPlayer)
        {
            playerHit = true;
        }
    }
    return playerHit;
}

int EnemyManager::GetFrame_(const Turtle& turtle) const
{
    if (turtle.state == State::Overturned)
        return 3 + (static_cast<int>(std::fmod(turtle.animationTime, 0.36f) / 0.18f) % 2);
    if (turtle.state == State::Recovering) return 5;
    if (turtle.state == State::Defeated) return 6;
    const int step = static_cast<int>(std::fmod(turtle.animationTime, 0.48f) / 0.12f) % 4;
    return step == 3 ? 1 : step;
}

void EnemyManager::Draw() const
{
    if (m_spriteSheet == -1) return;
    // 生成した1774x887画像の7コマ。上段右端の空白は含めない。
    static const SpriteFrame frames[] =
    {
        {  46, 176, 370, 254, 185.0f, 252.0f },
        { 501, 176, 357, 254, 178.5f, 252.0f },
        { 932, 176, 370, 254, 185.0f, 252.0f },
        {  38, 603, 388, 252, 194.0f, 250.0f },
        { 487, 616, 385, 239, 192.5f, 237.0f },
        { 960, 564, 338, 291, 169.0f, 289.0f },
        {1360, 516, 389, 297, 194.5f, 295.0f },
    };
    const float scale = 0.16f;
    for (const Turtle& turtle : m_turtles)
    {
        if (!turtle.active || turtle.state == State::Respawning) continue;
        const SpriteFrame& frame = frames[GetFrame_(turtle)];
        const int centerX = frame.width / 2, centerY = frame.height / 2;
        const float sign = turtle.direction < 0 ? -1.0f : 1.0f;
        const float anchorX = turtle.x + turtle.width * 0.5f;
        const float y = turtle.y + turtle.height + (centerY - frame.pivotY) * scale;
        const auto drawAt = [&](float x)
        {
            DrawRectRotaGraph3(static_cast<int>(std::lround(x + sign * (centerX - frame.pivotX) * scale)),
                static_cast<int>(std::lround(y)), frame.x, frame.y, frame.width, frame.height,
                centerX, centerY, scale, scale, 0.0, m_spriteSheet, TRUE,
                turtle.direction < 0 ? TRUE : FALSE);
        };
        drawAt(anchorX);
        if (turtle.state != State::Defeated)
        {
            const float extent = frame.width * scale;
            if (anchorX - extent < 0.0f) drawAt(anchorX + SCREEN_W);
            if (anchorX + extent > SCREEN_W) drawAt(anchorX - SCREEN_W);
        }
    }
}

