# TASKS

## 未完了
- [ ] FBX: 実機で見た目を確認する（Faulキャラクターのアルベド色味・テクスチャの鮮明さ）
- [ ] FBX: SkyboxベースのIBL (diffuse irradiance / prefiltered environment map) をアンビエント項に導入
- [ ] FBX: AA対応 (MSAA or FXAA/TAA) の検討・実装
- [ ] FBX: カスケードシャドウ対応の要否検討（現状Directional 1灯・2048固定・カスケードなし）
- [ ] Editor: 削除機能を実機で動作確認（Deleteキー / Inspectorの削除ボタン）
- [ ] ゲーム: コアループの実機動作確認（敵の継続スポーン・難易度上昇・90秒でボース・180秒でクリア・
      GameStateがResultへ遷移すること）
- [ ] ゲーム: パークシステムの実機動作確認（キル→レベルアップ→3択→選択→効果反映を一通り）
- [ ] ゲーム: リザルトUIの実機動作確認（クリア→タイトル遷移、ゲームオーバー→Retry/Title選択→遷移）
- [ ] ゲーム: WaveDebugPanelでWaveComponentの数値（スポーン間隔/難易度成長率/ボース出現時間/クリア時間）をプレイ感触に応じて調整
- [ ] ゲーム: FrostOrb(サイズ1.5倍・距離拡大後)/SingleShotWeaponDebugPanel/WaveDebugPanelの実機動作確認
- [ ] ゲーム: カメラ角度(現状Offset{0,440,-300}、約56度)が実機でPOE2の見た目に近いか確認・微調整
- [ ] ゲーム: FireBolt弾速(ProjectileSpeed 25→60)・ログファイルの上書きリセットを実機確認
- [ ] ゲーム: 敵の種類追加・専用モデル差し替え（現状プレイヤーモデル代用の1種のみ）
- [ ] ゲームタイトルの決定
- [ ] ゲーム: プレイヤーAtkPowerステータスと武器ダメージの合算方式を検討（今回は武器レベルのみで計算、スコープ外とした）
- [ ] ゲーム: Homing Missileの実機動作確認（パーク選択で出現、SearchRadius内への自動発射、
      飛行中の追尾挙動、対象死亡時の再捕捉、序盤2発キルのバランス感触）
- [ ] ゲーム: Chain Lightningの実機動作確認（パーク選択で出現、自動発射、複数の敵への連鎖挙動、
      ダメージ減衰の感触）
- [ ] ゲーム: Nova(`Herald.efk`)/Homing Missile(`PhantasmMeteor.efk`/`Skill1Hit.efk`)/
      Chain Lightning(`AttackHit.efk`、HitEffectScale=4倍済み)のエフェクトは専用素材が無いための
      仮流用。後日差し替え前提（差し替え用素材はユーザーが用意し次第、`App/Assets/Effect/`配下に
      フォルダ分けして配置しWeaponDataのEffectPath列を差し替える）
- [ ] ゲーム: Chain LightningのAttackHit.efkは4倍に拡大したが、それでも代用素材のため
      わかりにくい可能性がある（ユーザー指摘）。実機確認の上、専用素材への差し替えや
      別のエフェクト(例: LightningStrike系)への一時変更を検討する
- [ ] ゲーム: 必殺技(Ultimate)の実機動作確認（15体撃破→オーラ表示→Q/PadR1で発動→カメラが
      即座に切り替わるか→RiseHeightまで上昇→ビームエフェクト再生終了待ち→終了後に瞬時に
      元の座標・カメラへ戻るか(地面へのめり込みが無いか)→全体ダメージ+爆発エフェクト、
      という一連の流れ。UltimateDebugPanelからRiseHeight等を変更して反映されるかも確認）
- [ ] ゲーム: 必殺技の実機バランス確認（Damage=50でザコ即死・ボース約3発撃破の感触が
      意図通りか、実際にプレイして確認。値はUltimateDebugPanelからGUI調整可能）
- [ ] ゲーム: 必殺技のビーム回転(ComputeBeamRotationFromDirection)が実機で正しい向きに見えるか確認。
      素材の既定軸を+Z(Effekseer Editorの青軸)と仮定して計算しているが、実際に見て
      向きがおかしければ軸の前提を再修正する必要がある
- [ ] ゲーム: RiseHeight3倍化(150→450)に伴い、カメラのCameraDistance/CameraHeightが
      遠すぎる/近すぎる見た目になっていないか確認（連動して自動調整される値ではないため）
- [ ] ゲーム: BeamCameraOffset(20)/ActivationHeightOffset(5)の実機確認。ビームがカメラの
      手前にずれて視認しやすくなったか、爆発が地面にめり込まなくなったか確認。
      値はUltimateDebugPanelからGUI調整可能
- [ ] ゲーム: 必殺技演出中の他攻撃抑制の実機動作確認（発動中に他の武器が発動しないか、
      既存の弾/オーブ/ハザードが完全に止まって見えるか、ビーム終了と同時に元通り
      動き出すかを実際にプレイして確認）
- [ ] ゲーム: 必殺技の画面全体を覆うフラッシュ演出は見送り済み(SceneManager/TransitionRenderer
      がシーン切り替え専用の状態機械と密結合のため)。必要なら将来的にTransitionRenderer側を
      汎用化する対応を検討
- [ ] **要ユーザー対応**: `C:\Users\kirit\Desktop\Effekseer素材`は全て`.efkproj`
      (Effekseer Editorのプロジェクト形式)で、このゲームが読み込める`.efk`(コンパイル済み
      バイナリ)ではないため使用できなかった。Effekseer Editorで`.efk`としてエクスポートして
      もらう必要がある(詳細はDECISIONS.md参照)。エクスポート済み`.efk`(+参照テクスチャ・
      モデル)を渡してもらえれば、`App/Assets/Effect/`へ配置しWeaponData/UltimateDataの
      エフェクトパスを差し替える対応がすぐにできる。なお`C:\Users\kirit\Desktop\Dev\Effekseer.exe`
      (Editor本体)の起動を確認済みだが、ClaudeはGUIアプリを操作する手段(画面を見る・
      マウス/キーボード操作をシミュレートする)を持たないため、エクスポート作業自体は
      代行できない
- [ ] ゲーム: 各WeaponData/UltimateDataのエフェクトパスは';'区切りで複数指定して同時再生
      できるようになった(`ecs::effectutil`)。使えるアセットが揃い次第、実際に複数エフェクトを
      組み合わせて試すこと（現状は全て1個ずつしか設定していない）
