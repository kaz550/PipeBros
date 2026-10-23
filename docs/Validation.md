# 確認結果

対象はアップロードされたsrc.zip、確認日は2026年10月23日。

| 項目 | 結果 |
| --- | --- |
| 自作ソース | 49ファイルにコメント追記 |
| 主要関数の説明 | 130か所 |
| C++トークン列の比較 | コメントと空白を除き全49ファイルで元と一致 |
| 自作ソースの保存形式 | UTF-8 BOM付き・CRLF |
| 既存のCP932ソース | 日本語をデコードしてUTF-8へ変換 |
| 第三者コードとライセンス | 3ファイルすべて元とバイト単位で一致 |
| 相対include | プロジェクト内ヘッダーがすべて存在。外部DxLib.hは環境依存 |
| ソースの追加・削除 | なし |
| 実行文・宣言・定数・文字列 | 変更なし |
| Windowsでのビルド・実行 | 未実施 |

## ファイル別の変更

追加行数は既存コメントの説明修正を含む差分です。元の動作を改善するコード変更は含めていません。

| ファイル | 元の文字コード | 追加行数 |
| --- | --- | --- |
| src/Audio/Audio.cpp | CP932 | 13 |
| src/Audio/Audio.h | UTF-8 | 5 |
| src/Core/App.cpp | UTF-8 | 11 |
| src/Core/App.h | UTF-8 | 4 |
| src/Core/Config.h | UTF-8 | 4 |
| src/Core/Fps.cpp | UTF-8 | 6 |
| src/Core/Fps.h | CP932 | 4 |
| src/Core/Quality.cpp | CP932 | 7 |
| src/Core/Quality.h | CP932 | 4 |
| src/Core/Time.cpp | CP932 | 6 |
| src/Core/Time.h | UTF-8 | 5 |
| src/Game/Aabb.h | UTF-8 | 5 |
| src/Game/Background.cpp | UTF-8 | 10 |
| src/Game/Background.h | UTF-8 | 4 |
| src/Game/Collision.h | UTF-8 | 4 |
| src/Game/EnemyManager.cpp | UTF-8 | 19 |
| src/Game/EnemyManager.h | UTF-8 | 7 |
| src/Game/GridPos.h | UTF-8 | 4 |
| src/Game/Obstacle.cpp | CP932 | 7 |
| src/Game/Obstacle.h | UTF-8 | 4 |
| src/Game/ObstacleManager.cpp | UTF-8 | 12 |
| src/Game/ObstacleManager.h | UTF-8 | 4 |
| src/Game/Player.cpp | UTF-8 | 24 |
| src/Game/Player.h | UTF-8 | 10 |
| src/Game/Score.cpp | UTF-8 | 7 |
| src/Game/Score.h | UTF-8 | 4 |
| src/Game/StageData.cpp | UTF-8 | 10 |
| src/Game/StageData.h | UTF-8 | 6 |
| src/Game/StageRenderer.cpp | UTF-8 | 15 |
| src/Game/StageRenderer.h | UTF-8 | 6 |
| src/Game/Vec2.h | UTF-8 | 4 |
| src/Input/Input.cpp | UTF-8 | 13 |
| src/Input/Input.h | UTF-8 | 8 |
| src/Render/Camera2D.cpp | UTF-8 | 7 |
| src/Render/Camera2D.h | UTF-8 | 4 |
| src/Render/RenderUtil.cpp | UTF-8 | 4 |
| src/Render/RenderUtil.h | UTF-8 | 4 |
| src/Scene/GameScene.cpp | UTF-8 | 28 |
| src/Scene/GameScene.h | UTF-8 | 7 |
| src/Scene/IScene.h | UTF-8 | 5 |
| src/Scene/ResultScene.cpp | UTF-8 | 8 |
| src/Scene/ResultScene.h | UTF-8 | 4 |
| src/Scene/SceneContext.h | UTF-8 | 7 |
| src/Scene/SceneManager.cpp | UTF-8 | 9 |
| src/Scene/SceneManager.h | UTF-8 | 4 |
| src/Scene/SceneType.h | UTF-8 | 4 |
| src/Scene/TitleScene.cpp | UTF-8 | 9 |
| src/Scene/TitleScene.h | UTF-8 | 4 |
| src/main.cpp | UTF-8 | 4 |
