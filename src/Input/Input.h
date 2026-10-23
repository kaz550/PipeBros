// 土管兄弟 ソース解説
// キー・物理パッド1/2をActionへ変換し、前回と今回の押下状態を保持する。
// メニュー入力は共通。ForPlayerでゲーム用の入力だけを各プレイヤーへ分離する。

#pragma once
#include <array>

// 物理キーの番号をゲームの意味へ置き換える。COUNTは状態配列の要素数。
enum class Action
{
    Left,
    Right,
    Up,
    Down,
    Decide,
    Back,
    Quit,

    ToggleFps, // F1
    ForceLow,  // F2

    Jump,        // SPACE / PAD 1
    Run,         // SHIFT / PAD 2
    Restart,     // R
    ToggleDebug, // F3
    ToTitle,     // T

    ToggleStageSelect, // F4（F3のデバッグ表示中）

    COUNT
};

class Input
{
public:
    void Update();
    // ゲーム用の入力を分離。1人時は従来のWASD・矢印を両方利用できる。
    Input ForPlayer(int playerIndex, int playerCount) const;

    // Pressed=押している間、Triggered=押した瞬間、Released=離した瞬間。履歴比較のみで再取得しない。
    bool Pressed(Action a) const;
    bool Triggered(Action a) const;
    bool Released(Action a) const;

    // 未使用候補: 現行Player/メニューはActionを直接見るため、この合成軸の呼出はない。
    float GetMoveX() const { return m_moveX; }
    float GetMoveY() const { return m_moveY; }

private:
    std::array<int, (int)Action::COUNT> m_prev{};
    std::array<int, (int)Action::COUNT> m_curr{};
    // 共通メニューとは別の履歴を持ち、1P/2Pで押下・解放の瞬間が混ざらないようにする。
    std::array<int, (int)Action::COUNT> m_singlePrev{}, m_singleCurr{};
    std::array<std::array<int, (int)Action::COUNT>, 2> m_playerPrev{}, m_playerCurr{};
    float m_moveX = 0.0f;
    float m_moveY = 0.0f;

    int GetActionDown_(Action a, int pad) const;

    void SetMoveAxes_();
    static int IsKeyDown_(int key);
    static int IsBitOn_(int state, int bit);
};