- [ ] ゲーム: パーク選択と必殺技演出の競合修正の実機動作確認（必殺技の全体ダメージで複数の
      敵を倒し複数レベル分のXPが一度に入った場合に、演出終了後にレベルアップ回数分だけ
      連続してパーク選択が提示されるか、実際にプレイして確認）

- [ ] ゲーム: 新メニュー階層(Title→Hub→Menu/StatusUpgrade)+ゴールド強化システムの
      実機手動操作確認（実際にキー入力でSelect/Cancel/MenuUp/MenuDownを操作し、
      各シーンへの遷移・戻り、ステータス強化の購入・ゴールド増減・レベル上昇の反映、
      アプリ再起動後の値の引き継ぎが正しく動くか）

- [ ] ゲーム: 敵の出現率向上・Meteor武器・強化確認ダイアログの実機手動操作確認
      （スポーン間隔短縮の体感確認、Meteorの発動確認、StatusUpgradeSceneの確認
      ダイアログ・ゴールド不足/MAXメッセージの表示確認）

- [ ] ゲーム: 新規武器3種(Void Beam/Bone Spear/Cleave)の実機手動操作確認
      （WeaponInventoryDebugPanelから取得orパーク選択で出現→各武器が自動発動するか、
      Void Beamが直線上の敵複数体を貫通ヒットするか、Bone Spearが命中しても消滅せず
      PierceCount分だけ貫通するか、Cleaveが扇状範囲の敵をなぎ払い実際にノックバックで
      吹き飛ぶか、ノックバック中に敵が通常の追従移動へ変な動きで戻らないかを実際に
      プレイして確認）
- [ ] エンジン: EnemyKnockbackComponent/EnemyKnockbackSystem(新規)の実機動作確認
      （Cleave以外の攻撃中は通常通りEnemyChaseSystemが追従することの回帰確認も含む）
- [ ] ゲーム: 新規スキル「Flicker Strike」の実機手動操作確認（パーク選択での出現→取得、
      マウスカーソルで敵を指定してRキー/PadYで発動、初撃+チャージ分のワープ攻撃が
      正しい回数発生するか(チャージ0/3/5個で1回/7回/11回)、近くに敵がいなくなった時点で
      打ち切られるか、演出中は無敵かつ他の武器が発動しないか、演出後は最後にワープした場所に
      留まるか、敵撃破でパワーチャージが正しく加算され上限で頭打ちになるか）
- [ ] エンジン: IsPlayerActionLocked()統合リファクタの回帰確認（必殺技演出中の既存動作
      (他武器停止・パーク選択保留・プレイヤー操作ロック)が従来通り機能するか、Flicker Strike
      演出中も同様に機能するか）
- [ ] **未解決バグ**: エフェクトが一定周期で一瞬・部分的に四角形ポリゴンになる問題。
      先読み対応(PreloadWeaponEffects)では解消せず、FrostOrbの周回オーブ・IceSpike等
      継続的にループ再生されるエフェクトで発生している模様。原因未特定
      (EffekseerのHandle再発行タイミングを疑っているが未検証)。実機の録画等、
      追加の手がかりが無いと切り分けが進めにくい状況
- [ ] ゲーム: Flicker Strikeの仕様変更・バランス調整後の実機動作確認（狙い方向に敵がいる
      状態でRキーを押すと初撃が出るか、方向上に敵がいない場合は何も起きずクールダウンも
      消費しないか、ワープ演出中にAttack/Attack2/Ultimateを押すとその場でシーケンスが
      打ち切られるか、開始時にパワーチャージ3個を所持しているか、序盤の雑魚敵が
      確実に一撃で倒せるか）

## 完了
- [x] AGENTS.md 作成、claude/ 配下に進捗復旧用ファイル（STATUS.md / TASKS.md / DECISIONS.md / STRUCTURE.md）を整備 (2026-07-15)
- [x] CLAUDE.md に「Claudeは指示なくgit操作をしない」を追記 (2026-07-15)
- [x] FBXグラフィックス品質の現状分析・改善点の洗い出し (2026-07-15)
- [x] FBX: Albedo/EmissiveテクスチャをsRGBフォーマットでSRV作成 (Texture.h/.cpp, TextureManager.h/.cpp, FbxResource.cpp) (2026-07-15)
- [x] FBX: サンプラーを異方性フィルタ (D3D12_FILTER_ANISOTROPIC, MaxAnisotropy=16) に変更 (FbxPipeline.cpp) (2026-07-15)
- [x] FBX: WIC/TGA読み込みテクスチャのミップマップ生成 (DirectXTex GenerateMipMaps) (Texture.cpp) (2026-07-15)
- [x] CoreEngine/App のDebug|x64ビルド確認済み (2026-07-15)
- [x] Editor: 配置オブジェクトの削除機能を追加 (InputManager.cpp, EditorSystem.h/.cpp, EditorUI.cpp) (2026-07-15)
- [x] claude/GAME_DESIGN.md 作成（ジャンル・世界観・コアループのヒアリング結果をまとめ） (2026-07-15)
- [x] ゲーム: 初回武器(FireBolt)の実装。Projectile汎用パイプライン+SingleShot発射ロジック+CSVマスタデータ+
      PhysicsSystem::OverlapSphere(新規)。PlayerAimComponent/PlayerAimSystemが未接続だった問題も解消 (2026-07-15)
- [x] ゲーム: FireBoltをAuto(自動)からManual(左クリック発射)に変更 (PlayerAimComponent.WantsToFire追加,
      PlayerInputSystem, SingleShotWeaponSystem, GameSceneFactory) (2026-07-15)
- [x] ゲーム: GameStatusDebugPanel追加。プレイヤー/敵のランタイムHP等をImGuiでリアルタイム表示 (2026-07-15)
- [x] ゲーム: FireBolt武器を実機確認済み（左クリック発射・命中・爆発・敵HP減少をGame Statusで確認） (2026-07-15)
- [x] ゲーム: 敵の移動速度を60→40に修正（プレイヤーの実移動速度50より少し遅く） (2026-07-15)
- [x] ゲーム: コアループを実装（WaveComponent, EnemySpawnSystem新規）。敵の継続スポーン・時間経過での
      HP/攻撃力強化・ボース出現・クリア判定。GameSceneFactory::CreateEnemyを位置/難易度倍率/ボース有無を
      受け取る形に変更、固定生成を廃止 (2026-07-15)
- [x] ゲーム: GameStateSystemのバグ修正。GameOver/GameClearRequested検知時にEnterResult()が
      呼ばれておらずResultステートへ遷移していなかった問題を修正 (2026-07-15)
