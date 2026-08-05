#pragma once

#include <Data/Storage/Reflection.h>

namespace data
{
    struct WaveData
    {
        int Id = 0;

        float SpawnInterval = 1.5f;
        int SpawnCountPerTick = 1;
        float SpawnMarginMin = 4.0f;
        float SpawnMarginMax = 12.0f;

        float SpawnCountGrowthStepInterval = 60.0f;
        float SpawnCountGrowthPerStep = 1.0f;

        float StatGrowthStepInterval = 120.0f;
        float StatGrowthPerStep = 0.4f;

        float MiniBossFirstSpawnTime = 30.0f;
        float MiniBossInterval = 30.0f;
        float BossPowerGrowthPerSpawn = 1.58f;
        float MidBossFirstSpawnTime = 60.0f;
        float MidBossInterval = 60.0f;
        float MidBossPowerMultiplier = 5.0f;
        float FinalBossSpawnTime = 180.0f;

        float ClearTime = 300.0f;
        int MaxAliveEnemy = 0;

        REFLECT_BEGIN(WaveData, "wave_data")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_FLOAT(SpawnInterval)
            REFLECT_FIELD_INT(SpawnCountPerTick)
            REFLECT_FIELD_FLOAT(SpawnMarginMin)
            REFLECT_FIELD_FLOAT(SpawnMarginMax)
            REFLECT_FIELD_FLOAT(SpawnCountGrowthStepInterval)
            REFLECT_FIELD_FLOAT(SpawnCountGrowthPerStep)
            REFLECT_FIELD_FLOAT(StatGrowthStepInterval)
            REFLECT_FIELD_FLOAT(StatGrowthPerStep)
            REFLECT_FIELD_FLOAT(MiniBossFirstSpawnTime)
            REFLECT_FIELD_FLOAT(MiniBossInterval)
            REFLECT_FIELD_FLOAT(BossPowerGrowthPerSpawn)
            REFLECT_FIELD_FLOAT(MidBossFirstSpawnTime)
            REFLECT_FIELD_FLOAT(MidBossInterval)
            REFLECT_FIELD_FLOAT(MidBossPowerMultiplier)
            REFLECT_FIELD_FLOAT(FinalBossSpawnTime)
            REFLECT_FIELD_FLOAT(ClearTime)
            REFLECT_FIELD_INT(MaxAliveEnemy)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::WaveData);