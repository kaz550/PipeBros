#include "GameScene.h"
#include "SceneManager.h"
#include "../Input/Input.h"
#include "../Core/Config.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>
#include <cstring>

GameScene::GameScene(SceneManager* mgr)
    : m_mgr(mgr)
{
}

GameScene::~GameScene()
{
    if (DxLib_IsInit() == 0) return;
    if (m_coinGraph != -1) DeleteGraph(m_coinGraph);
    if (m_hudFont != -1) DeleteFontToHandle(m_hudFont);
    if (m_scoreFont != -1) DeleteFontToHandle(m_scoreFont);
    if (m_smallFont != -1) DeleteFontToHandle(m_smallFont);
}

EnemyManager::Lineup GameScene::GetStageLineup_(int stageNumber)
{
    if (stageNumber < 1) return EnemyManager::Lineup::TurtlesOnly;
    // 01:カメ、02:カニ、03:混合、04:ボーナス。以降も4構成を繰り返す。
    switch ((stageNumber - 1) % 4)
    {
    case 0: return EnemyManager::Lineup::TurtlesOnly;
    case 1: return EnemyManager::Lineup::CrabsOnly;
    case 2: return EnemyManager::Lineup::Mixed;
    default: return EnemyManager::Lineup::None;
    }
}

const char* GameScene::GetStageKindName_(int stageNumber)
{
    switch (GetStageLineup_(stageNumber))
    {
    case EnemyManager::Lineup::TurtlesOnly: return "TURTLES";
    case EnemyManager::Lineup::CrabsOnly: return "CRABS";
    case EnemyManager::Lineup::Mixed: return "MIXED";
    default: return "BONUS - COLLECT COINS";
    }
}

float GameScene::GetStageTimeLimit_(int stageNumber)
{
    return GetStageLineup_(stageNumber) == EnemyManager::Lineup::None ? BONUS_TIME_LIMIT : 0.0f;
}

void GameScene::Enter()
{
    // Resultと同じ書体・サイズ・太さ。Rリスタートでは既存ハンドルを再利用する。
    if (m_hudFont == -1)
        m_hudFont = CreateFontToHandle("Arial", 26, 7, DX_FONTTYPE_ANTIALIASING_4X4);
    if (m_scoreFont == -1)
        m_scoreFont = CreateFontToHandle("Arial", 32, 8, DX_FONTTYPE_ANTIALIASING_4X4);
    if (m_smallFont == -1)
        m_smallFont = CreateFontToHandle("Arial", 18, 4, DX_FONTTYPE_ANTIALIASING_4X4);
    m_stageNumber = 1;
    m_elapsedTime = 0.0f;
    m_lives = INITIAL_LIVES;
    m_score = m_hitCount = m_stageStartScore = 0;
    m_stageSelectOpen = false;
    m_selectedStage = 1;
    m_player.SetStageDebugVisible(false);
    RestartStage_();
    if (m_mgr)
    {
        SceneContext& context = m_mgr->Context();
        context.resultKind = ResultKind::None;
        context.lastScore = 0.0f;
        context.lastLives = INITIAL_LIVES;
        context.lastDefeated = context.lastEnemyCount = context.lastHits = 0;
        context.lastPlayTime = 0.0f;
        // 舞台は共通で、敵の構成を切り替える。stageCount=0は最終ステージ未設定。
        context.stageIndex = 0;
        context.stageCount = 0;
    }
}

