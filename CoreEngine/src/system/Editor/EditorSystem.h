#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>
#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<string>

namespace sys
{
	/// <summary>
	/// EditorManager::IsEditing() の間だけ Engine::Update() から呼ばれる、
	/// クリック選択・地面ドラッグ移動・パレット配置・削除のシステム。
	/// </summary>
	class ENGINE_API EditorSystem : public utility::Singleton<EditorSystem>
	{
		SINGLETON_CLASS(EditorSystem);
	public:
		SINGLETON_ACCESSOR(EditorSystem);

		/// <summary>Edit モード中、毎フレーム Engine::Update() から呼ぶこと。</summary>
		void Update(entt::registry& registry);

		/// <summary>
		/// 次のクリックで指定キーのオブジェクトを配置するモードに入る。
		/// </summary>
		void ArmPlacement(const std::string& placementKey);

		/// <summary>配置待機中かどうか(UI表示用)</summary>
		bool IsPlacementArmed() const { return !mPendingPlacementKey.empty(); }

		/// <summary>待機中の配置キー(UI表示用)</summary>
		const std::string& GetPendingPlacementKey() const { return mPendingPlacementKey; }

		/// <summary>配置待機をキャンセルする</summary>
		void CancelPlacement() { mPendingPlacementKey.clear(); }

		/// <summary>
		/// 選択中の配置オブジェクト(SelectedTag + PlaceableTag)を削除する。
		/// </summary>
		void DeleteSelected(entt::registry& registry);

	private:
		bool UpdatePlacement(entt::registry& registry, bool selectPressed);
		void UpdateSelection(entt::registry& registry, bool selectPressed);
		void UpdateDrag(entt::registry& registry, bool selectHeld);

		entt::entity SpawnPlacedObject(
			entt::registry& registry,
			const std::string& key,
			const DirectX::XMFLOAT3& groundHitPos);

		/// <summary>
		/// Sprite(UI画像)を指定の仮想スクリーン座標に配置する。
		/// </summary>
		entt::entity SpawnPlacedSprite(
			entt::registry& registry,
			const std::string& texturePath,
			const DirectX::XMFLOAT2& screenPos);

		/// <summary>
		/// マウス位置(仮想スクリーン座標)と重なる PlaceableTag+Sprite エンティティのうち、
		/// 最も手前(Layer値が最大)のものを返す。無ければ entt::null。
		/// </summary>
		entt::entity PickSpriteAt(entt::registry& registry, const DirectX::XMFLOAT2& screenPos) const;

	private:
		/// <summary>次のクリックで配置するオブジェクトの種類キー。空なら配置モードではない。
		/// "Sprite:"で始まる場合はUI画像配置(以降がテクスチャパス)、それ以外は
		/// PrimitiveResourceManagerのキー(Box/Sphere)またはPointLightとして扱う。</summary>
		std::string mPendingPlacementKey;

		/// <summary>ドラッグ中かどうか</summary>
		bool mIsDragging = false;

		/// <summary>ドラッグ中に固定する地面平面の高さ(選択オブジェクトの Y。3Dドラッグ用)</summary>
		float mDragPlaneY = 0.0f;

		/// <summary>ドラッグ開始時点でのマウスとSprite座標の差分(2Dドラッグ用)</summary>
		DirectX::XMFLOAT2 mDragOffset2D = { 0.0f, 0.0f };
	};
}
