#pragma once

#include"../Registry/DataRegistry.h"
#include"../Registry/ConfigRegistry.h"
#include"../Reflection.h"
#include<ImGui/imgui.h>
#include<string>
#include<unordered_map>
#include<functional>
#include<stdexcept>

namespace data
{
    /// <summary>
    /// ImGuiEditVisitor  （ImGui ウィジェットでフィールドを編集）
    /// </summary>
    class ImGuiEditVisitor final : public IFieldVisitor
    {
    public:
        bool  AnyChanged = false;
        bool  PkChanged = false;
        int   Row = 0;
        int   Col = 0;

        void OnInt(const std::string& name, int& v, eFieldFlag flags) override
        {
            char id[32]; snprintf(id, sizeof(id), "##r%dc%d", Row, Col++);
            ImGui::SetNextItemWidth(-1.f);
            if (ImGui::InputInt(id, &v)) { AnyChanged = true; if (HasFlag(flags, eFieldFlag::PrimaryKey)) PkChanged = true; }
        }
        void OnFloat(const std::string&, float& v, eFieldFlag) override
        {
            char id[32]; snprintf(id, sizeof(id), "##r%dc%d", Row, Col++);
            ImGui::SetNextItemWidth(-1.f);
            if (ImGui::InputFloat(id, &v, 0.f, 0.f, "%.3f")) AnyChanged = true;
        }
        void OnBool(const std::string&, bool& v, eFieldFlag) override
        {
            char id[32]; snprintf(id, sizeof(id), "##r%dc%d", Row, Col++);
            if (ImGui::Checkbox(id, &v)) AnyChanged = true;
        }
        void OnString(const std::string&, std::string& v, eFieldFlag) override
        {
            char id[32]; snprintf(id, sizeof(id), "##r%dc%d", Row, Col++);
            constexpr int kBuf = 256;
            char buf[kBuf]; strncpy_s(buf, v.c_str(), kBuf - 1);
            ImGui::SetNextItemWidth(-1.f);
            if (ImGui::InputText(id, buf, kBuf)) { v = buf; AnyChanged = true; }
        }
    };

    /// <summary>
    /// ImGuiDragVisitor  （ConfigManager 向け DragInt/DragFloat）
    /// </summary>
    class ImGuiDragVisitor final : public IFieldVisitor
    {
    public:
        bool AnyChanged = false;

        void OnInt(const std::string& name, int& v, eFieldFlag) override
        {
            if (ImGui::DragInt(name.c_str(), &v)) AnyChanged = true;
        }
        void OnFloat(const std::string& name, float& v, eFieldFlag) override
        {
            if (ImGui::DragFloat(name.c_str(), &v, 0.01f)) AnyChanged = true;
        }
        void OnBool(const std::string& name, bool& v, eFieldFlag) override
        {
            if (ImGui::Checkbox(name.c_str(), &v)) AnyChanged = true;
        }
        void OnString(const std::string& name, std::string& v, eFieldFlag) override
        {
            constexpr int kBuf = 256;
            char buf[kBuf]; strncpy_s(buf, v.c_str(), kBuf - 1);
            if (ImGui::InputText(name.c_str(), buf, kBuf)) { v = buf; AnyChanged = true; }
        }
    };


    /// <summary>
    /// ImGuiのテーブルエディタ
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template<typename T>
    class DataInspector
    {
    public:
        explicit DataInspector(DataManager<T>& mgr) : mMgr(mgr) {}

        void Draw(const char* windowLabel = nullptr)
        {
            const auto& fields = TypeDescriptor<T>::Fields();
            const char* label = windowLabel ? windowLabel : TypeDescriptor<T>::TableName();

            if (!ImGui::Begin(label)) { ImGui::End(); return; }

            // ステータス（mLastMessage はRelease でも保持されているため安全）
            if (!mMgr.GetLastMessage().empty())
                ImGui::TextColored({ 0.3f, 1.f, 0.3f, 1.f }, "%s", mMgr.GetLastMessage().c_str());

            // ツールバー
            if (ImGui::Button("Load CSV"))     TryCall([&] { mMgr.LoadFromCsv(); });
            ImGui::SameLine();
            if (ImGui::Button("Load DB"))      TryCall([&] { mMgr.LoadFromDb(); });
            ImGui::SameLine();
            if (ImGui::Button("Save CSV->DB")) TryCall([&] { mMgr.SaveCsvToDb(); });
            ImGui::SameLine();
            if (ImGui::Button("Save to CSV"))  TryCall([&] { mMgr.SaveToCsv(); });
            ImGui::SameLine();
            if (ImGui::Button("Add Row"))
            {
                mMgr.GetAll().emplace_back();
                TryCall([&] { mMgr.RebuildIndex(); });
            }

            ImGui::Separator();

            // テーブル
            const int colCount = (int)fields.size() + 1;
            const ImGuiTableFlags flags =
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX |
                ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit;
            const float tableHeight = ImGui::GetTextLineHeightWithSpacing()
                * (float)std::min((int)mMgr.GetAll().size() + 2, 20);

            if (ImGui::BeginTable("##data", colCount, flags, ImVec2(0.f, tableHeight)))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                for (const auto& f : fields)
                {
                    std::string header = HasFlag(f.Flags, eFieldFlag::PrimaryKey) ? "[PK] " + f.Name : f.Name;
                    ImGui::TableSetupColumn(header.c_str(), ImGuiTableColumnFlags_WidthFixed,
                        HasFlag(f.Flags, eFieldFlag::PrimaryKey) ? 80.f : 100.f);
                }
                ImGui::TableSetupColumn("##del", ImGuiTableColumnFlags_WidthFixed, 30.f);
                ImGui::TableHeadersRow();

                // 重複 ID 収集（行ハイライト用）
                std::unordered_map<int, int> idCount;
                for (auto& item : mMgr.GetAll())
                {
                    PkExtractVisitor vis; VisitFields(item, vis);
                    if (vis.Found) idCount[vis.Value]++;
                }

                int deleteIdx = -1;
                auto& items = mMgr.GetAll();
                for (int row = 0; row < (int)items.size(); ++row)
                {
                    ImGui::TableNextRow();
                    ImGui::PushID(row);

                    // 重複行ハイライト
                    PkExtractVisitor pkVis; VisitFields(items[row], pkVis);
                    if (pkVis.Found && idCount[pkVis.Value] > 1)
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(160, 40, 40, 100));

                    // 各フィールドのウィジェット
                    ImGuiEditVisitor editVis;
                    editVis.Row = row;
                    for (int col = 0; col < (int)fields.size(); ++col)
                    {
                        ImGui::TableSetColumnIndex(col);
                        editVis.Col = col;
                        fields[col].Accept(&items[row], editVis);
                    }
                    if (editVis.PkChanged)
                        TryCall([&] { mMgr.RebuildIndex(); });

                    ImGui::TableSetColumnIndex((int)fields.size());
                    if (ImGui::SmallButton("x")) deleteIdx = row;
                    ImGui::PopID();
                }

