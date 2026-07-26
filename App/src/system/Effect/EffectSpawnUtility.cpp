#include "apppch.h"
#include "EffectSpawnUtility.h"

#include<Data/Effect/EffectAssetData.h>

#include<string_view>
#include<charconv>

namespace
{
    /// <summary>
    /// ';'区切りの文字列を空要素を除いてトークンへ分割し、1つずつfuncへ渡す。
    /// ワンショット演出はヒットの度に呼ばれるため、std::vector<std::string>を
    /// 返す実装（分割ごとにvector+string確保）を避け、string_viewをコールバックへ
    /// 渡す方式にしてヒープ確保を発生させない。
    /// </summary>
    template<typename Func>
    void ForEachPath(const std::string& delimited, Func&& func)
    {
        size_t start = 0;

        while (start <= delimited.size())
        {
            size_t pos = delimited.find(';', start);
            if (pos == std::string::npos) pos = delimited.size();

            const std::string_view token(delimited.data() + start, pos - start);
            if (!token.empty()) func(token);

            start = pos + 1;
        }
    }

    /// <summary>
    /// PlayOneShotCombined()で生成したエンティティにのみ付与するタグ。同時に存在する
    /// ワンショットエフェクト数を数えるためのもので、PlayLoopingCombined由来の
    /// オーラ・軌跡等(間引きたくない、数も少ない)とは区別する。
    /// </summary>
    struct OneShotEffectTag {};

    /// <summary>
    /// 同時に存在してよいワンショットエフェクトエンティティ数の目安上限。これを超えている間は
    /// 新規のワンショット演出(PlayOneShotCombined)の生成を間引いて負荷を抑える
    /// (ダメージ判定は各武器側で既に適用済みのため、間引かれるのは見た目の演出のみ)。
    /// 攻撃回数パーク×複数武器×大量の敵が同時に絡むと、個々の武器側の上限
    /// (VoidBeamWeaponData::MaxHitEffects等)だけでは防ぎきれない組み合わせ的な増加が
    /// 起きうるため、ここで全体の安全弁を設ける。
    /// </summary>
    constexpr size_t kMaxConcurrentOneShotEffects = 300;

    /// <summary>
    /// 同時に存在してよいパーティクルインスタンス数の目安上限。
    ///
    /// 上のエンティティ数上限(300)は「演出をいくつ再生中か」しか見ておらず、
    /// 実際の負荷を決めるパーティクル数とは対応しない。1つの演出が何百個の
    /// パーティクルを持つ素材もあるため、実測では300エンティティで
    /// 18,000〜23,000インスタンスに達し、Effekseerの更新・頂点生成が
    /// CPU時間の大半を占めていた(いずれもインスタンス数にほぼ比例する)。
    ///
    /// そこで負荷の実体であるインスタンス数そのものを予算として持ち、
    /// 超過中は新規のワンショット演出を間引く。密集戦闘でのみ効き、
    /// 通常時の見た目は変わらない(間引かれるのは演出だけで、ダメージ判定は
    /// 各武器側で適用済みのためゲーム挙動には影響しない)。
    /// </summary>
    constexpr int32_t kMaxConcurrentParticleInstances = 12000;
}

