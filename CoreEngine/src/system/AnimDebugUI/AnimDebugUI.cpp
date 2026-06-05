#include "pch.h"
#include  "AnimDebugUI.h"

#include <imgui/imgui.h>
#include <system/ImGui/ImGuiManager.h>
#include <ecs/component/model/ModelComponent.h>
#include <ecs/component/model/ModelAnimComponent.h>
#include<graphics/Model/Resouce/ModelResouce.h>

namespace sys
{

    void AnimationDebugUI::Register(entt::registry& registry)
    {
        sys::ImGuiManager::Get().AddDebugUI([&registry]()
            {
#if defined(_DEBUG) || defined(DEV_TOOL_ENABLED)

                if (!ImGui::Begin("Animation Debug"))
                {
                    ImGui::End();
                    return;
                }

                int entityCount = 0;

                // Model と ModelAnimComponent 両方を持つエンティティを列挙
                registry.view<ecs::Model, ecs::ModelAnimComponent>()
                    .each([&](entt::entity entity,
                        ecs::Model& model,
                        ecs::ModelAnimComponent& anim)
                        {
                            ImGui::PushID(static_cast<int>(entity));
                            entityCount++;

                            const bool open = ImGui::CollapsingHeader(
                                std::format("Entity #{}", static_cast<uint32_t>(entity)).c_str(),
                                ImGuiTreeNodeFlags_DefaultOpen);

                            if (!open) { ImGui::PopID(); return; }

                            auto* res = model.Resource;
                            if (!res || !res->IsLoaded())
                            {
                                ImGui::TextColored({ 1,0.3f,0.3f,1 }, "Resource is null or not loaded.");
                                ImGui::PopID();
                                return;
                            }

                            // ── クリップ一覧 ─────────────────────────────────────
                            ImGui::SeparatorText("Clips");
                            ImGui::Text("Total: %d", res->GetClipCount());
                            for (int i = 0; i < res->GetClipCount(); ++i)
                            {
                                const auto& clip = res->GetAnimClips()[i];
                                const bool isCur = (i == anim.CurrentClipIndex);
                                const bool isPrev = (i == anim.PrevClipIndex);

                                if (isCur)  ImGui::TextColored({ 0.3f,1,0.3f,1 },
                                    "  [%d] '%s' %.2fs (CURRENT)", i, clip.Name.c_str(), clip.Duration);
                                else if (isPrev) ImGui::TextColored({ 1,1,0.3f,1 },
                                    "  [%d] '%s' %.2fs (PREV)", i, clip.Name.c_str(), clip.Duration);
                                else ImGui::Text("  [%d] '%s' %.2fs", i, clip.Name.c_str(), clip.Duration);
                            }

                            // ── 再生状態 ─────────────────────────────────────────
                            ImGui::SeparatorText("Playback");
                            ImGui::Text("Clip=%d  Time=%.3f  Playing=%s",
                                anim.CurrentClipIndex, anim.CurrentTime,
                                anim.IsPlaying ? "YES" : "NO");
                            if (anim.IsBlending())
                                ImGui::Text("Blend: prev=%d factor=%.2f",
                                    anim.PrevClipIndex, anim.GetBlendFactor());

                            ImGui::DragFloat("PlaySpeed", &anim.PlaySpeed, 0.01f, -3.f, 3.f);
                            if (ImGui::Button("Pause"))  anim.Pause();
                            ImGui::SameLine();
                            if (ImGui::Button("Resume")) anim.Resume();
                            ImGui::SameLine();
                            if (ImGui::Button("Rewind")) anim.Rewind();

                            // ── ボーン情報 ────────────────────────────────────────
                            ImGui::SeparatorText("Bones");
                            ImGui::Text("BoneCount=%d  BoneMatrices=%d",
                                res->GetBoneCount(), (int)anim.BoneMatrices.size());

                            // トラック解決状況
                            if (res->GetClipCount() > 0 &&
                                anim.CurrentClipIndex < res->GetClipCount())
                            {
                                const auto& clip = res->GetAnimClips()[anim.CurrentClipIndex];
                                if (ImGui::TreeNode("Track Resolution"))
                                {
                                    int ok = 0, ng = 0;
                                    if (clip.IsBaked)
                                    {
                                        for (const auto& t : clip.BakedTracks)
                                        {
                                            const bool resolved = (t.BoneIndex >= 0);
                                            resolved ? ok++ : ng++;
                                            ImGui::TextColored(
                                                resolved ? ImVec4{ 0.3f,1,0.3f,1 } : ImVec4{ 1,0.3f,0.3f,1 },
                                                "  '%s' -> [%d] %s",
                                                t.BoneName.c_str(), t.BoneIndex,
                                                resolved ? "OK" : "NOT FOUND");
                                        }
                                    }
                                    else
                                    {
                                        for (const auto& t : clip.SparseTracks)
                                        {
                                            const bool resolved = (t.BoneIndex >= 0);
                                            resolved ? ok++ : ng++;
                                            ImGui::TextColored(
                                                resolved ? ImVec4{ 0.3f,1,0.3f,1 } : ImVec4{ 1,0.3f,0.3f,1 },
                                                "  '%s' -> [%d] %s",
                                                t.BoneName.c_str(), t.BoneIndex,
                                                resolved ? "OK" : "NOT FOUND");
                                        }
                                    }
                                    ImGui::Text("Resolved: %d / %d", ok, ok + ng);
                                    ImGui::TreePop();
                                }
                            }

                            // ボーン行列の値（先頭5本）
                            if (!anim.BoneMatrices.empty() && ImGui::TreeNode("Bone Matrices (first 5)"))
                            {
                                const int n = std::min((int)anim.BoneMatrices.size(), 5);
                                for (int i = 0; i < n; ++i)
                                {
                                    const auto& m = anim.BoneMatrices[i];
                                    const auto& b = res->GetBones()[i];
                                    ImGui::Text("[%d] '%s'", i, b.Name.c_str());
                                    ImGui::Text("  r0=(%.2f %.2f %.2f %.2f)", m._11, m._12, m._13, m._14);
                                    ImGui::Text("  r1=(%.2f %.2f %.2f %.2f)", m._21, m._22, m._23, m._24);
                                    ImGui::Text("  r2=(%.2f %.2f %.2f %.2f)", m._31, m._32, m._33, m._34);
                                    ImGui::Text("  r3=(%.2f %.2f %.2f %.2f)", m._41, m._42, m._43, m._44);
                                }
                                ImGui::TreePop();
                            }

                            ImGui::PopID();
                        }); // each()

                if (entityCount == 0)
                    ImGui::TextColored({ 1,0.5f,0,1 },
                        "No entity with Model + ModelAnimComponent found.");

                ImGui::End();

#endif
            });
    }

} // namespace sys