- [x] ゲーム: 敵のスポーンを画面外基準に変更（カメラ設定から画面内半径を動的算出、その外側から湧く） (2026-07-15)
- [x] ゲーム: 制限時間を画面上部にテキスト表示（WaveTimerUiSystem新規） (2026-07-15)
- [x] エンジン: PhysicsSystem::ClearCollisionEvents()が一度も呼ばれていなかったバグを修正
      （Engine::UpdateGameplay()のUpdateフェーズ直後に呼ぶよう追加） (2026-07-15)
- [x] エンジン: TextRendererの点滅バグを修正（単一バッファを3フレームで共有していた点、
      D3D12MA::Allocationを即破棄していた点の2つ。FRAME_COUNT分のバッファに分離） (2026-07-15)
- [x] ゲーム: 敵が硬すぎる問題を調整（SingleShotWeaponData BaseDamage 8→12、
      WaveComponent.StatGrowthPerSecond 0.01→0.004） (2026-07-15)
- [x] エンジン: 衝突継続中はクールダウンが明けても再ダメージが入らないバグを修正。
      CollisionStayEvent新設(OnContactAdded/OnContactPersisted双方から発行)、
      PlayerContactDamageSystemはこちらを購読するよう変更 (2026-07-15)
- [x] ゲーム: 攻撃エフェクトが原点(0,0,0)に一瞬表示されるバグを修正。EffectObject::Play()に
      ワールド座標ではなくeffect.Offsetを渡していた箇所を修正、EffekseerManager::Update()の
      位置計算を1箇所に統一 (2026-07-15)
- [x] ゲーム: 敵の体力が減らない/クラッシュするバグを修正。ProjectileCollisionSystemがview走査中に
      エンティティ生成しイテレータを不正化していた問題を修正。EnemyDeathSystem新設(HP0の敵を破棄) (2026-07-15)
- [x] エンジン: アクセス違反クラッシュ(0xC0000005)を修正。PlayerContactDamageSystemの
      size_hint()==0による空判定が複数コンポーネントビューでは不正確だった問題を修正 (2026-07-15)
- [x] ゲーム: Release版で攻撃が出現しないバグを修正。SingleShotWeaponDataテーブルがDB(db.db)へ
      未同期だったため、SaveCsvToDb()で同期 (2026-07-15)
- [x] ゲーム: パークシステムを実装。PlayerLevelComponent/EnemyDeathSystemでのXP付与、
      PerkSelectSystem新規(3択UI・MenuLeft/MenuRight選択・Select確定・効果適用)。
      PlayerStatusComponent.Current.MoveSpeedをPlayerMovementSystemへ接続(比率ベース) (2026-07-15)
- [x] ゲーム: GameStatusDebugPanelにプレイヤーLevel/EXP、敵ごとの撃破時EXPの表示を追加 (2026-07-15)
- [x] ゲーム: リザルト(クリア/ゲームオーバー)UIを実装。ResultSystem新規
      (クリア: Select確定でタイトルへ。ゲームオーバー: Retry/Titleの2択) (2026-07-15)
- [x] ゲーム: Retry後にTimeScale=0のまま固まって見えるバグを修正。
      GameSceneFactory::CreateStateController()で明示的にSetTimeScale(1.0)するよう変更 (2026-07-15)
- [x] ゲーム: MenuSceneの魔法選択画像がX軸方向にずれて表示されるバグを修正。CreateSpells()の
      初期座標にMenuPagingSystemと同じcenterXオフセットを追加 (2026-07-15)
- [ ] ゲーム: Menu画面の画面遷移(Select確定→GameScene、Cancel→Title)が実際に動くか実機確認待ち
- [x] ゲーム: GameOver→Title→Menuの経路で選択の切り替えが起きないバグを修正。TimeScaleが
      0.0のまま持ち越されていたのが原因。TitleScene/MenuSceneのInitialize()で1.0へリセット (2026-07-15)
- [x] ゲーム: 2種類目の武器(AreaAttack: 氷のとげ)を実装。AreaAttackWeaponData(CSV)/
      AreaAttackWeaponRuntimeComponent/AreaAttackWeaponSystem新規。SingleShotと同じデータ駆動
      パターン。プレイヤーの初期装備にFireBoltと併せて追加。db.dbへもテーブル同期済み (2026-07-15)
- [x] ゲーム: AreaAttack(氷のとげ)をSingleShotと同じ「向いている方向」基準・右クリック発動に修正。
      当初MoveDirectionComponent(移動方向)+Auto制御で実装していたが、狙い方向が
      PlayerAimComponent(マウス/右スティック)であるべき、かつSingleShotと同じ初期武器のため
      右クリック(Manual)で発動すべき、との指摘を受けて修正。新規"Attack2"アクション(右クリック/
      PadL2)を追加し、PlayerAimComponent::WantsToFireSecondaryで制御 (2026-07-15)
- [x] ゲーム: FireBolt/IceSpikeの判定半径が敵の実サイズ(コライダーhalfExtent X/Z=5、全幅10m)に対して
      小さすぎ、ダメージが入りにくかった問題を調整。両方とも敵3体分程度(半径15m)に拡大
      (SingleShotWeaponData: BaseExplosionRadius 1.5→15/ExplosionRadiusPerLevel 0.4→2、
      AreaAttackWeaponData: BaseRadius 2→15/RadiusPerLevel 0.3→2)。db.dbへも同期済み (2026-07-15)
- [x] エンジン: OverlapSphere等アドホックな当たり判定を可視化するDebugWireSphereComponentを新規
      追加。PhysicsDebugRenderer(ImGui「Physics Debug」→「Show Colliders」)が対象にする
      (登録済みJolt Bodyしか描画できなかった問題を解消) (2026-07-15)
- [x] ゲーム: IceSpike(AreaAttack)を単一固定点への一撃から、狙い方向のSearchRadius内の敵を
      最大MaxTargets体まで自動検出し各敵の座標へ個別に氷柱(ハザード)を落とす方式に再設計。
      各氷柱はDuration(5秒)持続しTickInterval(0.5秒)ごとにダメージを反復する
      (AreaAttackHazardComponent/AreaAttackHazardSystem新規)。判定半径はDebugWireSphere
      とEffekseerのScale(概算)の両方で確認できるようにした。db.dbのarea_attack_weapons
      テーブルはスキーマ変更のためDROP TABLEしてから再同期した (2026-07-15)
- [x] ゲーム: FireBolt(ProjectileCollisionSystem)の爆発エフェクトにもDebugWireSphere可視化と
      判定半径ベースのEffekseer Scale概算を適用 (2026-07-15)
- [x] ゲーム: IceSpikeの「1回終わったら同じ場所にもう1発出た」ように見える問題を修正
      (ハザードのエフェクトIsLoop=trueが原因。false化し、着弾時1回のみ再生・当たり判定の持続は
      AreaAttackHazardSystemが別途担当するよう分離)。判定半径も3倍(15m)に拡大 (2026-07-15)
