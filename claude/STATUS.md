# STATUS

最終更新: 2026-07-17(続き)

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

- **新規武器3種を追加（ユーザー指示: 攻撃案の提示→Void Beam/Bone Spear/Cleaveの3案を全て採用）**。
  いずれもNova/Homing Missile/Chain Lightning/Meteorと同じ「狙い不要・自動発動・パーク経由
  でのみ取得」ファミリーに統一(既存の手動入力2枠(Attack/Attack2)を使い切っており、
  3つ目の手動武器を増やすとInputManager改修が必要になるため)。
  - **Void Beam(貫通レーザー)**: 新規`eWeaponType::VoidBeam`/`data::VoidBeamWeaponData`/
    `VoidBeamRuntimeComponent`/`VoidBeamWeaponSystem`。最も近い敵の方向へ直線を伸ばし、
    `PhysicsSystem::OverlapSphere`で拾った候補を「線分への垂線距離」で数式フィルタして
    直線上の敵全員を貫通ヒットさせる(新規の物理クエリ形状は追加せず、既存のOverlapSphere+
    数式フィルタで完結)。Chain Lightningと同じく移動する実体を持たない瞬間ヒット方式。
  - **Bone Spear(貫通弾)**: 新規`eWeaponType::BoneSpear`/`data::BoneSpearWeaponData`/
    `BoneSpearRuntimeComponent`/`BoneSpearWeaponSystem`。既存`ProjectileComponent`へ
    `PierceCount`フィールドを追加(通常弾は0のままで既存武器に影響無し)、
    `ProjectileCollisionSystem`を「PierceCountが残っている間は消滅せず貫通する」よう拡張。
    Homing Missileと同じ発射パイプラインを流用するが誘導はしない(直進)。
  - **Cleave(近接扇状攻撃)**: 新規`eWeaponType::Cleave`/`data::CleaveWeaponData`/
    `CleaveRuntimeComponent`/`CleaveWeaponSystem`。狙い方向を中心とした扇状範囲を
    OverlapSphere+角度フィルタで判定し、範囲内の敵全員にダメージ+ノックバックを与える。
    ノックバック実現のため新規`ecs::EnemyKnockbackComponent`/`EnemyKnockbackSystem`
    (`App/src/system/Enemy/Knockback/`)を追加。`EnemyChaseSystem`は毎フレーム
    `RigidBodyComponent::MoveVelocity`を無条件上書きするため、このコンポーネント保持中は
    追従移動を丸ごとスキップするガードを1行追加(2システムが同一フレームで速度を
    取り合わないようにするための明示的な排他制御。仮に実行順で偶然競合しなくても、
    将来の登録順変更で壊れる暗黙の依存を避けるため意図的に追加)。
  - 3種とも`PerkDefinition`にAcquireWeaponパークを追加、`WeaponInventoryDebugPanel`/
    `GameStatusDebugPanel`にも表示・追加操作を登録済み。db.dbへ`void_beam_weapons`/
    `bone_spear_weapons`/`cleave_weapons`新規テーブルを同期済み(一時的な`SaveCsvToDb()`
    呼び出しでDebugビルドを1回起動→db.db書き込み確認→コード側は元に戻す、確立済みの手順)。
  - 新規作成した.h/.cppファイル(15個)は作成直後、BOM無しでREFLECT_FIELDマクロが
    壊れる既知の問題(DECISIONS.md参照)が実際に発生したため、PowerShellでUTF-8 BOM付きへ
    再保存して解消した。Debug/Release両方ビルド成功、実機起動でエラーなし確認済み
    (プレースホルダーエフェクト: VoidBeam=LightningStrike.efk、Bone Spear=Sword1.efk/
    HitEffect.efk、Cleave=Sword6.efk、いずれも仮流用)。
  - 実機での手動プレイ確認(パーク選択でのランダム出現→取得→各武器の発動・貫通/ノックバック
    挙動の目視確認)はまだ未実施。
- **バグ修正: PlayerSaveData保存時のクラッシュ**（ユーザー報告: `Assets/Bin/Save/`フォルダが
  存在せず`ConfigManager<PlayerSaveData>::Save()`で例外）。`std::ofstream`は中間ディレクトリを
  自動作成しないため、`JsonSerializer::SaveToFile()`側で保存前に
  `std::filesystem::create_directories()`するよう修正(`ConfigManager<T>`利用箇所全てに
  共通する根本修正、詳細はDECISIONS.md)。フォルダを実際に削除した状態から起動させ、
  自動生成・エラーなしを確認済み。Debug/Release両方ビルド確認済み。

