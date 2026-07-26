#pragma once

#include<Utility/Export/Export.h>

namespace ecs
{
    /// <summary>
    /// 謨ｵ蛟倶ｽ薙＃縺ｨ縺ｮ謗･隗ｦ謾ｻ謦・け繝ｼ繝ｫ繧ｿ繧､繝縲・
    /// 螟壽ｮｵ繝偵ャ繝医・騾｣邯壹ヲ繝・ヨ繧帝亟縺舌◆繧√↓逕ｨ縺・ｋ縲・
    /// 謾ｻ謦・鴨閾ｪ菴薙・ EnemyStatusComponent::Current.AtkPower 繧貞盾辣ｧ縺吶ｋ・医％縺薙↓縺ｯ謖√◆縺ｪ縺・ｼ峨・
    /// </summary>
    struct ENGINE_API EnemyAttackComponent
    {
        /// <summary>謾ｻ謦・俣髫費ｼ育ｧ抵ｼ峨ゅ％縺ｮ髢馴囈繧堤ｩｺ縺代↑縺・→蜀榊ｺｦ繝繝｡繝ｼ繧ｸ繧剃ｸ弱∴縺ｪ縺・・/summary>
        float AttackInterval = 0.5f;

        /// <summary>谿九ｊ繧ｯ繝ｼ繝ｫ繧ｿ繧､繝・育ｧ抵ｼ峨・莉･荳九〒謾ｻ謦・庄閭ｽ縲ゅす繧ｹ繝・Β縺梧ｯ弱ヵ繝ｬ繝ｼ繝貂帷ｮ励☆繧九・/summary>
        float CooldownTimer = 0.0f;
    };
}