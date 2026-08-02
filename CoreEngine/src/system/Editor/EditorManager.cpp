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

		// シーン全体を Play 開始前の初期状態に作り直す。
		sys::SceneManager::Get().ReloadCurrentScene();

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
