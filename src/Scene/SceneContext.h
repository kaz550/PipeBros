#pragma once
#include "../Core/Quality.h"

enum class ResultKind
{
    None,
    GameOver,
    StageClear,
    AllClear,
};

struct SceneContext
{
    float lastScore = 0.0f;
    float hiScore = 0.0f;

    // ゲーム終了時にリザルトへ渡す情報。
    int lastLives = 0;
    int lastDefeated = 0;
    int lastEnemyCount = 0;
    int lastHits = 0;
    float lastPlayTime = 0.0f;

    // タイトルの終了メニューからAppの終了経路へ通知する。
    bool quitRequested = false;
    bool showFps = true;
    float fps = 60.0f;
    QualityLevel quality = QualityLevel::High;

    ResultKind resultKind = ResultKind::None;
    int stageIndex = 0;
    int stageCount = 1;
};

