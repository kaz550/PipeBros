#include "EnemyManager.h"
#include "StageData.h"
#include "../Core/Config.h"
#include "DxLib.h"
#include <cmath>

EnemyManager::~EnemyManager()
{
    if (m_spriteSheet != -1 && DxLib_IsInit() != 0)
        DeleteGraph(m_spriteSheet);
    if (m_crabSpriteSheet != -1 && DxLib_IsInit() != 0)
        DeleteGraph(m_crabSpriteSheet);
}

float EnemyManager::GetWalkSpeed_(const Turtle& turtle)
{
    int level = turtle.completedLoops / LOOPS_PER_SPEEDUP;
    if (level > MAX_SPEED_LEVEL) level = MAX_SPEED_LEVEL;
    const float angerRate = turtle.kind == Kind::Crab && turtle.angry ? 1.5f : 1.0f;
    return BASE_WALK_SPEED * (1.0f + SPEED_GAIN_PER_LEVEL * level) * angerRate;
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

bool EnemyManager::LoadCrabSprite_()
{
    if (m_crabSpriteSheet != -1) return true;
    m_crabSpriteSheet = LoadGraph("asset/images/enemy_crab.png");
    if (m_crabSpriteSheet == -1)
        m_crabSpriteSheet = LoadGraph("assets/images/enemy_crab.png");
    if (m_crabSpriteSheet == -1)
    {
        m_error = "Could not load asset/images/enemy_crab.png (or assets/images/).";
        return false;
    }
    int width = 0, height = 0;
    if (GetGraphSize(m_crabSpriteSheet, &width, &height) == -1 || width != 1536 || height != 1024)
    {
        m_error = "enemy_crab.png must be the supplied 1536 x 1024 image.";
        DeleteGraph(m_crabSpriteSheet);
        m_crabSpriteSheet = -1;
        return false;
    }
    return true;
}

void EnemyManager::Reset(const StageData& stage, Lineup lineup)
{
    m_turtles.clear();
    m_defeatedCount = 0;
    m_error.clear();
    if (!stage.IsLoaded())
    {
        m_error = "Stage data is not loaded.";
        return;
    }
    if (lineup == Lineup::None) return; // ボーナスステージは敵を配置しない。
    // 地形・個体数・出現位置はTMX、種類の構成はステージ設定で選ぶ。
    // FromMapなら、従来のenemyTypeプロパティをそのまま使う。
    const auto getKind = [lineup](const StageEnemySpawn& spawn, int index)
    {
        switch (lineup)
        {
        case Lineup::TurtlesOnly: return Kind::Turtle;
        case Lineup::CrabsOnly: return Kind::Crab;
        case Lineup::Mixed: return index % 2 == 0 ? Kind::Turtle : Kind::Crab;
        default: return spawn.enemyType == "crab" ? Kind::Crab : Kind::Turtle;
        }
    };
    bool needsTurtle = false, needsCrab = false;
    for (int i = 0; i < static_cast<int>(stage.GetEnemySpawns().size()); ++i)
    {
        if (getKind(stage.GetEnemySpawns()[i], i) == Kind::Crab) needsCrab = true;
        else needsTurtle = true;
    }
    if (needsTurtle && !LoadSprite_()) return;
    if (needsCrab && !LoadCrabSprite_()) return;
    for (int i = 0; i < static_cast<int>(stage.GetEnemySpawns().size()); ++i)
    {
        Turtle turtle;
        turtle.kind = getKind(stage.GetEnemySpawns()[i], i);
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
    const int completedLoops = turtle.completedLoops;
    const Kind kind = turtle.kind;
    const bool angry = turtle.angry;
    turtle = Turtle{};
    turtle.kind = kind;
    turtle.angry = angry;
    if (kind == Kind::Crab) turtle.width = 56.0f;
    turtle.completedLoops = completedLoops;
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
        const float advance = GetWalkSpeed_(turtle) * dt;
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
        {
            // 下段から入り、上段へ戻るたびに1周。上限到達後は加算を止める。
            if (turtle.completedLoops < LOOPS_PER_SPEEDUP * MAX_SPEED_LEVEL)
                ++turtle.completedLoops;
            RespawnTurtle_(turtle, stage, turtle.spawnIndex);
        }
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
            turtle.angry = false;
            turtle.animationTime = 0.0f;
        }
    }
    turtle.vx = turtle.state == State::Walking ? turtle.direction * GetWalkSpeed_(turtle) : 0.0f;
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
            const float velocityA = walkingA ? a.direction * GetWalkSpeed_(a) : 0.0f;
            const float velocityB = walkingB ? b.direction * GetWalkSpeed_(b) : 0.0f;
            const float closingSpeed = (velocityA - velocityB) * side;
            // 離れ始めた組み合わせは反転しない。連続フレームでの向きの振動を防ぐ。
            if (closingSpeed <= 0.0f) continue;
            const float contactDistance = (a.width + b.width) * 0.5f + gap;
            if (std::fabs(distance) > contactDistance + closingSpeed * dt) continue;

            if (walkingA && a.direction == side)
            {
                a.direction = -side;
                a.vx = a.direction * GetWalkSpeed_(a);
            }
            if (walkingB && b.direction == -side)
            {
                b.direction = side;
                b.vx = b.direction * GetWalkSpeed_(b);
            }
        }
    }
}

