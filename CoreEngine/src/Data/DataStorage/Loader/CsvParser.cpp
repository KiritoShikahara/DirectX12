#include "pch.h"
#include"CsvParser.h"

std::vector<std::string> data::CsvParser::SplitCsv(const std::string& line)
{
    std::vector<std::string> result;
    std::string token;
    bool inQuote = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        const char c = line[i];

        if (inQuote)
        {
            if (c == '"')
            {
                // 次の文字も " なら エスケープされた " → トークンに追加
                if (i + 1 < line.size() && line[i + 1] == '"')
                {
                    token += '"';
                    ++i;
                }
                else
                {
                    inQuote = false;
                }
            }
            else
            {
                token += c;
            }
        }
        else
        {
            if (c == '"')
            {
                inQuote = true;
            }
            else if (c == ',')
            {
                result.push_back(token);
                token.clear();
            }
            else
            {
                token += c;
            }
        }
    }
    result.push_back(token);
    return result;
}

std::string data::CsvParser::Trim(const std::string& s)
{
    const size_t start = s.find_first_not_of(" \t\r\n");
    const size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    return s.substr(start, end - start + 1);
}
