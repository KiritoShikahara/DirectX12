#include "pch.h"
#include "EditorUI.h"

#include<ImGui/imgui.h>

#include<system/ImGui/ImGuiManager.h>
#include<system/Editor/EditorManager.h>
#include<system/Editor/EditorSystem.h>
#include<system/Scene/Manager/SceneManager.h>
#include<system/Scene/Factory/SceneFactory.h>

#include<ecs/entity/EntityTag.h>
#include<ecs/component/Common/NameComponent.h>
#include<ecs/Serialization/PlaceableComponentReflection.h>

using namespace DirectX;

namespace sys
{
	namespace
	{
		/// <summary>
		/// Inspector パネル用の ImGui 編集ビジター。
		/// Data/Storage/Inspector/DataInspector.h の ImGuiEditVisitor と同型のパターン。
		/// </summary>
		class ImGuiComponentEditVisitor final : public ecs::IComponentFieldVisitor
		{
		public:
			void OnInt(const std::string& name, int& v) override
			{
				ImGui::DragInt(name.c_str(), &v);
			}
			void OnFloat(const std::string& name, float& v) override
			{
				ImGui::DragFloat(name.c_str(), &v, 0.05f);
			}
			void OnBool(const std::string& name, bool& v) override
			{
				ImGui::Checkbox(name.c_str(), &v);
			}
			void OnString(const std::string& name, std::string& v) override
			{
				char buf[256];
				strncpy_s(buf, v.c_str(), sizeof(buf) - 1);
				if (ImGui::InputText(name.c_str(), buf, sizeof(buf))) v = buf;
			}
			void OnFloat2(const std::string& name, XMFLOAT2& v) override
			{
				ImGui::DragFloat2(name.c_str(), &v.x, 0.05f);
			}
			void OnFloat3(const std::string& name, XMFLOAT3& v) override
			{
				ImGui::DragFloat3(name.c_str(), &v.x, 0.05f);
			}
			void OnFloat4(const std::string& name, XMFLOAT4& v) override
			{
				ImGui::DragFloat4(name.c_str(), &v.x, 0.05f);
			}
		};
	}

	bool EditorUI::Initialize(entt::registry& registry)
	{
		if (mIsInitialized) return true;

		auto& imgui = sys::ImGuiManager::Get();
		imgui.AddDebugUI([this, &registry]() { DrawEditorPanel(registry); }, "EditorPanel");
		imgui.AddDebugUI([this, &registry]() { DrawHierarchyPanel(registry); }, "EditorHierarchy");
		imgui.AddDebugUI([this, &registry]() { DrawInspectorPanel(registry); }, "EditorInspector");

		mIsInitialized = true;
		return true;
	}

	void EditorUI::Finalize()
	{
		if (!mIsInitialized) return;

		auto& imgui = sys::ImGuiManager::Get();
		imgui.RemoveDebugUI("EditorPanel");
		imgui.RemoveDebugUI("EditorHierarchy");
		imgui.RemoveDebugUI("EditorInspector");

		mIsInitialized = false;
	}

	void EditorUI::DrawEditorPanel(entt::registry& registry)
	{
		if (!ImGui::Begin("Editor"))
		{
			ImGui::End();
			return;
		}

		auto& editorMgr = sys::EditorManager::Get();
		auto& editorSys = sys::EditorSystem::Get();

		if (editorMgr.IsEditing())
		{
			if (ImGui::Button("Play")) editorMgr.EnterPlayMode(registry);
		}
		else
		{
			if (ImGui::Button("Stop")) editorMgr.ExitPlayMode(registry);
			ImGui::SameLine();
			ImGui::TextColored({ 1.0f, 0.6f, 0.2f, 1.0f }, "Playing...");
		}

		ImGui::Separator();

		ImGui::BeginDisabled(editorMgr.IsPlaying());

		static char sPathBuf[256] = "Assets/Bin/EditorLayout/DefaultScene.json";
		ImGui::InputText("Layout File", sPathBuf, sizeof(sPathBuf));

		if (ImGui::Button("Save Layout")) editorMgr.SaveLayout(registry, sPathBuf);
		ImGui::SameLine();
		if (ImGui::Button("Load Layout")) editorMgr.LoadLayout(registry, sPathBuf);

		ImGui::Separator();
		ImGui::Text("Placement Palette");
		if (ImGui::Button("Box"))         editorSys.ArmPlacement("Box");
		ImGui::SameLine();
		if (ImGui::Button("Sphere"))      editorSys.ArmPlacement("Sphere");
		ImGui::SameLine();
		if (ImGui::Button("Point Light")) editorSys.ArmPlacement("PointLight");

		static char sSpritePathBuf[256] = "Assets/Texture/UI/WeaponSelect/Fire.png";
		ImGui::InputText("Texture Path", sSpritePathBuf, sizeof(sSpritePathBuf));
		if (ImGui::Button("Sprite"))
		{
			editorSys.ArmPlacement(std::string("Sprite:") + sSpritePathBuf);
		}

		if (editorSys.IsPlacementArmed())
		{
			ImGui::TextColored({ 0.3f, 1.0f, 0.3f, 1.0f },
				"Placing '%s'... click in the scene", editorSys.GetPendingPlacementKey().c_str());
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) editorSys.CancelPlacement();
		}

