# 土管兄弟 不要コード候補と整理方針

確認日 2026年10月23日

添付されたsrc.zipを基準に、現行のTitle・Game・Resultから使われていないコードを調べた。今回は候補の列挙とコメント整備を行い、関数・フィールド・ファイルの削除やゲームロジックの修正は行っていない。

調査範囲は添付srcのみ。Visual Studioの.sln・.vcxproj・.filtersと、このZIP外のテストやツールは含まれない。以下の「未使用」はこの範囲での判断であり、ビルド対象に登録されていないことまでは意味しない。特に仮想関数は、関数名の出現回数だけで未使用とは扱わず、SceneManagerからの呼出を確認した。

## 先に整理できる候補

以下の13ファイルは旧ランナーの機能または未接続の補助で、現行シーンからの生成・呼出・参照が見つからなかった。クラス内部の呼出が残っていても、シーンからそのクラスへ到達する入口がない組を削除候補とした。

| 対象 | 現行で使われない根拠 | 整理する範囲 |
| --- | --- | --- |
| [Game/Background.h](../src/Game/Background.h) / [Game/Background.cpp](../src/Game/Background.cpp) | Title/Game/Result/AppにBackgroundの生成・メンバー・呼出・includeがない。現行背景はStageRendererやタイトル画像で描く。 | 2ファイルを一組で整理。旧3層スクロールの定数・状態・描画も一緒に除去できる。 |
| [Game/ObstacleManager.h](../src/Game/ObstacleManager.h) / [Game/ObstacleManager.cpp](../src/Game/ObstacleManager.cpp) | ObstacleManagerを作る・更新する現行の呼出元がない。 | 2ファイルを一組で整理。出現タイマー・難易度・乱数生成もこの旧経路に属する。 |
| [Game/Obstacle.h](../src/Game/Obstacle.h) / [Game/Obstacle.cpp](../src/Game/Obstacle.cpp) | 利用元は上記の旧ObstacleManagerのみ。EnemyManagerはこのクラスを使わない。 | ObstacleManagerと一緒に2ファイルを整理。 |
| [Game/Collision.h](../src/Game/Collision.h) | IntersectAabbの宣言/定義以外に呼出がなく、このヘッダーのincludeもない。 | ヘッダー1個。現行の敵・地形の独自判定とは別なので、置き換え作業は不要。 |
| [Game/GridPos.h](../src/Game/GridPos.h) | GridPosの使用・includeがこのファイルの外にない。TMXの読込も使用しない。 | ヘッダー1個。 |
| [Render/Camera2D.h](../src/Render/Camera2D.h) / [Render/Camera2D.cpp](../src/Render/Camera2D.cpp) | Camera2Dの生成・AddShake・OffsetX/Yの利用が現行描画にない。 | 未接続の画面揺れを保存する予定がなければ2ファイルを整理。 |
| [Game/Vec2.h](../src/Game/Vec2.h) | 使用元は上記の未使用Camera2Dだけ。Playerの位置はfloatフィールドで管理する。 | Camera2Dを除いた後でヘッダー1個を整理。 |
| [Render/RenderUtil.h](../src/Render/RenderUtil.h) / [Render/RenderUtil.cpp](../src/Render/RenderUtil.cpp) | RenderUtil::DrawGlowBoxの宣言・定義以外の呼出がない。現在のパネルは各シーンで描く。 | 2ファイルを一組で整理。QualityクラスはAppから使うため連動して消さない。 |

削除する場合は、まずVisual Studioのプロジェクトとフィルターの登録を外し、必要なファイルだけをディスクから削除してリビルドする。今回は元の構成を保つため、これら13ファイルにも役割と未使用候補のコメントを付けて同梱している。

## ファイル全体を消さない候補

