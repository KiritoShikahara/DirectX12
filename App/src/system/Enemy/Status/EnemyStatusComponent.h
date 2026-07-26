#pragma once

#include<Utility/Export/Export.h>

namespace ecs
{
    /// <summary>
    /// 謨ｵ縺ｮ蝓ｺ遉弱せ繝・・繧ｿ繧ｹ・医・繧ｹ繧ｿ繝・・繧ｿ逕ｱ譚･縺ｮ荳榊､牙､・峨・
    /// 繧ｦ繧ｧ繝ｼ繝門ｼｷ蛹門燕縺ｮ邏縺ｮ蛟､繧剃ｿ晄戟縺吶ｋ縲・
    /// </summary>
    struct ENGINE_API EnemyBaseStatus
    {
        float MaxHp = 10.0f;     // 譛螟ｧHP
        float MoveSpeed = 2.0f;  // 遘ｻ蜍暮溷ｺｦ m/s
        float AtkPower = 1.0f;   // 謗･隗ｦ繝繝｡繝ｼ繧ｸ
        float ExperienceValue = 1.0f; // 謦・ｴ譎ゅ↓繝励Ξ繧､繝､繝ｼ縺ｸ荳弱∴繧狗ｵ碁ｨ灘､(EnemyData.csv縺ｮExp縺ｨ蟇ｾ蠢・
        float GoldValue = 3.0f;       // 謦・ｴ譎ゅ↓繝励Ξ繧､繝､繝ｼ縺ｸ荳弱∴繧九ざ繝ｼ繝ｫ繝・EnemyData.csv縺ｮGoldValue縺ｨ
                                       // 蟇ｾ蠢懊ゅ・繝ｼ繧ｹ縺ｯGameSceneFactory縺ｧ蛟咲紫驕ｩ逕ｨ)
    };

    /// <summary>
    /// 繧ｦ繧ｧ繝ｼ繝夜ｲ陦後↓繧医ｋ蠑ｷ蛹也紫・井ｹ礼ｮ励・縺ｿ繝ｻ1.0蝓ｺ貅厄ｼ峨・
    /// 繝励Ξ繧､繝､繝ｼ縺ｮ繝代・繧ｯ蠑ｷ蛹悶→蜷梧ｧ倥√え繧ｧ繝ｼ繝悶＃縺ｨ縺ｫ蜉邂怜粋謌舌☆繧九・
    /// 萓・ 1繧ｦ繧ｧ繝ｼ繝・+10% 縺ｪ繧・MulMaxHp += 0.1f 繧帝ｲ陦後・縺溘・縺ｫ蜉邂励・
    /// </summary>
    struct ENGINE_API EnemyWaveModifier
    {
        float MulMaxHp = 1.0f;
        float MulMoveSpeed = 1.0f;
        float MulAtkPower = 1.0f;
    };

    /// <summary>
    /// Base ﾃ・WaveModifier 縺ｮ遒ｺ螳夂ｵ先棡繧ｭ繝｣繝・す繝･縲・
    /// 豈弱ヵ繝ｬ繝ｼ繝險育ｮ励○縺壹√え繧ｧ繝ｼ繝門､牙喧譎ゅ・ Recompute() 縺ｧ縺ｮ縺ｿ譖ｴ譁ｰ縺吶ｋ縲・
    /// </summary>
    struct ENGINE_API EnemyCurrentStatus
    {
        float MaxHp = 10.0f;
        float MoveSpeed = 2.0f;
        float AtkPower = 1.0f;
    };

    /// <summary>
    /// 謨ｵ縺ｮ繧ｹ繝・・繧ｿ繧ｹ繧ｳ繝ｳ繝昴・繝阪Φ繝医・
    /// Base・井ｸ榊､会ｼ・/ WaveModifier・医え繧ｧ繝ｼ繝門ｼｷ蛹也紫・・/ Current・育｢ｺ螳壼､・・/ CurrentHp・育樟蝨ｨHP・峨・
    /// </summary>
    struct ENGINE_API EnemyStatusComponent
    {
		int EnemyId = 0; // 謨ｵ縺ｮ遞ｮ鬘曵D縲ゅ・繧ｹ繧ｿ繝・・繧ｿ縺ｮID縺ｨ蟇ｾ蠢懊☆繧九・

        EnemyBaseStatus    Base;
        EnemyWaveModifier  WaveMod;
        EnemyCurrentStatus Current;

        /// <summary>迴ｾ蝨ｨHP縲ら函謌千峩蠕後・ Recompute() 竊・CurrentHp = Current.MaxHp 縺ｧ蛻晄悄蛹悶☆繧九・/summary>
        float CurrentHp = 10.0f;

        /// <summary>蠢・ｮｺ謚縺ｮ遽・峇繝繝｡繝ｼ繧ｸ繧貞女縺代◆縺九・nemyDeathSystem縺後％繧後ｒ隕九※縲∝ｿ・ｮｺ謚閾ｪ霄ｫ縺ｮ
        /// 謦・ｴ縺ｧ縺ｯ蠢・ｮｺ謚繧ｲ繝ｼ繧ｸ(PlayerUltimateComponent::KillCount)繧貞刈邂励＠縺ｪ縺・ｈ縺・↓縺吶ｋ
        /// (逋ｺ蜍慕峩蠕後↓繧ｲ繝ｼ繧ｸ縺悟・縺ｳ雋ｯ縺ｾ縺｣縺ｦ縺励∪縺・・蟾ｱ蜿ら・逧・↑謖吝虚繧帝亟縺舌◆繧・縲・
        /// 繧ｴ繝ｼ繝ｫ繝峨・邨碁ｨ灘､繝ｻ繝代Ρ繝ｼ繝√Ε繝ｼ繧ｸ縺ｯ騾壼ｸｸ縺ｮ謦・ｴ縺ｨ蜷梧ｧ倥↓蜉邂励＆繧後ｋ縲・/summary>
        bool DamagedByUltimate = false;

        /// <summary>Base ﾃ・WaveModifier 繧定ｨ育ｮ励＠縺ｦ Current 縺ｫ蜿肴丐縺吶ｋ縲・/summary>
        void Recompute()
        {
            Current.MaxHp = Base.MaxHp * WaveMod.MulMaxHp;
            Current.MoveSpeed = Base.MoveSpeed * WaveMod.MulMoveSpeed;
            Current.AtkPower = Base.AtkPower * WaveMod.MulAtkPower;
        }
    };
}