- [x] ゲーム: IceSpikeのDuration 5→2秒、FireInterval 1.5→1秒に短縮(検証しやすくする目的) (2026-07-15)
- [x] ゲーム: GameStatusDebugPanelに「Weapons」欄を追加。各武器(SingleShot/AreaAttack)の
      Lv・制御方式・クールタイム残り秒数/READY状態をGUI上で確認できるようにした (2026-07-15)
- [x] ゲーム: FrostOrbのサイズ(HitRadius 3→4.5・1.5倍)と周回距離(OrbitRadius 9→15)を拡大 (2026-07-16)
- [x] ゲーム: SingleShotWeaponData用のDataInspectorベースのデバッグパネル(SingleShotWeaponDebugPanel)を
      追加。SingleShot武器はDATA_MGRから毎回直接マスタデータを読む設計のためApply操作は不要、
      CSV/DB編集機能のみで今後の同期漏れを防止 (2026-07-16)
- [x] ゲーム: WaveComponentの数値をWaveData(CSV/DB)駆動化。WaveDebugPanel新規追加(DataInspector編集+
      Apply to Running Waveボタン)。CreateStateController()もWaveData(Id=0)から初期化するよう変更。
      db.dbへorbit_weapons更新・wave_data新規テーブルを同期済み。Debug/Release両方ビルド確認済み (2026-07-16)
- [x] エンジン: ログファイル(ecse_log.txt)が起動のたびに追記され続けていた問題を修正。
      FileLogSinkのfopen_sモードを"a"→"w"に変更し、常に最後の起動1回分のみが残るようにした。
      実機で2回起動→ログに"Logger initialized"が1回のみであることを確認済み (2026-07-16)
- [x] ゲーム: カメラの俯瞰角度を調整。「真上すぎる」との指摘で前回の約70度から約56度
      (`follow.Offset={0,440,-300}`)へ戻した。実機での見た目確認は未実施 (2026-07-16)
- [x] ゲーム: プレイヤー移動速度(PlayerMovementComponent::MaxSpeed)を50→75に変更。
      当初ユーザー指示の「Fire」をプレイヤー自身と誤解釈して対応したが、値自体はユーザー確認済みで
      このまま採用が確定 (2026-07-16)
- [x] ゲーム: 「Fire」は最初に作成した攻撃(FireBolt)を指すと訂正を受け、
      SingleShotWeaponDataのProjectileSpeedを25→60に変更。db.dbへ同期済み、
      Debug/Release両方ビルド確認済み (2026-07-16)
- [x] ゲーム: パーク経由でのみ取得できる武器の仕組みを新規実装。ePerkEffectType::AcquireWeapon
      (PerkDefinitionにAcquireWeaponType/AcquireWeaponId追加、未所持+空きスロット時のみ候補化)。
      GameSceneFactory::AddWeaponToPlayer()/RemoveWeaponFromPlayer()を新設し、初期武器3種の
      生成コード(重複していた)もこれを使うようリファクタリング (2026-07-16)
- [x] ゲーム: 1つ目のパーク武器「Nova」を実装。eWeaponType::Nova/data::NovaWeaponData(CSV/DB)/
      NovaWeaponRuntimeComponent/NovaWeaponSystem新規。発動トリガーが無くPulseInterval秒ごとに
      プレイヤー自身を中心とした円形範囲へ即座にダメージ(PhysicsSystem::OverlapSphere再利用)。
      GameStatusDebugPanelのWeapons欄にもクールダウン表示を追加。db.dbへnova_weapons新規
      テーブルを同期済み。Debug/Release両方ビルド成功、実機起動でクラッシュ無し確認済み (2026-07-16)
- [x] ゲーム: デバッグ用WeaponInventoryDebugPanelを新規追加。パークを介さず任意の武器を
      ImGuiから追加・削除できる(AddWeaponToPlayer/RemoveWeaponFromPlayerを直接呼ぶ) (2026-07-16)
- [x] ゲーム: Nova武器の実機動作確認完了 (2026-07-16)
- [x] ゲーム: 2つ目のパーク武器「Homing Missile」を実装。eWeaponType::Homing/
      data::HomingMissileWeaponData(CSV/DB)/HomingMissileRuntimeComponent/
      HomingMissileWeaponSystem/HomingMissileSteeringSystem新規。狙い不要の完全自動発動で、
      SearchRadius内に敵がいる場合のみ発射(いなければクールダウン非消費で待機)。既存の
      Projectile汎用パイプラインを再利用し、ProjectileComponentにIsHoming/Target/TurnSpeed/
      HomingSearchRadiusを追加しただけで対応(既存の通常弾には無影響)。db.dbへ
      homing_missile_weapons新規テーブルを同期済み。Debug/Release両方ビルド成功、
      実機起動でクラッシュ無し確認済み (2026-07-16)
- [x] ゲーム: Nova/Homing Missileのエフェクト素材について、ユーザーから第三者Effekseer素材の
      ダウンロード許可があったが、このサンドボックス環境にはブラウザ・汎用バイナリダウンロード
      手段が無くライセンス・互換性も確認できないため見送り、既存の仮流用パターン
      (Herald.efk/PhantasmMeteor.efk/Skill1Hit.efk)を踏襲する判断とした (2026-07-16)
- [x] ゲーム: Homing Missileのバランス調整（ユーザー指示）。弾速35→55・旋回性能180→320度/秒に
      向上、発射間隔1.5→0.6秒に短縮する代わりに火力8→6/2→1.5に低下。序盤の敵(MaxHp10)を
      2発で倒せる想定値に。db.dbへ同期済み、Debug/Release両方ビルド確認済み (2026-07-16)
- [x] ゲーム: 3つ目のパーク武器「Chain Lightning」を実装。eWeaponType::Chain/
      data::ChainLightningWeaponData(CSV/DB)/ChainLightningRuntimeComponent/
      ChainLightningWeaponSystem新規。狙い不要の完全自動発動で、SearchRadius内の最も近い敵へ
      雷撃(瞬時に命中、移動する実体は持たない)→命中した敵からJumpRadius内の未命中の敵へ最大
      MaxJumps回まで跳ね移り、跳ねるたびにダメージをDamageFalloffPerJumpで減衰。対象探索は
      AreaAttackWeaponSystemと同じPhysicsSystem::OverlapSphere+EnemyTagパターンを踏襲。
      エフェクトは専用素材が無いためAttackHit.efkを仮流用。db.dbへchain_lightning_weapons
      新規テーブルを同期済み。Debug/Release両方ビルド成功、実機起動でクラッシュ無し確認済み。
      これで初期武器3種+パーク武器3種(Nova/Homing Missile/Chain Lightning)が揃った (2026-07-16)
