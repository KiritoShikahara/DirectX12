# STATUS

最終更新: 2026-07-16

## ブランチ
feature/TestClaude（変更は全て未コミット。ユーザーの指示なしにコミット/push等のgit操作はしない）

## 直近やったこと
- 3Dウェーブサバイバル(POE2風ダークファンタジー)を開発中。初期武器3種+パーク武器3種
  (Nova/Homing Missile/Chain Lightning)+必殺技(Ultimate、状態ベースの演出フローに刷新済み)が
  出揃った。詳細はGAME_DESIGN.mdの「現状の実装状況まとめ」参照
- **AtkPower/Defenseステータスの接続**: `PlayerStatusComponent.Base.Defense`の初期値を
  0.0f→20.0f に変更(0のままだとMulDefense倍率が何倍されても0のままになり防御パークが
  無意味になるバグだったため)。新規`ecs::combatutil::GetAtkPowerMultiplier()`
  (`App/src/system/Player/Status/PlayerCombatUtil.h/.cpp`)を追加し、
  `PlayerMovementSystem`の速度倍率と同じ「Current/Base比」方式でAtkPowerパークの
  強化分を全武器(SingleShot/AreaAttack/Orbit/Nova/HomingMissile/ChainLightning)の
  ダメージ計算へ反映した。Defense側は既存の`PlayerContactDamageSystem`の半減点方式の
  被ダメージ軽減式(`kDefenseHalfPoint/(kDefenseHalfPoint+defense)`)がそのまま活きる形。
- **ダメージ数値表記**: 新規`ecs::DamageNumberComponent`/`DamageNumberSystem`
  (`App/src/system/UI/DamageNumber/`)を追加。ワールド座標→スクリーン座標への射影は
  カメラのViewProjection行列を使い、既存の`TextComponent`描画パイプラインへ乗せる形で
  実装(新規ワールド空間テキストレンダラーは作らずに済んだ)。
  `ecs::combatutil::SpawnDamageNumber(position, damage, isPlayerDamage)`を、
  敵へのダメージ適用箇所(武器系6種)と、プレイヤーへのダメージ適用箇所
  (`PlayerContactDamageSystem`)、必殺技のダメージ適用箇所(`PlayerUltimateSystem::Cast`)の
  全てに追加済み。`GameScene.cpp`の`CreateUserSystem()`にも
  `DamageNumberSystem`をPostUpdateで登録済み(`CameraPlayerFollowSystem`の直後、
  カメラの最新ViewProjectionを使うため)。
  Debug/Release両方ビルド確認済み、実機起動確認済み(エラーなし)。
- App.vcxproj/App.vcxproj.filtersへ新規5ファイル
  (PlayerCombatUtil.h/.cpp, DamageNumberComponent.h, DamageNumberSystem.h/.cpp)を登録済み。
- **hougu_pre.efkの導入完了**: ユーザーがEffekseer Editorで`hougu_pre.efk`(上昇後に
  再生するビームエフェクト)をバージョン1710(ランタイム対応上限内)で再エクスポート。
  `UltimateData.csv`の`BeamEffectPath`を`Tornade.efk`(プレースホルダー)から
  `hougu_pre.efk`へ差し替え、db.dbへも同期済み(一時的な`SaveCsvToDb()`呼び出しで
  Debugビルドを1回起動→db.db書き込み確認→コード側は元に戻す、という確立済みの手順で実施)。
  Debug/Release両方で実機起動確認済み、エラーログなし(既知のテクスチャ警告のみ)。
  `hougu_main.efk`(爆発エフェクト)は再エクスポート直後は1810のままだったが、
  ユーザーがEditor側の設定を見直して再々エクスポートし、バージョン1710に収まった。
  `ActivationEffectPath`を`Blow2.efk`から`hougu_main.efk`へ差し替え、db.db同期・
  Debug/Release両方で実機起動確認済み(エラーなし)。これで必殺技の専用エフェクトが
  上昇後ビーム(hougu_pre.efk)・爆発(hougu_main.efk)ともに導入完了。