                if (deleteIdx >= 0)
                {
                    items.erase(items.begin() + deleteIdx);
                    TryCall([&] { mMgr.RebuildIndex(); });
                }
                ImGui::EndTable();
            }

            ImGui::Text("Total: %d rows", (int)mMgr.GetAll().size());
            ImGui::End();
        }

    private:
        void TryCall(std::function<void()> fn)
        {
            try { fn(); }
            catch (const std::exception& e)
            {
                ImGui::SetTooltip("Error: %s", e.what());
            }
        }

        DataManager<T>& mMgr;
    };

    /// <summary>
    /// ConfigManager<T> の ImGui キーバリューエディタ
    /// </summary>
    template<typename T>
    class ConfigEditor
    {
    public:
        explicit ConfigEditor(ConfigManager<T>& mgr) : mMgr(mgr) {}

        void Draw(const char* windowLabel = nullptr)
        {
            const char* label = windowLabel ? windowLabel : TypeDescriptor<T>::TableName();
            if (!ImGui::Begin(label)) { ImGui::End(); return; }

            if (!mMgr.GetLastMessage().empty())
                ImGui::TextColored({ 0.3f, 1.f, 0.3f, 1.f }, "%s", mMgr.GetLastMessage().c_str());
            if (mMgr.IsDirty())
                ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f }, "* Unsaved changes");

            if (ImGui::Button("Load"))  mMgr.Load();
            ImGui::SameLine();
            if (ImGui::Button("Save"))  mMgr.Save();
            ImGui::SameLine();
            if (ImGui::Button("Reset")) mMgr.Reset();
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", mMgr.GetFilePath().c_str());
            ImGui::Separator();

            ImGuiDragVisitor visitor;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 200.f);
            VisitFields(mMgr.Get(), visitor);
            if (visitor.AnyChanged) mMgr.NotifyChanged();

            ImGui::End();
        }

    private:
        ConfigManager<T>& mMgr;
    };

    /// <summary>
    ///  DataRegistryInspector  （DataRegistry 全体のランチャーウィンドウ）
    /// </summary>
    class DataRegistryInspector
    {
    public:
        void Draw()
        {
            auto& reg = DataRegistry::Get();

            ImGui::SetNextWindowSize(ImVec2(260.f, 0.f), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Data Registry"))
            {
                const auto labels = reg.GetLabels();
                ImGui::Text("Registered: %d types", (int)labels.size());
                ImGui::Separator();

                for (int i = 0; i < (int)labels.size(); ++i)
                    ImGui::Checkbox(labels[i].c_str(), &mWindowOpen[i]);

                ImGui::Separator();
                if (ImGui::Button("Load All")) reg.LoadAll();
            }
            ImGui::End();
        }

    private:
        bool mWindowOpen[64] = {};   // 最大64型まで対応（必要なら動的化可）
    };

    /// <summary>
    /// ConfigRegistryEditor  （ConfigRegistry 全体のランチャーウィンドウ）
    /// </summary>
    class ConfigRegistryEditor
    {
    public:
        void Draw()
        {
            auto& reg = ConfigRegistry::Get();

            ImGui::SetNextWindowSize(ImVec2(260.f, 0.f), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Config Registry"))
            {
                const auto labels = reg.GetLabels();
                ImGui::Text("Registered: %d types", (int)labels.size());
                ImGui::Separator();

                for (int i = 0; i < (int)labels.size(); ++i)
                    ImGui::Checkbox(labels[i].c_str(), &mWindowOpen[i]);

                ImGui::Separator();
                if (ImGui::Button("Load All"))  reg.LoadAll();
                ImGui::SameLine();
                if (ImGui::Button("Save All"))  reg.SaveAll();
            }
            ImGui::End();
        }

    private:
        bool mWindowOpen[64] = {};
    };
}