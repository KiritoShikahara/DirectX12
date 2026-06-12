#pragma once

#include"DataReflection.h"
#include"CsvParser.h"
#include"SqliteManager.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <optional>
#include <stdexcept>

#ifdef _DEBUG
#include<ImGui/imgui.h>
#endif

namespace data
{
	struct DuplicateKeyError : std::runtime_error
	{
		int DuplicateId;
		explicit DuplicateKeyError(int id)
			: std::runtime_error("[DataManager] Duplicate primary key: " + std::to_string(id))
			, DuplicateId(id)
		{
		}
	};

	template<typename T>
	class DataManager
	{
	public:


        bool Initialize(const std::string& csvPath, const std::string& dbPath)
        {
            mCsvPath = csvPath;
            mDbPath = dbPath;
            return true;
        }

        void Load()
        {
#ifdef _DEBUG
            LoadFromCsv();
#else
            LoadFromDb();
#endif
        }

        void LoadFromCsv()
        {
            mItems = CsvParser::Load<T>(mCsvPath);
            RebuildIndex();           // 重複チェック + インデックス構築
            SetMessage("[CSV] Loaded " + std::to_string(mItems.size()) + " records.");
        }

        void LoadFromDb()
        {
            EnsureDb();
            mDb->EnsureTable<T>();
            mItems = mDb->LoadAll<T>();
            RebuildIndex();
            SetMessage("[DB] Loaded " + std::to_string(mItems.size()) + " records.");
        }

        // ── Save CSV → DB ─────────────────────────────────────────────
        // 重複チェックを行ってから書き込む。重複があれば DuplicateKeyError をスロー
        void SaveCsvToDb()
        {
            RebuildIndex();           // 書き込み前にもチェック
            EnsureDb();
            mDb->SaveAll<T>(mItems);
            SetMessage("[DB] Saved " + std::to_string(mItems.size()) + " records.");
        }

        // ── アクセサ ──────────────────────────────────────────────────
        std::vector<T>& GetAll() { return mItems; }
        const std::vector<T>& GetAll() const { return mItems; }

        // 線形検索（主キーが無い型・複合条件 向け）
        T* Find(std::function<bool(const T&)> pred)
        {
            for (auto& item : mItems)
                if (pred(item)) return &item;
            return nullptr;
        }

        // 主キー（REFLECT_FIELD_ID）による O(1) 検索
        // 主キーフィールドが未登録の型では常に nullptr を返す
        T* GetById(int id)
        {
            auto it = mIndexById.find(id);
            if (it == mIndexById.end()) return nullptr;
            return it->second;
        }
        const T* GetById(int id) const
        {
            auto it = mIndexById.find(id);
            if (it == mIndexById.end()) return nullptr;
            return it->second;
        }

        // オンデマンド DB 取得（Release 向け省メモリ運用）
        // mItems には追加しない。毎回 DB クエリを発行するため大量呼び出し非推奨
        std::optional<T> FetchById(int id)
        {
            EnsureDb();
            return mDb->template FetchById<T>(id);
        }

