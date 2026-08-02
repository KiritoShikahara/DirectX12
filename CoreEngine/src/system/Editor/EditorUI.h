#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>
#include<entt/entt.hpp>

namespace sys
{
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

		void DrawScenePanel();

		bool mIsInitialized = false;
	};
}