void EnemyManager::ApplyHit_(Turtle& turtle)
{
    if (!turtle.active || !turtle.grounded
        || turtle.state == State::Defeated || turtle.state == State::Respawning) return;
    if (turtle.state == State::Walking)
    {
        if (turtle.kind == Kind::Crab && !turtle.angry)
        {
            // カニの1回目は赤く怒って加速。次の衝撃で転倒する。
            turtle.angry = true;
            turtle.stateTime = 0.0f;
        }
        else
        {
            turtle.state = State::Overturned;
            turtle.stateTime = 6.0f;
            turtle.angry = false;
        }
    }
    else
    {
        // 転倒・復帰中の再打撃では通常状態へ戻す。
        turtle.state = State::Walking;
        turtle.stateTime = 0.0f;
        turtle.angry = false;
    }
    turtle.animationTime = 0.0f;
    turtle.vx = 0.0f;
    turtle.vy = -220.0f;
    turtle.grounded = false;
}

void EnemyManager::ApplyBump_(const StageData& stage, int solidId, float hitX)
{
    if (solidId == -1) return;
    const StageSolid& pow = stage.GetPow();
    if (pow.id == solidId && pow.bumpable)
    {
        // POWは足場や距離を問わず、その瞬間に接地している敵へ届く。
        // 通常の床叩きと同じ状態変化を1回だけ適用する。
        for (Turtle& turtle : m_turtles) ApplyHit_(turtle);
        return;
    }
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
            ApplyHit_(turtle);
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

void EnemyManager::Update(float dt, StageData& stage, const PlayerContact* players,
    ContactResult* results, int playerCount)
{
    for (int i = 0; i < playerCount; ++i) results[i] = ContactResult{};
    if (!stage.IsLoaded() || !std::isfinite(dt) || dt <= 0.0f) return;
    ResolveEnemyContacts_(dt);
    for (Turtle& turtle : m_turtles) UpdateTurtle_(turtle, dt, stage);
    for (int i = 0; i < playerCount; ++i)
    {
        if (!players[i].active) continue;
        const bool hitPow = stage.GetPowUses() > 0 && players[i].bumpedSolidId == stage.GetPow().id;
        ApplyBump_(stage, players[i].bumpedSolidId, players[i].hitX);
        if (hitPow) stage.UsePow();
    }
    ResolveEnemyContacts_(0.0f);
    for (Turtle& turtle : m_turtles)
    {
        if (!turtle.active || turtle.state == State::Defeated || turtle.state == State::Respawning) continue;
        // 1体に両者が同時接触した場合は中心が近い側。完全同距離なら1P。
        int kicker = -1;
        float nearest = static_cast<float>(SCREEN_W);
        for (int i = 0; i < playerCount; ++i)
        {
            if (!players[i].active || !TouchesPlayer_(turtle, players[i].body)) continue;
            if (turtle.state == State::Overturned || turtle.state == State::Recovering)
            {
                float distance = (players[i].body.l + players[i].body.r) * 0.5f - (turtle.x + turtle.width * 0.5f);
                distance -= std::round(distance / SCREEN_W) * SCREEN_W;
                if (std::fabs(distance) < nearest) { nearest = std::fabs(distance); kicker = i; }
            }
            else if (players[i].canHurt) results[i].hit = true;
        }
        if (kicker >= 0)
        {
            const Aabb& player = players[kicker].body;
            const float playerCenter = (player.l + player.r) * 0.5f;
            float enemyCenter = turtle.x + turtle.width * 0.5f;
            enemyCenter += std::round((playerCenter - enemyCenter) / SCREEN_W) * SCREEN_W;
            Kick_(turtle, playerCenter <= enemyCenter ? 1 : -1);
            ++results[kicker].defeated;
        }
    }
}

int EnemyManager::GetFrame_(const Turtle& turtle) const
{
    if (turtle.kind == Kind::Crab)
    {
        if (turtle.state == State::Overturned)
            return 6 + (static_cast<int>(std::fmod(turtle.animationTime, 0.36f) / 0.18f) % 2);
        if (turtle.state == State::Recovering) return 7;
        if (turtle.state == State::Defeated) return 8;
        const float walkTime = turtle.animationTime * (GetWalkSpeed_(turtle) / BASE_WALK_SPEED);
        const int step = static_cast<int>(std::fmod(walkTime, 0.48f) / 0.12f) % 4;
        return (turtle.angry ? 3 : 0) + (step == 3 ? 1 : step);
    }
    if (turtle.state == State::Overturned)
        return 3 + (static_cast<int>(std::fmod(turtle.animationTime, 0.36f) / 0.18f) % 2);
    if (turtle.state == State::Recovering) return 5;
    if (turtle.state == State::Defeated) return 6;
    // 歩行速度に合わせて足のアニメーションも速くする。
    const float walkTime = turtle.animationTime * (GetWalkSpeed_(turtle) / BASE_WALK_SPEED);
    const int step = static_cast<int>(std::fmod(walkTime, 0.48f) / 0.12f) % 4;
    return step == 3 ? 1 : step;
}

void EnemyManager::Draw() const
{
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
    // enemy_crab.png: 通常歩き3、怒り歩き3、転倒2、撃破1。
    static const SpriteFrame crabFrames[] =
    {
        { 66, 80, 414, 263, 207.0f, 259.0f },
        { 559, 81, 405, 263, 202.5f, 259.0f },
        { 1046, 80, 421, 263, 210.5f, 259.0f },
        { 59, 377, 431, 288, 215.5f, 284.0f },
        { 554, 377, 432, 291, 216.0f, 287.0f },
        { 1039, 376, 440, 291, 220.0f, 287.0f },
        { 51, 728, 452, 217, 226.0f, 213.0f },
        { 554, 720, 442, 245, 221.0f, 241.0f },
        { 1064, 706, 438, 271, 219.0f, 267.0f },
    };
    for (const Turtle& turtle : m_turtles)
    {
        if (!turtle.active || turtle.state == State::Respawning) continue;
        const bool isCrab = turtle.kind == Kind::Crab;
        const int graph = isCrab ? m_crabSpriteSheet : m_spriteSheet;
        if (graph == -1) continue;
        const float scale = isCrab ? 0.15f : 0.16f;
        const SpriteFrame& frame = isCrab ? crabFrames[GetFrame_(turtle)] : frames[GetFrame_(turtle)];
        const int centerX = frame.width / 2, centerY = frame.height / 2;
        const float sign = turtle.direction < 0 ? -1.0f : 1.0f;
        const float anchorX = turtle.x + turtle.width * 0.5f;
        const float y = turtle.y + turtle.height + (centerY - frame.pivotY) * scale;
        const auto drawAt = [&](float x)
        {
            DrawRectRotaGraph3(static_cast<int>(std::lround(x + sign * (centerX - frame.pivotX) * scale)),
                static_cast<int>(std::lround(y)), frame.x, frame.y, frame.width, frame.height,
                centerX, centerY, scale, scale, 0.0, graph, TRUE,
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
