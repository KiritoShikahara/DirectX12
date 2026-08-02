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
        ///中ボスが出現するまでの経過時間、秒。1回だけ
        ///</summary>
        float MidBossSpawnTime = 480.0f;
        ///<summary>
        ///中ボスを既にスポーンしたか
        ///</summary>
        bool MidBossSpawned = false;

        ///<summary>
        ///最強ボスが出現するまでの経過時間、秒。1回だけ
        ///</summary>
        float FinalBossSpawnTime = 800.0f;
        ///<summary>
        ///最強ボスを既にスポーンしたか
        ///</summary>
        bool FinalBossSpawned = false;

        ///<summary>
        ///この秒数生存するとクリアになる
        ///</summary>
        float ClearTime = 900.0f;
    };
}