void GameScene::RestartStage_()
{
    // 得点・残機・合計プレイ時間を保ち、ステージの状態だけを最初に戻す。
    m_stageStartScore = m_score;
    const bool debugVisible = m_player.IsStageDebugVisible();
    m_player.Reset(PLAYER_START_X, GROUND_Y);
    if (debugVisible) m_player.SetStageDebugVisible(true);
    m_isBonus = GetStageLineup_(m_stageNumber) == EnemyManager::Lineup::None;
    m_enemyManager.Reset(m_player.GetStageData(), GetStageLineup_(m_stageNumber));
    m_bonusCoins.clear();
    m_bonusCollected = 0;
    m_bonusAnimationTime = 0.0f;
    m_bonusError.clear();
    m_stageSelectOpen = false;
    if (m_isBonus)
    {
        while (m_player.GetStageData().GetPowUses() > 0) m_player.UsePow();
        for (const StageBonusCoin& coin : m_player.GetStageData().GetBonusCoins())
            m_bonusCoins.push_back({ coin.x, coin.y, false });
        if (m_bonusCoins.empty()) m_bonusError = "No coin points in the TMX BonusCoins layer.";
        LoadBonusSprite_();
    }
    m_stageTimeLimit = GetStageTimeLimit_(m_stageNumber);
    m_remainingTime = m_stageTimeLimit;
    m_invulnerabilityTimer = 0.0f;
    m_respawnTimer = 0.0f;
    m_stageIntroTimer = STAGE_INTRO_DURATION;
    m_finishTimer = 0.0f;
    m_isFinished = m_isClear = m_resultRequested = false;
    if (m_mgr)
    {
        m_mgr->Context().stageIndex = m_stageNumber - 1;
        m_mgr->Context().resultKind = ResultKind::None;
    }
}

void GameScene::RespawnPlayer_()
{
    m_respawnTimer = 0.0f;
    m_player.Respawn();
    // 待機時間を含めず、姿を現した時点から無敵2秒を開始する。
    m_invulnerabilityTimer = 2.0f;
}

void GameScene::Finish_(bool stageClear)
{
    if (m_isFinished) return;
    m_isFinished = true;
    m_respawnTimer = 0.0f;
    m_isClear = stageClear;
    m_finishTimer = 0.0f;
    if (stageClear) return; // クリアは同じGameScene内で次のステージへ進める。
    if (m_mgr)
    {
        SceneContext& context = m_mgr->Context();
        context.lastScore = static_cast<float>(m_score);
        context.lastLives = m_lives;
        context.lastDefeated = m_enemyManager.GetDefeatedCount();
        context.lastEnemyCount = m_enemyManager.GetTotalCount();
        context.lastHits = m_hitCount;
        context.lastPlayTime = m_elapsedTime;
        context.resultKind = stageClear ? ResultKind::StageClear : ResultKind::GameOver;
    }
}