- **必殺技の火力調整**（ユーザー指示: 「ラスボス以外ワンパンできるレベルの火力」）。
  `UltimateData.Damage`を999→50に変更。敵のステータスはスポーン時に確定し以降フレーム毎の
  再計算はしないため、ザコ敵の最大HPはクリア間際(180秒)でも`10×(1+0.004×180)≒17.2`程度、
  対してボースは`10×(1+0.004×90)×10倍補正=136`固定(スポーン時刻90秒で確定)。
  Damage=50はザコを確実に即死させつつボースは複数発(約3発)必要という調整。
  db.db同期・Debug/Release両方で実機起動確認済み。

- **必殺技の演出調整**（ユーザー指示）。`BeamScale`8→16(hougu_preを2倍)、
  `ActivationScale`8→80(hougu_mainを10倍)、`RiseHeight`150→450(上昇高度を3倍)に変更。
  加えて、ビームの先端をカメラ方向へ向ける機能を新規実装。
  `ecs::EffectComponent`へ`Rotation`(オイラー角、ラジアン)フィールドを新設し
  `EffekseerManager::Update`が毎フレーム`Effect.SetRotation()`を適用するようにした
  (Scaleと同じパターンで汎用化、他のエフェクトにも今後使える)。
  `ecs::effectutil::PlayOneShotCombined`に回転引数を追加(デフォルト0で既存呼び出し箇所は
  無変更)。`PlayerUltimateSystem`にビーム位置→カメラ位置の方向ベクトルからオイラー角を
  算出する`ComputeBeamRotationTowards`を追加し、ビーム再生時にのみ適用。
  素材の既定の向きの軸は当初+Y(上向き)と仮定していたが、ユーザーから
  「Effekseer Editorの青軸(+Z)方向に伸びている」と指摘を受け、+Z前提の計算式に修正済み
  (ピッチ=asin(-dir.y)、ヨー=atan2(dir.x, dir.z))。db.db同期・Debug/Release両方で
  実機起動確認済み(エラーなし)。

- **必殺技の演出微調整**（ユーザー指示、実機で見た結果のフィードバック）。
  (1) ビーム位置がプレイヤー座標そのままだとカメラの正面へまっすぐ延びる形になり
  奥行きが見えず視認しづらいとの指摘を受け、`BeamCameraOffset`(新規、デフォルト20)を追加。
  カメラ方向へこの距離だけ手前にずらして再生するようにした。
  (2) `ActivationScale`を80(10倍)→56(7倍)に縮小。
  (3) 爆発エフェクトがY=0(発動前座標そのまま)だと地面に少しめり込むとの指摘を受け、
  `ActivationHeightOffset`(新規、デフォルト5)を追加、この高さだけ上げて再生するようにした。
  併せて`SqliteManager`へ`DropTable<T>()`を新設(`EnsureTable`のCREATE TABLE IF NOT EXISTSは
  既存テーブルの列を更新しないため、フィールド追加時にテーブルを作り直す汎用手段として追加)。
  db.db同期(DropTable→SaveCsvToDb)・Debug/Release両方で実機起動確認済み(エラーなし)。