- **新規スキル「Flicker Strike」を追加（ユーザー指示）**。パワーチャージ(敵撃破ごとに+1、
  上限はFlickerStrikeWeaponData::MaxCharge、時間経過での減衰なし)を消費する連続ワープ攻撃。
  マウスカーソル位置のレイキャスト(`CameraSystem::ScreenPointToRay`+`PhysicsSystem::
  TryPickEntity`、Editorのオブジェクト選択と同じ仕組み)で指定した敵へ初撃→所持チャージを
  全消費し、チャージ1個につき2回、近くの敵(直前の対象は除外)へ次々ワープ攻撃する
  (近くに対象がいなくなった時点で残り回数は打ち切り＝チャージの無駄撃ち、明示的な仕様)。
  新規`eWeaponType::FlickerStrike`、パーク経由で取得。既存の手動入力2枠(Attack/Attack2)は
  使用済みのため新規`"FlickerStrike"`アクション(Rキー/PadY)を追加し、
  `PlayerAimComponent::WantsToFireTertiary`で受ける。ワープはUltimateと同じ
  `TransformDirtyTag`方式でテレポートし、演出中は無敵化・最後にワープした場所に留まる
  (元の座標には戻さない、PoE準拠)。
  - **アーキテクチャ改善**: 既存の`IsPlayerUltimateActive()`ガードが15箇所に散らばっていたが、
    2つ目の排他スキルを追加するにあたり同じガードを重複させず、新規`ecs::IsPlayerActionLocked()`
    (`system/Player/PlayerActionLock.h`)に統合。該当15箇所全てをこちらへ差し替えた
    (将来3つ目の排他スキルが増えても1箇所の追記で済む)。
  - **重複コード解消**: Chain LightningがPrivateで持っていた「近くの未処理の敵を探す」ロジックを
    `App/src/system/Enemy/EnemyTargetUtil.h/.cpp`へ切り出し、Chain LightningとFlicker Strike
    両方から共有するようリファクタ。
  - db.dbへ`flicker_strike_weapons`新規テーブルを同期済み。新規作成した9ファイルもBOM無しで
    保存されREFLECT_FIELDマクロが壊れる既知の問題が再発したため、BOM付きへ再保存して解消。
    Debug/Release両方ビルド成功、実機起動でエラーなし確認済み。
  - 実機での手動プレイ確認(パーク選択でのランダム出現→取得→カーソルで敵を指定してRキーで
    発動→ワープ演出・ダメージ・パワーチャージの消費確認)はまだ未実施。

- **バグ調査+対策: エフェクトが一瞬四角形ポリゴンになる**（ユーザー報告。FireBolt/IceSpike/
  FrostOrb等の既存エフェクトで発生、新武器4種(Void Beam/Bone Spear/Cleave/Flicker Strike)を
  同一プレイ中に発動した後に起きたとのこと）。調査の結果、新武器4種の新規プレースホルダー
  素材自体はバージョン(1710、対応範囲内)・参照テクスチャ(Texture/Parts配下に実在)とも
  問題なし。ただし各エフェクトはこれまで`ecs::effectutil::PlayOneShotCombined`等の
  実際の発動時に初めて`EffekseerManager::GetEffect()`で遅延読み込みされる設計だったため、
  新武器をプレイ中に初めて発動した瞬間にテクスチャ読み込みが走り、その間の数フレームだけ
  他の再生中エフェクトの描画が乱れた可能性が高いと判断（確定はできていないが、状況・
  タイミングと整合する）。
  対策として、新規`ecs::effectutil::PreloadEffect()`と`GameScene::PreloadWeaponEffects()`を
  追加し、ロード画面中(`LoadResource()`直後)に全武器・必殺技のエフェクト素材をまとめて
  先読みするようにした。新しい武器種別を追加した場合は`PreloadWeaponEffects()`にも
  追記が必要(コメントで明示済み)。Debug/Release両方ビルド確認済み、起動時の
  プリロードでエラーなし確認済み。
  **状況更新**: ユーザーから追加報告あり。先読み対応後も症状継続。「ずっとではなく
  一定周期で一瞬・一部だけ」「FrostOrbの周回オーブ・IceSpikeでも発生、他のエフェクトも
  怪しい」とのことで、初回読み込み時のもたつき(先読みで対策済み)とは別の、周期的な
  現象と判明。継続的にループ再生されるエフェクト(オーブ等)に絡む症状の可能性が高いが、
  静的なコード確認だけでは原因を特定できておらず未解決。EffekseerのHandle管理・
  EffectObject::Play()でのStop→Play(ハンドル再発行)まわりを疑っているが未検証。
  実機の映像(録画等)が無いと以降の切り分けが難しい状況。

- **Flicker Strikeの仕様変更（ユーザー指示）**。(1) 対象指定方式を、マウスカーソルの
  レイキャスト指定から狙い方向(PlayerAimComponent::Direction、他の武器と同じ基準)ベースに
  変更。狙い方向へInitialTargetMaxRange・InitialSearchWidthで定義される直線範囲内の
  最も近い敵を探し、いなければ何も起きない(クールダウン消費なし)。
  `FlickerStrikeWeaponSystem::PickDirectionalTarget`(VoidBeamWeaponSystemと同様の
  線分投影+垂線距離の数式)を新設、カーソルレイキャスト版の`PickInitialTarget`は削除。
  (2) ワープ演出中に他の手動スキル(Attack/Attack2/Ultimate)の入力があった場合、
  プレイヤーの操作意思を優先してその時点でシーケンスを打ち切るようにした
  (`UpdateActiveSequence`冒頭で`InputManager::IsActionPressed`を直接ポーリング、
  `PlayerAimComponent`経由ではなく生入力を見ることで、`IsPlayerActionLocked()`による
  入力ロック中でも中断を検知できるようにした)。チャージ消費の計算式(0個→1回、
  3個→7回、5個→11回)自体は変更なし。`FlickerStrikeWeaponData`へ`InitialSearchWidth`
  列を追加(スキーマ変更のためdb.dbをDROP→再同期済み)。Debug/Release両方ビルド・
  起動確認済み。
  (3) バランス調整（ユーザー指示）。ゲーム開始時のパワーチャージ所持数を0→3個に変更。
  `FlickerStrikeWeaponData`へ`InitialCharge`(デフォルト3)を新設し、
  `GameSceneFactory::CreatePlayer`が`PlayerPowerChargeComponent`生成時にこの値を
  読んで初期値として設定するようにした(MaxChargeと同じくデータ駆動)。
  火力は「序盤の雑魚敵を一撃」の要求を受け、`BaseDamage`を10→15に変更(雑魚敵の
  Lv1時点のMaxHp目安10前後を確実に一撃で倒せる値、FireBolt等の既存武器の調整方針を踏襲)。
  スキーマ変更のためdb.dbを再度DROP→再同期済み。Debug/Release両方ビルド・起動確認済み。
  実機での操作感・バランス確認はまだ。
  (4) 追加調整（ユーザー指示）。2発目以降のワープ先探索範囲(WarpSearchRadius)が
  狭いとの指摘を受け40→200(5倍)に変更。命中エフェクト(HitEffectPath)は
  既に毎ヒット再生する実装済みだったが、素材をLight4.efk(光/ビーム系)から
  AttackHit.efkへ変更(Chain Lightning等で実績のある、より打撃感の分かりやすい素材)。
  値の変更のみ(スキーマ変更なし)のためdb.dbはDROP不要、SaveCsvToDbのみで同期済み。
  Debug/Release両方ビルド・起動確認済み。

