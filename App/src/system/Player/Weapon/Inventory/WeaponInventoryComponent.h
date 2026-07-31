#pragma once

#include<vector>
#include<entt/entt.hpp>

namespace ecs
{
	/// <summary>
	/// 繝励Ξ繧､繝､繝ｼ縺梧園謖√☆繧区ｭｦ蝎ｨ縺ｮ邂｡逅・ｒ縺吶ｋ繧ｳ繝ｳ繝昴・繝阪Φ繝医・
    /// 繧ｹ繝ｭ繝・ヨ荳企剞繧定ｶ・∴縺ｦ譁ｰ隕乗ｭｦ蝎ｨ繧貞叙蠕励☆繧九％縺ｨ縺ｯ縺ｧ縺阪↑縺・・
    /// ・井ｸ企剞蛻ｰ驕泌ｾ後・繝ｬ繝吶Ν繧｢繝・・縺ｮ縺ｿ蜿ｯ閭ｽ縺ｨ縺吶ｋ諠ｳ螳夲ｼ・
	/// </summary>
	struct WeaponInventoryComponent
	{
        /// <summary>謇謖√＠縺ｦ縺・ｋ豁ｦ蝎ｨ繧ｨ繝ｳ繝・ぅ繝・ぅ・育函謌宣・ｼ・/summary>
        std::vector<entt::entity> Weapons;

        /// <summary>蜷梧凾謇謖√〒縺阪ｋ豁ｦ蝎ｨ縺ｮ譛螟ｧ謨ｰ</summary>
        int MaxSlots = 10;

        /// <summary>迴ｾ蝨ｨ縺ｮ謇謖∵焚</summary>
        int Count() const { return static_cast<int>(Weapons.size()); }

        /// <summary>譁ｰ隕乗ｭｦ蝎ｨ繧貞叙蠕励〒縺阪ｋ縺具ｼ育ｩｺ縺阪せ繝ｭ繝・ヨ縺後≠繧九°・・/summary>
        bool HasFreeSlot() const { return Count() < MaxSlots; }
	};

