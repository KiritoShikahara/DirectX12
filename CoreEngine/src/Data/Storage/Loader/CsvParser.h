#pragma once
#include"../Reflection.h"
#include<string>
#include<vector>
#include<fstream>
#include<sstream>
#include<stdexcept>

namespace data
{
    /// <summary>
    /// 読み込み抽象化用
    /// </summary>
    class CsvDeserializeVisitor final : public IFieldVisitor
    {
    public:
        explicit CsvDeserializeVisitor(std::string value) : mValue(std::move(value)) {}

        void OnInt(const std::string&, int& v, eFieldFlag) override { v = mValue.empty() ? 0 : std::stoi(mValue); }
        void OnFloat(const std::string&, float& v, eFieldFlag) override { v = mValue.empty() ? 0.f : std::stof(mValue); }
        void OnBool(const std::string&, bool& v, eFieldFlag) override { v = (mValue == "1" || mValue == "true" || mValue == "True"); }
        void OnString(const std::string&, std::string& v, eFieldFlag) override { v = mValue; }

    private:
        std::string mValue;   // 値で保持（一時オブジェクトのダングリング参照を防ぐ）
    };

    /// <summary>
    /// 書き込み抽象化
    /// </summary>
    class CsvSerializeVisitor final : public IFieldVisitor
    {
    public:
        explicit CsvSerializeVisitor(std::ostream& out) : mOut(out) {}

        void OnInt(const std::string&, int& v, eFieldFlag) override { mOut << v; }
        void OnFloat(const std::string&, float& v, eFieldFlag) override { mOut << v; }
        void OnBool(const std::string&, bool& v, eFieldFlag) override { mOut << (v ? "1" : "0"); }
        void OnString(const std::string&, std::string& v, eFieldFlag) override
        {
            // RFC 4180: " → "" エスケープ、特殊文字を含む場合はクォートで囲む
            std::string s = v;
            const bool needsQuote = (s.find(',') != std::string::npos ||
                s.find('\n') != std::string::npos ||
                s.find('\r') != std::string::npos ||
                s.find('"') != std::string::npos);
            size_t pos = 0;
            while ((pos = s.find('"', pos)) != std::string::npos)
            {
                s.replace(pos, 1, "\"\""); pos += 2;
            }

            if (needsQuote) mOut << '"' << s << '"';
            else            mOut << s;
        }

    private:
        std::ostream& mOut;
    };

    /// <summary>
    /// CSV読み込み
    /// </summary>
    class CsvParser
    {
    public:
        template<typename T>
        static std::vector<T> Load(const std::string& filePath)
        {
            const auto& fields = TypeDescriptor<T>::Fields();

            std::ifstream file(filePath);
            if (!file.is_open())
                throw std::runtime_error("[CsvParser] Failed to open: " + filePath);

            std::vector<T> result;
            std::string line;
            if (!std::getline(file, line)) return result;

            // UTF-8 BOM (EF BB BF) を除去
            // Excel で「CSV UTF-8（コンマ区切り）」として保存すると先頭に付与される。
            // 除去しないとヘッダー1列目の名前マッチングが必ず失敗する。
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF)
            {
                line.erase(0, 3);
            }

            // ヘッダー名 → フィールドインデックス
            std::vector<std::string> headers = SplitCsv(line);
            std::vector<int> colToField(headers.size(), -1);
            for (int col = 0; col < (int)headers.size(); ++col)
            {
                const std::string h = Trim(headers[col]);
                for (int fi = 0; fi < (int)fields.size(); ++fi)
                    if (fields[fi].Name == h) { colToField[col] = fi; break; }
            }

            while (std::getline(file, line))
            {
                if (line.empty()) continue;
                std::vector<std::string> cols = SplitCsv(line);
                T item{};

                for (int col = 0; col < (int)cols.size() && col < (int)colToField.size(); ++col)
                {
                    const int fi = colToField[col];
                    if (fi < 0) continue;
                    CsvDeserializeVisitor visitor(Trim(cols[col]));
                    fields[fi].Accept(&item, visitor);
                }
                result.push_back(std::move(item));
            }
            return result;
        }

        template<typename T>
        static void Save(const std::string& filePath, const std::vector<T>& items)
        {
            const auto& fields = TypeDescriptor<T>::Fields();

            std::ofstream file(filePath);
            if (!file.is_open())
                throw std::runtime_error("[CsvParser] Failed to write: " + filePath);

            // ヘッダー
            for (int i = 0; i < (int)fields.size(); ++i)
            {
                if (i > 0) file << ','; file << fields[i].Name;
            }
            file << '\n';

            // データ
            for (const T& item : items)
            {
                for (int i = 0; i < (int)fields.size(); ++i)
                {
                    if (i > 0) file << ',';
                    CsvSerializeVisitor visitor(file);
                    fields[i].Accept(const_cast<T*>(&item), visitor);
                }
                file << '\n';
            }
        }

    private:
        static std::vector<std::string> SplitCsv(const std::string& line)
        {
            std::vector<std::string> result;
            std::string token;
            bool inQuote = false;
            for (size_t i = 0; i < line.size(); ++i)
            {
                const char c = line[i];
                if (inQuote)
                {
                    if (c == '"' && i + 1 < line.size() && line[i + 1] == '"')
                    {
                        token += '"'; ++i;
                    }
                    else if (c == '"') { inQuote = false; }
                    else { token += c; }
                }
                else
                {
                    if (c == '"') { inQuote = true; }
                    else if (c == ',') { result.push_back(token); token.clear(); }
                    else { token += c; }
                }
            }
            result.push_back(token);
            return result;
        }

        static std::string Trim(const std::string& s)
        {
            const size_t start = s.find_first_not_of(" \t\r\n");
            const size_t end = s.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) return {};
            return s.substr(start, end - start + 1);
        }
    };

}