- **必殺技演出中の他攻撃の抑制**（ユーザー指示）。必殺技演出中(IsActive)は他の武器が
  一切発動・動作しないよう、新規`ecs::IsPlayerUltimateActive(registry)`
  (`PlayerUltimateComponent.h`にinline関数として追加)を、武器系10システム
  (SingleShot/AreaAttack/AreaAttackHazard/Orbit/Nova/HomingMissile/HomingMissileSteering/
  ChainLightning/ProjectileMovement/ProjectileCollision)のUpdate()冒頭に追加し、
  演出中は丸ごとスキップ(新規攻撃の発動だけでなく、既存の弾の移動・命中判定、
  周回オーブの動き、ハザードの継続ダメージも含めて完全に一時停止する)。
  併せて`PlayerUltimateSystem`に`SetOtherEffectsVisible(registry, bool)`を追加し、
  必殺技発動(Activate)時に他の全エフェクト(EffectComponent::IsVisible)を非表示化、
  ビーム(hougu_pre)再生終了時(FinishAndExplode冒頭)に再表示するようにした。
  IsVisible=falseにするとEffekseer側の再生も一時停止するため(EffectObject::SetVisible)、
  非表示中は見た目も動きも完全に止まる。Debug/Release両方ビルド確認済み、
  実機起動でエラーなし。

- **パーク選択と必殺技演出の競合修正**（ユーザー指示）。(1) 必殺技演出中(IsActive)に
  レベルアップしてもパーク選択画面へ遷移しないよう、GameStateSystemのInGame→PerkSelect
  遷移条件へ`!ecs::IsPlayerUltimateActive(registry)`を追加(演出終了後まで保留)。
  (2) 1回のXP付与で複数レベル分の閾値を同時に超えても1回しかパーク選択が提示されない
  バグを修正。GameStateComponent::LevelUpRequested(bool)をPendingLevelUpCount(int)へ変更、
  EnemyDeathSystem::AwardExperienceのif文をwhileループにしレベルアップ回数分加算。
  GameStateSystemはPerkSelectDone時にカウンタを1減算し、まだ残っていれば
  PerkSelectComponentを外したままPerkSelect状態に留まる(PerkSelectSystemが次フレームで
  自動的に新しい3択を生成する既存の仕組みをそのまま再利用、PerkSelectSystem自体は無改修)。
  Debug/Release両方ビルド確認済み、実機起動でエラーなし。

- **新メニュー階層(ハブ画面)のガワを実装**（ユーザー指示。ゴールドによるステータス恒久強化
  機能の第一段階、シーン遷移のみ先行実装）。Title→Hub→(Menu武器選択 or StatusUpgrade
  ステータス強化)という階層に変更した。
  - 新規`scene::HubScene`(`App/src/Scene/Hub/`)：背景+「武器・ステージ選択」「ステータス
    強化」の2択。新規`ecs::HubMenuComponent`/`HubMenuInputSystem`(`App/src/system/HubMenu/`)
    がMenuLeft/MenuRightでカーソル移動・Selectで確定(選択肢に応じてMenuScene/
    StatusUpgradeSceneへ)・Cancelでタイトルへ戻る処理を担当(PerkSelectSystemの3択UIと
    同じ配色・ハイライトパターンを踏襲)。
  - 新規`scene::StatusUpgradeScene`(`App/src/Scene/StatusUpgrade/`)：現状は仮の
    プレースホルダー(背景+「準備中」テキストのみ)。Cancelで`HubScene`へ戻る
    (`StatusUpgradeInputSystem`が担当)。PlayerSaveData/StatUpgradeData実装後、
    ここへ本来の強化UI・ゴールド消費操作を追加する。
  - 既存の遷移を変更：`TitleInputSystem`のSelect先をMenuScene→HubSceneに変更。
    `MenuControllerSystem`のCancel先をTitleScene→HubSceneに変更（階層を1段飛ばさない
    ため）。
  - `macros.h`へ`HUB_SCENE_NAME`/`STATUS_UPGRADE_SCENE_NAME`を追加。
  - 動作確認：`START_SCENE_NAME`を一時的にHub/StatusUpgradeへ切り替えてそれぞれ実機起動
    確認済み(ログに"Hub Scene."/"StatusUpgrade Scene."が出力されエラーなし)、確認後
    `GAME_SCENE_NAME`(開発中の起動ショートカット)へ戻した。Debug/Release両方ビルド確認済み。
