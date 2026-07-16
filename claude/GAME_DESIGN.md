# GAME_DESIGN

`App/`で開発するゲームの概要・仕様。コードから自明でない設計意図をまとめる。
実装の詳細(ファイル配置等)は `STRUCTURE.md` を参照。

最終更新: 2026-07-15 (ユーザーへのヒアリングに基づく初版)

## タイトル
未定。

## ジャンル
3Dウェーブサバイバル型アクション（Vampire Survivors系のコアループを3Dで再構成したもの）。

## 世界観・トーン
Path of Exile 2 のようなダークファンタジー。陰鬱で重厚なトーンを目指す。
現在の地面(`Field.fbx`)・プレイヤーモデル(`Faul.fbx`)・敵モデルは全て仮素材であり、
世界観に合わせて後日差し替える前提（今のグラフィック改善作業は仮素材のクオリティ向上目的であり、
最終的なアートディレクションを決めるものではないことに注意）。

## カメラ・視点
POE2のような、非常に急な俯瞰角度のアイソメトリック視点。固定角度・回転不可を想定。

- 実際に使われる値は `GameSceneFactory::CreateCamera()` で `Offset = {0, 72, -38}` に上書きされており、
  既にかなり急な俯瞰角度になっている（`CameraFollowOffsetComponent`のデフォルト値`{0,5,-10}`は未使用）。
  POE2相当のアイソメトリック視点として十分か、実機で見て判断する必要がある（未確認）。

## コアループ
1. 敵が時間経過とともに継続的に湧き、時間経過に応じて敵の強さが連続的に上昇していく
   （明確な「ウェーブ1/ウェーブ2…」の区切りではなく、Survivors系のような滑らかな難易度カーブ）。
   → 実装済み(`EnemySpawnSystem`)。実機動作確認は未実施。
2. 一定時間経過でボスが出現する。 → 実装済み(90秒)。実機動作確認は未実施。
3. プレイヤーは遠距離武器・魔法を中心に敵を殲滅しながら生き延びる。
   → 初回武器FireBoltは実装・実機確認済み（左クリック発射のManual制御）。
4. 敵を倒す・時間経過等のトリガーでパーク(強化)を選択できる機会が挟まる。 → 未実装(PerkSelect UI)。
5. 制限時間を生き残ればクリア。 → クリア判定(180秒)・Result画面表示とも実装済み(`ResultSystem`)。

## 勝敗条件
- クリア: 制限時間まで生存する。
- ゲームオーバー: プレイヤーHPが0になる
  (既存実装 `PlayerContactDamageSystem` が `CurrentHp <= 0` で `GameOverRequested` をセットする)。

## プレイヤー
- キャラクター名: Faul（アセット名。仮素材）。
- 移動: WASD/スティックでXZ平面移動 (`PlayerMovementSystem`)。
- 照準: マウス座標 or 右スティックで方向決定 (`PlayerAimSystem`)。マウス/パッド両対応。
- ステータス: HP・移動速度・攻撃力・防御力・クールダウン倍率をBase/Modifier/Currentの3層で保持
  (`PlayerStatusComponent`)。Modifier層はパークによる乗算強化を前提にした構造。

## 武器・パークシステム
- 攻撃方向性: 遠距離主体。自動発射される弾・魔法で敵を囲むように殲滅する
  (Vampire Survivors型のオートアタック中心。Attack_A〜Dの近接アニメーションは現状未使用の想定)。
- 武器インベントリ: 最大6スロット、SingleShot/SelfDefense/AreaAttackの種別、
  Manual/Autoの制御方式が既存設計にある (`WeaponInventoryComponent`)。
- パーク: ステータス強化に加えて新しい武器・魔法の獲得も含む
  (Vampire Survivors型。既存の`WeaponInventoryComponent`の複数スロット設計と合致する)。
  パークによるレベルアップの導線(PerkSelect UI)はまだ無いため、現状武器は常にLv1で発射される。
