#pragma once

namespace ecs
{
    /// <summary>
    /// ウェーブサバイバルのコアループ（敵の継続スポーン・難易度上昇・ボース出現・クリア判定）を
    /// 管理するランタイム状態。GameStateComponent と同じ状態管理エンティティに付与する。
    ///
    /// 数値は全て個人開発プロトタイプの初期値であり、プレイ感触に応じて調整すること。
    /// </summary>
    struct WaveComponent
    {
        /// <summary>InGame状態になってからの経過時間(秒)</summary>
        float ElapsedTime = 0.0f;

        // ── 敵の継続スポーン ──────────────────────────────────
        /// <summary>通常の敵を湧かせる間隔(秒)</summary>
        float SpawnInterval = 1.5f;
        /// <summary>1回のスポーンタイミングで湧かせる敵の数(経過時間による成長前の基準値)</summary>
        int SpawnCountPerTick = 1;
        /// <summary>
        /// スポーン数の成長ステップ間隔(秒)。敵ステータス成長(StatGrowthStepInterval)と同様、
        /// この秒数が経過するたびにSpawnCountGrowthPerStep分だけSpawnCountPerTickが階段状に増える
        /// (EnemySpawnSystem::ComputeStepGrowth参照)。
        /// </summary>
        float SpawnCountGrowthStepInterval = 60.0f;
        /// <summary>1ステップごとのスポーン数増加倍率(1.0=+100%、つまり倍増)</summary>
        float SpawnCountGrowthPerStep = 1.0f;
        /// <summary>
        /// 次のスポーンまでの残り時間(秒)。CameraSystem::Update()はUpdateフェーズ
        /// (EnemySpawnSystemが動く場所)より後、フレームの最後に実行されるため、
        /// シーン開始直後(0秒)だとまだ新しいカメラの行列が計算されておらず、
        /// ComputeVisibleRadiusがフォールバック半径(30m)を使ってプレイヤーのすぐ近くに
        /// 湧いてしまう。カメラが最低1フレーム更新される猶予を持たせるための遅延値。
        /// </summary>
        float SpawnTimer = 0.2f;
        /// <summary>
        /// スポーン位置は「画面に映っている範囲の半径 + このマージン(m)」の円周上からランダムに選ぶ
        /// （必ず画面外から湧かせるため。画面内半径はカメラ設定から毎回動的に算出する）
        /// </summary>
        float SpawnMarginMin = 4.0f;
        float SpawnMarginMax = 12.0f;
        /// <summary>
        /// 同時に生存可能な敵の上限数。0以下で無制限（既定＝無効）。
        /// 上限に達している間は通常スポーンを一時停止する（ボースは常に対象外）。
        /// 性能対策ではなくゲームデザイン上の調整値（data::WaveData参照）。
        /// </summary>
        int MaxAliveEnemy = 0;

        // ── 難易度スケーリング ────────────────────────────────
        /// <summary>
        /// 敵ステータス(HP/攻撃力)強化ステップの間隔(秒)。滑らかな連続成長ではなく、
        /// この秒数が経過するたびにStatGrowthPerStep分だけ段階的に強くなる階段状にする
        /// (EnemySpawnSystem::ComputeWaveModifier参照)。
        /// </summary>
        float StatGrowthStepInterval = 120.0f;
        /// <summary>1ステップごとの成長倍率の増分(0.4=+40%)</summary>
        float StatGrowthPerStep = 0.4f;

        // ── ボース(3階級、data::BossData参照) ──────────────────
        /// <summary>小ボースが最初に出現するまでの経過時間(秒)</summary>
        float MiniBossFirstSpawnTime = 180.0f;
        /// <summary>以後、小ボースが繰り返し出現する間隔(秒)</summary>
        float MiniBossInterval = 180.0f;
        /// <summary>次回小ボースが出現する基準時刻(ElapsedTime基準、秒)。
        /// 出現のたびにMiniBossIntervalずつ加算する(GameSceneFactory::CreateStateControllerで
        /// MiniBossFirstSpawnTimeへ初期化する)</summary>
        float NextMiniBossSpawnTime = 180.0f;

        /// <summary>中ボースが出現するまでの経過時間(秒、1回だけ)</summary>
        float MidBossSpawnTime = 480.0f;
        /// <summary>中ボースを既にスポーンしたか</summary>
        bool MidBossSpawned = false;

        /// <summary>最強ボースが出現するまでの経過時間(秒、1回だけ)</summary>
        float FinalBossSpawnTime = 800.0f;
        /// <summary>最強ボースを既にスポーンしたか</summary>
        bool FinalBossSpawned = false;

        // ── クリア条件 ────────────────────────────────────────
        /// <summary>この秒数生存するとクリアになる</summary>
        float ClearTime = 900.0f;
    };
}
