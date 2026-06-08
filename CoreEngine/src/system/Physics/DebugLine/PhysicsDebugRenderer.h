#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<ImGui/imgui.h>

namespace sys
{
	class PhysicsDebugRenderer : public utility::Singleton<PhysicsDebugRenderer>
	{
		SINGLETON_CLASS(PhysicsDebugRenderer);
	public:
		SINGLETON_ACCESSOR(PhysicsDebugRenderer);

        /// <summary>
        /// ImGuiManager にデバッグウィンドウを登録する。アプリ起動時に一度だけ呼ぶ。
        /// </summary>
        void Initialize(entt::registry& registry);

        /// <summary>
        /// コライダーをワイヤーフレームで描画する。
        /// ImGui::NewFrame() の後、ImGui::Render() の前に呼ぶこと。
        /// </summary>
        void Draw(entt::registry& registry);


	private:
        // 設定
        ImU32     mBoxColor = IM_COL32(0, 255, 0, 200); // 緑
        ImU32     mSphereColor = IM_COL32(0, 200, 255, 200); // シアン
        ImU32     mCapsuleColor = IM_COL32(255, 165, 0, 200); // オレンジ
        ImU32     mSensorColor = IM_COL32(255, 255, 0, 200); // 黄
        int       mCircleSegments = 16; // 円の分割数
        bool      mEnabled = false;

    private:
        // デバックウィンドウ
        void ImGuiWindow();

        // 形状ごとのワイヤーフレーム
               /// <summary>Box のワイヤーフレームを描画する（12辺）</summary>
        void DrawBox(
            ImDrawList* drawList,
            const DirectX::XMFLOAT3& center,
            const DirectX::XMFLOAT4& rotation,
            const DirectX::XMFLOAT3& halfExtent,
            const DirectX::XMMATRIX& viewProj,
            ImU32                         color,
            const DirectX::XMFLOAT2& screenSize) const;

        /// <summary>Sphere のワイヤーフレームを描画する（XYZ 各軸に円）</summary>
        void DrawSphere(
            ImDrawList* drawList,
            const DirectX::XMFLOAT3& center,
            float                         radius,
            const DirectX::XMMATRIX& viewProj,
            ImU32                         color,
            const DirectX::XMFLOAT2& screenSize) const;

        /// <summary>Capsule のワイヤーフレームを描画する（円柱＋上下半球）</summary>
        void DrawCapsule(
            ImDrawList* drawList,
            const DirectX::XMFLOAT3& center,
            const DirectX::XMFLOAT4& rotation,
            float                         radius,
            float                         halfHeight,
            const DirectX::XMMATRIX& viewProj,
            ImU32                         color,
            const DirectX::XMFLOAT2& screenSize) const;


        /// <summary>
        /// ワールド座標 → スクリーン座標に投影する。
        /// カメラの後ろ（w <= 0）にある場合は false を返す。
        /// </summary>
        bool WorldToScreen(
            const DirectX::XMFLOAT3& worldPos,
            const DirectX::XMMATRIX& viewProj,
            const DirectX::XMFLOAT2& screenSize,
            ImVec2& outScreen) const;

        /// <summary>
        /// 2点をワールド座標でつなぐ線を描画する。
        /// どちらかがカメラ後方の場合はスキップする。
        /// </summary>
        void DrawLine3D(
            ImDrawList* drawList,
            const DirectX::XMFLOAT3& from,
            const DirectX::XMFLOAT3& to,
            const DirectX::XMMATRIX& viewProj,
            const DirectX::XMFLOAT2& screenSize,
            ImU32                     color) const;

        /// <summary>
        /// ローカル座標の点をクォータニオン回転 → ワールド位置に変換する
        /// </summary>
        static DirectX::XMFLOAT3 LocalToWorld(
            const DirectX::XMFLOAT3& localPos,
            const DirectX::XMFLOAT3& center,
            const DirectX::XMFLOAT4& rotation);
	};
}