		ImGui::EndDisabled();

		// シーン切り替えはPlay/Editどちらのモードでも常に使えるようにする
		// (上のBeginDisabled(IsPlaying())の対象外にする)ため、EndDisabled()の後に置く。
		DrawScenePanel();

		ImGui::End();
	}

	void EditorUI::DrawScenePanel()
	{
		ImGui::Separator();
		ImGui::Text("Scene Switch (no transition)");

		auto& sceneManager = sys::SceneManager::Get();
		const std::string& currentScene = sceneManager.GetCurrentSceneName();
		ImGui::Text("Current: %s", currentScene.c_str());

		for (const auto& name : sys::SceneFactory::Get().GetRegisteredNames())
		{
			const bool isCurrent = (name == currentScene);

			ImGui::BeginDisabled(isCurrent);
			if (ImGui::Button(name.c_str()))
			{
				// トランジションなしで即座に切り替える(ChangeSceneWithTransitionは使わない)。
				// 実際の切り替えはSceneManager::PostUpdate()(毎フレーム無条件で呼ばれる、
				// Play/Editモードを問わない)で次フレーム冒頭に適用される。
				sceneManager.ChangeScene(name);
			}
			ImGui::EndDisabled();
		}
	}

	void EditorUI::DrawHierarchyPanel(entt::registry& registry)
	{
		if (!ImGui::Begin("Hierarchy"))
		{
			ImGui::End();
			return;
		}

		registry.view<ecs::PlaceableTag>().each(
			[&](entt::entity entity)
			{
				std::string label;
				if (auto* name = registry.try_get<ecs::NameComponent>(entity))
				{
					label = name->Name;
				}
				else
				{
					label = "Entity_" + std::to_string(static_cast<uint32_t>(entity));
				}

				const bool isSelected = registry.all_of<ecs::SelectedTag>(entity);
				if (ImGui::Selectable(label.c_str(), isSelected))
				{
					registry.clear<ecs::SelectedTag>();
					registry.emplace<ecs::SelectedTag>(entity);
				}
			});

		ImGui::End();
	}

	void EditorUI::DrawInspectorPanel(entt::registry& registry)
	{
		if (!ImGui::Begin("Inspector"))
		{
			ImGui::End();
			return;
		}

		auto view = registry.view<ecs::SelectedTag>();
		if (view.begin() == view.end())
		{
			ImGui::TextDisabled("No selection");
			ImGui::End();
			return;
		}

		const entt::entity entity = *view.begin();
		ImGuiComponentEditVisitor visitor;

		if (auto* name = registry.try_get<ecs::NameComponent>(entity))
		{
			char buf[256];
			strncpy_s(buf, name->Name.c_str(), sizeof(buf) - 1);
			if (ImGui::InputText("Name", buf, sizeof(buf))) name->Name = buf;
			ImGui::Separator();
		}

		// Play中は編集操作(削除含む)を無効化する(Placement Palette等と同じ方針)
		ImGui::BeginDisabled(sys::EditorManager::Get().IsPlaying());
		ImGui::PushStyleColor(ImGuiCol_Button, { 0.6f, 0.15f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.8f, 0.2f, 0.2f, 1.0f });
		const bool deletePressed = ImGui::Button("Delete Object");
		ImGui::PopStyleColor(2);
		ImGui::EndDisabled();
		ImGui::Separator();

		if (deletePressed)
		{
			sys::EditorSystem::Get().DeleteSelected(registry);
			ImGui::End();
			return;
		}

		if (auto* transform = registry.try_get<ecs::Transform>(entity))
		{
			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
				ecs::VisitComponentFields(*transform, visitor);
		}
		if (auto* fbx = registry.try_get<ecs::FbxComponent>(entity))
		{
			if (ImGui::CollapsingHeader("Fbx"))
				ecs::VisitComponentFields(*fbx, visitor);
		}
		if (auto* sprite = registry.try_get<ecs::Sprite>(entity))
		{
			if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
				ecs::VisitComponentFields(*sprite, visitor);
		}
		if (auto* light = registry.try_get<ecs::DirectionalLightComponent>(entity))
		{
			if (ImGui::CollapsingHeader("DirectionalLight"))
				ecs::VisitComponentFields(*light, visitor);
		}
		if (auto* light = registry.try_get<ecs::PointLightComponent>(entity))
		{
			if (ImGui::CollapsingHeader("PointLight"))
				ecs::VisitComponentFields(*light, visitor);
		}
		if (auto* light = registry.try_get<ecs::SpotLightComponent>(entity))
		{
			if (ImGui::CollapsingHeader("SpotLight"))
				ecs::VisitComponentFields(*light, visitor);
		}
		if (auto* collider = registry.try_get<ecs::ColliderComponent>(entity))
		{
			if (ImGui::CollapsingHeader("Collider"))
				ecs::VisitComponentFields(*collider, visitor);
		}
		if (auto* rigidBody = registry.try_get<ecs::RigidBodyComponent>(entity))
		{
			if (ImGui::CollapsingHeader("RigidBody"))
				ecs::VisitComponentFields(*rigidBody, visitor);
		}

		ImGui::End();
	}
}