- 初回武器「FireBolt」(SingleShot, WeaponID=0)を実装済み。左クリック("Attack"アクション)でプレイヤーの
  照準方向(`PlayerAimComponent`)へ発射する(Manual制御)。命中した敵の周囲に球形範囲爆発を発生させる
  （当たり判定は敵のみ、ノックバックなし、HP減少のみ）。何にも当たらなかった弾はProjectileLifeTime
  (デフォルト2秒、CSVで変数化)経過で自然消滅する。ダメージ・爆発半径はレベルに応じて線形成長する
  (`data::SingleShotWeaponData` = `Assets/Data/Weapon/SingleShotWeaponData.csv`)。
  新しいSingleShot武器はCSV行を追加するだけで増やせる。
  飛翔体は`ProjectileComponent`ベースの汎用パイプラインで動いており、将来の他の遠距離武器
  （種別問わず）でも同じ仕組みを再利用できる設計にしてある。
- 2種類目の武器「IceSpike」(AreaAttack, WeaponID=0)を実装済み。FireBoltと同じ初期武器のため
  右クリック("Attack2"アクション)で発動するManual制御。プレイヤーの狙い方向
  (`PlayerAimComponent::Direction`＝マウス座標/右スティック、FireBoltと同じ基準)へ`ForwardOffset`
  離れた地点を中心に`SearchRadius`内から敵を最大`MaxTargets`体まで自動検出し、各敵の座標へ
  個別に氷柱(ハザード)を落とす(`AreaAttackHazardComponent`/`AreaAttackHazardSystem`)。各氷柱は
  `Duration`(5秒)持続し`TickInterval`(0.5秒)ごとにダメージを反復する。ダメージ・判定半径は
  レベルに応じて線形成長する(`data::AreaAttackWeaponData` = `Assets/Data/Weapon/AreaAttackWeaponData.csv`)。
  専用のエフェクトアセットが無いため`LightningStrike.efk`を仮で流用（後日差し替え前提）。
  プレイヤーはFireBoltと併せて初期装備する。
- 3種類目の武器「FrostOrb」(SelfDefense、WeaponID=0)を実装済み。発動トリガーが無く、装備した
  瞬間からOrbCount体のオーブがプレイヤーを中心にOrbitRadius(m)の円周上をOrbitSpeed(度/秒)で
  周回し続ける常時稼働の持続武器。各オーブはHitRadius(m)の当たり判定を持ち、敵と接触している
  間はHitInterval(秒)ごとにダメージを反復する(`OrbitOrbComponent`/`OrbitWeaponSystem`新規)。
  ダメージはレベルに応じて線形成長する(`data::OrbitWeaponData` = `Assets/Data/Weapon/OrbitWeaponData.csv`)。
  オーブ自体は常時ループエフェクトで表示し、命中時は別途ワンショットのエフェクトを再生する。
  消滅しない持続武器が敵と密着し続けても再ダメージできるよう、CoreEngineへ
  `SensorStayEvent`(既存の`CollisionStayEvent`と対になる、密着中は毎フレーム発行されるセンサー
  イベント)を新規追加した。プレイヤーはFireBolt/IceSpikeと併せて初期装備する。
- 当たり判定の可視化: `DebugWireSphereComponent`(新規、CoreEngine)を持つエンティティは
  `PhysicsDebugRenderer`(ImGui「Physics Debug」→「Show Colliders」)がワイヤーフレーム球で
  描画する。`OverlapSphere`ベースの当たり判定(FireBoltの爆発、IceSpikeの氷柱)は登録済み
  Jolt Bodyではないため、これが無いと実際の判定半径を目視確認できなかった。あわせて
  Effekseerの見た目のサイズも判定半径に応じて概算スケール(Scale)している
  （素材が概ね半径2m相当で作られている前提の暫定計算のため、正確な一致ではない）。
