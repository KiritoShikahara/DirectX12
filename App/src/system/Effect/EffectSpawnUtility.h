#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<Data/Storage/Reflection.h>
#include<string>
#include<vector>

namespace ecs::effectutil
{
    /// <summary>
    /// ';'区切りで1つ以上のエフェクトアセットパスを指定できるようにするためのユーティリティ。
    /// 複数指定した場合、それぞれ独立したエンティティとして同じ位置/追従先で同時に再生されるため、
    /// 結果的に複数のエフェクトを組み合わせて1つの演出として見せることができる
    /// （例: 閃光+衝撃波+火花、を3つのエフェクトを重ねて表現する等）。
    /// </summary>

    /// <summary>
    /// ワンショット再生: 指定位置に、';'区切りの各エフェクトを1つずつ独立したエンティティで
    /// 1回だけ再生する（再生終了後、各エンティティは自動的に破棄される）。
    /// outEntitiesを渡すと生成したエンティティを追加する。再生終了(≒破棄)の監視には
    /// AnyPlaying()を使う。rotationは素材の既定の向き(オイラー角、ラジアン)からの回転で、
    /// 省略時{0,0,0}は素材の既定の向きのまま再生する。
    /// </summary>
    /// <summary>
    /// bypassBudget=true にすると同時演出数・パーティクル数による間引き(kMaxConcurrentOneShotEffects/
    /// kMaxConcurrentParticleInstances)を無視して必ず生成する。必殺技のビーム/メインのような
    /// 「絶対に再生されなければ演出フローが破綻する」必須シネマティック演出に使う
    /// (通常のヒット演出は間引かれてもゲーム挙動に影響しないためfalseのままにする)。
    /// </summary>
    void PlayOneShotCombined(
        const std::string& delimitedPaths,
        const DirectX::XMFLOAT3& position,
        float scale,
        std::vector<entt::entity>* outEntities = nullptr,
        const DirectX::XMFLOAT3& rotation = { 0.f, 0.f, 0.f },
        bool bypassBudget = false);

    /// <summary>
    /// entitiesのうち、有効かつ再生中(EffectComponent::Effect.IsPlaying())のものが
    /// 1つでもあればtrueを返す。無効なエンティティ(既に再生終了して破棄済み)はfalse扱いにする。
    /// PlayOneShotCombinedで生成したエンティティ群の「全て再生し終わったか」の監視に使う。
    /// </summary>
    bool AnyPlaying(entt::registry& registry, const std::vector<entt::entity>& entities);

    /// <summary>
    /// ループ再生: parentEntity(Transformを持つこと)に追従する、';'区切りの各エフェクトを
    /// それぞれ独立したエンティティでループ再生する。生成したエンティティは全てoutEntitiesへ
    /// 追加するので、呼び出し側はparentEntityが破棄される前に必ずこれらをdestroyしてループを
    /// 止めること（EffekseerManagerはParent無効時、位置をOffset固定として扱い続けてしまうため）。
    /// </summary>
    void PlayLoopingCombined(
        const std::string& delimitedPaths,
        entt::entity parentEntity,
        float scale,
        std::vector<entt::entity>& outEntities);

    /// <summary>
    /// ';'区切りの各エフェクトアセットをEffekseerManagerのキャッシュへ先読みする(再生はしない)。
    /// 各武器の初回発動時(PlayOneShotCombined/PlayLoopingCombined)まで読み込みを遅延させると、
    /// プレイ中に初めて発動した瞬間だけテクスチャ読み込みが走り、その間の1〜数フレームだけ
    /// 他の再生中エフェクトの描画がちらつく可能性があるため、ロード画面中(GameScene::
    /// LoadResource())にまとめて読み込んでおく。
    /// </summary>
    void PreloadEffect(const std::string& delimitedPaths);

    /// <summary>
    /// ';'区切りのエフェクト素材ID列(例:"201;202")を、data::EffectAssetDataで解決した
    /// パスの';'区切り文字列(例:"Assets/Effect/LightningStrike.efk;Assets/Effect/chain_hit.efk")
    /// へ変換する。数値変換に失敗したトークンや、EffectAssetDataに未登録のIDは
    /// 結果から除く(その素材だけ無かったことになる。呼び出し側の追加対応は不要)。
    ///
    /// 各武器システムはマスタデータ取得直後にこれを1回呼び、戻り値を以降は従来通り
    /// (PlayOneShotCombined/GetEffect等への)パス文字列として扱う。
    /// </summary>
    std::string ResolveEffectIds(const std::string& idsCsv);

    namespace detail
    {
        /// <summary>
        /// PreloadAllEffectPathFields<T>()の実装用ビジター。文字列フィールドの名前のサフィックスで
        /// 値の意味を判別し、それぞれPreloadEffect()へ渡す:
        ///   ・"EffectPath"で終わる(命名規約: XxxEffectPath) … 値がそのままパス文字列
        ///     (data::UltimateData等、Id=0の単一行しか持たずレベル重複問題が無いデータ用)
        ///   ・"EffectIds"で終わる(命名規約: XxxEffectIds) … 値がエフェクト素材ID列(';'区切り)
        ///     (data::XxxWeaponData等、レベル数だけ行を持つデータ用。ResolveEffectIdsでパスへ解決してから渡す)
        /// </summary>
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

    /// <summary>
    /// rowsの各要素をリフレクション(data::VisitFields)で走査し、フィールド名が"EffectPath"/
    /// "EffectIds"で終わる文字列フィールド(命名規約: XxxEffectPath / XxxEffectIds)を
    /// 全てPreloadEffect()へ渡す。GameScene::PreloadWeaponEffects()が武器/必殺技マスタデータ
    /// ごとに個別のフィールド名を列挙していた重複を解消するために追加した。新しいフィールドを
    /// 追加した場合、命名規約に従っていれば自動的に先読み対象になる(呼び出し側の追記は不要)。
    /// </summary>
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
