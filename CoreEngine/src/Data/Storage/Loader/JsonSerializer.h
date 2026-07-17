#pragma once

#include"../Reflection.h"
#include<json/json.hpp>
#include<string>
#include<fstream>
#include<filesystem>
#include<stdexcept>

namespace data
{
    /// <summary>
    /// JsonSerializeVisitor  （構造体 → JSON）
    /// </summary>
    class JsonSerializeVisitor final : public IFieldVisitor
    {
    public:
        explicit JsonSerializeVisitor(nlohmann::json& j) : mJson(j) {}
        void OnInt(const std::string& name, int& v, eFieldFlag) override { mJson[name] = v; }
        void OnFloat(const std::string& name, float& v, eFieldFlag) override { mJson[name] = v; }
        void OnBool(const std::string& name, bool& v, eFieldFlag) override { mJson[name] = v; }
        void OnString(const std::string& name, std::string& v, eFieldFlag) override { mJson[name] = v; }
    private:
        nlohmann::json& mJson;
    };

    /// <summary>
    /// JsonDeserializeVisitor  （JSON → 構造体）
    /// 存在しないキーはスキップ（部分更新・後方互換対応）
    /// </summary>
    class JsonDeserializeVisitor final : public IFieldVisitor
    {
    public:
        explicit JsonDeserializeVisitor(const nlohmann::json& j) : mJson(j) {}

        void OnInt(const std::string& name, int& v, eFieldFlag) override
        {
            if (mJson.contains(name) && mJson[name].is_number_integer()) v = mJson[name].get<int>();
        }

        void OnFloat(const std::string& name, float& v, eFieldFlag) override
        {
            if (mJson.contains(name) && mJson[name].is_number()) v = mJson[name].get<float>();
        }

        void OnBool(const std::string& name, bool& v, eFieldFlag) override
        {
            if (mJson.contains(name) && mJson[name].is_boolean()) v = mJson[name].get<bool>();
        }

        void OnString(const std::string& name, std::string& v, eFieldFlag) override
        {
            if (mJson.contains(name) && mJson[name].is_string()) v = mJson[name].get<std::string>();
        }

    private:
        const nlohmann::json& mJson;
    };

    /// <summary>
    /// JsonSerializer  （ファイルI/O）
    /// </summary>
    class JsonSerializer
    {
    public:
        template<typename T>
        static nlohmann::json Serialize(const T& obj)
        {
            nlohmann::json j;
            JsonSerializeVisitor visitor(j);
            VisitFields(obj, visitor);
            return j;
        }

        template<typename T>
        static void Deserialize(const nlohmann::json& j, T& out)
        {
            JsonDeserializeVisitor visitor(j);
            VisitFields(out, visitor);
        }

        template<typename T>
        static void SaveToFile(const std::string& filePath, const T& obj, int indent = 4)
        {
            // std::ofstreamは中間ディレクトリを自動作成しないため、
            // 保存先フォルダが存在しない場合(初回実行・フォルダの誤削除等)に
            // is_open()==falseとなり例外が飛んでいた。書き込み前に必ず作成しておく。
            const std::filesystem::path path(filePath);
            const std::filesystem::path parent = path.parent_path();
            if (!parent.empty())
            {
                std::filesystem::create_directories(parent);
            }

            std::ofstream f(filePath);
            if (!f.is_open())
                throw std::runtime_error("[JsonSerializer] Failed to write: " + filePath);
            f << Serialize(obj).dump(indent);
        }

        template<typename T>
        static bool LoadFromFile(const std::string& filePath, T& out)
        {
            std::ifstream f(filePath);
            if (!f.is_open()) return false;
            try
            {
                Deserialize(nlohmann::json::parse(f), out);
                return true;
            }
            catch (const nlohmann::json::exception& e)
            {
                throw std::runtime_error(
                    std::string("[JsonSerializer] Parse error in ") + filePath + ": " + e.what());
            }
        }
    };
}