    /// <summary>
    /// 豁ｦ蝎ｨ縺ｮ謾ｻ謦・ｨｮ蛻･縲・
    /// 蜿門ｾ励☆繧九・繧ｹ繧ｿ蝙具ｼ・ata::SingleShotWeaponData 遲会ｼ峨・驕ｸ謚槭↓菴ｿ縺・・
    /// </summary>
    enum class eWeaponType
    {
        SingleShot, // 蜊倡匱蝙具ｼ夂漁縺｣縺滓婿蜷代∈蠑ｾ繧堤匱蟆・☆繧・
        SelfDefense,// 閾ｪ陦帛梛・壹・繝ｬ繧､繝､繝ｼ蜻ｨ蝗ｲ縺ｫ謖∫ｶ夂噪縺ｪ蠖薙◆繧雁愛螳壹ｒ蠑ｵ繧・
        AreaAttack, // 遽・峇謾ｻ謦・ｼ壽欠螳壼慍轤ｹ/閾ｪ讖溷捉霎ｺ縺ｫ遽・峇繝繝｡繝ｼ繧ｸ繧堤匱逕溘＆縺帙ｋ
        Nova,       // 閾ｪ蟾ｱ荳ｭ蠢・梛・夂匱蜍輔ヨ繝ｪ繧ｬ繝ｼ縺檎┌縺上∝捉譛溽噪縺ｫ繝励Ξ繧､繝､繝ｼ閾ｪ霄ｫ繧剃ｸｭ蠢・→縺励◆遽・峇繝繝｡繝ｼ繧ｸ繧堤匱逕溘＆縺帙ｋ
        Homing,     // 霑ｽ蟆ｾ蝙具ｼ夂漁縺・ｸ崎ｦ√〒閾ｪ蜍慕噪縺ｫ霑代￥縺ｮ謨ｵ縺ｸ霑ｽ蟆ｾ蠑ｾ繧堤匱蟆・☆繧・
        Chain,      // 騾｣骼門梛・夂漁縺・ｸ崎ｦ√〒閾ｪ蜍慕噪縺ｫ霑代￥縺ｮ謨ｵ繧呈茶縺｡縲∝多荳ｭ縺励◆謨ｵ縺九ｉ蛻･縺ｮ謨ｵ縺ｸ霍ｳ縺ｭ遘ｻ繧・
        Meteor,     // 髫慕浹蝙具ｼ夂漁縺・ｸ崎ｦ√〒閾ｪ蜍慕噪縺ｫ縲∝捉蝗ｲ縺ｮ謨ｵ隍・焚菴薙・鬆ｭ荳翫∈髫慕浹繧定誠縺ｨ縺礼ｯ・峇繝繝｡繝ｼ繧ｸ繧剃ｸ弱∴繧・
        VoidBeam,   // 雋ｫ騾壹Ξ繝ｼ繧ｶ繝ｼ蝙具ｼ夂漁縺・ｸ崎ｦ√〒閾ｪ蜍慕噪縺ｫ縲∵怙繧りｿ代＞謨ｵ縺ｮ譁ｹ蜷代∈逶ｴ邱夂憾縺ｮ遽・峇繧定ｲｫ騾壹＆縺帙ｋ
        BoneSpear,  // 雋ｫ騾壼ｼｾ蝙具ｼ夂漁縺・ｸ崎ｦ√〒閾ｪ蜍慕噪縺ｫ縲∵怙繧りｿ代＞謨ｵ縺ｸ逶ｴ騾ｲ縺吶ｋ雋ｫ騾壼ｼｾ繧堤匱蟆・☆繧・
        Cleave,     // 霑第磁謇・憾蝙具ｼ夂漁縺・ｸ崎ｦ√〒閾ｪ蜍慕噪縺ｫ縲∫漁縺・婿蜷代・謇・憾遽・峇蜀・・謨ｵ繧偵↑縺取鴛縺・ヮ繝・け繝舌ャ繧ｯ縺輔○繧・
        FlickerStrike, // 繝ｯ繝ｼ繝鈴｣謦・梛・壽焔蜍・迢吶＞謖・ｮ・縲ゅヱ繝ｯ繝ｼ繝√Ε繝ｼ繧ｸ繧貞・豸郁ｲｻ縺励∬ｿ代￥縺ｮ謨ｵ縺ｸ谺｡縲・Ρ繝ｼ繝玲判謦・☆繧・
        Ricochet,      // 反射増殖型：狙い不要で自動的に、近くの敵へ球体の弾を発射する。命中すると増殖しながら跳ね返り、世代の上限まで繰り返す
    };

    /// <summary>
    /// 豁ｦ蝎ｨ縺ｮ謫堺ｽ懈婿蠑上・
    /// </summary>
    enum class eWeaponControl
    {
        Manual, // 謇句虚・壼・蜉幢ｼ・antsToFire・峨′遶九▲縺溘ヵ繝ｬ繝ｼ繝縺ｫ縺ｮ縺ｿ逋ｺ蟆・愛螳壹ｒ陦後≧
        Auto,   // 閾ｪ蜍包ｼ咾T 縺梧・縺代ｋ縺溘・縺ｫ繧ｷ繧ｹ繝・Β縺瑚・襍ｰ縺ｧ逋ｺ蟆・☆繧・
    };


    /// <summary>
    /// 豁ｦ蝎ｨ縺ｮ蜈ｱ騾壽ュ蝣ｱ縺ｮ謇謖・
    /// 繝・・繧ｿ蜿門ｾ励↓蠢・ｦ√↑諠・ｱ縺ｮ菫晄戟
    /// </summary>
    struct WeaponComponent
    {
        // 豁ｦ蝎ｨ遞ｮ鬘曵D
        int WeaponID = 0;

        // 謾ｻ謦・ｨｮ蛻･
        eWeaponType Type = eWeaponType::SingleShot;

        // 譛螟ｧ繝ｬ繝吶Ν
        int MaxLevel = 0;

        // 迴ｾ蝨ｨ縺ｮ繝ｬ繝吶Ν
        int Level = 0;

        // 繧｢繧ｿ繝・メ蜈・・繧ｨ繝ｳ繝・ぅ繝・ぅ
        entt::entity Owner = entt::null;

        // 謫堺ｽ懈婿蠑・
        eWeaponControl Control = eWeaponControl::Auto;

    };
}