- **ゴールド強化システムの本実装完了**（ユーザー指示、ハブ画面のガワに続く第2フェーズ）。
  - 新規`data::PlayerSaveData`(`App/src/Data/Save/PlayerSaveData.h`)：Gold・MaxHp/AtkPower/
    Defense/CooldownRateの強化レベルを保持。`ConfigManager<T>`(JSON、`WindowConfig`と同じ
    仕組み)で`Assets/Bin/Save/player_save.json`へ永続化。Debug/Releaseで挙動を分けず常に
    同じファイルを読み書きする。`EnsurePlayerSaveDataLoaded()`(inline関数)で二重登録を
    防ぎつつ各所から参照できるようにした。
  - 新規`data::StatUpgradeData`(CSV/DB、`App/Assets/Data/StatUpgrade/StatUpgradeData.csv`)：
    対象ステータス(Id=0:MaxHp/1:AtkPower/2:Defense/3:CooldownRate)ごとにBaseCost・
    CostGrowthPerLevel・ValuePerLevel・MaxLevelを定義。コスト=BaseCost+CostGrowthPerLevel×
    現在レベルで、レベルが上がるごとに増加する。
  - `GameSceneFactory::CreatePlayer`に`ApplyStatUpgrades()`を追加。PlayerSaveDataの強化
    レベル×StatUpgradeData::ValuePerLevelを`PlayerStatusComponent::Base`へ加算してから
    `Recompute()`する。
  - `StatusUpgradeScene`を本実装（プレースホルダーから置き換え）。新規
    `ecs::StatusUpgradeComponent`(カーソル位置)/`StatusUpgradeInputSystem`
    (MenuUp/MenuDownで選択・Selectで購入・Cancelで戻る・所持ゴールドと各行のLv/コストを
    毎フレーム最新表示に更新)。表示名はCSVではなくwstringリテラルで直接持つ
    (PerkDefinitionと同じ方針、UTF-8→UTF-16変換ユーティリティを新設せずに済む)。
  - `InputManager`に`MenuUp`/`MenuDown`アクションを新設(W/↑・S/↓・DPadUp/DPadDown)。
  - `EnemyDeathSystem`に`AwardGold()`を追加。`EnemyBaseStatus::GoldValue`(新規、デフォルト
    3.0、ExperienceValueと同じ扱い＝現状EnemyData.csv未接続)を撃破時に加算、即座に
    `PlayerSaveData`へ保存する(ボースは`kBossGoldMultiplier`(20倍)適用)。
  - デバッグ用に`debug::PlayerSaveDebugPanel`(新規)を追加。`data::ConfigEditor<T>`
    (Load/Save/Resetボタン+ドラッグ編集可能なフィールド一覧、既存の汎用ImGuiインフラを
    そのまま利用)をラップしたもので、GameScene/StatusUpgradeScene両方に登録。
    ImGui上でGold等を直接編集できる。
  - 実機確認：Debug/Release両方でGameScene/StatusUpgradeScene起動確認済み。
    `player_save.json`の書き込み(Save())・読み込み(手動でGold=500等を書き込んだ状態からの
    読込→ApplyStatUpgradesが反映)双方を一時コードで検証し、エラーなし確認済み。
    StatUpgradeDataのdb.db同期も一時コードで実施・確認済み。検証用の一時コード・JSONは
    全て元に戻した。
  - **見つかった注意点**: Write toolで既存ファイル(BOM付き)を全文上書きすると、BOMが
    再度失われる。今回`StatusUpgradeScene.cpp`/`StatusUpgradeInputSystem.cpp`で発生し、
    日本語wstringリテラルがコードページ932として誤解釈されて「'dataRegistry'は
    未定義」等の意味不明な連鎖エラーになった。BOM再付与で解決(詳細はDECISIONS.md)。
  - **見つかった注意点2**: EnTTの`view.each()`は、データを持たない空のタグ型
    (`StatusUpgradeGoldUiTag`等)についてはコールバックへ引数を渡さない
    (empty型最適化)。データを持つタグ(`OptionIndex`等のフィールドあり)とは
    ラムダの引数の数が変わる点に注意（`std::invoke`のオーバーロード解決エラーとして現れる）。

