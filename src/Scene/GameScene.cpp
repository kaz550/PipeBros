#include "GameScene.h"
#include "SceneManager.h"
#include "../Input/Input.h"
#include "../Core/Config.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "DxLib.h"

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
    if (m_hudFont == -1)
        m_hudFont = CreateFontToHandle("Arial", 26, 7, DX_FONTTYPE_ANTIALIASING_4X4);
    if (m_scoreFont == -1)
        m_scoreFont = CreateFontToHandle("Arial", 32, 8, DX_FONTTYPE_ANTIALIASING_4X4);
    if (m_smallFont == -1)
        m_smallFont = CreateFontToHandle("Arial", 18, 4, DX_FONTTYPE_ANTIALIASING_4X4);
    m_playerCount = m_mgr && m_mgr->Context().playerCount == 2 ? 2 : 1;
    m_stageNumber = 1;
    m_elapsedTime = 0.0f;
    for (int i = 0; i < 2; ++i)
    {
        m_playerStates[i] = PlayerState{};
        if (i < m_playerCount) m_playerStates[i].lives = INITIAL_LIVES;
    }
    m_stageSelectOpen = false;
    m_selectedStage = 1;
    m_showStageDebug = false;
    RestartStage_();
    if (m_mgr)
    {
        SceneContext& context = m_mgr->Context();
        context.resultKind = ResultKind::None;
        context.lastScore = 0.0f;
        context.lastLives = INITIAL_LIVES;
        context.lastDefeated = context.lastEnemyCount = context.lastHits = 0;
        context.lastPlayTime = 0.0f;
        context.stageIndex = 0;
        context.stageCount = 0;
        for (int i = 0; i < 2; ++i)
        {
            context.playerScores[i] = context.playerHits[i] = context.playerDefeated[i] = 0;
            context.playerLives[i] = m_playerStates[i].lives;
        }
    }
}

bool GameScene::HasLivingPlayer_() const
{
    for (int i = 0; i < m_playerCount; ++i)
        if (m_playerStates[i].lives > 0) return true;
    return false;
}

void GameScene::AddScore_(int playerIndex, int points)
{
    PlayerState& state = m_playerStates[playerIndex];
    state.score = (std::min)(999999, state.score + points);
}

void GameScene::RestartStage_()
{
    m_stageError.clear();
    if (!m_stage.Load("assets/stages/stage01.tmx")) m_stageError = m_stage.GetError();
    if (!m_stageRenderer.Load("assets/stages/stage01.tmx"))
    {
        m_stageError += " " + m_stageRenderer.GetError();
        m_showStageDebug = true;
    }
    m_isBonus = GetStageLineup_(m_stageNumber) == EnemyManager::Lineup::None;
    if (m_isBonus) while (m_stage.GetPowUses() > 0) m_stage.UsePow();
    m_stageRenderer.SetPowState(m_stage.GetPow(), m_stage.GetPowUses());
    for (int i = 0; i < m_playerCount; ++i)
    {
        m_players[i].Reset(m_stage, i);
        m_playerStates[i].invulnerability = m_playerStates[i].respawn = 0.0f;
        m_playerStates[i].defeated = 0;
    }
    m_enemyManager.Reset(m_stage, GetStageLineup_(m_stageNumber));
    m_bonusCoins.clear();
    m_bonusCollected = 0;
    m_bonusAnimationTime = 0.0f;
    m_bonusError.clear();
    m_stageSelectOpen = false;
    if (m_isBonus)
    {
        for (const StageBonusCoin& coin : m_stage.GetBonusCoins())
            m_bonusCoins.push_back({ coin.x, coin.y, false });
        if (m_bonusCoins.empty()) m_bonusError = "No coin points in the TMX BonusCoins layer.";
        LoadBonusSprite_();
    }
    m_stageTimeLimit = GetStageTimeLimit_(m_stageNumber);
    m_remainingTime = m_stageTimeLimit;
    m_stageIntroTimer = STAGE_INTRO_DURATION;
    m_finishTimer = 0.0f;
    m_isFinished = m_isClear = m_resultRequested = false;
    if (m_mgr)
    {
        m_mgr->Context().stageIndex = m_stageNumber - 1;
        m_mgr->Context().resultKind = ResultKind::None;
    }
}

void GameScene::RespawnPlayer_(int playerIndex)
{
    PlayerState& state = m_playerStates[playerIndex];
    state.respawn = 0.0f;
    m_players[playerIndex].Respawn();
    state.invulnerability = 2.0f;
}