- **エフェクトの「一瞬四角形になる」バグを修正**（ユーザー指示。**ユーザーが実機プレイで
  解消を確認済み**）。
  - 調査: `EffekseerManager::Update()`は、`IsLoop=true`のエフェクト(FrostOrbの周回オーブ、
    IceSpike等、および各種投射武器のトレイル(FireBolt/Homing Missile/Bone Spear等)が
    該当)について、素材自体の再生時間が尽きて`IsPlaying()==false`になるたびに
    `Effect.Play()`(内部で`Stop()`→新規`Play()`)を呼んで再始動していることを確認。
    Effekseerのハンドルは`m_NextHandle`による単調増加カウンタで、値の使い回し
    (エイリアシング)は起きないと確認済み(ソース確認: `Effekseer.Manager.cpp`の
    `AddDrawSet`)。一方、生成直後のインスタンスはビルボードの向き等、前フレームとの
    差分に依存する項目がまだ確定しておらず、素の四角形に近い見た目で1フレームだけ
    描画される可能性が高いと判断（多くのパーティクルエンジンで見られる一般的な挙動で、
    「周期的・瞬間的・IsLoop=trueの持続エフェクトで発生」という報告内容と整合する）。
  - 対策: 新規`graphics::EffectObject::SetRenderingVisible(bool)`を追加
    (`SetVisible`と異なり`SetShown`のみ変更し`SetPaused`はしない＝内部シミュレーションは
    止めない)。`ecs::EffectComponent`へ`IsHiddenAfterLoopRestart`(新規)を追加し、
    `EffekseerManager::Update()`でループ再始動した直後の1フレームだけ
    `SetRenderingVisible(false)`で非表示にし、次のUpdateで`IsPlaying()==true`になった
    (=内部状態が1tick進んだ)時点で`SetRenderingVisible(true)`に戻すようにした。
    内部シミュレーションは止めていないため、ゲームロジック上の再生継続・位置追従等には
    影響しない。
  - Debug/Release両方ビルド確認済み、GameScene(FrostOrb等が実際に稼働する状態)で
    実機起動しクラッシュ無し確認済み。

- **Flicker Strikeのヒットエフェクトを4倍に拡大**（ユーザー指摘: 「小さすぎるかな」）。
  ヒットエフェクトが対象の座標(+HeightOffset)で正しく再生されていることをコードで
  確認した上で(`FlickerStrikeWeaponSystem::WarpAndHit`)、`HitEffectScale`を1.0→4.0に
  変更(同じ代用素材`AttackHit.efk`を使うChain Lightningと同じ基準)。値の変更のみ
  (スキーマ変更なし)のためdb.dbはSaveCsvToDbのみで同期済み。Debug/Release両方
  ビルド・起動確認済み。

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

- **Flicker Strikeのヒットエフェクトを4倍に拡大**（ユーザー指摘: 「小さすぎるかな」）。
  ヒットエフェクトが対象の座標(+HeightOffset)で正しく再生されていることをコードで確認した上で
  (`FlickerStrikeWeaponSystem::WarpAndHit`)、`HitEffectScale`を1.0→4.0に変更
  (同じ代用素材`AttackHit.efk`を使うChain Lightningと同じ基準)。値の変更のみ
  (スキーマ変更なし)のためdb.dbはSaveCsvToDbのみで同期済み。Debug/Release両方
  ビルド・起動確認済み。

- **敵の出現数を20倍に**（ユーザー指示）。`data::WaveData`/`ecs::WaveComponent`へ
  `SpawnCountPerTick`(新規、デフォルト1)を追加し、20に設定。`EnemySpawnSystem`は
  1回のスポーンタイミングで`SpawnCountPerTick`体まとめて湧かせるよう変更(重なって
  湧かないよう、1体ごとに独立してランダムなスポーン位置を求める)。
  `GameSceneFactory::CreateStateController`/`WaveDebugPanel::ApplyToRunningWave`にも
  この値の反映処理を追加(既存の他フィールドと同じパターン)。スキーマ変更のため
  db.dbはDropTable→SaveCsvToDbで再同期済み。Debug/Release両方ビルド・起動確認済み
  (20体まとめてスポーンしてもクラッシュ・パフォーマンス上の異常なし)。