- パーク選択UIは `GameState::PerkSelect` ステートとして用意されており、実装済み(`PerkSelectSystem`新規)。
  実機動作確認は未実施。キル数ベースでレベルアップ(`PlayerLevelComponent`新規、`EnemyDeathSystem`が
  撃破時にXP付与・閾値到達判定)し、PerkSelectへ入ると3択(最大HP+15%/攻撃間隔-10%/移動速度+10%/
  武器レベルアップ、から重複無しで3件抽選)をテキストUIで提示、MenuLeft/MenuRightで選択・Selectで確定。
  GameStateSystemはステート遷移のみを担当し、UI生成・入力・効果適用は全てPerkSelectSystemに閉じる設計
  (疎結合)。プレイヤーのAtkPower/Defenseステータスは武器ダメージ計算・被ダメージ軽減にまだ未接続のため
  (Defenseは初期値0で乗算バフが効かない問題もある)、それらを対象にしたパークは意図的に含めていない。
- パーク経由でのみ取得できる武器の仕組みを追加。`ePerkEffectType::AcquireWeapon`(`PerkDefinition`に
  `AcquireWeaponType`/`AcquireWeaponId`を追加)で、未所持かつ空きスロットがある場合のみ候補に出る。
  初期武器3種・パーク武器共通で`GameSceneFactory::AddWeaponToPlayer()`/`RemoveWeaponFromPlayer()`
  を経由するよう統一（重複していた初期武器生成コードもこれを使うようリファクタリング）。
  1つ目のパーク武器「Nova」を実装済み(`eWeaponType::Nova`、`data::NovaWeaponData`、
  `NovaWeaponRuntimeComponent`、`NovaWeaponSystem`新規)。発動トリガーが無く、PulseInterval秒
  ごとにプレイヤー自身を中心とした円形範囲へ即座にダメージを与える完全自動の持続武器
  （`PhysicsSystem::OverlapSphere`を再利用、他のWeaponDataと同じBase+PerLevel成長・
  HitRadiusMultiplierによる判定/見た目分離のパターンを踏襲）。
  今後Chain Lightning・Homing Missileも同じ仕組みで追加予定。
  デバッグ用に`WeaponInventoryDebugPanel`(新規)を追加し、パークを介さず任意の武器を
  ImGuiから追加・削除できるようにした(`AddWeaponToPlayer`/`RemoveWeaponFromPlayer`を直接呼ぶ)。
- 2つ目のパーク武器「Homing Missile」を実装済み(`eWeaponType::Homing`、
  `data::HomingMissileWeaponData`、`HomingMissileRuntimeComponent`、`HomingMissileWeaponSystem`/
  `HomingMissileSteeringSystem`新規)。狙い不要の完全自動発動で、SearchRadius内に敵がいる
  場合のみFireInterval間隔で最も近い敵へ追尾弾を発射する(いなければクールダウンを消費せず待機)。
  発射・命中判定は既存のProjectile汎用パイプライン(`ProjectileComponent`/
  `ProjectileMovementSystem`/`ProjectileCollisionSystem`)をそのまま再利用し、
  `ProjectileComponent`に`IsHoming`/`Target`/`TurnSpeed`/`HomingSearchRadius`を追加しただけで
  対応（既存のFireBolt等、通常弾には一切影響しない）。`HomingMissileSteeringSystem`が毎フレーム
  対象方向へTurnSpeedの範囲内でDirectionを回転させ、対象を見失った場合はHomingSearchRadius内で
  最も近い敵に自動で切り替える。エフェクトは専用素材が無いため`PhantasmMeteor.efk`(飛翔)/
  `Skill1Hit.efk`(着弾)を仮で流用（後日差し替え前提。第三者サイトからの素材ダウンロードは
  ライセンス・互換性を確認できないため見送り、既存の仮流用パターンを踏襲した）。
  db.dbへ`homing_missile_weapons`新規テーブルを同期済み。Debug/Release両方ビルド成功、
  実機起動でクラッシュ無し確認済み(実際のプレイでの追尾挙動確認は未実施)。
  実機確認後、ユーザー指示によりバランス調整: 弾速35→55・旋回性能180→320度/秒に向上、
  火力を下げる代わりに発射間隔1.5→0.6秒に短縮(BaseDamage 8→6、DamagePerLevel 2→1.5)。
  序盤の敵(EnemyData.MaxHp=10)を2発で倒せる想定の数値（Defenseは未接続のため単純に
  BaseDamage×2>MaxHpで計算）。db.db同期済み。
