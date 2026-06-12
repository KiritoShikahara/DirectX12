#pragma once

#include<json/json.hpp>
#include"DataReflection.h"

#include <string>
#include <fstream>
#include <stdexcept>

namespace data
{
	/// <summary>
	///  JsonSerializer
	///  reflect::TypeDescriptor<T> の情報を使って
	/// </summary>
	class JsonSerializer
	{
	public:
        /// <summary>
        /// 構造体からJSONオブジェクト
        /// </summary>
        template<typename T>
		static nlohmann::json Serialize(const T& obj);

        /// <summary>
        /// JSONオブジェクトから構造体
        /// </summary>
        template<typename T>
        static void Deserialize(const nlohmann::json& j, T& out);

        /// <summary>
        /// ファイル書き出し
        /// </summary>
        template<typename T>
        static void SaveToFile(const std::string& filePath, const T& obj, int indent = 4);

        /// <summary>
        /// ファイル読み込み
        /// </summary>
        template<typename T>
        static bool LoadFromFile(const std::string& filePath, T& out);

	};

	template<typename T>
	inline nlohmann::json JsonSerializer::Serialize(const T& obj)
	{
        nlohmann::json j;
        for (const auto& field : reflect::TypeDescriptor<T>::Fields())
        {
            reflect::FieldValue ptr = field.GetPtr(const_cast<T*>(&obj));
            std::visit([&](auto* p)
                {
                    j[field.Name] = *p;
                }, ptr);
        }
        return j;
    }

    template<typename T>
    inline void JsonSerializer::Deserialize(const nlohmann::json& j, T& out)
    {
        for (const auto& field : reflect::TypeDescriptor<T>::Fields())
        {
            if (!j.contains(field.Name)) continue;   // キーが無ければスキップ

            reflect::FieldValue ptr = field.GetPtr(&out);
            const auto& jv = j[field.Name];

            std::visit([&](auto* p)
                {
                    using P = std::decay_t<decltype(*p)>;
                    if constexpr (std::is_same_v<P, int>)
                    {
                        if (jv.is_number_integer()) *p = jv.get<int>();
                    }
                    else if constexpr (std::is_same_v<P, float>)
                    {
                        if (jv.is_number()) *p = jv.get<float>();
                    }
                    else if constexpr (std::is_same_v<P, bool>)
                    {
                        if (jv.is_boolean()) *p = jv.get<bool>();
                    }
                    else if constexpr (std::is_same_v<P, std::string>)
                    {
                        if (jv.is_string()) *p = jv.get<std::string>();
                    }
                }, ptr);
        }
    }

    template<typename T>
    inline void JsonSerializer::SaveToFile(const std::string& filePath, const T& obj, int indent)
    {
        std::ofstream f(filePath);
        if (!f.is_open())
            throw std::runtime_error("[JsonSerializer] Failed to write: " + filePath);
        f << Serialize(obj).dump(indent);
    }
    template<typename T>
    inline bool JsonSerializer::LoadFromFile(const std::string& filePath, T& out)
    {
        std::ifstream f(filePath);
        if (!f.is_open()) return false;     // ファイル不在 = 初回起動扱い

        try
        {
            nlohmann::json j = nlohmann::json::parse(f);
            Deserialize(j, out);
            return true;
        }
        catch (const nlohmann::json::exception& e)
        {
            throw std::runtime_error(
                std::string("[JsonSerializer] JSON parse error in ") + filePath + ": " + e.what());
        }
    }
}