- [x] ゲーム: Chain Lightningのヒットエフェクトを4倍に拡大（ユーザー指示。代用素材のため
      視認しづらい懸念あり）。マジックナンバー化を避けるためChainLightningWeaponDataへ
      HitEffectScale列を新設(デフォルト4.0)、ChainLightningWeaponSystem::SpawnHitEffectで
      effect.Scaleへ適用。スキーマ変更のためdb.dbのchain_lightning_weaponsテーブルをDROPして
      再同期済み。Debug/Release両方ビルド確認済み (2026-07-16)
- [x] ゲーム: 必殺技(Ultimate)を新規実装（FGO宝具風の演出をイメージ、ユーザー指示）。
      data::UltimateData(CSV/DB)/PlayerUltimateComponent/PlayerUltimateSystem新規。
      撃破数(RequiredKillCount、デフォルト15体)でゲージ蓄積(EnemyDeathSystem::
      AwardUltimateChargeが加算)→満タン中はオーラ(ループエフェクト)をプレイヤーに付与→
      新規アクション"Ultimate"(Qキー/PadR1、InputManagerに追加)で発動→ステージ上の敵全員へ
      大ダメージ+Duration秒間のカメラズームイン(Fov)+スローモーション(TimeScale)+
      プレイヤー無敵化(PlayerStatusComponent::IsInvincible新設、PlayerContactDamageSystemが
      判定自体をスキップ)。演出時間はrawDeltaTimeで計測しTimeScaleの影響を受けない。
      画面全体を覆うフラッシュ演出はSceneManager/TransitionRendererがシーン切り替え専用の
      状態機械と密結合のため見送り。エフェクトは専用素材が無いためLight3.efk(オーラ)/
      Tornade.efk(発動演出)を仮流用。db.dbへultimate_data新規テーブルを同期済み。
      Debug/Release両方ビルド成功、実機起動でクラッシュ無し確認済み (2026-07-16)
- [x] ゲーム: 必殺技のカメラワークを刷新（ユーザー指示、FGO宝具風）。発動時のプレイヤー座標
      (StartPosition)・正面方向(ForwardDir、Transform::GetForward()で捕捉)を基準に、
      上昇(AscendDuration、RigidBodyのMoveVelocity経由)→上空詠唱(保持、全体ダメージ+
      地上側でActivationEffectPath再生)→下降(DescendDuration)の3フェーズに変更。
      カメラはPlayerUltimateSystem::UpdateCameraがCameraPlayerFollowSystemの後段で
      Transformを直接上書きし、プレイヤー正面(CameraDistance)・低い位置(CameraHeight)から
      見上げる構図に固定(注視点は現在のプレイヤー座標+CameraLookOffset)。db.dbのultimate_data
      をスキーマ変更(DROP→再同期)。Debug/Release両方ビルド確認済み (2026-07-16)
- [x] ゲーム: エフェクトの複数同時再生に対応（ユーザー指示）。ecs::effectutil::
      PlayOneShotCombined/PlayLoopingCombinedを新設。';'区切りで複数のエフェクトパスを
      指定すると、独立したエンティティとして全て同時に(組み合わせて)再生される。
      NovaWeaponSystem::Pulse/ChainLightningWeaponSystem::SpawnHitEffect/
      ProjectileCollisionSystem::SpawnExplosionEffect(FireBolt・Homing Missile共通)/
      PlayerUltimateSystemのオーラ・詠唱エフェクトに適用。弾/オーブに追従し続けるループ
      エフェクト(FireBolt/Homing Missileの飛翔中、FrostOrbの周回中)は破棄時の子エンティティ
      孤立(リーク)リスクがあるため単一のまま据え置き。Debug/Release両方ビルド確認済み (2026-07-16)
- [x] ゲーム: ユーザーが用意した`Desktop/Effekseer素材`の調査。全て`.efkproj`(Effekseer
      Editorのプロジェクト形式、XML)であり、このゲームが読み込める`.efk`(コンパイル済み
      バイナリ、"SKFE"シグネチャ)ではないと判明。プロジェクト内のEffekseerランタイムSDKには
      変換機能が無く、Effekseer Editor(GUI)でのエクスポートが必要なためこの環境では対応
      できず、ユーザーへエクスポート依頼が必要と判断(詳細はDECISIONS.md) (2026-07-16)
- [x] エンジン: Nova/Homing Missile(実際にはFireBolt含む全Projectile系武器共通)の当たり判定
      可視化ワイヤー(DebugWireSphereComponent)が消えないバグを修正。原因は前回のエフェクト
      複数同時再生対応リファクタリングで、ワイヤー可視化エンティティをEffectComponent(autoDelete
      で自動破棄)から分離した際、ワイヤー側の生存期間管理を追加し忘れていたこと。
      TemporaryLifetimeComponent/TemporaryLifetimeSystem新規(一定時間後に自動破棄する汎用の
      仕組み)を追加し、NovaWeaponSystem::Pulse/ProjectileCollisionSystem::SpawnExplosionEffectの
      ワイヤーエンティティに付与(0.3秒で消える)。Debug/Release両方ビルド確認済み (2026-07-16)
- [x] ゲーム: 必殺技演出中(IsActive)はプレイヤー操作を一切受け付けないよう修正。
      PlayerInputSystemがPlayerUltimateComponent::IsActiveを見て、移動入力・攻撃入力
      (WantsToFire/WantsToFireSecondary)を全て無効化する。Debug/Release両方ビルド確認済み (2026-07-16)
- [x] ゲーム: 必殺技のカメラワーク「上昇中は追従せず、プレイヤーの方を向くだけ」を確認。
      既存の実装(発動時のStartPosition/ForwardDirを基準にカメラ位置を固定し、LookAtだけを
      毎フレーム現在のプレイヤー座標へ更新する設計)が既にこの要件を満たしていることを
      コードレビューで再確認済み(位置は発動時に固定、姿勢=LookAtのみ追従する) (2026-07-16)
- [x] エンジン: Effekseer Editorをこの環境で操作できるか実際に確認。
      `C:\Users\kirit\Desktop\Dev\Effekseer.exe`を起動できることを確認したが、
      コマンドラインヘッドレスモードは無くGUI専用と判明。ClaudeはGUI操作の手段
      (画面を見る・マウス/キーボード操作をシミュレートする)を持たないため操作できないと
      ユーザーへ回答 (2026-07-16)
