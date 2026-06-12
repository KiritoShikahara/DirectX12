#pragma once

#include"DataReflection.h"
#include"DataManager.h"

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <memory>
#include <cassert>
#include <string>
#include <vector>
#include <functional>

#ifdef _DEBUG
#include<ImGui/imgui.h>
#endif

#include<Utility/Singleton/Singleton.hpp>

namespace data
{
	class DataRegistry : public utility::Singleton<DataRegistry>
	{
		SINGLETON_CLASS(DataRegistry);
	public:
		SINGLETON_ACCESSOR(DataRegistry);

		/// <summary>
		/// 登録
		/// </summary>
		template<typename T>
		void Register(const std::string& csvPath, const std::string& dbPath);

		/// <summary>
		/// 全体ロード
		/// </summary>
		void LoadAll();

		template<typename T>
		DataManager<T>& GetType();

		/// <summary>
		/// 登録確認
		/// </summary>
		template<typename T>
		bool IsRegistered();


#ifdef _DEBUG
		void DrawImGui()
		{
			// ランチャーウィンドウ：型一覧 + 表示切替チェックボックス
			ImGui::SetNextWindowSize(ImVec2(260.f, 0.f), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Data Registry"))
			{
				ImGui::Text("Registered: %d types", (int)mOrder.size());
				ImGui::Separator();

				for (auto& key : mOrder)
				{
					auto& e = mEntries.at(key);
					ImGui::Checkbox(e.Label.c_str(), &e.WindowOpen);
				}

				ImGui::Separator();
				if (ImGui::Button("Load All"))  LoadAll();
				ImGui::SameLine();
				if (ImGui::Button("Close All"))
					for (auto& [k, e] : mEntries) e.WindowOpen = false;
				ImGui::SameLine();
				if (ImGui::Button("Open All"))
					for (auto& [k, e] : mEntries) e.WindowOpen = true;
			}
			ImGui::End();

			// 各型のエディタウィンドウ
			for (auto& key : mOrder)
			{
				auto& e = mEntries.at(key);
				if (e.WindowOpen && e.DrawFn)
					e.DrawFn();
			}
		}
#endif
	private:
		struct Entry
		{
			// DataManager<T>* を void* で保持し、デリータで型を復元して delete
			std::unique_ptr<void, void(*)(void*)> Ptr{ nullptr, [](void*) {} };
			std::function<void()>                 LoadFn;
			std::function<void()>                 DrawFn;
			std::string                           Label;
			bool                                  WindowOpen = true;
		};

	private:

		std::unordered_map<std::type_index, Entry> mEntries;
		std::vector<std::type_index>               mOrder;
	};

	template<typename T>
	inline void DataRegistry::Register(const std::string& csvPath, const std::string& dbPath)
	{
		const std::type_index key = typeid(T);
		assert(mEntries.find(key) == mEntries.end()
			&& "[DataRegistry] Type already registered.");

		DataManager<T>* raw = new DataManager<T>();
		raw->Initialize(csvPath, dbPath);

		Entry entry;
		entry.Ptr = { raw, [](void* p) { delete static_cast<DataManager<T>*>(p); } };
		entry.LoadFn = [raw]() { raw->Load(); };
		entry.Label = reflect::TypeDescriptor<T>::TableName();

#ifdef _DEBUG
		entry.DrawFn = [raw]() { raw->DrawImGui(); };
#endif
		mEntries.emplace(key, std::move(entry));
		mOrder.push_back(key);
	}

	template<typename T>
	inline DataManager<T>& DataRegistry::GetType()
	{
		const std::type_index key = typeid(T);
		auto it = mEntries.find(key);
		assert(it != mEntries.end()
			&& "[DataRegistry] Type not registered. Call Register<T>() first.");

		return *static_cast<DataManager<T>*>(it->second.Ptr.get());
	}
}