- 3つ目のパーク武器「Chain Lightning」を実装済み(`eWeaponType::Chain`、
  `data::ChainLightningWeaponData`、`ChainLightningRuntimeComponent`、
  `ChainLightningWeaponSystem`新規)。狙い不要の完全自動発動で、SearchRadius内に敵がいる
  場合のみFireInterval間隔で最も近い敵へ雷撃(瞬時に命中、移動する実体は持たない)。
  命中した敵からJumpRadius内の未命中の敵へ最大MaxJumps回まで自動で跳ね移り、跳ねるたびに
  ダメージへDamageFalloffPerJumpを乗算して減衰させる。対象探索は`AreaAttackWeaponSystem`と
  同じ`PhysicsSystem::OverlapSphere`+`EnemyTag`フィルタのパターンを踏襲。エフェクトは
  専用素材が無いため`AttackHit.efk`(命中のたびに1回再生)を仮で流用。
  db.dbへ`chain_lightning_weapons`新規テーブルを同期済み。Debug/Release両方ビルド成功、
  実機起動でクラッシュ無し確認済み(実際のプレイでの連鎖挙動確認は未実施)。
  これで初期武器3種+パーク武器3種(Nova/Homing Missile/Chain Lightning)が揃い、
  当面計画していたパーク武器の実装は一区切り。
  ユーザー指示によりChain Lightningのヒットエフェクト(`AttackHit.efk`)を4倍に拡大。
  マジックナンバー化を避けるため`ChainLightningWeaponData`へ`HitEffectScale`列(デフォルト4.0)を
  追加(スキーマ変更のためdb.dbのテーブルをDROPして再同期)。ユーザーからは「代用素材のため
  拡大してもわかりにくいかも」との懸念も出ている。

## 必殺技(Ultimate)
FGO(Fate/Grand Order)の宝具演出をイメージした、プレイヤー専用の必殺技を実装済み。
何度かユーザーフィードバックを受けて演出フローを刷新し、現在は**時間ではなく状態(位置到達・
エフェクト再生終了)で遷移する設計**になっている(以前の時間ベース設計は着地位置のズレで
地面にめり込むバグがあったため)。
- 発動条件: `UltimateData.RequiredKillCount`(CSV/DB、デフォルト15体)分の敵を倒すとゲージ満タン。
  満タンの間はプレイヤーに`AuraEffectPath`のオーラ(ループ)エフェクトを纏わせる
  (`PlayerUltimateSystem::StartAura`。プレイヤーをParentにした子エンティティとして生成し
  自動追従させ、`AuraScale`でプレイヤーのTransform.Scale=0.2分を補正した見た目倍率にしている)。
