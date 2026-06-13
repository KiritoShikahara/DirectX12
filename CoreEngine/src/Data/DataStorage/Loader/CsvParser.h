#pragma once
#include"../DataReflection.h"

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

namespace data
{
    class CsvParser
    {
    public:
        /// <summary>
        /// 読み込み
        /// </summary>
        template<typename T>
        static std::vector<T> Load(const std::string& filePath);

        /// <summary>
        /// 書き込み
        /// </summary>
        template<typename T>
        static void Save(const std::string& filePath, const std::vector<T>& items);

    private:
        /// <summary>
        /// CSV分割
        /// </summary>
        static std::vector<std::string> SplitCsv(const std::string& line);

        static std::string Trim(const std::string& s);

    };



    template<typename T>
    inline std::vector<T> CsvParser::Load(const std::string& filePath)
    {
        const auto& fields = reflect::TypeDescriptor<T>::Fields();

        std::ifstream file(filePath);
        if (!file.is_open())
            throw std::runtime_error("[CsvParser] Failed to open: " + filePath);

        std::vector<T> result;
        std::string line;

        // ヘッダー行を読んでカラム順を解決
        if (!std::getline(file, line))
            return result;

        std::vector<std::string> headers = SplitCsv(line);

        // ヘッダー名 → フィールドインデックス のマップ
        std::vector<int> colToField(headers.size(), -1);
        for (int col = 0; col < (int)headers.size(); ++col)
        {
            const std::string h = Trim(headers[col]);
            for (int fi = 0; fi < (int)fields.size(); ++fi)
            {
                if (fields[fi].Name == h)
                {
                    colToField[col] = fi;
                    break;
                }
            }
        }

        // データ行
        while (std::getline(file, line))
        {
            if (line.empty()) continue;

            std::vector<std::string> cols = SplitCsv(line);
            T item{};

            for (int col = 0; col < (int)cols.size() && col < (int)colToField.size(); ++col)
            {
                const int fi = colToField[col];
                if (fi < 0) continue;

                reflect::FieldValue ptr = fields[fi].GetPtr(&item);
                const std::string   val = Trim(cols[col]);

                std::visit([&](auto* p)
                    {
                        using P = std::decay_t<decltype(*p)>;
                        if constexpr (std::is_same_v<P, int>)
                            *p = val.empty() ? 0 : std::stoi(val);
                        else if constexpr (std::is_same_v<P, float>)
                            *p = val.empty() ? 0.f : std::stof(val);
                        else if constexpr (std::is_same_v<P, bool>)
                            *p = (val == "1" || val == "true" || val == "True");
                        else if constexpr (std::is_same_v<P, std::string>)
                            *p = val;
                    }, ptr);
            }

            result.push_back(std::move(item));
        }

        return result;
    }

    template<typename T>
    inline void CsvParser::Save(const std::string& filePath, const std::vector<T>& items)
    {
        const auto& fields = reflect::TypeDescriptor<T>::Fields();

        std::ofstream file(filePath);
        if (!file.is_open())
            throw std::runtime_error("[CsvParser] Failed to write: " + filePath);

        // ヘッダー
        for (int i = 0; i < (int)fields.size(); ++i)
        {
            if (i > 0) file << ',';
            file << fields[i].Name;
        }
        file << '\n';

        // データ
        for (const T& item : items)
        {
            for (int i = 0; i < (int)fields.size(); ++i)
            {
                if (i > 0) file << ',';

                reflect::FieldValue ptr = fields[i].GetPtr(const_cast<T*>(&item));
                std::visit([&](auto* p)
                    {
                        using P = std::decay_t<decltype(*p)>;
                        if constexpr (std::is_same_v<P, bool>)
                        {
                            file << (*p ? "1" : "0");
                        }
                        else if constexpr (std::is_same_v<P, std::string>)
                        {
                            std::string s = *p;
                            bool needsQuote = (s.find(',') != std::string::npos ||
                                s.find('\n') != std::string::npos ||
                                s.find('\r') != std::string::npos ||
                                s.find('"') != std::string::npos);

                            size_t pos = 0;
                            while ((pos = s.find('"', pos)) != std::string::npos)
                            {
                                s.replace(pos, 1, "\"\"");
                                pos += 2;
                            }

                            if (needsQuote)
                                file << '"' << s << '"';
                            else
                                file << s;
                        }
                        else
                        {
                            file << *p;
                        }
                    }, ptr);
            }
            file << '\n';
        }
    }
}