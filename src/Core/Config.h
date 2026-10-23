// 土管兄弟 ソース解説
// 画面サイズ、目標FPS、初期音量を共有する定数。
// 画面サイズはTMXの読込条件・左右ループにも使うため、地形と合わせて変更する。

#pragma once

// Window / Screen
constexpr int SCREEN_W = 1280;
constexpr int SCREEN_H = 720;

// Target FPS
constexpr int TARGET_FPS = 60;

// Audio volume (0-255)
constexpr int BGM_VOL = 180;
constexpr int SE_VOL = 220;