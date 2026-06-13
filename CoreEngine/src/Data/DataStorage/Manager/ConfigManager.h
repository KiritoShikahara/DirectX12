#pragma once

#include"JsonSerializer.h"
#include"DataReflection.h"

#include <string>
#include <memory>
#include <functional>

#ifdef _DEBUG
#include<ImGui/imgui.h>
#endif

namespace data
{
	/// <summary>
	/// JSONファイルへの自動Save/LoadおよびImGui編集UIを提供する軽量コンフィグ管理クラス
	/// </summary>
	/// <remarks>
	/// ゲーム設定やグラフィック設定など「1構造体 = 1ファイル」の運用向け。
	/// DataManager（CSV/DB）系統とは独立。
	/// </remarks>
	template<typename T>
	class ConfigManager
	{
	public:
		/// <summary>
		/// 管理する設定ファイルのパスを指定して初期化します
		/// </summary>
		explicit ConfigManager(std::string filePath)
			: mFilePath(std::move(filePath))
		{
		}

		/// <summary>
		/// ファイルから設定データをロードします。ファイルが存在しない場合はデフォルト値のまま進行します
		/// </summary>
		bool Load()
		{
			const bool loaded = JsonSerializer::LoadFromFile(mFilePath, mData);
			if (loaded)
				SetMessage("[Config] Loaded: " + mFilePath);
			else
				SetMessage("[Config] File not found, using defaults: " + mFilePath);
			mDirty = false;
			return loaded;
		}

		/// <summary>
		/// 現在の設定データをファイルに保存します
		/// </summary>
		void Save()
		{
			JsonSerializer::SaveToFile(mFilePath, mData);
			SetMessage("[Config] Saved: " + mFilePath);
			mDirty = false;
		}

		// --- アクセサ ---

		/// <summary>
		/// 設定データの参照を取得します
		/// </summary>
		T& Get() { return mData; }

		/// <summary>
		/// 設定データの定数参照を取得します
		/// </summary>
		const T& Get() const { return mData; }

		/// <summary>
		/// 未保存の変更（ダーティフラグ）があるかどうかを取得します
		/// </summary>
		bool     IsDirty() const { return mDirty; }

		/// <summary>
		/// 値が変更されたタイミングで呼び出されるコールバック関数を登録します
		/// </summary>
		void SetOnChanged(std::function<void(const T&)> cb) { mOnChanged = std::move(cb); }

#ifdef _DEBUG
		/// <summary>
		/// デバッグ用のImGui設定編集ウィンドウを描画します
		/// </summary>
		void DrawImGui(const char* windowLabel = nullptr)
		{
			const auto& fields = reflect::TypeDescriptor<T>::Fields();
			const char* table = reflect::TypeDescriptor<T>::TableName();
			const char* label = windowLabel ? windowLabel : table;

			if (!ImGui::Begin(label)) { ImGui::End(); return; }

			// ステータスメッセージ
			if (mMessageTimer > 0.f)
			{
				mMessageTimer -= ImGui::GetIO().DeltaTime;
				ImGui::TextColored({ 0.3f, 1.f, 0.3f, 1.f }, "%s", mMessage.c_str());
			}

			// ダーティマーク
			if (mDirty)
				ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f }, "* Unsaved changes");

			// ツールバー
			if (ImGui::Button("Load"))   Load();
			ImGui::SameLine();
			if (ImGui::Button("Save"))   Save();
			ImGui::SameLine();
			if (ImGui::Button("Reset"))
			{
				mData = T{};           // デフォルト値にリセット
				mDirty = true;
				NotifyChanged();
			}

			ImGui::SameLine();
			ImGui::TextDisabled("(%s)", mFilePath.c_str());
			ImGui::Separator();

			// フィールド一覧（キーバリュー形式で縦リストに表示）
			constexpr float kLabelWidth = 200.f;

			for (const auto& field : fields)
			{
				ImGui::PushID(field.Name.c_str());

				reflect::FieldValue ptr = field.GetPtr(&mData);
				bool changed = false;

				std::visit([&](auto* p)
					{
						using P = std::decay_t<decltype(*p)>;
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - kLabelWidth);

						if constexpr (std::is_same_v<P, int>)
							changed = ImGui::DragInt(field.Name.c_str(), p);
						else if constexpr (std::is_same_v<P, float>)
							changed = ImGui::DragFloat(field.Name.c_str(), p, 0.01f);
						else if constexpr (std::is_same_v<P, bool>)
							changed = ImGui::Checkbox(field.Name.c_str(), p);
						else if constexpr (std::is_same_v<P, std::string>)
						{
							constexpr int kBufSize = 256;
							char buf[kBufSize];
							strncpy_s(buf, p->c_str(), kBufSize - 1);
							if (ImGui::InputText(field.Name.c_str(), buf, kBufSize))
							{
								*p = buf;
								changed = true;
							}
						}
					}, ptr);

				if (changed)
				{
					mDirty = true;
					NotifyChanged();
				}

				ImGui::PopID();
			}

			ImGui::End();
		}
#endif // _DEBUG

	private:
		void NotifyChanged()
		{
			if (mOnChanged) mOnChanged(mData);
		}

		void SetMessage(std::string msg)
		{
			mMessage = std::move(msg);
			mMessageTimer = kMessageDuration;
		}

		T                             mData{};
		std::string                   mFilePath;
		std::function<void(const T&)> mOnChanged;
		bool                          mDirty = false;

		std::string mMessage;
		float       mMessageTimer = 0.f;
		static constexpr float kMessageDuration = 3.f;
	};
}


