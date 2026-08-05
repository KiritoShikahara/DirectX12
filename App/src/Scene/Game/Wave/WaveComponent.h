#pragma once

namespace ecs
{
    ///<summary>
    ///ウェーブサバイバルのコアループを管理するランタイム状態。数値は初期値でプレイ感触に応じて調整すること
    ///</summary>
    struct WaveComponent
    {
        ///<summary>
        ///InGame状態になってからの経過時間、秒
        ///</summary>
        float ElapsedTime = 0.0f;

        ///<summary>
        ///通常の敵を湧かせる間隔、秒
        ///</summary>
        float SpawnInterval = 1.5f;
        ///<summary>
        ///1回のスポーンタイミングで湧かせる敵の数、経過時間による成長前の基準値
        ///</summary>
        int SpawnCountPerTick = 1;
        ///<summary>
        ///スポーン数の成長ステップ間隔、秒。この秒数が経過するたびにSpawnCountGrowthPerStep分だけSpawnCountPerTickが階段状に増える
        ///</summary>
        float SpawnCountGrowthStepInterval = 60.0f;
        ///<summary>
        ///1ステップごとのスポーン数増加倍率。1.0で+100%、つまり倍増
        ///</summary>
        float SpawnCountGrowthPerStep = 1.0f;
        ///<summary>
        ///次のスポーンまでの残り時間、秒。カメラが最低1フレーム更新される猶予を持たせるための遅延値
        ///</summary>
        float SpawnTimer = 0.2f;
        ///<summary>
        ///スポーン位置は画面に映っている範囲の半径にこのマージンを足した円周上からランダムに選ぶ
        ///</summary>
        float SpawnMarginMin = 4.0f;
        float SpawnMarginMax = 12.0f;
        ///<summary>
        ///同時に生存可能な敵の上限数。0以下で無制限、既定は無効。上限中は通常スポーンを一時停止しボスは対象外
        ///</summary>
        int MaxAliveEnemy = 0;

        ///<summary>
        ///敵ステータスHP/攻撃力強化ステップの間隔、秒。この秒数が経過するたびにStatGrowthPerStep分だけ段階的に強くなる
        ///</summary>
        float StatGrowthStepInterval = 120.0f;
        ///<summary>
        ///1ステップごとの成長倍率の増分。0.4で+40%
        ///</summary>
        float StatGrowthPerStep = 0.4f;

        ///<summary>
        ///小ボスが最初に出現するまでの経過時間、秒
        ///</summary>
        float MiniBossFirstSpawnTime = 180.0f;
        ///<summary>
        ///以後、小ボスが繰り返し出現する間隔、秒
        ///</summary>
        float MiniBossInterval = 180.0f;
        ///<summary>
        ///次回小ボスが出現する基準時刻。出現のたびにMiniBossIntervalずつ加算する
        ///</summary>
        float NextMiniBossSpawnTime = 180.0f;
        ///<summary>
        ///これまでに出現させた小ボスの数。BossPowerGrowthPerSpawnによる指数的な強化の指数として使う
        ///</summary>
        int MiniBossSpawnCount = 0;
        ///<summary>
        ///小ボスが出現するたびに、直前の小ボスに対してHP/攻撃力を何倍にするか(2.5なら毎回2.5倍)
        ///</summary>
        float BossPowerGrowthPerSpawn = 1.58f;

        ///<summary>
        ///中ボスが最初に出現するまでの経過時間、秒
        ///</summary>
        float MidBossFirstSpawnTime = 60.0f;
        ///<summary>
        ///以後、中ボスが繰り返し出現する間隔、秒
        ///</summary>
        float MidBossInterval = 60.0f;
        ///<summary>
        ///次回中ボスが出現する基準時刻。出現のたびにMidBossIntervalずつ加算する
        ///</summary>
        float NextMidBossSpawnTime = 60.0f;
        ///<summary>
        ///中ボスの強さを、直近に出現した小ボスの何倍にするか。小ボスは出現ごとに強くなり続けるため、
        ///中ボスもこれに追従させることで終盤も小ボスに対して一定の脅威度を保つ
        ///</summary>
        float MidBossPowerMultiplier = 5.0f;

        ///<summary>
        ///大ボスが出現するまでの経過時間、秒。1回だけ
        ///</summary>
        float FinalBossSpawnTime = 800.0f;
        ///<summary>
        ///大ボスを既にスポーンしたか
        ///</summary>
        bool FinalBossSpawned = false;

        ///<summary>
        ///この秒数生存するとクリアになる
        ///</summary>
        float ClearTime = 900.0f;
    };
}