        // ── インデックス手動再構築 ────────────────────────────────────
        // mItems を直接変更した後に呼ぶ
        void RebuildIndex()
        {
            mIndexById.clear();
            const int pkFieldIdx = FindPrimaryKeyFieldIndex();
            if (pkFieldIdx < 0) return;   // 主キーフィールド未登録なら何もしない

            const auto& fields = reflect::TypeDescriptor<T>::Fields();

            for (auto& item : mItems)
            {
                reflect::FieldValue fv = fields[pkFieldIdx].GetPtr(&item);
                int* idPtr = std::get_if<int*>(&fv);
                if (!idPtr) continue;

                const int id = **idPtr;
                auto [it, inserted] = mIndexById.emplace(id, &item);
                if (!inserted)
                    throw DuplicateKeyError(id);
            }
        }

#ifdef _DEBUG
        void DrawImGui(const char* windowLabel = nullptr)
        {
            const auto& fields = reflect::TypeDescriptor<T>::Fields();
            const char* table = reflect::TypeDescriptor<T>::TableName();
            const char* label = windowLabel ? windowLabel : table;

            if (!ImGui::Begin(label)) { ImGui::End(); return; }

            // ── ステータスメッセージ ──────────────────────────────────
            if (mMessageTimer > 0.f)
            {
                mMessageTimer -= ImGui::GetIO().DeltaTime;
                const ImVec4 color = mMessageIsError
                    ? ImVec4{ 1.f, 0.3f, 0.3f, 1.f }
                : ImVec4{ 0.3f, 1.f, 0.3f, 1.f };
                ImGui::TextColored(color, "%s", mMessage.c_str());
            }

            // ── ツールバー ────────────────────────────────────────────
            if (ImGui::Button("Load CSV"))
                TryCall([&] { LoadFromCsv(); });
            ImGui::SameLine();
            if (ImGui::Button("Load DB"))
                TryCall([&] { LoadFromDb(); });
            ImGui::SameLine();
            if (ImGui::Button("Save CSV->DB"))
                TryCall([&] { SaveCsvToDb(); });
            ImGui::SameLine();
            if (ImGui::Button("Save to CSV"))
            {
                TryCall([&] {
                    CsvParser::Save<T>(mCsvPath, mItems);
                    SetMessage("[CSV] Saved " + std::to_string(mItems.size()) + " records.");
                    });
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Row"))
            {
                mItems.emplace_back();
                // インデックスを再構築（重複があれば警告表示）
                TryCall([&] { RebuildIndex(); });
            }

            // ── 重複警告インジケーター ────────────────────────────────
            const int pkIdx = FindPrimaryKeyFieldIndex();
            if (pkIdx >= 0)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("(PK: %s)", fields[pkIdx].Name.c_str());
            }

            ImGui::Separator();

            // ── テーブル ──────────────────────────────────────────────
            const int colCount = (int)fields.size() + 1; // +1 for 削除ボタン
            const ImGuiTableFlags flags =
                ImGuiTableFlags_Borders |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_ScrollX |
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_SizingFixedFit;

            const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
            const float tableHeight = rowHeight * (float)std::min((int)mItems.size() + 2, 20);

            if (ImGui::BeginTable("##data", colCount, flags, ImVec2(0.f, tableHeight)))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                for (const auto& f : fields)
                {
                    // 主キーカラムはラベルに "[PK]" を付加
                    if (reflect::HasFlag(f.Flags, reflect::eFieldFlag::PrimaryKey))
                    {
                        std::string header = "[PK] " + f.Name;
                        ImGui::TableSetupColumn(header.c_str(),
                            ImGuiTableColumnFlags_WidthFixed, 80.f);
                    }
                    else
                    {
                        ImGui::TableSetupColumn(f.Name.c_str(),
                            ImGuiTableColumnFlags_WidthFixed, 100.f);
                    }
                }
                ImGui::TableSetupColumn("##del", ImGuiTableColumnFlags_WidthFixed, 30.f);
                ImGui::TableHeadersRow();

                // 重複している ID を収集（行ハイライト用）
                std::unordered_map<int, int> idCount;
                if (pkIdx >= 0)
                {
                    for (auto& item : mItems)
                    {
                        reflect::FieldValue fv = fields[pkIdx].GetPtr(&item);
                        if (int** pp = std::get_if<int*>(&fv)) idCount[**pp]++;
                    }
                }

                int deleteIdx = -1;
                for (int row = 0; row < (int)mItems.size(); ++row)
                {
                    ImGui::TableNextRow();
                    ImGui::PushID(row);

                    // 重複行を赤くハイライト
                    bool isDuplicate = false;
                    if (pkIdx >= 0)
                    {
                        reflect::FieldValue fv = fields[pkIdx].GetPtr(&mItems[row]);
                        if (int** pp = std::get_if<int*>(&fv))
                            isDuplicate = (idCount[**pp] > 1);
                    }
                    if (isDuplicate)
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                            IM_COL32(160, 40, 40, 100));

                    for (int col = 0; col < (int)fields.size(); ++col)
                    {
                        ImGui::TableSetColumnIndex(col);
                        DrawFieldWidget(fields[col], &mItems[row], row, col);
                    }

                    ImGui::TableSetColumnIndex((int)fields.size());
                    if (ImGui::SmallButton("x"))
                        deleteIdx = row;

                    ImGui::PopID();
                }