- 操作: 新規アクション"Ultimate"(Qキー / PadR1、`InputManager`に追加)。
- 発動演出は`ecs::eUltimatePhase`(Ascending/PlayingBeam)で管理する状態遷移で、以下の流れになる
  (`PlayerUltimateSystem`)。
  1. 発動した瞬間のプレイヤー座標(`StartPosition`)・正面方向(`ForwardDir`、
     `Transform::GetForward()`で捕捉)を基準に、カメラを**即座に**プレイヤー正面・低い位置
     (見上げる構図)へ固定する(`UpdateCamera`。移動は伴わず、その場でスナップする)。
  2. プレイヤーが正面方向へ`RiseSpeed`(m/s)でまっすぐ上昇する(`RigidBodyComponent::MoveVelocity`
     経由。Dynamic Bodyの位置権威は物理側にあるためTransform直接書き換えではなく
     PlayerMovementSystemと同じ速度ベースの制御方式)。発動時のY座標から`RiseHeight`(m、GUIから
     調整可能)だけ上昇した時点で自動的に上昇を止める(時間ではなく到達位置で判定するため、
     上昇量のズレが起きない)。横方向の入力も無効化され、まっすぐ上下にのみ動く。
  3. 上昇完了位置で`BeamEffectPath`のビームエフェクトを再生し、**その再生が終わるまで待つ**
     (`ecs::effectutil::AnyPlaying()`でエフェクトの再生終了を監視。`MaxBeamDuration`秒を
     超えたら、万一再生が終わらなくても強制的に次へ進む安全装置がある)。この間プレイヤーは
     上空で静止する。
  4. ビーム終了と同時に、プレイヤー座標を`StartPosition`へ**瞬時にテレポート**して戻す
     (`Transform::SetPosition()` + `TransformDirtyTag`を付与し、`PhysicsSystem::
     SyncFromTransform`でJolt側の位置も明示的に上書きする。速度ベースの降下だと着地位置が
     ズレて地面にめり込むバグがあったため、この方式に変更した)。カメラも同じフレームで
     通常の`CameraPlayerFollowSystem`制御に復帰する(`IsActive`をfalseにするだけで、次フレーム
     から自動的に通常追従へ切り替わる)。
  5. 元の座標で`Damage`(デフォルト999)をステージ上の敵全員へ即座に与え(範囲制限なし、
     `EnemyTag`を持つ全エンティティが対象。命中判定・撃破処理自体は既存の`EnemyDeathSystem`に
     そのまま任せる)、`ActivationEffectPath`の爆発エフェクトを再生する。
  - 無敵化: 全体を通して`PlayerStatusComponent::IsInvincible`でプレイヤーを無敵化する
    (`PlayerContactDamageSystem`がこの間ダメージ判定自体をスキップする)。
  - 入力無効化: 演出中(`IsActive`)は`PlayerInputSystem`がプレイヤーの移動・攻撃入力を
    全て無視するため、操作は一切できない。
  - スローモーション(`TimeScale`変更)は撤去済み(ユーザー指示。「スローにする必要ないかも」との
    フィードバックを受けて削除。副次的に、TimeScaleが物理シミュレーションの進行速度も
    遅くする仕様のため上昇量が縮む問題も同時に解消した)。カメラのズーム(Fov変更)も
    別途撤去済み(不要と判断)。
  - GUIからの調整: `debug::UltimateDebugPanel`(新規、DataInspector)でRiseHeight等を
    ImGuiからその場で編集・確認できる。
- 未実装/制約: 画面全体を覆うようなポストエフェクト的な「フラッシュ」は、既存の
  `SceneManager`/`TransitionRenderer`のフェード機構がシーン切り替え専用の状態機械と
  密結合しているため今回は見送り、カメラワーク+Effekseerエフェクトの組み合わせのみで演出している。
- エフェクトは専用素材が無いため`Light3.efk`(オーラ)/`Tornade.efk`(上昇後のビーム)/
  `Blow2.efk`(復帰後の爆発)を仮で流用。
- db.dbへ`ultimate_data`テーブルを同期済み(演出フロー刷新に伴うフィールド再構成でスキーマ変更、
  DROP TABLE後に再同期)。Debug/Release両方ビルド成功、実機起動でクラッシュ無し確認済み
  (実際のプレイでの発動確認は未実施)。

## エフェクト: 複数同時再生への対応
ユーザー指示により、1エフェクトずつしか再生できなかった仕組みを拡張し、';'区切りで複数の
エフェクトパスを指定すると全て同時に(組み合わせて)再生できるようにした。
- 新規: `ecs::effectutil::PlayOneShotCombined`(ワンショット、指定位置に';'区切りの各エフェクトを
  独立したエンティティで1回ずつ再生。生成したエンティティ一覧を任意で受け取れる)/
  `PlayLoopingCombined`(ループ、parentEntityに追従する各エフェクトを独立したエンティティで
  再生し続ける。生成エンティティは呼び出し側がparent破棄前に責任を持ってdestroyする必要がある)/
  `AnyPlaying`(渡したエンティティ群のうち、有効かつ再生中のものが1つでもあるか調べる。
  `PlayOneShotCombined`が返したエンティティ一覧と組み合わせて「複数エフェクトが全て
  再生し終わったか」を監視できる。必殺技のビーム演出終了待ちで使用)。