- **レベルアップの経験値増加率を緩和 + パワーチャージ上限をレベルに応じて増加**
  （ユーザー指示、2件まとめて対応）。
  1. `PlayerLevelComponent::ExperienceGrowthRate`を1.2(20%/レベル)→1.08(8%/レベル)に変更。
     複利で急激に重くなりレベルが上がりにくくなっていたのを緩和(この値はCSV/DB非経由の
     構造体デフォルト値がそのまま使われる設計のため、コード変更のみで反映される)。
  2. パワーチャージの上限をプレイヤーレベルに応じて増やせるように変更。
     `FlickerStrikeWeaponData`へ`MaxChargePerLevel`(新規、デフォルト1)を追加し、
     実際の上限 = `MaxCharge`(Lv1時点の初期値、5のまま) + `MaxChargePerLevel`×(Lv-1)
     という計算式にした。この計算を`EnemyDeathSystem::AwardPowerCharge`と
     `GameStatusDebugPanel`の2箇所で重複していたため、新規`ecs::ComputeMaxPowerCharge()`
     (`PlayerPowerChargeComponent.h`にinline関数として追加)へ切り出し、両方から共通で
     呼ぶようにリファクタリングした。
  スキーマ変更(MaxChargePerLevel追加)のためdb.dbはDropTable→再同期済み。Debug/Release
  両方ビルド・起動確認済み。

- **Flicker Strike中の自動発動武器を止めないよう修正**（ユーザー指摘: 「フリッカーストライク
  中にオートの攻撃も行われなくなっている」）。`ecs::IsPlayerActionLocked()`
  (Ultimate中またはFlicker Strike中の両方でtrue)を全武器Systemが一律で参照していたため、
  自動発動武器(Nova/Homing Missile/Chain Lightning/Meteor/SelfDefense(Orbit)/Void Beam/
  Bone Spear/Cleaveと、それらが使う共有のProjectileMovementSystem/ProjectileCollisionSystem/
  HomingMissileSteeringSystem、計11システム)もFlicker Strike中に止まってしまっていた。
  これら11システムのガードを`IsPlayerActionLocked()`→`IsPlayerUltimateActive()`のみに
  変更し、必殺技演出中のみ止まる(Flicker Strike中は自動発動を継続する)ようにした。
  手動発動武器(SingleShot/AreaAttack/AreaAttackHazard)・`PlayerInputSystem`・
  `GameStateSystem`は`IsPlayerActionLocked()`のまま変更なし(Flicker Strike中も引き続き
  ブロックされ、手動攻撃の入力自体は`FlickerStrikeWeaponSystem::UpdateActiveSequence`の
  `IsOtherManualSkillPressed`検知でシーケンスを打ち切る、という既存の仕様どおり)。
  `PlayerActionLock.h`のコメントも、どのSystemがどちらの判定関数を使うべきかを明記する形に
  更新。db.db変更なし(純粋なC++ロジック変更)。Debug/Release両方ビルド・起動確認済み。

- **ステータス強化項目の追加・Flicker Strikeエフェクト変更・新規敵タイプ追加**
  （ユーザー指示、3件まとめて対応。4件目の武器CSV/レベル制ID方式変更は設計確認待ち）。
  1. `StatusUpgradeScene`の強化項目に「移動速度」を追加(MaxHp/AtkPower/Defense/
     CooldownRateの4種のうち、`PlayerStatusComponent`が持つ他のBase値と違い唯一
     未接続だった`MoveSpeed`を追加)。`eStatUpgradeType::MoveSpeed`(=4)、
     `PlayerSaveData::MoveSpeedLevel`、`StatUpgradeData`のId=4行を追加。
     `StatusUpgradeComponent::kOptionCount`を4→5に変更(画面レイアウトは
     kOptionCountから動的計算されるため自動で追従)。
  2. Flicker Strikeのヒットエフェクトを雷風に変更(`AttackHit.efk`→
     `LightningStrike.efk;Light4.efk`の組み合わせ)、`HitEffectScale`を4→7に拡大
     (派手さの要望)。
  3. 新しい敵の種類「Scout」を追加(`data::EnemyData`のId=1、MaxHp=6・MoveSpeed=55と
     素早く脆い性能、水色系のCustomColorで視覚的に区別)。これに伴い、これまで
     `EnemyData.csv`が定義だけされ実際には使われていなかった(`GameSceneFactory::
     CreateEnemy`が常に構造体デフォルト値を使っていた)接続漏れを解消：CreateEnemyへ
     `enemyId`引数を追加し、実際に`data::EnemyData`からMaxHp/AtkPower/Exp/GoldValue/
     MoveSpeedを読んで反映するようにした(`EnemyChaseComponent::MoveSpeed`も含む)。
     `EnemyData`へ`GoldValue`列を新設(`EnemyBaseStatus::GoldValue`も同様に接続)。
     `EnemySpawnSystem`は通常スポーン時に登録済みの敵タイプから一様ランダムに1つ選ぶ
     `PickRandomEnemyId()`を新設(ボースは常にId=0の強化版で固定)。
     `EnemyStatusDebugPanel::ApplyRowToEnemies`にもExperienceValue/GoldValueの反映を追加。
  - db.db同期：StatUpgradeData/FlickerStrikeWeaponDataは値変更のみでSaveCsvToDbのみ、
    EnemyDataは列追加のためDropTable→再同期。Debug/Release両方ビルド・起動確認済み
    (SpawnCountPerTick=20・ランダム種類選択込みで10秒間クラッシュなし確認)。