                if (deleteIdx >= 0)
                {
                    mItems.erase(mItems.begin() + deleteIdx);
                    TryCall([&] { RebuildIndex(); });
                }

                ImGui::EndTable();
            }

            ImGui::Text("Total: %d rows", (int)mItems.size());
            ImGui::End();
        }

    private:
        // フィールド1個分のウィジェット
        // 編集後に RebuildIndex を呼んで重複をリアルタイム検出
        void DrawFieldWidget(const reflect::FieldInfo& field, T* item, int row, int col)
        {
            char id[32];
            snprintf(id, sizeof(id), "##r%dc%d", row, col);

            reflect::FieldValue ptr = field.GetPtr(item);
            bool changed = false;

            std::visit([&](auto* p)
                {
                    using P = std::decay_t<decltype(*p)>;
                    ImGui::SetNextItemWidth(-1.f);

                    if constexpr (std::is_same_v<P, int>)
                        changed = ImGui::InputInt(id, p);
                    else if constexpr (std::is_same_v<P, float>)
                        changed = ImGui::InputFloat(id, p, 0.f, 0.f, "%.3f");
                    else if constexpr (std::is_same_v<P, bool>)
                        changed = ImGui::Checkbox(id, p);
                    else if constexpr (std::is_same_v<P, std::string>)
                    {
                        constexpr int kBufSize = 256;
                        char buf[kBufSize];
                        strncpy_s(buf, p->c_str(), kBufSize - 1);
                        if (ImGui::InputText(id, buf, kBufSize))
                        {
                            *p = buf;
                            changed = true;
                        }
                    }
                }, ptr);

            // 主キーが変更されたらインデックスを即再構築
            if (changed && reflect::HasFlag(field.Flags, reflect::eFieldFlag::PrimaryKey))
                TryCall([&] { RebuildIndex(); });
        }

        // 例外をキャッチしてエラーメッセージに変換するヘルパー
        void TryCall(std::function<void()> fn)
        {
            try { fn(); }
            catch (const DuplicateKeyError& e)
            {
                SetMessage(e.what(), /*isError=*/true);
            }
            catch (const std::exception& e)
            {
                SetMessage(e.what(), /*isError=*/true);
            }
        }
#endif // _DEBUG

	private:
		static int FindPrimaryKeyFieldIndex()
		{
			const auto& fields = reflect::TypeDescriptor<T>::Fields();
			for (int i = 0; i < (int)fields.size(); ++i)
				if (reflect::HasFlag(fields[i].Flags, reflect::eFieldFlag::PrimaryKey))
					return i;
			return -1;
		}

		void EnsureDb()
		{
			if (!mDb)
				mDb = std::make_unique<SqliteManager>(mDbPath);
		}

		void SetMessage(std::string msg, bool isError = false)
		{
			mMessage = std::move(msg);
			mMessageTimer = kMessageDuration;
			mMessageIsError = isError;
		}

	private:
		std::vector<T>                    mItems;
		std::unordered_map<int, T*>       mIndexById;
		std::string                       mCsvPath;
		std::string                       mDbPath;
		std::unique_ptr<SqliteManager>    mDb;

		std::string mMessage;
		float       mMessageTimer = 0.f;
		bool        mMessageIsError = false;
		static constexpr float kMessageDuration = 4.f;
	};
}