| 対象 | 根拠 | 整理時に残すもの |
| --- | --- | --- |
| [Game/Aabb.h](../src/Game/Aabb.h)のIsHitAabb | 呼出は旧ObstacleManager::CheckCollisionだけ。上記の旧障害物を除けば呼出元がなくなる。 | Aabb構造体はPlayer・EnemyManager等で使用中なので、Aabb.h全体は削除しない。 |
| [Game/Player.h](../src/Game/Player.h)のGetY / GetHeight / GetVelocityX / GetVelocityY / IsGrounded / IsJumpHolding | 6個とも添付src内で呼出がなく、インラインの定義のみ。 | GetX・GetWidth・GetAabb・GetContactAabb・GetLastBumpedObjectIdは現行処理が使う。本体の位置・速度・接地フィールドも必要。 |
| [Input/Input.h](../src/Input/Input.h)と[Input/Input.cpp](../src/Input/Input.cpp)の合成軸 | GetMoveX/GetMoveYが呼ばれない。m_moveX/m_moveYはSetMoveAxes_で計算されるが、ゲーム処理はPressed等を直接参照する。 | 合成軸を使わない方針なら2ゲッター・2フィールド・SetMoveAxes_とUpdate/ForPlayer内の呼出をまとめて整理。Actionと前回/今回の配列は必要。 |
| [Core/Time.h](../src/Core/Time.h)と[Core/Time.cpp](../src/Core/Time.cpp)のTotalTime | TotalTimeは呼ばれず、m_totalTimeはResetとUpdateで維持するだけ。プレイ時間はGameScene::m_elapsedTimeで別に数える。 | TotalTime・m_totalTimeとその初期化/加算が候補。DeltaTimeと時間差の計算は全更新に必要。 |
| [Audio/Audio.h](../src/Audio/Audio.h)と[Audio/Audio.cpp](../src/Audio/Audio.cpp)のPlayBgm(path,bool) / StopBgm | パス指定のPlayBgmオーバーロードとStopBgmに外部の呼出がない。 | LoadBgmとPlayBgm(bool)はAppが使用中。将来のシーン別BGM用に残す判断もできる。 |
| [Core/App.cpp](../src/Core/App.cpp)のse_hit.wav登録 | キーhitのLoadSeはあるが、添付srcにPlaySe("hit")がない。decide/backはシーンで呼ばれる。 | hitの読込だけが未利用候補。これから打撃SEを実装するなら登録を残して呼出を追加する。 |

Aabb.hのIsHitAabbは「辺が触れても接触」、Collision.hのIntersectAabbは「辺だけなら非接触」で、境界の扱いが異なる。名前が似ていても同じ実装ではないため、機械的に統合しない。

## 記録するだけの成績データ

以下は値を更新・転記しているが、結果表示やゲーム進行では読み取られていない。単独フィールドだけでなく、記録元と転記先を一緒に整理する。

| 対象 | 現在の流れ | 判断 |
| --- | --- | --- |
| SceneContext::playerHits / lastHits | PlayerState::hitsを加算し、Finish_で個人配列と合計へ記録。Enterで0へ戻す。ResultSceneには表示しない。 | ミス回数を表示しないなら、PlayerState::hitsを含む一連の計数が整理候補。 |
| SceneContext::playerDefeated | PlayerState::defeatedへ累積し、Finish_で個人配列へ記録。ResultSceneのDEFEATEDは共有撃破数lastDefeatedを表示する。 | 個人別撃破数を表示しないなら、累積と転記の経路が整理候補。 |
| SceneContext::stageCount | 宣言の既定値をGameScene::Enterで0に設定するが、読んで終端を判定する処理がない。 | 最終面を設けない現在の進行では不要候補。 |
| StageEnemySpawn::name / StageEnemyExit::name | TMXの名前を保存し、出口ではspawn.nameを転記するが、EnemyManagerの移動・再出現では名前を使わない。 | 名前をデバッグ表示に使わないなら、文字列と読込/転記が整理候補。位置・direction・enemyTypeは別に判断する。 |

個人別撃破数を整理しても、EnemyManager::ContactResult::defeatedは消してはいけない。GameSceneはこの「そのフレームに本人が蹴った数」を使って個人へ加点する。EnemyManagerの共有m_defeatedCount・GetDefeatedCountも面クリアとDEFEATED表示に必要。

## 未到達でも保持する理由がある処理