- **武器マスタデータのID/レベル制方式への全面リファクタリング**（ユーザー指示: 「武器のCSVなどの
  形式を1001みたいな感じでIDの１、２桁目で武器種類。下２(３，４)桁で武器のレベルごとの
  変更されたステータスって感じにしてレベルアップの時に外部データから取得」。設計確認の結果、
  対象範囲=全武器(11種)を一気に変更、既存Base/PerLevel列=削除して行ベースの値のみに一本化、
  の2点をユーザーが選択)。
  全11種の武器(SingleShot/AreaAttack/Orbit/Nova/HomingMissile/ChainLightning/Meteor/
  VoidBeam/BoneSpear/Cleave/FlickerStrike)について、以下を一律で変更した。
  - `data::XxxWeaponData::Id`の意味を「武器ID(WeaponComponent::WeaponIDそのもの)」から
    「(WeaponID+1)*1000+Level」に変更。現状全武器がWeaponID=0のみのため、実質Id=1001〜1005
    (`WeaponComponent::MaxLevel`が現状全武器共通で5固定、`GameSceneFactory::AddWeaponToPlayer`
    参照)がLv1〜5に対応する。
  - 各構造体の`BaseDamage`/`DamagePerLevel`(7武器はさらに`BaseXxxRadius`/
    `XxxRadiusPerLevel`も)を廃止し、`Damage`(・`Radius`/`ExplosionRadius`)という
    単一フィールドに統合。CSVは1武器につき5行(Lv1〜5)を持ち、各行の値は旧来の
    `Base + PerLevel * (Level-1)`計算式が各レベルで出していた値と完全一致するよう
    算出済み(既存のゲームバランスを変えていない)。
  - 各`XxxWeaponSystem.cpp`の`GetById(weapon.WeaponID)` →
    `GetById((weapon.WeaponID + 1) * 1000 + weapon.Level)`に変更し、`levelIndex`を
    使った実行時計算コードを削除、`masterData.Damage`/`masterData.Radius`等を
    そのまま使うように変更。
  - db.db同期：11テーブル全てが列構成変更のため`DropTable`→`SaveCsvToDb`で再同期。
  - Debug/Release両方ビルド・起動確認済み(12秒間クラッシュなし、db.db内に新スキーマ・
    新IDの行が反映されていることを確認)。

- **全武器の10レベル化・パーク選択に攻撃力/防御力/攻撃回数を追加**（ユーザー指示3件）。
  1. 全11種の武器CSVをLv6〜10まで拡張(各5行→10行)。値は既存の成長式の延長線上で算出済み
     (既存Lv1〜5のバランスは変更なし)。`WeaponComponent::MaxLevel`を5→10に変更
     (`GameSceneFactory::AddWeaponToPlayer`、全武器共通)。
  2. パーク選択プールに攻撃力(`AtkPowerUp`、+10%)・防御力(`DefenseUp`、+15%)を追加。
     `PlayerStatusComponent::Modifier.MulAtkPower`/`MulDefense`は既に各武器のダメージ計算・
     被ダメージ軽減式に接続済みだったため接続作業は不要、プール追加のみで機能する。
  3. 「1回の発動で出る攻撃を2倍にする」パーク(`AttackCountUp`、+100%)を新設。
     `PlayerStatusComponent::Modifier.MulAttackCount`(基準1.0倍)を追加し、
     `ecs::combatutil::GetAttackCount()`で整数丸め・上限クランプ(最大6)した回数を取得できる
     ようにした。発動が明確な9種の武器(SingleShot/AreaAttack/Nova/HomingMissile/
     ChainLightning/Meteor/VoidBeam/BoneSpear/Cleave)のFire/Pulse/Zap/Swing呼び出しを
     この回数分ループするよう変更。単方向弾を撃つ4種(SingleShot/HomingMissile/BoneSpear/
     VoidBeam)は完全に重ならないよう`ecs::combatutil::ComputeSpreadDirection()`で扇状に
     角度をずらして発射する。常時周回するOrbit(発動という概念が無い持続武器)と、
     パワーチャージ経済を持つFlicker Strike(ChargeHitCountが既に同種の役割を担う)は対象外。
  - db.db同期：武器11テーブルは列構成変更なし(行数のみ増加)のためDropTable不要、
    SaveCsvToDbのみで反映(SaveAll内のDELETE→再INSERTで対応)。Debug/Release両方
    ビルド・起動確認済み(15秒間クラッシュなし)。

