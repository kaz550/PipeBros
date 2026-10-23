// 土管兄弟 ソース解説
// カメ・カニの状態、移動、周回加速、打撃と2人分の接触をまとめて扱う。
// GameSceneが1フレームに1回だけ更新し、ContactResultで各プレイヤーへの結果を受け取る。

#pragma once
#include "Aabb.h"
#include <string>
#include <vector>

class StageData;

// 種類ごとに画像ハンドルを共有し、カメとカニを管理する。
class EnemyManager
{
public:
    enum class Lineup { FromMap, TurtlesOnly, CrabsOnly, Mixed, None };
    EnemyManager() = default;
    ~EnemyManager();
    EnemyManager(const EnemyManager&) = delete;
    EnemyManager& operator=(const EnemyManager&) = delete;
    void Reset(const StageData& stage, Lineup lineup = Lineup::FromMap);
    // GameSceneから渡す1フレーム分の個人情報。active=falseは復帰待ち・OUT等。
    struct PlayerContact
    {
        Aabb body{};
        int bumpedSolidId = -1;
        float hitX = 0.0f;
        bool canHurt = false;
        bool active = false;
    };
    // 返すhitは本人のミス、defeatedは本人がこのフレームに蹴った数。
    struct ContactResult { bool hit = false; int defeated = 0; };
    // 敵の時間を1回だけ進め、全員の打撃後に接触を調べる。
    // POWは衝撃を伝えてから共有のStageDataで消費する。
    void Update(float dt, StageData& stage, const PlayerContact* players,
        ContactResult* results, int playerCount);
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
    // 基本速度px/秒。1周ごとに25%を加え、4段階=2倍まで。怒りの倍率は別途掛ける。
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
