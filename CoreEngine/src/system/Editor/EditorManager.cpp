#include "pch.h"
#include "EditorManager.h"

#include <ecs/Serialization/EditorSerialization.h>
#include <system/Scene/Manager/SceneManager.h>

namespace sys
{
	void EditorManager::EnterPlayMode(entt::registry& registry)
	{
		if (mMode == eEditorMode::Play)
		{
			return;
		}

		mPlaySnapshot = ecs::EditorSerialization::SerializeToString(registry);
		mMode = eEditorMode::Play;
	}

	void EditorManager::ExitPlayMode(entt::registry& registry)
	{
		if (mMode == eEditorMode::Edit)
		{
			return;
		}

		// シーン全体(プレイヤー・敵・UI・スコア等)を Play 開始前の初期状態に作り直す。
		// SceneManager::ReloadCurrentScene() が Local エンティティの全破棄と
		// UserSystem の再登録を行うため、Play 中に発生した変化(移動・スポーン・
		// スコア加算等)はここで完全にリセットされる。
		// 破棄されるエンティティが RigidBodyComponent を持っていれば
		// on_destroy フック(PhysicsSystem::OnRigidBodyComponentDestroyed)が
		// Jolt Body を自動で除去する。
		sys::SceneManager::Get().ReloadCurrentScene();

		// エディタで配置したオブジェクト(PlaceableTag)は上記のシーン再構築対象外
		// (シーンの Initialize() は関知しない)なので、Play 開始時点のスナップショットから
		// 別途復元する。
		ecs::EditorSerialization::DeserializeFromString(registry, mPlaySnapshot);

		mMode = eEditorMode::Edit;
	}

	bool EditorManager::SaveLayout(entt::registry& registry, const std::string& filePath)
	{
		return ecs::EditorSerialization::SaveToFile(registry, filePath);
	}

	bool EditorManager::LoadLayout(entt::registry& registry, const std::string& filePath)
	{
		return ecs::EditorSerialization::LoadFromFile(registry, filePath);
	}
}
