#pragma once

#include<graphics/Effect/Object/EffectObject.h>
#include <Effekseer.h>
#include <DirectXMath.h>
#include <entt/entt.hpp>

namespace ecs
{
    /// <summary>
    /// エフェクト再生コンポーネント。
    /// Transform を持つエンティティに付けると位置・回転・スケールを自動同期する。
    /// Transform なしのエンティティに付けた場合は Offset を位置として使用する。
    /// </summary>
    struct EffectComponent
    {
        /// <summary>エフェクトアセット（EffekseerManager::GetEffect() で取得）</summary>
        Effekseer::EffectRef Asset;

        /// <summary>再生インスタンス</summary>
        graphics::EffectObject Effect;

        /// <summary>
        /// 親エンティティ（Transform を持つ）。
        /// entt::null の場合は自身の Transform を使う。
        /// </summary>
        entt::entity Parent = entt::null;

        /// <summary>親からのオフセット座標</summary>
        DirectX::XMFLOAT3 Offset = { 0.f, 0.f, 0.f };

        /// <summary>
        /// エフェクト自体のスケール。
        /// Transform を持つ場合は Transform.Scale と乗算される。
        /// </summary>
        DirectX::XMFLOAT3 Scale = { 1.f, 1.f, 1.f };

        /// <summary>
        /// エフェクト自体の回転（オイラー角、ラジアン）。既定値{0,0,0}は素材が想定する
        /// 既定の向きのまま再生する（毎フレームEffekseerManager::Updateが適用する）。
        /// </summary>
        DirectX::XMFLOAT3 Rotation = { 0.f, 0.f, 0.f };

        /// <summary>ループ再生するか</summary>
        bool IsLoop = false;

        /// <summary>表示するか</summary>
        bool IsVisible = true;
        bool LastIsVisible = false;

        /// <summary>
        /// 再生開始直後に非表示のままにしておく残りフレーム数
        /// (EffekseerManager::Update専用の内部状態。他システムから触らないこと)。
        ///
        /// 生成直後のインスタンスはビルボードの向き等、前フレームとの差分に依存する項目が
        /// まだ確定しておらず、素の四角形に近い見た目で描画されてしまうことがあるため、
        /// 内部状態が整うまで隠しておく。
        ///
        /// 【重要】以前は「次のUpdateでIsPlaying()がtrueになったら表示を戻す」という
        /// 実装だったが、Effekseerのワーカースレッドを有効にすると内部状態が確定する
        /// タイミングが変わり、そのまま破棄されて一度も表示されない・崩れた見た目のまま
        /// 表示され続ける、という不具合になった。
        /// 実行環境やスレッド構成に依存しないよう、明示的なフレーム数で管理する。
        /// </summary>
        int HiddenFramesRemaining = 0;

        // ── 最後にEffekseerへ適用した変換(EffekseerManager::Update専用の内部状態) ──
        // Effekseer側のSetLocation/SetRotation/SetScaleはいずれも呼ぶたびに
        // Handleでのstd::map検索(count+operator[]で2回)と行列の再構築を行うため、
        // 全エフェクトに毎フレーム無条件で呼ぶと同時再生数に比例した無視できない負荷になる。
        // 実際には回転・スケールが変化しないエフェクトが大半のため、前回適用値と比較して
        // 変化したものだけを呼び直す。
        // HasAppliedTransform==falseの間は初回(またはPlay()による再始動直後で
        // Effekseer側の変換がリセットされた状態)とみなし、比較せず必ず適用する。
        DirectX::XMFLOAT3 LastAppliedLocation = { 0.f, 0.f, 0.f };
        DirectX::XMFLOAT3 LastAppliedRotation = { 0.f, 0.f, 0.f };
        DirectX::XMFLOAT3 LastAppliedScale = { 0.f, 0.f, 0.f };
        bool HasAppliedTransform = false;
    };

} // namespace ecs