- **Flicker Strikeが初撃から連鎖しなくなるバグを修正、パーク選択に最大レベル(CSVデータ化)を追加**
  （ユーザー報告「フリックストライクが正常に動作しなくなった。１回目から連鎖しない」+指示2件）。
  1. **バグ修正**: 前回の武器CSV Id方式変更(Id=(WeaponID+1)*1000+Level化)で、
     `data::FlickerStrikeWeaponData`の`Id=0`の行が存在しなくなったにも関わらず、
     `PlayerPowerChargeComponent.h::ComputeMaxPowerCharge`と`GameSceneFactory::CreatePlayer`の
     2箇所が旧方式のまま`GetById(0)`を呼び続けていたため、常にnullptrが返り
     「パワーチャージ上限が常に0」「ゲーム開始時の所持チャージが0のまま」になっていた
     (Flicker Strikeの追加ワープ回数=chargeCount×ChargeHitCountが常に0になり、
     初撃はワープするが連鎖しない、という報告内容と一致)。
     `FlickerStrikeWeaponData.h`に`kFlickerStrikeGlobalConfigId=1001`(Lv1行。MaxCharge/
     MaxChargePerLevel/InitialChargeはレベル非依存の値のため常にLv1行を参照する)を追加し、
     両箇所をこの定数経由に修正。
  2. **Flicker Strikeは攻撃回数パークを反映しない設計を明文化**: 元々`GetAttackCount()`を
     呼んでいなかった(Orbit同様に対象外)ため機能追加は無いが、`FlickerStrikeWeaponSystem::Update`
     冒頭に「ChargeHitCountと二重増幅するため意図的に対象外」というコメントを追加し、
     将来の実装ミスを防止。
  3. **パーク選択の最大レベル制**: 新規`data::PerkData`(CSV/DB、`Id`=`ePerkEffectType`の値、
     `MaxLevel`列)を追加。`PlayerPerkLevelComponent`(プレイヤーへ付与、`GetPerkPool()`の
     インデックスごとの選択回数を記録)と`ecs::GetPerkMaxLevel()`を新設し、
     `PerkSelectSystem::EnterPerkSelect`で種別ごとの選択回数がMaxLevelに達した候補を
     プールから除外するようにした。MaxLevelはPerkData.csvで管理: MaxHpUp=10,
     CooldownDown=5, MoveSpeedUp=5, AtkPowerUp=10, DefenseUp=10, **AttackCountUp=1**
     (ユーザー指示通り、同時攻撃数+100%は1回のみ選択可能=最大2倍までに制限)、
     WeaponLevelUp/AcquireWeapon=99(既存の別系統判定で実質上限管理済みのため無制限扱い)。
  - db.db同期：PerkDataは新規テーブルのためSaveCsvToDbで初回投入(EnsureTableだけでは
    空のままのため)。武器11テーブルは今回スキーマ変更なし。Debug/Release両方ビルド・
    起動確認済み(15秒間クラッシュなし)。

- **ゴールド強化項目「ゴールド獲得量」の追加、パーク選択に「HP回復」「経験値獲得量アップ」を追加**
  （ユーザー指示「さらにコインでの強化項目の追加とパーク選択項目の追加」）。
  1. `eStatUpgradeType::GoldGainRate`(=5)を追加。`StatUpgradeData.csv`に
     `ゴールド獲得量,BaseCost=100,CostGrowthPerLevel=50,ValuePerLevel=0.1,MaxLevel=10`を追加
     (最大+100%=獲得ゴールド2倍)。`PlayerSaveData::GoldGainRateLevel`を追加し、
     `EnemyDeathSystem::AwardGold`が撃破ゴールド合計へ
     `1.0 + ValuePerLevel×レベル`の倍率をかけてから加算するよう変更(戻り値をintからfloatへ、
     丸めを乗算後に一度だけ行うよう修正)。`StatusUpgradeComponent::kOptionCount`を5→6、
     `StatusUpgradeInputSystem`のラベル・GetLevel/IncrementLevelスイッチに追加。
  2. パーク選択プールに2件追加(既存Id0-7を変えないよう`ePerkEffectType`末尾に追加)。
     - `HealHp`(HP回復30%、即時効果でModifierは変更しない。WeaponLevelUpと同じ「一回性」の
       扱いのためMaxLevel=99=実質無制限)。
     - `ExperienceGainUp`(経験値獲得量+15%)。`PlayerLevelComponent::MulExperienceGain`を
       新設し、`EnemyDeathSystem::AwardExperience`で経験値加算時に乗算するようにした
       (MaxLevel=10、他のModifier系パークと同じ上限)。
  - db.db同期：StatUpgradeData/PerkDataは既存テーブルへの行追加のみ(DropTable不要)。
    PlayerSaveData(JSON)は新フィールドがデフォルト値(0)で自動的に補完される
    (MoveSpeedLevel追加時と同じ挙動)。Debug/Release両方ビルド・起動確認済み
    (15秒間クラッシュなし)。