void GameScene::Update(float dt, const Input& input)
{
    if (!std::isfinite(dt) || dt <= 0.0f) return;
    if (input.Triggered(Action::Restart))
    {
        Enter();
        return;
    }
    // F3は開始表示・復帰待ち中も操作可能。入力はGameSceneで1回だけ処理する。
    if (input.Triggered(Action::ToggleDebug))
    {
        m_player.SetStageDebugVisible(!m_player.IsStageDebugVisible());
        if (!m_player.IsStageDebugVisible()) m_stageSelectOpen = false;
    }
    if (input.Triggered(Action::ToggleStageSelect) && m_player.IsStageDebugVisible())
    {
        if (m_stageSelectOpen) m_stageSelectOpen = false;
        else
        {
            m_stageSelectOpen = true;
            m_selectedStage = m_stageNumber < DEBUG_MAX_STAGE ? m_stageNumber : DEBUG_MAX_STAGE;
            m_waitStageSelectRelease = true;
        }
        return;
    }
    if (m_stageSelectOpen)
    {
        UpdateStageSelect_(input);
        return;
    }
    if (m_stageIntroTimer > 0.0f)
    {
        m_stageIntroTimer = m_stageIntroTimer > dt ? m_stageIntroTimer - dt : 0.0f;
        m_player.UpdateVisualEffects(dt);
        return;
    }
    if (m_isFinished)
    {
        m_player.UpdateVisualEffects(dt);
        if (m_isClear)
        {
            // 最後に蹴った敵が飛び出す演出だけは継続する。
            m_enemyManager.Update(dt, m_player.GetStageData(), -1, 0.0f,
                m_player.GetAabb(), false);
        }
        m_finishTimer += dt;
        if (m_isClear && m_finishTimer >= STAGE_CLEAR_DURATION)
        {
            ++m_stageNumber;
            RestartStage_();
            return;
        }
        if (!m_isClear && m_finishTimer >= 1.2f && !m_resultRequested && m_mgr)
        {
            m_resultRequested = true;
            m_mgr->RequestChange(SceneType::Result);
        }
        return;
    }

    if (m_stageTimeLimit > 0.0f)
    {
        // タイムアップを先に確定し、同じフレームの接触で二重に残機を減らさない。
        const float playDt = dt < m_remainingTime ? dt : m_remainingTime;
        m_elapsedTime += playDt;
        m_remainingTime = m_remainingTime > dt ? m_remainingTime - dt : 0.0f;
        if (m_remainingTime <= 0.0f)
        {
            if (m_isBonus)
            {
                Finish_(true); // 時間切れは残機を減らさず次の通常ステージへ。
                return;
            }
            ++m_hitCount;
            --m_lives;
            if (m_lives <= 0)
            {
                m_lives = 0;
                Finish_(false);
            }
            else
            {
                RestartStage_();
            }
            return;
        }
    }
    else
    {
        // 通常ステージは経過時間の記録のみ。タイムアップは発生しない。
        m_elapsedTime += dt;
    }
    if (m_isBonus)
    {
        UpdateBonus_(dt, input);
        return;
    }
    if (m_respawnTimer > 0.0f)
    {
        m_player.UpdateVisualEffects(dt);
        // プレイヤーは不在。敵の移動・周回・復帰・蹴られた後の飛行だけを進める。
        m_enemyManager.Update(dt, m_player.GetStageData(), -1, 0.0f,
            m_player.GetAabb(), false, false);
        m_respawnTimer = m_respawnTimer > dt ? m_respawnTimer - dt : 0.0f;
        if (m_respawnTimer <= 0.0f) RespawnPlayer_();
        return;
    }
    m_invulnerabilityTimer = m_invulnerabilityTimer > dt
        ? m_invulnerabilityTimer - dt : 0.0f;
    m_player.Update(dt, input);
    const bool hit = m_enemyManager.Update(dt, m_player.GetStageData(),
        m_player.GetLastBumpedObjectId(), m_player.GetX() + m_player.GetWidth() * 0.5f,
        m_player.GetAabb(), m_invulnerabilityTimer <= 0.0f);
    // 3回目も先に敵へ衝撃を伝え、その後に表示と当たり判定を消す。
    const StageData& stage = m_player.GetStageData();
    if (m_player.GetLastBumpedObjectId() != -1
        && m_player.GetLastBumpedObjectId() == stage.GetPow().id)
        m_player.UsePow();

    // やり直し前の得点に今回の撃破分を加える。再出現した敵も改めて加点する。
    m_score = m_stageStartScore + m_enemyManager.GetDefeatedCount() * SCORE_PER_ENEMY;
    if (hit)
    {
        ++m_hitCount;
        --m_lives;
        if (m_lives <= 0)
        {
            m_lives = 0;
            Finish_(false);
        }
        else
        {
            m_invulnerabilityTimer = 0.0f;
            m_respawnTimer = RESPAWN_DELAY;
            if (m_respawnTimer <= 0.0f) RespawnPlayer_();
        }
    }
    if (!m_isFinished && m_enemyManager.GetError().empty()
        && m_enemyManager.GetTotalCount() > 0
        && m_enemyManager.GetDefeatedCount() >= m_enemyManager.GetTotalCount())
    {
        Finish_(true);
    }
}