- 適用済み箇所: `NovaWeaponSystem::Pulse`、`ChainLightningWeaponSystem::SpawnHitEffect`、
  `ProjectileCollisionSystem::SpawnExplosionEffect`(FireBolt/Homing Missile両方の着弾エフェクトが
  対象)、`PlayerUltimateSystem`のオーラ・ビーム・爆発エフェクト。
- 意図的に対象外: 弾自体に追従し続けるループエフェクト(FireBolt/Homing Missileの飛翔中エフェクト、
  FrostOrbの周回中エフェクト)は、複数化すると弾/オーブ破棄時に子エフェクトエンティティが
  親を失って位置固定のまま残り続ける(リーク)リスクがあるため、今回は単一エフェクトのまま
  据え置いた(将来的に対応する場合は、弾/オーブの破棄経路で子エフェクトも合わせてdestroyする
  処理を追加する必要がある)。
- 各WeaponData/UltimateDataのエフェクトパス系フィールド(EffectPath/HitEffectPath/
  ExplosionEffectPath/AuraEffectPath/ActivationEffectPath)は、';'区切りで複数指定できる
  ようになった(現状はまだ全て1個ずつしか設定していない。使えるアセットが揃い次第、
  組み合わせを試すこと)。

## エフェクト素材について(重要な制約)
ユーザーが `C:\Users\kirit\Desktop\Effekseer素材` にEffekseerのサンプルエフェクト一式を
用意してくれたが、中身を確認したところ全て`.efkproj`(Effekseer **Editor**のプロジェクト
ファイル、実体はXML)であり、このゲームのランタイムが読み込める`.efk`(コンパイル済み
バイナリ、先頭4バイトが"SKFE")形式ではなかった。プロジェクト内の`CoreEngine/external/Effekseer`
(ランタイムSDKのみ、エディタは含まれない)にも`.efkproj`を読み込む機能は無く、`.efkproj`を
`.efk`へ変換するには**Effekseer Editor(GUIアプリケーション)**での書き出しが必要で、この
サンドボックス環境にはGUI操作・変換ツールが無いため対応できなかった。
- 対応方針: ユーザー側でEffekseer Editorを使い、使いたい`.efkproj`を開いて`.efk`として
  エクスポートしてもらう必要がある。エクスポート済みの`.efk`(と参照しているテクスチャ・
  モデルファイル)を渡してもらえれば、`App/Assets/Effect/`配下へフォルダ分けして配置し、
  各WeaponData/UltimateDataのエフェクトパス列を差し替える作業はすぐに対応できる。
- 上記の理由により、今回は引き続き既存の代用エフェクト(Herald.efk/PhantasmMeteor.efk/
  Skill1Hit.efk/AttackHit.efk/Light3.efk/Tornade.efk等、既にプロジェクトに`.efk`として
  存在するもの)を仮流用する形のままにしてある。

## 敵・ウェーブ・ボス
- 難易度カーブ: 時間経過に応じて敵が連続的に強く・多くなる。
- ボス: 一定時間経過で出現（具体的な出現間隔・ボスの種類・数は未定）。
- 現状の実装: `EnemyData.csv` でデータ駆動の設計になっているが、現状1体分のデータのみ登録。
  実際にスポーンする敵は専用モデルではなくプレイヤーモデルを着色した代用品。
  AIは自機へ直進するのみ（索敵・攻撃モーションなし、接触ダメージのみ）。
  時間経過によるステータス上昇の仕組み（`WaveModifier`）はデータ構造としては存在するが、
  実際に時間で連続変化させる処理は未実装。