void GameScene::Finish_(bool stageClear)
{
    if (m_isFinished) return;
    m_isFinished = true;
    m_isClear = stageClear;
    m_finishTimer = 0.0f;
    if (stageClear) return;
    if (m_mgr)
    {
        SceneContext& context = m_mgr->Context();
        context.lastScore = 0.0f;
        context.lastLives = context.lastHits = 0;
        for (int i = 0; i < m_playerCount; ++i)
        {
            const PlayerState& state = m_playerStates[i];
            context.playerScores[i] = state.score;
            context.playerLives[i] = state.lives;
            context.playerHits[i] = state.hits;
            context.playerDefeated[i] = state.defeated;
            context.lastScore = (std::max)(context.lastScore, static_cast<float>(state.score));
            context.lastLives += state.lives;
            context.lastHits += state.hits;
        }
        context.lastDefeated = m_enemyManager.GetDefeatedCount();
        context.lastEnemyCount = m_enemyManager.GetTotalCount();
        context.lastPlayTime = m_elapsedTime;
        context.resultKind = ResultKind::GameOver;
    }
}

void GameScene::Update(float dt, const Input& input)
{
    if (!std::isfinite(dt) || dt <= 0.0f) return;
    if (input.Triggered(Action::Restart)) { Enter(); return; }
    if (input.Triggered(Action::ToggleDebug))
    {
        m_showStageDebug = !m_showStageDebug;
        if (!m_showStageDebug) m_stageSelectOpen = false;
    }
    if (input.Triggered(Action::ToggleStageSelect) && m_showStageDebug)
    {
        if (m_stageSelectOpen) m_stageSelectOpen = false;
        else
        {
            m_stageSelectOpen = true;
            m_selectedStage = (std::min)(m_stageNumber, DEBUG_MAX_STAGE);
            m_waitStageSelectRelease = true;
        }
        return;
    }
    if (m_stageSelectOpen) { UpdateStageSelect_(input); return; }
    m_stageRenderer.Update(dt);
    if (m_stageIntroTimer > 0.0f)
    {
        m_stageIntroTimer = (std::max)(0.0f, m_stageIntroTimer - dt);
        return;
    }
    if (m_isFinished)
    {
        if (m_isClear) m_enemyManager.Update(dt, m_stage, nullptr, nullptr, 0);
        m_finishTimer += dt;
        if (m_isClear && m_finishTimer >= STAGE_CLEAR_DURATION)
        {
            ++m_stageNumber;
            RestartStage_();
        }
        else if (!m_isClear && m_finishTimer >= 1.2f && !m_resultRequested && m_mgr)
        {
            m_resultRequested = true;
            m_mgr->RequestChange(SceneType::Result);
        }
        return;
    }
    if (m_stageTimeLimit > 0.0f)
    {
        m_elapsedTime += (std::min)(dt, m_remainingTime);
        m_remainingTime = (std::max)(0.0f, m_remainingTime - dt);
        if (m_remainingTime <= 0.0f)
        {
            if (m_isBonus) { Finish_(true); return; }
            // 今後、通常面へ時間制限を設定した場合だけ全生存者が1ミスする。
            for (int i = 0; i < m_playerCount; ++i)
                if (m_playerStates[i].lives > 0) { --m_playerStates[i].lives; ++m_playerStates[i].hits; }
            if (HasLivingPlayer_()) RestartStage_();
            else Finish_(false);
            return;
        }
    }
    else m_elapsedTime += dt;
    if (m_isBonus) { UpdateBonus_(dt, input); return; }

    Aabb contactBefore[2]{};
    EnemyManager::PlayerContact contacts[2];
    EnemyManager::ContactResult results[2];
    for (int i = 0; i < m_playerCount; ++i)
    {
        PlayerState& state = m_playerStates[i];
        if (state.lives <= 0) continue;
        if (state.respawn > 0.0f)
        {
            state.respawn = (std::max)(0.0f, state.respawn - dt);
            if (state.respawn > 0.0f) continue;
            RespawnPlayer_(i);
            // 復活フレームは入力・接触を行わず、待機中の押下を持ち越さない。
            continue;
        }
        state.invulnerability = (std::max)(0.0f, state.invulnerability - dt);
        contactBefore[i] = m_players[i].GetContactAabb();
        m_players[i].Update(dt, input.ForPlayer(i, m_playerCount));
        contacts[i].body = m_players[i].GetAabb();
        contacts[i].bumpedSolidId = m_players[i].GetLastBumpedObjectId();
        contacts[i].hitX = m_players[i].GetX() + m_players[i].GetWidth() * 0.5f;
        contacts[i].active = true;
        contacts[i].canHurt = state.invulnerability <= 0.0f;
        for (const StageSolid& solid : m_stage.GetSolids())
            if (solid.id == contacts[i].bumpedSolidId) { m_stageRenderer.Bump(solid, contacts[i].hitX); break; }
    }
    if (m_playerCount == 2 && contacts[0].active && contacts[1].active)
        Player::ResolveContact(m_players[0], m_players[1], contactBefore[0], contactBefore[1]);
    // 押し合いで移動した後の位置で、敵への接触・蹴りを判定する。
    for (int i = 0; i < m_playerCount; ++i)
        if (contacts[i].active) contacts[i].body = m_players[i].GetAabb();
    // 人数にかかわらず敵の物理・周回タイマーは1フレームに1回だけ進む。
    m_enemyManager.Update(dt, m_stage, contacts, results, m_playerCount);
    m_stageRenderer.SetPowState(m_stage.GetPow(), m_stage.GetPowUses());
    for (int i = 0; i < m_playerCount; ++i)
    {
        PlayerState& state = m_playerStates[i];
        state.defeated += results[i].defeated;
        AddScore_(i, results[i].defeated * SCORE_PER_ENEMY);
        if (!results[i].hit) continue;
        ++state.hits;
        state.lives = (std::max)(0, state.lives - 1);
        state.invulnerability = 0.0f;
        state.respawn = state.lives > 0 ? RESPAWN_DELAY : 0.0f;
        if (state.lives > 0 && state.respawn <= 0.0f) RespawnPlayer_(i);
    }
    if (!HasLivingPlayer_()) Finish_(false);
    else if (m_enemyManager.GetError().empty() && m_enemyManager.GetTotalCount() > 0
        && m_enemyManager.GetDefeatedCount() >= m_enemyManager.GetTotalCount()) Finish_(true);
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

void GameScene::DrawStageDebug_() const
{
    if (!m_showStageDebug || !m_stage.IsLoaded()) return;
    for (const StageSolid& solid : m_stage.GetSolids())
    {
        bool bumped = false;
        for (int i = 0; i < m_playerCount; ++i)
            if (m_playerStates[i].lives > 0 && m_playerStates[i].respawn <= 0.0f
                && m_players[i].GetLastBumpedObjectId() == solid.id) bumped = true;
        const int color = bumped ? GetColor(255, 180, 40) : GetColor(80, 220, 160);
        DrawBox(static_cast<int>(solid.x), static_cast<int>(solid.y),
            static_cast<int>(solid.x + solid.width), static_cast<int>(solid.y + solid.height), color, FALSE);
        DrawFormatString(static_cast<int>(solid.x) + 6, static_cast<int>(solid.y) + 4, color, "ID %d", solid.id);
    }
    DrawString(20, 150, "F3: COLLISION DEBUG", GetColor(80, 220, 160));
}

void GameScene::Draw()
{
    m_stageRenderer.Draw();
    DrawStageDebug_();
    for (int i = 0; i < m_playerCount; ++i)
    {
        const PlayerState& state = m_playerStates[i];
        if (state.lives <= 0 || state.respawn > 0.0f) continue;
        int alpha = state.invulnerability > 0.0f ? 128 : 255;
        if (state.invulnerability > 0.0f && state.invulnerability <= 0.6f
            && static_cast<int>((0.6f - state.invulnerability) / 0.075f) % 2 != 0) alpha = 0;
        m_players[i].Draw(alpha);
    }
    m_enemyManager.Draw();
    if (m_isBonus) DrawBonus_();
    const int white = GetColor(235, 237, 225), gold = GetColor(255, 201, 96);
    char text[160];
    for (int i = 0; i < m_playerCount; ++i)
    {
        const PlayerState& state = m_playerStates[i];
        const int color = i == 0 ? gold : GetColor(100, 215, 255);
        const auto drawHud = [&](const char* label, int y, int font, int ink)
        {
            const int x = i == 0 ? 24 : SCREEN_W - 24 - GetTextWidth_(label, font);
            DrawText_(x, y, label, ink, font);
        };
        std::snprintf(text, sizeof(text), "PLAYER %d", i + 1);
        drawHud(text, 12, m_hudFont, color);
        std::snprintf(text, sizeof(text), "%06d", state.score);
        drawHud(text, 44, m_scoreFont, white);
        if (state.lives > 0) std::snprintf(text, sizeof(text), "LIVES %d", state.lives);
        else std::snprintf(text, sizeof(text), "OUT");
        drawHud(text, 82, m_smallFont, white);
    }
    std::snprintf(text, sizeof(text), "STAGE %02d", m_stageNumber);
    if (m_playerCount == 2) DrawCentered_(text, 12, gold, m_hudFont);
    else DrawText_(SCREEN_W - 24 - GetTextWidth_(text, m_hudFont), 16, text, gold, m_hudFont);
    if (m_stageTimeLimit > 0.0f)
    {
        std::snprintf(text, sizeof(text), m_isBonus ? "BONUS TIME %02d" : "TIME %03d",
            static_cast<int>(std::ceil(m_remainingTime)));
        DrawCentered_(text, m_playerCount == 2 ? 48 : 16,
            m_remainingTime <= 10.0f ? GetColor(255, 80, 80) : white, m_hudFont);
    }
    DrawText_(24, 646, m_playerCount == 2
        ? "P1: A,D / SPACE / L-SHIFT   P2: ARROWS / ENTER / R-SHIFT   F3: DEBUG   F4: STAGE   R: RESTART"
        : "ARROWS / A,D: MOVE   SHIFT: RUN   SPACE: JUMP   F3: DEBUG   F4: STAGE   R: RESTART", white, m_smallFont);
    if (m_isBonus)
        std::snprintf(text, sizeof(text), "COINS: %d / %d   PERFECT BONUS: %d EACH", m_bonusCollected,
            static_cast<int>(m_bonusCoins.size()), BONUS_PERFECT_SCORE);
    else
        std::snprintf(text, sizeof(text), "DEFEATED: %d / %d", m_enemyManager.GetDefeatedCount(), m_enemyManager.GetTotalCount());
    DrawText_(24, 670, text, white, m_smallFont);
    std::string error = m_stageError;
    if (!m_enemyManager.GetError().empty()) error = m_enemyManager.GetError();
    if (!m_bonusError.empty()) error = m_bonusError;
    for (int i = 0; i < m_playerCount; ++i)
        if (!m_players[i].GetError().empty()) error = m_players[i].GetError();
    if (!error.empty()) DrawText_(24, 622, error.c_str(), GetColor(255, 150, 120), m_smallFont);
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
    Aabb before[2]{}, after[2]{}, contactBefore[2]{};
    for (int i = 0; i < m_playerCount; ++i)
    {
        if (m_playerStates[i].lives <= 0) continue;
        before[i] = m_players[i].GetAabb();
        contactBefore[i] = m_players[i].GetContactAabb();
        m_players[i].Update(dt, input.ForPlayer(i, m_playerCount));
        after[i] = m_players[i].GetAabb();
    }
    if (m_playerCount == 2 && m_playerStates[0].lives > 0 && m_playerStates[1].lives > 0)
    {
        Player::ResolveContact(m_players[0], m_players[1], contactBefore[0], contactBefore[1]);
        after[0] = m_players[0].GetAabb();
        after[1] = m_players[1].GetAabb();
    }
    m_bonusAnimationTime = std::fmod(m_bonusAnimationTime + dt, 0.6f);
    for (BonusCoin& coin : m_bonusCoins)
    {
        if (coin.collected) continue;
        int collector = -1;
        float firstTouch = 2.0f;
        for (int i = 0; i < m_playerCount; ++i)
        {
            if (m_playerStates[i].lives <= 0) continue;
            const float fromX = (before[i].l + before[i].r) * 0.5f;
            const float fromY = (before[i].t + before[i].b) * 0.5f;
            float toX = (after[i].l + after[i].r) * 0.5f;
            toX += std::round((fromX - toX) / SCREEN_W) * SCREEN_W;
            const float toY = (after[i].t + after[i].b) * 0.5f;
            const float halfX = (after[i].r - after[i].l + BONUS_COIN_SIZE) * 0.5f;
            const float halfY = (after[i].b - after[i].t + BONUS_COIN_SIZE) * 0.5f;
            for (int copy = -1; copy <= 1; ++copy)
            {
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
                        if (a > b) std::swap(a, b);
                        enter = (std::max)(enter, a);
                        leave = (std::min)(leave, b);
                        if (enter > leave) valid = false;
                    }
                }
                // 同一コインは最初に到達した側だけへ加点する。
                if (valid && enter < firstTouch) { firstTouch = enter; collector = i; }
            }
        }
        if (collector >= 0)
        {
            coin.collected = true;
            ++m_bonusCollected;
            AddScore_(collector, BONUS_COIN_SCORE);
        }
    }
    if (!m_bonusCoins.empty() && m_bonusCollected == static_cast<int>(m_bonusCoins.size()))
    {
        // 全回収は協力報酬。残機のある参加者全員へ加点する。
        for (int i = 0; i < m_playerCount; ++i)
            if (m_playerStates[i].lives > 0) AddScore_(i, BONUS_PERFECT_SCORE);
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
        if (!HasLivingPlayer_())
            for (int i = 0; i < m_playerCount; ++i) m_playerStates[i].lives = INITIAL_LIVES;
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
