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
	///
	/// 選択判定は物理コライダーへの Jolt レイキャスト(PhysicsSystem::TryPickEntity)を
	/// 使うため、Collider の無いエンティティは 3D クリックでは選択できない
	/// (Hierarchy パネルからの選択は別途 SelectedTag を直接付け替えるため可能)。
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
		/// EditorUI のパレットボタンから呼ぶ。
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
		/// 選択が無い、または対象が PlaceableTag でない場合は何もしない。
		/// Deleteキー(EditorSystem::Update経由)と EditorUI の削除ボタンの両方から呼ばれる。
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

	private:
		/// <summary>次のクリックで配置するオブジェクトの種類キー。空なら配置モードではない。</summary>
		std::string mPendingPlacementKey;

		/// <summary>ドラッグ中かどうか</summary>
		bool mIsDragging = false;

		/// <summary>ドラッグ中に固定する地面平面の高さ(選択オブジェクトの Y)</summary>
		float mDragPlaneY = 0.0f;
	};
}