int GameScene::GetTextWidth_(const char* text, int font) const
{
    const int length = static_cast<int>(std::strlen(text));
    return font != -1 ? GetDrawStringWidthToHandle(text, length, font)
        : GetDrawStringWidth(text, length);
}

void GameScene::DrawText_(int x, int y, const char* text, int color, int font) const
{
    if (font != -1) DrawStringToHandle(x, y, text, color, font);
    else DrawString(x, y, text, color);
}

void GameScene::DrawCentered_(const char* text, int y, int color, int font) const
{
    DrawText_((SCREEN_W - GetTextWidth_(text, font)) / 2, y, text, color, font);
}

void GameScene::Draw()
{
    int playerAlpha = 255;
    if (m_invulnerabilityTimer > 0.0f)
    {
        playerAlpha = 128;
        // 無敵2秒のうち、最後の0.6秒は0.075秒ごとに半透明／非表示を切り替える。
        const float blinkDuration = 0.6f;
        const float blinkInterval = 0.075f;
        if (m_invulnerabilityTimer <= blinkDuration)
        {
            const float blinkTime = blinkDuration - m_invulnerabilityTimer;
            const int phase = static_cast<int>(blinkTime / blinkInterval);
            if (phase % 2 != 0) playerAlpha = 0;
        }
    }
    if (m_respawnTimer > 0.0f) playerAlpha = 0;
    m_player.Draw(playerAlpha);
    m_enemyManager.Draw();
    if (m_isBonus) DrawBonus_();
    const int white = GetColor(235, 237, 225);
    const int gold = GetColor(255, 201, 96);
    char text[160];
    DrawText_(24, 12, "PLAYER 1", gold, m_hudFont);
    std::snprintf(text, sizeof(text), "%06d", m_score);
    DrawText_(24, 44, text, white, m_scoreFont);
    std::snprintf(text, sizeof(text), "LIVES %d", m_lives);
    DrawText_(24, 82, text, white, m_smallFont);
    std::snprintf(text, sizeof(text), "STAGE %02d", m_stageNumber);
    DrawText_(SCREEN_W - 24 - GetTextWidth_(text, m_hudFont), 16, text, gold, m_hudFont);
    if (m_stageTimeLimit > 0.0f)
    {
        const int seconds = static_cast<int>(std::ceil(m_remainingTime));
        std::snprintf(text, sizeof(text), m_isBonus ? "BONUS TIME %02d" : "TIME %03d", seconds);
        DrawCentered_(text, 16, m_remainingTime <= 10.0f ? GetColor(255, 80, 80) : white, m_hudFont);
    }
    // 大きくしたスコアと重ならないよう、補助情報は床より下へまとめる。
    DrawText_(24, 646, "ARROWS / A,D: MOVE   SHIFT: RUN   SPACE: JUMP   F3: DEBUG   F4: STAGE   R: RESTART", white, m_smallFont);
    if (m_isBonus)
        std::snprintf(text, sizeof(text), "COINS: %d / %d   PERFECT BONUS: %d", m_bonusCollected,
            static_cast<int>(m_bonusCoins.size()), BONUS_PERFECT_SCORE);
    else
        std::snprintf(text, sizeof(text), "DEFEATED: %d / %d   INVUL: %.1f",
            m_enemyManager.GetDefeatedCount(), m_enemyManager.GetTotalCount(), m_invulnerabilityTimer);
    DrawText_(24, 670, text, white, m_smallFont);
    if (!m_enemyManager.GetError().empty())
        DrawText_(24, 622, m_enemyManager.GetError().c_str(), GetColor(255, 150, 120), m_smallFont);
    if (!m_bonusError.empty())
        DrawText_(24, 622, m_bonusError.c_str(), GetColor(255, 150, 120), m_smallFont);
    if (m_stageIntroTimer > 0.0f || m_isFinished)
    {
        char title[64];
        if (m_stageIntroTimer > 0.0f)
            std::snprintf(title, sizeof(title), m_isBonus ? "BONUS STAGE %02d" : "STAGE %02d", m_stageNumber);
        else if (m_isBonus && m_isClear)
            std::snprintf(title, sizeof(title), "%s", !m_bonusCoins.empty()
                && m_bonusCollected == static_cast<int>(m_bonusCoins.size()) ? "PERFECT!" : "BONUS FINISH");
        else
            std::snprintf(title, sizeof(title), "%s", m_isClear ? "STAGE CLEAR!" : "GAME OVER");
        const int x = (SCREEN_W - GetTextWidth_(title, m_scoreFont)) / 2;
        DrawText_(x + 2, 220, title, GetColor(0, 0, 0), m_scoreFont);
        DrawText_(x, 218, title, gold, m_scoreFont);
        if (m_isBonus && !m_isFinished)
            DrawCentered_("COLLECT ALL COINS!", 260, white, m_smallFont);
        if (m_isFinished)
            DrawCentered_(m_isClear ? "NEXT STAGE..." : "MOVING TO RESULT...", 260, white, m_smallFont);
    }
    if (m_stageSelectOpen) DrawStageSelect_();
}

