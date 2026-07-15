#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>
#include<entt/entt.hpp>
#include<string>

namespace sys
{
	enum class eEditorMode
	{
		Edit, // 編集中。ゲームロジック・物理は停止し、EditorSystemの配置/選択/ドラッグのみ動く
		Play, // 再生中。通常のゲームループがそのまま動く
	};

	/// <summary>
	/// エディタの Edit/Play モードを管理する(UE の Play-In-Editor 相当)。
	///
	/// EnterPlayMode() でその時点の配置状態(PlaceableTag エンティティ群)を
	/// インメモリの JSON スナップショットとして保持し、
	/// ExitPlayMode() でそこから復元する。
	///
	/// SaveLayout/LoadLayout はファイルへの保存/復元で、
	/// スナップショットと同じ ecs::EditorSerialization を共用する(二重実装しない)。
	/// </summary>
	class ENGINE_API EditorManager : public utility::Singleton<EditorManager>
	{
		SINGLETON_CLASS(EditorManager);
	public:
		SINGLETON_ACCESSOR(EditorManager);

		bool IsEditing() const { return mMode == eEditorMode::Edit; }
		bool IsPlaying() const { return mMode == eEditorMode::Play; }

		/// <summary>
		/// 現在の配置状態(PlaceableTag エンティティ)をスナップショットして Play モードへ移行する。
		/// 既に Play 中の場合は何もしない。
		/// </summary>
		void EnterPlayMode(entt::registry& registry);

		/// <summary>
		/// PlaceableTag エンティティをスナップショットから復元して Edit モードへ戻る。
		/// (Jolt Body の後始末は RigidBodyComponent の on_destroy フックが自動で行う)
		/// 既に Edit 中の場合は何もしない。
		/// </summary>
		void ExitPlayMode(entt::registry& registry);

		/// <summary>現在の配置状態をファイルへ保存する。失敗時 false。</summary>
		bool SaveLayout(entt::registry& registry, const std::string& filePath);

		/// <summary>ファイルから配置状態を読み込む。失敗時 false。</summary>
		bool LoadLayout(entt::registry& registry, const std::string& filePath);

	private:
		eEditorMode mMode = eEditorMode::Edit;

		/// <summary>EnterPlayMode() 時点の PlaceableTag エンティティの JSON スナップショット</summary>
		std::string mPlaySnapshot;
	};
}
