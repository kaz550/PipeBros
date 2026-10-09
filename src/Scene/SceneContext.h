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

    bool showFps = true;
    float fps = 60.0f;
    QualityLevel quality = QualityLevel::High;

    ResultKind resultKind = ResultKind::None;
    int stageIndex = 0;
    int stageCount = 3;
};