namespace ecs::effectutil
{
    void PlayOneShotCombined(
        const std::string& delimitedPaths,
        const DirectX::XMFLOAT3& position,
        float scale,
        std::vector<entt::entity>* outEntities,
        const DirectX::XMFLOAT3& rotation,
        bool bypassBudget)
    {
        auto& registry = ecs::EntityManager::Get().GetRegistry();

        // bypassBudget=true の演出(必殺技のビーム/メイン等の必須シネマティック)は間引かない。
        // 通常のヒット演出は負荷対策で間引くが、必殺技演出は「発動時に他エフェクトを一時停止
        // (破棄ではない)する」ため、それらのインスタンスがGetLastInstanceCountに残り続け、
        // 戦闘中は容易に上限超過して必須演出まで弾かれてしまう。必須演出は必ず再生させる。
        if (!bypassBudget)
        {
            if (registry.view<OneShotEffectTag>().size() >= kMaxConcurrentOneShotEffects)
            {
                return;
            }

            // 負荷の実体であるパーティクル数で間引く(kMaxConcurrentParticleInstances参照)。
            // 直近フレームの実測値を使うため1フレーム遅れるが、
            // 予算超過が続く間は抑制され続けるので制御としては十分
            if (graphics::EffekseerManager::Get().GetLastInstanceCount() >= kMaxConcurrentParticleInstances)
            {
                return;
            }
        }

        ForEachPath(delimitedPaths, [&](std::string_view path)
        {
            auto& manager = ::ecs::EntityManager::Get();
            auto entity = manager.CreateEntity();

            auto& transform = manager.AddComponent<ecs::Transform>(entity);
            transform.SetPosition(position);

            auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
            effect.Asset = graphics::EffekseerManager::Get().GetEffect(path);
            effect.IsLoop = false;
            effect.Scale = { scale, scale, scale };
            effect.Rotation = rotation;
            registry.emplace<OneShotEffectTag>(entity);
            // autoDelete=true: 再生終了フレームでEffekseerManager::Updateがこのエンティティを破棄する
            effect.Effect.Play(effect.Asset, position, true);
            // Play()直後の1フレーム目は次のEffekseerManager::Updateまで反映されないため、
            // 生成直後から正しい向きで表示されるようここで先行して適用する
            effect.Effect.SetRotation(rotation);
            graphics::EffekseerManager::MarkSpawnHidden(effect);

            if (outEntities != nullptr) outEntities->push_back(entity);
        });
    }

    bool AnyPlaying(entt::registry& registry, const std::vector<entt::entity>& entities)
    {
        for (entt::entity entity : entities)
        {
            if (!registry.valid(entity)) continue;

            const auto* effect = registry.try_get<ecs::EffectComponent>(entity);
            if (effect != nullptr && effect->Effect.IsPlaying()) return true;
        }
        return false;
    }

    void PlayLoopingCombined(
        const std::string& delimitedPaths,
        entt::entity parentEntity,
        float scale,
        std::vector<entt::entity>& outEntities)
    {
        auto& registry = ecs::EntityManager::Get().GetRegistry();
        const auto* parentTransform = registry.try_get<ecs::Transform>(parentEntity);
        if (parentTransform == nullptr) return;

        ForEachPath(delimitedPaths, [&](std::string_view path)
        {
            auto& manager = ::ecs::EntityManager::Get();
            auto entity = manager.CreateEntity();

            auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
            effect.Parent = parentEntity;
            effect.Asset = graphics::EffekseerManager::Get().GetEffect(path);
            effect.IsLoop = true;
            effect.Scale = { scale, scale, scale };
            effect.Effect.Play(effect.Asset, parentTransform->GetPosition());
            graphics::EffekseerManager::MarkSpawnHidden(effect);

            outEntities.push_back(entity);
        });
    }

    void PreloadEffect(const std::string& delimitedPaths)
    {
        ForEachPath(delimitedPaths, [](std::string_view path)
        {
            graphics::EffekseerManager::Get().GetEffect(path);
        });
    }

    std::string ResolveEffectIds(const std::string& idsCsv)
    {
        std::string result;
        ForEachPath(idsCsv, [&](std::string_view idToken)
        {
            int id = 0;
            const auto parseResult = std::from_chars(idToken.data(), idToken.data() + idToken.size(), id);
            if (parseResult.ec != std::errc()) return; // 数値変換失敗はスキップ

            const auto* asset = DATA_MGR(data::EffectAssetData).GetById(id);
            if (asset == nullptr) return; // 未登録IDはスキップ

            if (!result.empty()) result += ';';
            result += asset->Path;
        });
        return result;
    }
}
