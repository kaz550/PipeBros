// 土管兄弟 ソース解説
// 人数、ハイスコア、結果、終了要求などをシーン切替後へ引き継ぐ共有データ。
// GameSceneの個体状態とは別物。2人時のlastScoreは合計ではなく高い方の個人得点。

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
    // タイトルで選び、リトライでも保持する。
    int playerCount = 1;
    int playerScores[2] = { 0, 0 };
    int playerLives[2] = { 0, 0 };
    // 未使用候補: 次の個人ミス数・撃破数はGameSceneが記録するが、ResultSceneは未表示。
    int playerHits[2] = { 0, 0 };
    int playerDefeated[2] = { 0, 0 }; // 今回のステージ
    float lastScore = 0.0f; // 1人時は従来どおり。2人時は高い方の個人得点。
    float hiScore = 0.0f;

    // ゲーム終了時にリザルトへ渡す情報。
    int lastLives = 0;
    int lastDefeated = 0;
    int lastEnemyCount = 0;
    // 未使用候補: 合計ミス数の格納先。現在は描画や判定で読まれていない。
    int lastHits = 0;
    float lastPlayTime = 0.0f;

    // タイトルの終了メニューからAppの終了経路へ通知する。
    bool quitRequested = false;
    bool showFps = true;
    float fps = 60.0f;
    QualityLevel quality = QualityLevel::High;

    ResultKind resultKind = ResultKind::None;
    int stageIndex = 0;
    // 未使用候補: 総面数の予約。現在は0（終端なし）を書くだけで判定に使わない。
    int stageCount = 1;
};

