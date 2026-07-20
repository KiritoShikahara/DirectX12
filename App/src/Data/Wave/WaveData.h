#pragma once

#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// ウェーブサバイバルのコアループ用バランス調整データ(CSV/DB)。
    /// WaveComponentの初期値として使う。常にId=0の単一行のみを使う設定値。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    /// </summary>
    struct WaveData
    {
        int Id = 0; // 常に0固定（単一設定行のためのダミー主キー）

        float SpawnInterval = 1.5f;         // 通常の敵を湧かせる間隔(秒)
        int SpawnCountPerTick = 1;          // 1回のスポーンタイミングで湧かせる敵の数
        float SpawnMarginMin = 4.0f;        // 画面外スポーンの最小マージン(m)
        float SpawnMarginMax = 12.0f;       // 画面外スポーンの最大マージン(m)
        float StatGrowthPerSecond = 0.004f; // 経過時間1秒あたりの敵ステータス成長率
        float BossSpawnTime = 90.0f;        // ボースが出現するまでの経過時間(秒)
        float ClearTime = 180.0f;           // この秒数生存するとクリアになる

        // 敵の同時生存数上限。0以下で無制限（既定＝無効）。
        // ボース出現はイベント性のため常に対象外。
        //
        // 元は性能対策として導入したが、Release/Develop構成での実測では敵の処理コストは
        // 無視できる水準(GameplayUpdate・Physicsとも1フレームあたり0.1ms未満)であり、
        // 性能上の必要性は無い。現在は「敵が増えすぎて画面が埋まるのを防ぐ」ための
        // ゲームデザイン上の調整値として位置づけ、必要な場合のみ有効にする。
        int MaxAliveEnemy = 0;

        REFLECT_BEGIN(WaveData, "wave_data")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_FLOAT(SpawnInterval)
            REFLECT_FIELD_INT(SpawnCountPerTick)
            REFLECT_FIELD_FLOAT(SpawnMarginMin)
            REFLECT_FIELD_FLOAT(SpawnMarginMax)
            REFLECT_FIELD_FLOAT(StatGrowthPerSecond)
            REFLECT_FIELD_FLOAT(BossSpawnTime)
            REFLECT_FIELD_FLOAT(ClearTime)
            REFLECT_FIELD_INT(MaxAliveEnemy)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::WaveData);
