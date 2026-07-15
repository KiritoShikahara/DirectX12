#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>
#include<entt/entt.hpp>

namespace sys
{
	/// <summary>
	/// Editor / Hierarchy / Inspector の3つの ImGui パネルを提供する。
	/// EditorManager(Play/Stop・Save/Load)と EditorSystem(選択/配置)の
	/// 状態を表示・操作する、_DEBUG専用の開発者ツール。
	/// </summary>
	class ENGINE_API EditorUI : public utility::Singleton<EditorUI>
	{
		SINGLETON_CLASS(EditorUI);
	public:
		SINGLETON_ACCESSOR(EditorUI);

		/// <summary>ImGuiにEditor/Hierarchy/Inspectorパネルを登録する</summary>
		bool Initialize(entt::registry& registry);

		/// <summary>登録したパネルを解除する</summary>
		void Finalize();

	private:
		void DrawEditorPanel(entt::registry& registry);
		void DrawHierarchyPanel(entt::registry& registry);
		void DrawInspectorPanel(entt::registry& registry);

		bool mIsInitialized = false;
	};
}