- **敵の出現率向上・ステータス強化の確認表記・新規攻撃「Meteor」追加**（ユーザー指示、
  3件まとめて対応）。
  - `WaveData.CSV`の`SpawnInterval`を2.5→1.5秒に短縮(敵の出現率向上)。`WaveComponent`の
    フォールバック既定値も合わせて変更。db.db同期済み。
  - `StatusUpgradeScene`に確認ダイアログ・フィードバックメッセージを追加。
    `StatusUpgradeComponent`へ`IsConfirming`/`MessageTimer`/`Message`を追加。Selectで
    即購入せず「◯◯を強化しますか？(Cost:XG) [Select]:はい [Cancel]:いいえ」の確認を
    挟むようにし(`TryOpenConfirm`→`ConfirmPurchase`)、ゴールド不足時は確認を出さず
    「ゴールドが足りません」、レベル上限到達時は「既に最大レベルです」をそれぞれ
    2秒間表示するようにした(`ShowMessage`、`StatusUpgradeMessageUiTag`新規)。
    確認ダイアログ表示中はカーソル移動・HubSceneへの遷移を止める。
  - 新規武器「Meteor」を追加(`eWeaponType::Meteor`、`data::MeteorWeaponData`、
    `MeteorWeaponRuntimeComponent`、`MeteorWeaponSystem`新規)。狙い不要の完全自動発動で、
    SearchRadius内の敵からランダムに最大MeteorCount体(初期値3)を選び、それぞれの頭上へ
    隕石を落として範囲ダメージを与える(既存のOverlapSphere+EnemyTagパターンを踏襲、
    対象が1体も居ない場合はクールダウンを消費せず待機する他の自動武器と同じ方針)。
    エフェクトは専用素材が無いため`PhantasmMeteor.efk;Fire7.efk`を仮流用(';'区切りの
    複数同時再生)。`PerkDefinition`にAcquireWeaponパーク「新武器: Meteor」を追加。
    `WeaponInventoryDebugPanel`にもMeteorを追加(新規武器追加時の追記箇所として
    コメントで明示されていたため対応)。db.dbへ`meteor_weapons`新規テーブルを同期済み。
  - Debug/Release両方ビルド確認済み、実機起動でエラーなし(検証用の一時コード・
    起動シーン切り替え・テスト用JSONは全て元に戻した)。

## 次にやるべきこと
- 敵の出現率向上・Meteor武器・強化確認ダイアログの実機手動操作確認（スポーン間隔が
  体感で速くなっているか、Meteorをデバッグパネルまたはパークで取得して発動を確認、
  StatusUpgradeSceneでSelect→確認ダイアログ→はい/いいえ、ゴールド不足時のメッセージ、
  MAX到達時のメッセージがそれぞれ正しく表示されるか）
- パーク選択と必殺技演出の競合修正の実機動作確認（必殺技の全体ダメージで複数の敵を倒し
  複数レベル分のXPが一度に入った場合に、演出終了後にレベルアップ回数分だけ連続して
  パーク選択が提示されるか、実際にプレイして確認）
- 実機で手動操作しての一連の動作確認（Title→Select→Hub→両方の選択肢→各シーンへ
  遷移すること、Cancelでの戻り遷移も含む。StatusUpgradeSceneでMenuUp/MenuDownでの選択・
  Selectでの購入(ゴールド減少・レベル上昇・表示更新)・ゴールド不足時に購入できないこと・
  MaxLevel到達後は購入できずMAX表示になること。GameSceneで敵撃破時にゴールドが増加する
  こと。アプリを再起動してもゴールド・強化レベルが引き継がれること。
  自動テストでは起動確認・書き込み/読み込みの一時コード検証のみ実施済みで、
  実際のキー入力によるフロー確認・目視でのUI崩れ確認はまだ）