bool GameScene::LoadBonusSprite_()
{
    if (m_coinGraph != -1) return true;
    m_coinGraph = LoadGraph("assets/images/bonus_coin.png");
    int width = 0, height = 0;
    if (m_coinGraph == -1)
    {
        m_bonusError = "Could not load assets/images/bonus_coin.png";
        return false;
    }
    if (GetGraphSize(m_coinGraph, &width, &height) == -1 || width != 1254 || height != 1254)
    {
        DeleteGraph(m_coinGraph);
        m_coinGraph = -1;
        m_bonusError = "bonus_coin.png must be the supplied 1254 x 1254 image.";
        return false;
    }
    return true;
}

void GameScene::UpdateBonus_(float dt, const Input& input)
{
    const Aabb before = m_player.GetAabb();
    m_player.Update(dt, input);
    const Aabb after = m_player.GetAabb();
    m_bonusAnimationTime = std::fmod(m_bonusAnimationTime + dt, 0.6f);
    const float fromX = (before.l + before.r) * 0.5f;
    const float fromY = (before.t + before.b) * 0.5f;
    float toX = (after.l + after.r) * 0.5f;
    toX += std::round((fromX - toX) / SCREEN_W) * SCREEN_W;
    const float toY = (after.t + after.b) * 0.5f;
    const float halfX = (after.r - after.l + BONUS_COIN_SIZE) * 0.5f;
    const float halfY = (after.b - after.t + BONUS_COIN_SIZE) * 0.5f;
    for (BonusCoin& coin : m_bonusCoins)
    {
        if (coin.collected) continue;
        bool touched = false;
        for (int copy = -1; copy <= 1 && !touched; ++copy)
        {
            // 移動区間と拡張したコイン矩形を調べ、走行中の取り逃しを防ぐ。
            const float center[] = { coin.x + copy * static_cast<float>(SCREEN_W), coin.y };
            const float from[] = { fromX, fromY }, to[] = { toX, toY }, half[] = { halfX, halfY };
            float enter = 0.0f, leave = 1.0f;
            bool valid = true;
            for (int axis = 0; axis < 2; ++axis)
            {
                const float delta = to[axis] - from[axis];
                const float low = center[axis] - half[axis], high = center[axis] + half[axis];
                if (std::fabs(delta) < 0.00001f)
                {
                    if (from[axis] < low || from[axis] > high) valid = false;
                }
                else
                {
                    float a = (low - from[axis]) / delta, b = (high - from[axis]) / delta;
                    if (a > b) { const float temp = a; a = b; b = temp; }
                    if (a > enter) enter = a;
                    if (b < leave) leave = b;
                    if (enter > leave) valid = false;
                }
            }
            touched = valid;
        }
        if (touched) { coin.collected = true; ++m_bonusCollected; }
    }
    m_score = m_stageStartScore + m_bonusCollected * BONUS_COIN_SCORE;
    if (!m_bonusCoins.empty() && m_bonusCollected == static_cast<int>(m_bonusCoins.size()))
    {
        m_score += BONUS_PERFECT_SCORE;
        Finish_(true);
    }
}