- **ゴールド強化2項目・敵2種類の追加**（ユーザー指示「さらに項目を増やそう。敵の種類の追加もしよう」）。
  1. **HP自然回復**(`eStatUpgradeType::HpRegen`=6、+0.5/秒/レベル、最大レベル10で+5/秒)。
     `PlayerStatusComponent::Base/Current.HpRegenPerSecond`を新設し、`ApplyStatUpgrades`で
     加算。新規`ecs::PlayerRegenSystem`(毎フレームCurrentHpをMaxHpまで回復)を追加し、
     `PlayerContactDamageSystem`の直後に登録。
  2. **経験値獲得量**(`eStatUpgradeType::ExperienceGainRate`=7、永続、+10%/レベル)。
     `PlayerSaveData::ExperienceGainRateLevel`を追加し、`EnemyDeathSystem::AwardExperience`が
     この永続倍率と`ExperienceGainUp`パーク(`PlayerLevelComponent::MulExperienceGain`、今回の
     プレイのみ)の両方を乗算するようにした(GoldGainRateと同じ「PlayerStatusComponentには
     接続しない・獲得時に直接倍率をかける」方式)。
  - `StatusUpgradeComponent::kOptionCount`を6→8、UI(`StatusUpgradeInputSystem`)のラベル・
    GetLevel/IncrementLevelスイッチに追加。
  3. **新規敵タイプ2種**を追加(`data::EnemyData`)。
     - Id=2「Brute」: MaxHp=25・MoveSpeed=25・AtkPower=3(低速・高HP・高火力のタンク役)。
     - Id=3「Sprinter」: MaxHp=4・MoveSpeed=65・AtkPower=1(超高速・超低HPの特攻役。
       プレイヤーの実移動速度上限75より遅く調整済み)。
     `GameSceneFactory::CreateEnemy`の色分けロジックを、Id=1のみ対応する三項演算子から
     4種類(Grunt/Scout/Brute/Sprinter)+ボースを扱うswitch文へ一般化。
     `EnemySpawnSystem::PickRandomEnemyId`は登録済み全敵タイプから一様ランダムに選ぶ既存実装
     のため、変更なしで4種類ランダムスポーンに対応。
  - 新規ファイル`PlayerRegenSystem.h/.cpp`が`App.vcxproj`/`App.vcxproj.filters`の
    ClCompile/ClIncludeに未登録でリンクエラーになったため追加(前回追加した
    `PerkData.h`/`PlayerPerkLevelComponent.h`もヘッダーのみのため未登録だったが、
    こちらはビルドは通っていたので同様にIDE整理のため合わせて登録した)。
  - db.db同期：StatUpgradeData/EnemyDataは既存テーブルへの行追加のみ(DropTable不要)。
    Debug/Release両方ビルド・起動確認済み(20秒間クラッシュなし)。

- **システム監査に基づくエンジン側の修正3件**（ユーザー指示「処理効率やfbxのクオリティアップとか。
  修正するべき個所などを洗ってみて」→ forkによる監査後、提示した優先度上位3件を実施）。
  1. `FbxPipeline.cpp`: メインパスのラスタライザが`D3D12_CULL_MODE_NONE`(カリング無効)に
     なっていた。`ShadowPipeline.cpp`は同じ根拠で既に`D3D12_CULL_MODE_BACK`を使っており、
     整合性が無かった(意図的な両面描画の根拠となるマテリアル側の両面フラグも存在しない)。
     `BACK`に変更し、全キャラクター/敵の描画で頂点・ピクセル処理を約半減。
     ※DX12フリップモデルスワップチェーンの制約でPrintWindowによる3D描画内容の
     スクリーンショット検証ができなかったため、ユーザーに目視確認を依頼済み。
  2. `FbxRenderer::UpdateAndDraw`が毎フレーム`std::vector<RenderItem> items`をローカル
     生成していた(CLAUDE.md明記の「毎フレームのvector生成禁止」に抵触)。`RenderItem`を
     `DrawCall`と同じ形でFbxRenderer.hへ private nested struct として移動し、
     `mRenderItems`メンバ変数(毎フレーム`clear()`して再利用)へ昇格。
  3. `Assets/Fbx/Field/Texture/T_StoneTiles_02_Normal.png`が存在せず、起動毎に
     `Texture: File not found`警告が出続けていた。`Field.fbx.bin.txt`で確認したところ
     元のFBXマテリアル定義自体がこのファイルを参照しており(パスの問題ではなく実体が
     欠落)、PowerShell+System.Drawingでフラットな法線マップ(8x8、RGB(128,128,255)=
     タンジェント空間で真上を指す平坦法線)を新規生成して配置。警告は解消し、
     見た目は既存のデフォルト法線フォールバックと同等(退行なし)。
  - Debug/Release両方ビルド・起動確認済み(15秒間クラッシュなし、警告メッセージ解消)。

- **CullMode=BACK変更を差し戻し(ユーザー報告: マテリアルが消える・地面が見えない)**。
  前回のFbxPipeline.cpp修正(NONE→BACK)により、Field(地面)モデルの頂点巻き順が
  D3D12既定(時計回り=表面)と逆だったため、地面メッシュ全体がカリングされて非表示に
  なっていた。原因調査の結果に基づき`D3D12_CULL_MODE_NONE`へ差し戻し、事情をコード
  コメントに明記(FrontCounterClockwise=TRUEを試す、メッシュ単位でカリング方向を
  切り替え可能にする、等が今後の対応候補)。Debug/Release両方ビルド・起動確認済み。
  他の2件(FbxRendererのvectorメンバ化、StoneTiles法線マップ生成)はレンダリングの
  可視性に影響しない変更のため維持。

- **必殺技のカメラワークを「正面下から見上げ」→「背後上空から見下ろし」構図へ変更**
  （ユーザー指示「必殺技のカメラワークなんだけどプレイヤーの後ろから下を見る感じ」）。
  `PlayerUltimateSystem::UpdateCamera`のカメラ位置計算を`start + forward*CameraDistance`
  (正面方向)から`start - forward*CameraDistance`(背後方向)へ変更。`UltimateData`の
  `CameraDistance`を50→80、`CameraHeight`を15→500(上昇距離RiseHeight=450より高くし、
  上昇中ずっと見下ろす構図を維持する)、`CameraLookOffset`を40→20(見下ろす構図のため
  「顔の高さを狙う」必要が無くなった分を縮小)に変更。db.db同期(値変更のみ、
  DropTable不要)。Debug/Release両方ビルド・起動確認済み(クラッシュなし)。
  ※カメラワークの見た目(実際のフレーミング)はDX12描画内容のスクリーンショット検証が
  できないため未確認。ユーザーに実機での目視確認を依頼。