| 対象 | 現在の状態 | 方針 |
| --- | --- | --- |
| GameSceneの通常面タイムアップ分岐 | GetStageTimeLimit_は通常面で0、ボーナスで20を返すため、時間制限あり・ボーナス以外の残機減少分岐は通常進行で到達しない。 | タイマー機能を保持する既存要件があるため、未到達だけを理由に削除しない。ボーナスの残り時間・終了処理は使用中。 |
| EnemyManager::Lineup::FromMapとenemyType | 現行GameSceneは面ごとの構成を明示指定するため、通常進行ではFromMapへ入らない。 | TMX指定で敵種を選ぶ既定APIの互換用。マップ主導の構成を使わないと決めてから整理。 |
| ResultKind::StageClear / AllClearとResultSceneの表示分岐 | 現行GameSceneが結果画面へ渡すのはGameOver。通常クリアは同じシーンで次面へ進む。 | 結果画面の拡張予約。現行仕様のみに絞るなら列挙値と表示分岐を一緒に整理。 |
| Playerの地形失敗時の移動・床処理 | 正常にTMXを読めれば使わない。 | 読込エラー時の代替処理なので保持。 |
| フォント未作成時の既定描画 / DxLib_IsInitのチェック | 正常なロード中に通らない場合がある。 | 読込失敗や終了後の後片付けに必要。 |
| ISceneのEnter/Exit/Update/Draw | SceneManagerが基底クラス経由で呼ぶ。 | 具体的なクラス名の参照が少なくても現行の必須経路。 |
| tinyxml2とLICENSE.txt | StageData/StageRendererがXMLを読む。 | 外部コードは今回未編集。ライセンスごと保持する。 |

Audio::Initは現状常にtrueを返すため、App内の「音声初期化失敗」の分岐は今の実装では発生しない。ただし初期化インターフェースとしてAppから実際に呼ばれており、単純な未使用関数ではない。処理を追加する予定も踏まえて残している。

## 削除とは別に見つかった改善点

### 品質の自動復帰用カウンタ

App::RunはFPSサンプル更新のたびにQuality::Forceを呼ぶ。Forceはm_lowCount/m_highCountの両方を0に戻す。その後UpdateOnFpsSampleで40超を検出しても、m_highCountは毎回1までしか増えない。

例としてLow状態でFPSが60、60と2回届いても「Forceで0→判定で1」を繰り返し、Highへ戻す条件の2回に達しない。手動変更のときだけForceするなど、呼出方法を別途修正する必要がある。今回はロジックを変えず、この点をApp.cppとQuality.cppのコメントへ明記した。

現在の品質状態はAppのQUALITY表示へ使われるが、実際に描画を軽くするDrawGlowBoxは未使用。Quality一式が完全な未使用コードという意味ではない。

### 実装と食い違っていたコメント

Quality.cppの中間帯の説明は「連続性をリセットしない」となっていたが、実際は両カウンタを0にしているため、その動作に合わせてコメントだけを修正した。App.cppの自動High復帰の説明も、上記の実装に沿う注意書きへ改めた。Audio.cppの再生コメントは「多重再生」から、実際の呼出を説明する表現へ改めた。

## 調査と確認の方法

- 添付src.zipを展開し、全クラスの所有箇所・include・関数呼出・フィールドの読書きを確認した。
- 旧機能の内部呼出と、App/SceneManagerからつながる現行経路を分けて判断した。
- 自作49ファイルについて、文字列/文字リテラルを保持してC++トークン列を比較した。コメントと空白を除いた列は全ファイル一致した。
- 自作ソースはUTF-8 BOM付き、改行CRLFへ統一した。CP932だった既存の日本語コメントも変換した。
- ThirdPartyの3ファイルは元ZIPとバイト単位で一致することを確認した。
- プロジェクト内の相対includeが解決すること、配布ZIPが正常に展開できることを確認した。

Windows・Visual Studio・実際のDxLib SDKでの再ビルドと実行はこの環境では行っていない。今回の検証は、コメント追記で実行文・宣言・文字列を変えていないことの確認であり、元からの不具合を解消したという意味ではない。