- 必殺技の実機動作確認（ビームがカメラの手前にずれて視認しやすくなったか、先端が
  実際にカメラの方を向いているか、爆発が地面にめり込まなくなったか、各エフェクトの
  拡大サイズの見た目バランスを実際にプレイして確認。向きがおかしい場合は軸の向きの
  前提が違う可能性があるため報告してほしい）
- 必殺技演出中の他攻撃抑制の実機動作確認（発動中に他の武器が発動しないか、既存の
  弾/オーブ/ハザードが完全に止まって見えるか、ビーム終了と同時に元通り動き出すかを
  実際にプレイして確認）
- 必殺技の火力バランス確認（ザコが確実に即死するか、ボースが約3発で倒せる感触か、
  実際にプレイして確認。値は`UltimateDebugPanel`からGUIで調整可能）
- カメラのCameraDistance/CameraHeightはRiseHeight変更と連動していないため、上昇高度3倍化で
  ビームが画面内に収まりきらない/遠すぎる見た目になっていないか要確認（気になる場合は
  CameraDistance/CameraHeightも合わせて調整可能）
- AtkPower/Defense接続後のバランス確認（実際にプレイして、パーク由来のAtkPower上昇/
  Defense上昇がダメージ計算に反映されているか、ダメージ数値表記が見やすいか確認）
- Homing Missile/Chain Lightingの実機動作確認（前回までの継続タスク）
- 以前からの実機確認待ち項目(コアループ・初期パーク・リザルトUI・Menu遷移等)は`TASKS.md`参照
- 問題なければユーザーの指示でコミット

## 未解決の問題・注意点
- リポジトリ直下に `start-claude.cmd` が未追跡のまま存在する（コミットするか要確認）
- 新規Appファイル作成時はBOM付きUTF-8で保存すること（詳細はDECISIONS.md）
- **Effekseerランタイムのバージョン不一致**（hougu_pre/hougu_mainは解決済みだが、今後も
  再発しうる注意点として記録）。このプロジェクト同梱のランタイム(`CoreEngine/external/Effekseer`)は
  `SupportBinaryVersion = 1710`が上限。ユーザーが新しいEffekseer Editorでエクスポートした素材が
  1.8x以降の形式になっていると読み込み拒否される。バイナリ先頭"SKFE"直後4バイトの
  バージョン値で機械的に確認できる(詳細はDECISIONS.md)。今後もユーザーが新しい素材を
  追加する場合、Editor側のエクスポート設定次第で同じ問題が再発しうる
- 物理・時間まわりの注意点:
  - Dynamic RigidBodyを瞬時にテレポートしたい場合は、Transform::SetPosition()した上で
    `TransformDirtyTag`を付与すること(`PhysicsSystem::SyncFromTransform`がJolt側の位置も
    明示的に上書きしてくれる)。単にTransformだけ書き換えても次の物理ステップで
    Jolt側の位置に上書きされて元に戻ってしまう
- 既存ファイルのエンコーディングについて: このプロジェクトのBOM無しファイルはShift-JIS(CP932)の
  ものとUTF-8のものが混在している。既存のBOM無しファイルへ日本語コメントを追記する際は、
  事前にgrepでUTF-8として既存の日本語コメントがヒットするか確認し、ヒットしない
  (=Shift-JISの可能性)ファイルには英語コメントのみ追記する運用にしている（詳細はDECISIONS.md）
- CoreEngineプロジェクトを単体でビルドする場合(ソリューションを経由しない場合)、
  `$(SolutionDir)`マクロが解決されずインクルードエラーになる。
  `msbuild App.vcxproj /p:SolutionDir=<リポジトリルート>\` のように明示的に渡すこと
  (ソリューションファイルは`.slnx`形式で、標準msbuildからは直接ビルドできなかった)
