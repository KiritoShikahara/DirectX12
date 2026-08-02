#include "apppch.h"
#include "EffectSpawnUtility.h"

#include<Data/Effect/EffectAssetData.h>

#include<string_view>
#include<charconv>

namespace
{
    template<typename Func>
    void ForEachPath(const std::string& delimited, Func&& func)
    {
        // string_viewをコールバックへ渡し、分割ごとのvector/string確保を避ける
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

    struct OneShotEffectTag {};

    constexpr size_t kMaxConcurrentOneShotEffects = 300;

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

        // bypassBudget=trueの演出は間引かない。必殺技演出は他エフェクトを一時停止するだけでインスタンス数が残り続けるため、通常の間引きだと必須演出まで弾かれてしまう
        if (!bypassBudget)
        {
            if (registry.view<OneShotEffectTag>().size() >= kMaxConcurrentOneShotEffects)
            {
                return;
            }

            // 負荷の実体であるパーティクル数で間引く。直近フレームの実測値のため1フレーム遅れるが制御としては十分
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
            // Play直後の1フレーム目は次のUpdateまで反映されないため、生成直後から正しい向きで表示されるようここで先行して適用する
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
