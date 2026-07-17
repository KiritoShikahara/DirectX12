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
        /// <summary>1回のスポーンタイミングで湧かせる敵の数</summary>
        int SpawnCountPerTick = 1;
        /// <summary>次のスポーンまでの残り時間(秒)。0開始なのでInGame開始直後に1体目が湧く</summary>
        float SpawnTimer = 0.0f;
        /// <summary>
        /// スポーン位置は「画面に映っている範囲の半径 + このマージン(m)」の円周上からランダムに選ぶ
        /// （必ず画面外から湧かせるため。画面内半径はカメラ設定から毎回動的に算出する）
        /// </summary>
        float SpawnMarginMin = 4.0f;
        float SpawnMarginMax = 12.0f;

        // ── 難易度スケーリング ────────────────────────────────
        /// <summary>
        /// 経過時間1秒あたりの敵ステータス(HP/攻撃力)成長率。0.004 = 1秒ごとに+0.4%。
        /// 序盤(最初の数十秒)は武器2発で倒せる程度になるよう、緩やかな値にしてある。
        /// </summary>
        float StatGrowthPerSecond = 0.004f;

        // ── ボース ────────────────────────────────────────────
        /// <summary>ボースが出現するまでの経過時間(秒)</summary>
        float BossSpawnTime = 90.0f;
        /// <summary>ボースを既にスポーンしたか</summary>
        bool BossSpawned = false;

        // ── クリア条件 ────────────────────────────────────────
        /// <summary>この秒数生存するとクリアになる</summary>
        float ClearTime = 180.0f;
    };
}
