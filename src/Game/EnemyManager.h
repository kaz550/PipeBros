#pragma once
#include "Aabb.h"
#include <string>
#include <vector>

class StageData;

// 種類ごとに画像ハンドルを共有し、カメとカニを管理する。
class EnemyManager
{
public:
    enum class Lineup { FromMap, TurtlesOnly, CrabsOnly, Mixed };
    EnemyManager() = default;
    ~EnemyManager();
    EnemyManager(const EnemyManager&) = delete;
    EnemyManager& operator=(const EnemyManager&) = delete;
    void Reset(const StageData& stage, Lineup lineup = Lineup::FromMap);
    // 通常状態の敵と接触し、プレイヤーにミスが発生した場合はtrue。
    // playerActive=falseでは敵だけを更新し、プレイヤーへの接触・蹴り判定を省く。
    bool Update(float dt, const StageData& stage, int bumpedSolidId,
        float hitX, const Aabb& player, bool canHurtPlayer, bool playerActive = true);
    void Draw() const;
    int GetDefeatedCount() const { return m_defeatedCount; }
    int GetTotalCount() const { return static_cast<int>(m_turtles.size()); }
    const std::string& GetError() const { return m_error; }

private:
    enum class Kind { Turtle, Crab };
    enum class State { Walking, Overturned, Recovering, Defeated, Respawning };
    // 既存の移動・衝突処理を共有するため、内部名Turtleを維持する。
    struct Turtle
    {
        Kind kind = Kind::Turtle;
        bool angry = false;
        float x = 0.0f, y = 0.0f;
        float vx = 0.0f, vy = 0.0f;
        float width = 48.0f, height = 36.0f;
        float animationTime = 0.0f, stateTime = 0.0f;
        int direction = 1;
        int spawnIndex = 0;
        int completedLoops = 0;
        bool grounded = false, active = true;
        State state = State::Walking;
    };
    struct SpriteFrame
    {
        int x, y, width, height;
        float pivotX, pivotY;
    };
    static constexpr float BASE_WALK_SPEED = 85.0f;
    static constexpr int LOOPS_PER_SPEEDUP = 1;
    static constexpr float SPEED_GAIN_PER_LEVEL = 0.25f;
    static constexpr int MAX_SPEED_LEVEL = 4;
    static float GetWalkSpeed_(const Turtle& turtle);
    bool LoadSprite_();
    bool LoadCrabSprite_();
    void RespawnTurtle_(Turtle& turtle, const StageData& stage, int spawnIndex);
    bool TryExit_(Turtle& turtle, const StageData& stage, float dt);
    void UpdateTurtle_(Turtle& turtle, float dt, const StageData& stage);
    void ResolveEnemyContacts_(float dt);
    static void ApplyHit_(Turtle& turtle);
    void ApplyBump_(const StageData& stage, int solidId, float hitX);
    void Kick_(Turtle& turtle, int direction);
    bool TouchesPlayer_(const Turtle& turtle, const Aabb& player) const;
    int GetFrame_(const Turtle& turtle) const;
    std::vector<Turtle> m_turtles;
    int m_spriteSheet = -1;
    int m_crabSpriteSheet = -1;
    int m_defeatedCount = 0;
    std::string m_error;
};