- [x] **バグ修正**: 必殺技のプレイヤーY軸上昇がほとんど感じられない問題を修正。
      原因はTimeScale(スローモーション)がTime::GetFixedDeltaTime()経由でJoltの物理
      シミュレーション自体の進行も遅くする仕様のため、RigidBodyへRiseSpeedをそのまま
      与えても実際の上昇距離がTimeScale倍(デフォルト0.15倍)に縮んでいたこと。
      PlayerUltimateSystem::ComputeVerticalSpeedでRiseSpeed/TimeScaleに補正して解決 (2026-07-16)
- [x] ゲーム: ユーザー指示によりカメラのズーム(Fov変更)を撤去。UltimateData.CameraFov/
      PlayerUltimateComponent.SavedCameraFovフィールドごと削除。スキーマ変更のため
      db.dbのultimate_dataテーブルをDROPして再同期済み。Debug/Release両方ビルド確認済み (2026-07-16)
- [x] **バグ修正+演出フロー全面刷新**: 必殺技終了後に地面へめり込む問題を修正（ユーザー指示）。
      時間ベース(Duration/AscendDuration/DescendDuration/TimeScale)の設計から状態ベース
      (ecs::eUltimatePhase: Ascending/PlayingBeam)の設計へ変更。(1)カメラを即座にプレイヤー
      正面・低い位置へ固定 (2)RiseHeight(GUIから調整可)に到達するまで上昇、位置判定のため
      ズレが起きない (3)BeamEffectPathのビームを再生しecs::effectutil::AnyPlaying()で
      再生終了を監視(MaxBeamDuration超過時の安全装置あり) (4)ビーム終了と同時にプレイヤーを
      TransformDirtyTag経由で発動前の座標へ瞬時にテレポート(速度ベースの降下による着地位置の
      ズレが根本原因だったため、この方式に変更して解決) (5)全体ダメージ+爆発エフェクト再生。
      スローモーション(TimeScale)は不要と判断し撤去(副次的に上昇量問題も影響しなくなった)。
      ecs::effectutil::PlayOneShotCombinedに生成エンティティを受け取れる引数を追加、
      新設のAnyPlaying()と組み合わせて複数エフェクトの再生終了監視を可能にした。
      debug::UltimateDebugPanel(新規)でRiseHeight等をGUIから編集できるようにした。
      db.dbのultimate_dataをスキーマ変更(DROP→再同期)。Debug/Release両方ビルド確認済み (2026-07-16)
- [x] エンジン: ユーザーが用意した必殺技用エフェクト(hougu_pre.efk/hougu_main.efk、
      App/Assets/Effect/に配置済み・依存テクスチャも揃っている)の読み込みを検証。
      EffekseerManager::GetEffect()が"Failed to load effect"を出すことをログで確認し、
      原因を調査。バイナリ先頭のファイル形式バージョンが1810(v1.81相当)である一方、
      このプロジェクト同梱のEffekseerランタイムはSupportBinaryVersion=1710(v1.71)が
      上限でそれ以上を明示的に拒否する仕様(Effekseer.Effect.cpp)と判明。
      対応が必要なためユーザーへ判断を仰ぐこととし、UltimateData.csvは動作確認済みの
      プレースホルダーへ一旦戻した(db.db同期済み、Debug/Release両方ビルド確認済み) (2026-07-16)
- [x] ゲーム: AtkPower/Defenseステータスの接続（ユーザー指示）。PlayerStatusComponent.
      Base.Defenseの初期値を0.0f→20.0fに変更(0のままだと乗算バフが永久に効かない問題の
      修正も兼ねる)。新規ecs::combatutil::GetAtkPowerMultiplier()
      (App/src/system/Player/Status/PlayerCombatUtil.h/.cpp)をPlayerMovementSystemと同じ
      Current/Base比方式で追加し、全武器(SingleShot/AreaAttack/Orbit/Nova/HomingMissile/
      ChainLightning)のダメージ計算へ適用。Defense側は既存のPlayerContactDamageSystemの
      半減点方式がそのまま活きる形。App.vcxproj/.filtersへ新規ファイル登録。
      Debug/Release両方ビルド確認済み、実機起動でクラッシュ無し確認済み (2026-07-16)
- [x] ゲーム: 被弾時・与ダメージ時のダメージ数値表記を実装（ユーザー指示）。新規
      ecs::DamageNumberComponent/DamageNumberSystem(App/src/system/UI/DamageNumber/)を追加。
      カメラのViewProjection行列でワールド座標→スクリーン座標へ射影し、既存の
      TextComponent描画パイプラインへ乗せる方式(新規ワールド空間テキストレンダラーは
      作らずに済んだ)。ecs::combatutil::SpawnDamageNumber()を敵ダメージ適用箇所(武器系6種)、
      プレイヤー被ダメージ箇所(PlayerContactDamageSystem)、必殺技ダメージ箇所
      (PlayerUltimateSystem::Cast)の全てに追加。GameScene.cppのCreateUserSystem()へ
      DamageNumberSystemをPostUpdateで登録(CameraPlayerFollowSystemの直後)。
      App.vcxproj/.filtersへ新規ファイル登録。Debug/Release両方ビルド確認済み、
      実機起動でエラーなし確認済み (2026-07-16)
- [x] ゲーム: hougu_pre.efk(必殺技の上昇後ビームエフェクト)の導入完了。ユーザーが
      Effekseer Editorでバージョン1710(ランタイム対応上限内)で再エクスポート。
      UltimateData.csvのBeamEffectPathをTornade.efk(プレースホルダー)からhougu_pre.efkへ
      差し替え、db.dbへ同期済み(一時的なSaveCsvToDb()呼び出しでDebugビルドを1回起動→
      db.db書き込み確認→コード側は元に戻す、という確立済みの手順で実施)。
      Debug/Release両方で実機起動確認済み、エラーログなし(既知のテクスチャ警告のみ)。
      hougu_main.efk(爆発エフェクト)は再エクスポート後もバージョン1810のままで一旦未解決
      だった (2026-07-16)
- [x] ゲーム: hougu_main.efk(必殺技の爆発エフェクト)の導入完了。ユーザーがEffekseer
      Editor側の設定を見直して再々エクスポートし、バージョン1710に収まった。
      UltimateData.csvのActivationEffectPathをBlow2.efk(プレースホルダー)から
      hougu_main.efkへ差し替え、db.dbへ同期済み(一時的なSaveCsvToDb()呼び出しでDebugビルドを
      1回起動→db.db書き込み確認→コード側は元に戻す、という確立済みの手順で実施)。
      Debug/Release両方で実機起動確認済み、エラーログなし(既知のテクスチャ警告のみ)。
      これで必殺技の専用エフェクトが上昇後ビーム・爆発ともに導入完了 (2026-07-16)
