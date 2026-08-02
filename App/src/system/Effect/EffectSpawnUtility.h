#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<Data/Storage/Reflection.h>
#include<string>
#include<vector>

namespace ecs::effectutil
{
    ///<summary>
    ///指定位置に、';'区切りの各エフェクトを1つずつ独立したエンティティで1回だけ再生する。再生終了後、各エンティティは自動的に破棄される
    ///</summary>
    ///<summary>
    ///bypassBudget=trueにすると同時演出数・パーティクル数による間引きを無視して必ず生成する。必殺技のビーム等、必ず再生されなければならない演出に使う
    ///</summary>
    void PlayOneShotCombined(
        const std::string& delimitedPaths,
        const DirectX::XMFLOAT3& position,
        float scale,
        std::vector<entt::entity>* outEntities = nullptr,
        const DirectX::XMFLOAT3& rotation = { 0.f, 0.f, 0.f },
        bool bypassBudget = false);

    ///<summary>
    ///entitiesのうち、有効かつ再生中のものが1つでもあればtrueを返す
    ///</summary>
    bool AnyPlaying(entt::registry& registry, const std::vector<entt::entity>& entities);

    ///<summary>
    ///parentEntityに追従する、';'区切りの各エフェクトをそれぞれ独立したエンティティでループ再生する
    ///</summary>
    void PlayLoopingCombined(
        const std::string& delimitedPaths,
        entt::entity parentEntity,
        float scale,
        std::vector<entt::entity>& outEntities);

    ///<summary>
    ///';'区切りの各エフェクトアセットをEffekseerManagerのキャッシュへ先読みする、再生はしない
    ///</summary>
    void PreloadEffect(const std::string& delimitedPaths);

    ///<summary>
    ///';'区切りのエフェクト素材ID列を、data::EffectAssetDataで解決したパスの';'区切り文字列へ変換する。未登録IDはスキップする
    ///</summary>
    std::string ResolveEffectIds(const std::string& idsCsv);

    namespace detail
    {
        ///<summary>
        ///PreloadAllEffectPathFieldsの実装用ビジター。フィールド名の末尾がEffectPathならそのままパスとして、EffectIdsならResolveEffectIdsで解決してからPreloadEffectへ渡す
        ///</summary>
        class EffectPathPreloadVisitor : public data::IFieldVisitor
        {
        public:
            void OnInt(const std::string&, int&, data::eFieldFlag) override {}
            void OnFloat(const std::string&, float&, data::eFieldFlag) override {}
            void OnBool(const std::string&, bool&, data::eFieldFlag) override {}
            void OnString(const std::string& name, std::string& value, data::eFieldFlag) override
            {
                constexpr std::string_view kPathSuffix = "EffectPath";
                constexpr std::string_view kIdsSuffix = "EffectIds";

                if (HasSuffix(name, kPathSuffix))
                {
                    PreloadEffect(value);
                }
                else if (HasSuffix(name, kIdsSuffix))
                {
                    PreloadEffect(ResolveEffectIds(value));
                }
            }

        private:
            static bool HasSuffix(const std::string& name, std::string_view suffix)
            {
                return name.size() >= suffix.size() &&
                    name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
            }
        };
    }

    ///<summary>
    ///rowsの各要素をリフレクションで走査し、フィールド名がEffectPath/EffectIdsで終わる文字列フィールドを全てPreloadEffectへ渡す
    ///</summary>
    template<typename T>
    void PreloadAllEffectPathFields(const std::vector<T>& rows)
    {
        detail::EffectPathPreloadVisitor visitor;
        for (const auto& row : rows)
        {
            data::VisitFields(row, visitor);
        }
    }
}
