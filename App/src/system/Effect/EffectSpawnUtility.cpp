#include "apppch.h"
#include "EffectSpawnUtility.h"

namespace
{
    /// <summary>';'区切りの文字列を空要素を除いてトークンへ分割する</summary>
    std::vector<std::string> SplitPaths(const std::string& delimited)
    {
        std::vector<std::string> result;
        size_t start = 0;

        while (start <= delimited.size())
        {
            size_t pos = delimited.find(';', start);
            if (pos == std::string::npos) pos = delimited.size();

            std::string token = delimited.substr(start, pos - start);
            if (!token.empty()) result.push_back(std::move(token));

            start = pos + 1;
        }

        return result;
    }
}

namespace ecs::effectutil
{
    void PlayOneShotCombined(
        const std::string& delimitedPaths,
        const DirectX::XMFLOAT3& position,
        float scale,
        std::vector<entt::entity>* outEntities,
        const DirectX::XMFLOAT3& rotation)
    {
        for (const auto& path : SplitPaths(delimitedPaths))
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
            // autoDelete=true: 再生終了フレームでEffekseerManager::Updateがこのエンティティを破棄する
            effect.Effect.Play(effect.Asset, position, true);
            // Play()直後の1フレーム目は次のEffekseerManager::Updateまで反映されないため、
            // 生成直後から正しい向きで表示されるようここで先行して適用する
            effect.Effect.SetRotation(rotation);

            if (outEntities != nullptr) outEntities->push_back(entity);
        }
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

        for (const auto& path : SplitPaths(delimitedPaths))
        {
            auto& manager = ::ecs::EntityManager::Get();
            auto entity = manager.CreateEntity();

            auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
            effect.Parent = parentEntity;
            effect.Asset = graphics::EffekseerManager::Get().GetEffect(path);
            effect.IsLoop = true;
            effect.Scale = { scale, scale, scale };
            effect.Effect.Play(effect.Asset, parentTransform->GetPosition());

            outEntities.push_back(entity);
        }
    }
}