- [x] ゲーム: 必殺技の火力調整（ユーザー指示: 「ラスボス以外ワンパンできるレベルの火力」）。
      UltimateData.Damageを999→50に変更。敵のステータスはスポーン時に確定し以降フレーム毎の
      再計算はしないため、ザコ敵の最大HPはクリア間際(180秒)でも10×(1+0.004×180)≒17.2程度、
      対してボースは10×(1+0.004×90)×10倍補正=136固定(スポーン時刻90秒で確定)。
      Damage=50はザコを確実に即死させつつボースは複数発(約3発)必要という調整。
      db.db同期・Debug/Release両方で実機起動確認済み (2026-07-16)
- [x] ゲーム: 必殺技の演出調整（ユーザー指示）。BeamScale 8→16(hougu_preを2倍)、
      ActivationScale 8→80(hougu_mainを10倍)、RiseHeight 150→450(上昇高度を3倍)に変更。
      加えてビームの先端をカメラ方向へ向ける機能を新規実装。ecs::EffectComponentへ
      Rotation(オイラー角)フィールドを新設しEffekseerManager::Updateが毎フレーム
      Effect.SetRotation()を適用するようにした(Scaleと同じパターンで汎用化)。
      ecs::effectutil::PlayOneShotCombinedに回転引数を追加(デフォルト0で既存呼び出し箇所は
      無変更)。PlayerUltimateSystemにComputeBeamRotationTowards(ビーム位置→カメラ位置の
      方向ベクトルからオイラー角を算出)を追加。当初は素材の既定軸を+Y(上向き)と仮定して
      実装したが、ユーザーから「Effekseer Editorの青軸(+Z)方向に伸びている」と指摘を受け、
      +Z前提の計算式(ピッチ=asin(-dir.y)、ヨー=atan2(dir.x, dir.z))に修正済み。
      db.db同期・Debug/Release両方で実機起動確認済み(エラーなし) (2026-07-16)
- [x] ゲーム: 必殺技の演出微調整（ユーザー指示、実機フィードバック反映）。ビームが
      カメラの正面へまっすぐ延びると奥行きが見えず視認しづらいとの指摘を受け、
      UltimateData.BeamCameraOffset(新規、デフォルト20)を追加しカメラ方向へその距離だけ
      手前にずらして再生するよう変更(ComputeBeamRotationTowardsをComputeBeamRotationFrom
      Directionへリファクタリングし、方向ベクトルをオフセット計算と共用)。
      ActivationScaleを80(10倍)→56(7倍)に縮小。爆発エフェクトが地面に少しめり込むとの
      指摘を受け、ActivationHeightOffset(新規、デフォルト5)を追加。SqliteManagerへ
      DropTable<T>()を新設(EnsureTableのCREATE TABLE IF NOT EXISTSは既存テーブルの列を
      更新しないため、フィールド追加時の作り直し用汎用手段)。db.db同期・Debug/Release
      両方で実機起動確認済み(エラーなし) (2026-07-16)
- [x] ゲーム: 新メニュー階層(ハブ画面)のガワを実装（ユーザー指示。ゴールドによる
      ステータス恒久強化機能の第一段階、シーン遷移のみ先行実装）。Title→Hub→
      (Menu武器選択 or StatusUpgradeステータス強化)という階層に変更。新規HubScene
      (背景+2択、HubMenuComponent/HubMenuInputSystemが選択・遷移を担当、
      PerkSelectSystemの3択UIと同じ配色パターン踏襲)。新規StatusUpgradeSceneは
      現状プレースホルダー(背景+「準備中」テキストのみ、Cancelでハブへ戻る)。
      TitleInputSystem(Select先)/MenuControllerSystem(Cancel先)の遷移先をHubScene
      経由に変更。macros.hへHUB_SCENE_NAME/STATUS_UPGRADE_SCENE_NAME追加。
      START_SCENE_NAMEを一時的に切り替えてHub/StatusUpgrade両方の起動を実機確認済み
      (ログ出力確認、エラーなし)、確認後GAME_SCENE_NAMEへ戻した。Debug/Release両方
      ビルド確認済み。ゴールド永続化(PlayerSaveData)/強化バランスデータ
      (StatUpgradeData)/StatusUpgradeSceneの本実装は次フェーズ (2026-07-16)
- [x] ゲーム: ゴールド強化システムの本実装完了（ユーザー指示、ハブ画面のガワに続く
      第2フェーズ）。新規data::PlayerSaveData(Gold・MaxHp/AtkPower/Defense/
      CooldownRateの強化レベル)をConfigManager<T>(JSON、WindowConfigと同じ仕組み)で
      Assets/Bin/Save/player_save.jsonへ永続化(Debug/Releaseで挙動を分けない)。新規
      data::StatUpgradeData(CSV/DB)で対象ステータスごとのBaseCost/CostGrowthPerLevel/
      ValuePerLevel/MaxLevelを定義(コストはレベルごとに増加)。GameSceneFactory::
      CreatePlayerにApplyStatUpgrades()を追加しBaseへ反映。StatusUpgradeSceneを本実装
      (StatusUpgradeComponent/StatusUpgradeInputSystem、MenuUp/MenuDownで選択・Selectで
      購入・Cancelで戻る、表示名はPerkDefinitionと同じくwstringリテラルで直接持つ)。
      InputManagerにMenuUp/MenuDownアクションを新設。EnemyDeathSystemにAwardGold()を
      追加、EnemyBaseStatus::GoldValue(新規)を撃破時に加算しPlayerSaveDataへ即座に保存
      (ボースはkBossGoldMultiplier適用)。デバッグ用にdebug::PlayerSaveDebugPanel(新規、
      data::ConfigEditor<T>をラップ)を追加しGameScene/StatusUpgradeScene両方に登録、
      ImGuiでGold等を直接編集可能に。実機確認：Debug/Release両方でGameScene/
      StatusUpgradeScene起動確認済み、player_save.jsonの書き込み/読み込み・
      StatUpgradeDataのdb.db同期を一時コードで検証しエラーなし確認済み(検証用コードは
      全て元に戻した)。作業中、Write toolによる既存ファイル全文上書きでBOMが再度失われ
      日本語wstringリテラルが誤解釈される問題と、EnTTのview.each()が空のタグ型には
      コールバック引数を渡さない(empty型最適化)問題の2つを発見・修正した
      (詳細はDECISIONS.md) (2026-07-16)