## 開発方針・優先順位
コアループの完成を最優先する。後から武器種類の追加・複数ステージ化など拡張しやすい設計を
意識しつつ、まずは「敵が湧く→遠距離攻撃で倒す→時間経過で強化される→制限時間生存でクリア」
という一連の流れを最小構成で成立させることを優先する。
武器種類の拡充・複数ステージ対応はコアループの動作確認が済んでから着手する。

個人開発のプロトタイプ的な位置づけ（規模感については未確認、必要になれば再度確認する）。

## 現状の実装状況まとめ（このセクションはコード変更のたびに更新すること）

| 要素 | 状態 |
|---|---|
| Title→Hub→(Menu/StatusUpgrade)→Gameのシーン遷移 | 実装済み(`SceneManager`)。ハブ画面(`HubScene`)経由で武器・ステージ選択(`MenuScene`)/ステータス強化(`StatusUpgradeScene`)を選べる。`START_SCENE_NAME`は現在`TITLE_SCENE_NAME` |
| ゴールド・ステータス恒久強化 | 実装済み。敵撃破でゴールド取得(`EnemyDeathSystem::AwardGold`)、`StatusUpgradeScene`でMaxHp/AtkPower/Defense/CooldownRateを恒久強化(確認ダイアログ+ゴールド不足/MAX時のメッセージ表示あり)。`PlayerSaveData`(JSON)でアプリ再起動後も引き継がれる |
| プレイヤー移動・照準 | 実装済み |
| プレイヤー攻撃(遠距離武器発射) | 実装済み・実機確認済み(FireBolt/SingleShot、左クリックで発射のManual制御) |
| 敵の継続スポーン | 実装済み(`EnemySpawnSystem`)。実機動作確認は未実施 |
| 敵の複数種類・専用モデル | 未実装（現状1種・代用モデル。ボースは同モデルのパラメータ/色/スケール違い） |
| 敵AI(索敵・攻撃モーション) | 未実装（直進のみ） |
| 時間経過による敵強化(WaveModifier運用) | 実装済み(`EnemySpawnSystem`、HP/攻撃力が経過時間で線形成長)。実機動作確認は未実施 |
| ボス出現 | 実装済み(90秒でHP10倍/攻撃力3倍/見た目2.5倍の個体を1体スポーン)。実機動作確認は未実施 |
| クリア判定(制限時間生存) | 実装済み(180秒)。実機動作確認は未実施 |
| パーク選択UI | 実装済み(`PerkSelectSystem`)。キル数ベースのレベルアップ→3択→選択→効果適用まで一通り。実機動作確認は未実施 |
| パーク経由の武器取得(Nova/Homing Missile/Chain Lightning/Meteor) | 実装済み(`AcquireWeapon`パーク+各WeaponSystem)。初期武器3種+パーク武器4種。実機動作確認はNovaのみ完了、Homing/Chain/Meteorは未実施 |
| 必殺技(Ultimate) | 実装済み(`PlayerUltimateSystem`)。撃破数でゲージ蓄積→Q/PadR1で発動→RiseHeightまで上昇→ビームエフェクト再生終了待ち→元の座標へ瞬時にテレポート→全体大ダメージ+爆発エフェクト。プレイヤー正面・低い位置から見上げるカメラ固定、無敵、操作不可、GUIから調整可能(UltimateDebugPanel)。実機動作確認は未実施 |
| リザルト(クリア/ゲームオーバー)UI | 実装済み(`ResultSystem`新規)。クリア: Select確定でタイトルへ。ゲームオーバー: Retry/Titleの2択(MenuLeft/MenuRight+Select)。実機動作確認は未実施 |
| カメラ視点(POE2風アイソメトリック) | `CreateCamera`のOffsetは`{0,72,-38}`で既に急角度。POE2相当か要確認 |

## 未確定事項（次回以降のヒアリングで詰める）
- ゲームタイトル
- パークの具体的なラインナップ
- 武器・魔法の具体的な種類とビジュアル
- ボスの具体的な出現間隔・種類・数
- プロジェクトの最終的な規模感（個人プロトタイプで完結か、拡張していくか）