void GameScene::DrawBonus_() const
{
    if (m_coinGraph == -1) return;
    struct Frame { int x, y, width, height; };
    static const Frame frames[] = {
        { 118, 106, 427, 430 }, { 775, 106, 285, 430 },
        { 266, 717, 132, 423 }, { 785, 714, 274, 429 }
    };
    const int index = static_cast<int>(m_bonusAnimationTime / 0.15f) % 4;
    const Frame& frame = frames[index];
    const double scale = BONUS_COIN_SIZE / 430.0;
    for (const BonusCoin& coin : m_bonusCoins)
    {
        if (coin.collected) continue;
        for (int copy = -1; copy <= 1; ++copy)
        {
            const float x = coin.x + copy * static_cast<float>(SCREEN_W);
            if (x + BONUS_COIN_SIZE * 0.5f <= 0 || x - BONUS_COIN_SIZE * 0.5f >= SCREEN_W) continue;
            DrawRectRotaGraph3(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(coin.y)),
                frame.x, frame.y, frame.width, frame.height, frame.width / 2, frame.height / 2,
                scale, scale, 0.0, m_coinGraph, TRUE, FALSE, FALSE);
        }
    }
}

void GameScene::UpdateStageSelect_(const Input& input)
{
    if (!input.Pressed(Action::Decide)) m_waitStageSelectRelease = false;
    if (input.Triggered(Action::Back))
    {
        m_stageSelectOpen = false;
        return;
    }
    int change = 0;
    if (input.Triggered(Action::Up)) change = -1;
    else if (input.Triggered(Action::Down)) change = 1;
    else if (input.Triggered(Action::Left)) change = -4;
    else if (input.Triggered(Action::Right)) change = 4;
    m_selectedStage = (m_selectedStage - 1 + change + DEBUG_MAX_STAGE) % DEBUG_MAX_STAGE + 1;
    if (!m_waitStageSelectRelease && input.Triggered(Action::Decide))
    {
        m_stageNumber = m_selectedStage;
        if (m_lives <= 0) m_lives = INITIAL_LIVES;
        RestartStage_();
    }
}

void GameScene::DrawStageSelect_() const
{
    int mode = DX_BLENDMODE_NOBLEND, param = 0;
    GetDrawBlendMode(&mode, &param);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
    DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(4, 10, 15), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(270, 132, 1010, 610, GetColor(12, 29, 36), TRUE);
    DrawBox(270, 132, 1010, 610, GetColor(229, 167, 69), FALSE);
    const int gold = GetColor(255, 201, 96), white = GetColor(235, 237, 225);
    DrawCentered_("DEBUG - STAGE SELECT", 162, gold, m_scoreFont);
    char text[64];
    std::snprintf(text, sizeof(text), "STAGE %02d", m_selectedStage);
    DrawCentered_(text, 246, white, m_scoreFont);
    DrawCentered_(GetStageKindName_(m_selectedStage), 300, gold, m_hudFont);
    DrawCentered_("SCORE AND LIVES ARE KEPT", 354, white, m_smallFont);
    DrawCentered_("UP / DOWN: -1 / +1    LEFT / RIGHT: -4 / +4", 422, white, m_smallFont);
    DrawCentered_("ENTER / SPACE: START   F4 / PAD2: CLOSE", 472, gold, m_smallFont);
    DrawCentered_("F3: DEBUG OFF    ESC: QUIT", 552, white, m_smallFont);
    SetDrawBlendMode(mode, param);
}