- [x] ゲーム: 敵の出現率向上・ステータス強化の確認表記・新規攻撃「Meteor」追加
      （ユーザー指示、3件まとめて対応）。WaveData.CSVのSpawnIntervalを2.5→1.5秒に短縮
      (db.db同期済み)。StatusUpgradeSceneに確認ダイアログ・フィードバックメッセージを
      追加。StatusUpgradeComponentへIsConfirming/MessageTimer/Messageを追加、Selectで
      即購入せず「◯◯を強化しますか？(Cost:XG)」の確認を挟み、ゴールド不足時は
      「ゴールドが足りません」、MAX到達時は「既に最大レベルです」を2秒間表示。新規武器
      「Meteor」を追加(eWeaponType::Meteor、data::MeteorWeaponData、
      MeteorWeaponRuntimeComponent、MeteorWeaponSystem新規)。狙い不要の完全自動発動で、
      SearchRadius内の敵からランダムに最大MeteorCount体(初期値3)を選びそれぞれの頭上へ
      隕石を落として範囲ダメージ。エフェクトはPhantasmMeteor.efk;Fire7.efkを仮流用。
      PerkDefinitionにAcquireWeaponパーク「新武器: Meteor」を追加、
      WeaponInventoryDebugPanelにも追加。db.dbへmeteor_weapons新規テーブルを同期済み。
      Debug/Release両方ビルド確認済み、実機起動でエラーなし(検証用の一時コード・
      起動シーン切り替え・テスト用JSONは全て元に戻した) (2026-07-17)
- [x] ゲーム: パーク選択と必殺技演出の競合を修正（ユーザー指示）。必殺技演出中(IsActive)に
      レベルアップしてもパーク選択画面へ遷移しないよう、GameStateSystemのInGame→
      PerkSelect遷移条件へecs::IsPlayerUltimateActive(registry)の否定を追加(演出終了後まで
      保留)。また1回のXP付与で複数レベル分の閾値を同時に超えても1回しかパーク選択が
      提示されないバグを修正。GameStateComponent::LevelUpRequested(bool)を
      PendingLevelUpCount(int)へ変更、EnemyDeathSystem::AwardExperienceのif文をwhileループ
      にしレベルアップ回数分加算。GameStateSystemはPerkSelectDone時にカウンタを1減算し、
      残っていればPerkSelectComponentを外したままPerkSelect状態に留まる
      (PerkSelectSystemが次フレームで自動的に新しい3択を生成する既存の仕組みを再利用、
      PerkSelectSystem自体は無改修)。Debug/Release両方ビルド確認済み、実機起動でエラーなし
      (2026-07-16)
- [x] ゲーム: 必殺技演出中は他の攻撃が発生しないよう、また他の攻撃のエフェクトも
      非表示になるよう対応（ユーザー指示）。新規ecs::IsPlayerUltimateActive(registry)
      (PlayerUltimateComponent.hにinline関数として追加)を武器系10システム(SingleShot/
      AreaAttack/AreaAttackHazard/Orbit/Nova/HomingMissile/HomingMissileSteering/
      ChainLightning/ProjectileMovement/ProjectileCollision)のUpdate()冒頭に追加し、
      必殺技演出中はまるごとスキップ(新規発動だけでなく既存の弾の移動・命中判定・
      周回オーブ・ハザードの継続ダメージも完全停止)。PlayerUltimateSystemに
      SetOtherEffectsVisible(registry, bool)を追加し、発動(Activate)時に他の全エフェクトを
      非表示化(IsVisible=falseはEffekseer側の再生も一時停止させる)、ビーム(hougu_pre)
      再生終了時(FinishAndExplode冒頭)に再表示するようにした。db.db変更なし
      (C++側のみの変更)。Debug/Release両方で実機起動確認済み(エラーなし) (2026-07-16)
- [x] ゲーム: 新規武器3種「Void Beam(貫通レーザー)」「Bone Spear(貫通弾)」
      「Cleave(近接扇状攻撃)」を追加（ユーザー指示、攻撃案の提示→3案全て採用）。
      いずれもNova/Homing Missile/Chain Lightning/Meteorと同じ「狙い不要・自動発動・
      パーク経由でのみ取得」ファミリーに統一(既存の手動入力2枠(Attack/Attack2)を
      使い切っているため)。Void Beamは最も近い敵の方向へ直線を伸ばし、OverlapSphere+
      「線分への垂線距離」の数式フィルタで直線上の敵全員を貫通ヒットさせる(新規の物理
      クエリ形状は追加せず既存のOverlapSphereで完結、Chain Lightningと同じ実体を持たない
      瞬間ヒット方式)。Bone Spearは既存ProjectileComponentへPierceCountを追加し
      (通常弾は0のまま既存武器に影響無し)、ProjectileCollisionSystemを「貫通回数が
      残っている間は消滅しない」よう拡張、Homing Missileと同じ発射パイプラインを誘導無しで
      流用。CleaveはOverlapSphere+角度フィルタで扇状範囲を判定しダメージ+ノックバックを
      与える。ノックバック実現のため新規ecs::EnemyKnockbackComponent/EnemyKnockbackSystem
      (App/src/system/Enemy/Knockback/)を追加し、EnemyChaseSystemには当該コンポーネント
      保持中は追従移動を丸ごとスキップするガードを1行追加(2システムの速度上書き競合を
      実行順に依存せず明示的に防ぐため)。3種ともPerkDefinition/WeaponInventoryDebugPanel/
      GameStatusDebugPanelに登録済み。db.dbへvoid_beam_weapons/bone_spear_weapons/
      cleave_weapons新規テーブルを同期済み。新規作成した.h/.cppでBOM無しによる
      REFLECT_FIELDマクロ破損が実際に発生し、UTF-8 BOM付きへ再保存して解消(詳細は
      DECISIONS.md)。Debug/Release両方ビルド成功、実機起動でエラーなし確認済み
      (プレースホルダーエフェクトは仮流用、パーク選択での実際のランダム出現・取得・
      各武器の発動/貫通/ノックバック挙動の目視確認はまだ) (2026-07-17)
- [x] **バグ修正**: `Assets/Bin/Save/`フォルダが存在しない状態で`PlayerSaveData`の
      `Save()`が呼ばれると`std::ofstream`が開けず例外が飛んでいた問題を修正
      （ユーザー報告: `ConfigManager<PlayerSaveData>::Save()`でクラッシュ、
      player_save.jsonが存在しない）。`JsonSerializer::SaveToFile()`側で保存前に
      `std::filesystem::create_directories()`により親ディレクトリを作成するよう修正
      (ConfigManager<T>を使う全箇所に共通する根本修正、詳細はDECISIONS.md)。
      フォルダを実際に削除した状態から起動させ、自動生成・エラーなしを確認済み。
      Debug/Release両方ビルド確認済み (2026-07-17)