- **必殺技のpreビーム方向を地面向きに変更、カメラをさらに高く**（ユーザー指示・フィードバック）。
  1. preビーム(hougu_pre.efk)の先端がカメラ方向を向く仕様だったのを、常に真下(地面方向)を
     向くよう固定方向化(`ComputeBeamRotationFromDirection({0,-1,0})`)。カメラ座標を都度
     参照する必要が無くなったため関連コードを削除し、今回不要になった`NormalizeOrZero`
     ヘルパーも削除(死コード化を避けるため)。
     `UltimateData::BeamCameraOffset`(カメラ方向へずらす量)は用途が変わったため
     `BeamDownOffset`(真下へずらす量)へリネーム(列名変更のためdb.dbはDropTable→再同期)。
  2. カメラワークの見た目について「めっちゃいい、もっと高く」とフィードバックを受け、
     `UltimateData::CameraHeight`を500→900へさらに引き上げ(より見下ろす角度を強調)。
  - Debug/Release両方ビルド・db.db同期・起動確認済み(クラッシュなし)。

- **必殺技: preビーム終了後にmainエフェクトを再生してからその終了後に座標復元するよう修正、
  カメラ高さを700へ調整**（ユーザー指示）。
  `eUltimatePhase`に`PlayingMain`を新設(Ascending→PlayingBeam→PlayingMain→座標復元/
  ダメージの4段階に変更)。`PlayerUltimateComponent::BeamElapsedTime`を`PhaseElapsedTime`に
  リネームし、PlayingBeam/PlayingMainの両フェーズで使い回す(フェーズ切替時に0リセット)。
  `MainEffectEntities`を新設しメインエフェクトの再生終了監視に使用。
  `UltimateData::MaxMainDuration`を新設(メイン再生の安全装置タイムアウト、MaxBeamDurationと
  同じ役割)。メイン(hougu_main、ActivationEffectPath)は、ビーム終了時点のプレイヤー座標
  (まだ上空、地面へ戻す前)で再生するよう変更し、`FinishAndExplode`からエフェクト再生と
  地面めり込み対策の`ActivationHeightOffset`ロジックを削除(既にPlayingMain遷移時に
  同じフィールドを流用)。`CameraHeight`を900→700に調整。
  db.db同期(MaxMainDuration列追加のためDropTable→再同期)。Debug/Release両方ビルド・
  起動確認済み(クラッシュなし)。

- **必殺技中に敵が空まで追ってくるバグを修正**（ユーザー報告「敵の追従はY軸以外で行ってくれ」）。
  `EnemyChaseSystem`の方向計算自体は元々Y成分を無視していたが、間合い内で静止中の敵は
  `MoveVelocity`/`HasMoveRequest`を一切更新しないままだったため、必殺技で上昇するプレイヤーの
  物理ボディと接触して押し上げられた際の残留Y速度がクリアされず(敵は`GravityFactor=0`のため
  自然には落下しない)、そのまま浮遊し続けて「空まで追いかけてくる」ように見えていた。
  追従中・間合い内で静止中の両方の分岐で、毎フレーム明示的に`MoveVelocity={0,0,0}`+
  `HasMoveRequest=true`を設定してY速度を含めリセットするよう修正。
  Debug/Release両方ビルド・起動確認済み(クラッシュなし、db.db変更なし)。

- **敵の湧き出しY座標がプレイヤーの現在座標に依存していたバグを修正、必殺技の自爆撃破で
  ゲージが貯まる不具合を修正**（ユーザー指摘・報告2件）。
  1. `EnemySpawnSystem::ComputeSpawnPosition`が敵の湧き出しY座標に`playerPos.y`(プレイヤーの
     現在のY座標)をそのまま使っていたため、必殺技で上昇中に新しく湧いた敵がプレイヤーと
     同じ高さ(=空中)に出現していた。これが「敵が空まで追いかけてくる」ように見えていた
     主因(前回のEnemyChaseSystemのY速度リセット修正は別途正しい改善だが、主因はこちら)。
     地面は平面である前提の固定値`kSpawnGroundY`(=0.1、`GameSceneFactory::CreatePlayer`の
     初期Y座標と合わせた値)を使うよう修正。
  2. 必殺技の範囲ダメージによる撃破が、必殺技ゲージ(`PlayerUltimateComponent::KillCount`)へ
     加算されてしまっていた。`AwardUltimateCharge`には元々「発動中は加算しない」ガードが
     あったが、`FinishAndExplode`内でダメージ適用直後に`ultimate.IsActive=false`にしてしまう
     ため、実際の撃破判定(`EnemyDeathSystem::Update`)が反映されるのは次フレームで、
     その時点では既に`IsActive=false`になっておりガードが効かなかった。
     `EnemyStatusComponent::DamagedByUltimate`フラグを新設し、`FinishAndExplode`で
     大ダメージを与える際にtrueへ設定、`EnemyDeathSystem::Update`がこのフラグを見て
     必殺技ゲージへの加算対象から除外するよう修正(ゴールド・経験値・パワーチャージは
     従来どおり加算される)。
  - Debug/Release両方ビルド・起動確認済み(クラッシュなし、db.db変更なし)。
