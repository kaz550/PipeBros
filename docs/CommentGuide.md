# コメント付きソースの読み方

入力した結果がどの順で画面へ表れるかを追う場合は、次の順で読む。

1. src/main.cppからCore/App.cppへ進み、入力取得・dt・シーン更新・描画・終了を確認する。
2. Scene/SceneManager.cppで、要求したシーン切替がUpdate終了後に適用されることを見る。
3. Input/Input.cppで共通入力とForPlayerを確認する。1人/2人の履歴を別に持ち、相手の入力を混ぜない。
4. Scene/GameScene.cppのEnter、RestartStage_、Updateを読む。ゲーム全体の再開始と、得点・残機を残す面再配置を区別する。
5. Game/Player.cppで、操作から地形衝突までの更新を追い、ResolveContactで2人間の接触を読む。
6. Game/EnemyManager.cppで、敵全体を1回更新し、各プレイヤーへ結果を返す流れを見る。
7. Game/StageData.cppとStageRenderer.cppを並べ、当たり判定と見た目を分離して読む。
8. GameScene::UpdateBonus_とResultSceneを読み、個人への加点、協力報酬、共通ハイスコアを確認する。

各ファイルの先頭に担当と所有関係、主要関数の直前に目的・処理順・戻り値の意味を追記した。複雑な接触計算には、画面端の最短距離、相対移動中の接触時刻、押し戻しの意図をコメントしている。

位置は基本px、移動速度はpx/秒、重力はpx/秒^2、タイマーは秒。Yの正方向は下で、上向きのジャンプ初速は負値になる。MoveByContact_だけはdt=1で位置補正を地形へ通すため、dx/dyは速度ではなく補正量として読む。

未使用候補のファイルやAPIはソース内にも記した。最終的な判断理由と、残す必要がある関連データは[不要コード候補](UnusedCodeReview.md)を参照する。

今回のコメント追記はゲームの実行文・宣言・定数・文字列を変更しない。Windowsのmin/max対策の括弧もそのまま保持している。
