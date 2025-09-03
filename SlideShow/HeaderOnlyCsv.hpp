#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iterator>
#include <exception>
#include "Shlwapi.h"
#pragma comment( lib, "Shlwapi.lib" ) 

class csv
{
public:

    static std::wstring Utf8ToWstring(const std::string& utf8)
    {
        if (utf8.empty()) return std::wstring();

        int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (len == 0) throw std::runtime_error("UTF-8 to UTF-16 conversion failed.");

        std::wstring result(len - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], len);
        return result;
    }

    static std::string WstringToUtf8(const std::wstring& wstr)
    {
        if (wstr.empty()) return std::string();

        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len == 0) throw std::runtime_error("UTF-16 to UTF-8 conversion failed.");

        std::string result(len - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
        return result;
    }

    // Unicode文字セット
    // UTF8とUTF16は違う。std::wstringはUTF16
    static std::vector<std::vector<std::wstring>> Read(const std::wstring& filepath)
    {
        // パスをUTF-8に変換
        int size = WideCharToMultiByte(CP_UTF8, 0, filepath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string narrowPath(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, filepath.c_str(), -1, &narrowPath[0], size, nullptr, nullptr);

        std::ifstream ifs(narrowPath, std::ios::binary);
        if (!ifs.is_open()) {
            throw std::runtime_error("Cannot open file: " + narrowPath);
        }

        // ファイル全体を読み込み
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

        // BOM削除（0xEF 0xBB 0xBF）
        if (content.size() >= 3 &&
            static_cast<unsigned char>(content[0]) == 0xEF &&
            static_cast<unsigned char>(content[1]) == 0xBB &&
            static_cast<unsigned char>(content[2]) == 0xBF) {
            content = content.substr(3);
        }

        // UTF-8 → wstring 変換
        std::wstring wcontent = Utf8ToWstring(content);

        // パース
        std::vector<std::vector<std::wstring>> csvData;
        std::vector<std::wstring> row;
        std::wstring field;
        bool inQuotes = false;

        for (size_t i = 0; i < wcontent.size(); ++i) {
            wchar_t ch = wcontent[i];

            if (ch == L'"') {
                if (inQuotes && i + 1 < wcontent.size() && wcontent[i + 1] == L'"') {
                    // エスケープされた " を追加
                    field += L'"';
                    ++i;
                }
                else {
                    inQuotes = !inQuotes;
                }
            }
            else if (ch == L',' && !inQuotes) {
                row.push_back(field);
                field.clear();
            }
            else if ((ch == L'\n' || ch == L'\r') && !inQuotes) {
                // \r\n または \n の場合
                if (ch == L'\r' && i + 1 < wcontent.size() && wcontent[i + 1] == L'\n') {
                    ++i; // \r\n をスキップ
                }
                row.push_back(field);
                field.clear();
                csvData.push_back(row);
                row.clear();
            }
            else {
                field += ch;
            }
        }

        // 最後の行があれば追加
        if (!field.empty() || !row.empty()) {
            row.push_back(field);
            csvData.push_back(row);
        }

        return csvData;
    }

    static std::vector<std::vector<std::wstring> > Read_old(const std::wstring& filepath)
    {
        std::vector<std::vector<std::wstring> > csvData;
        int result = PathFileExists(filepath.c_str());
        if (result == 0)
        {
            int size = WideCharToMultiByte(CP_UTF8, 0, filepath.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string result(size - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, filepath.c_str(), -1, &result[0], size, nullptr, nullptr);

            std::string work = "cannot open " + result;
            throw std::exception(work.c_str());
        }

        // 「"」記号で囲まれているとセル内改行ができることに注意
        // 「"」記号で囲まれているとセル内で「,」が使用できることに注意
        std::wifstream ifs(filepath);
        if (ifs.is_open() == false)
        {
            int size = WideCharToMultiByte(CP_UTF8, 0, filepath.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string result(size - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, filepath.c_str(), -1, &result[0], size, nullptr, nullptr);

            std::string work = "cannot open " + result;
            throw std::exception(work.c_str());
        }

        std::wstring buffComma;
        bool doubleQuoteMode = false;
        std::vector<std::wstring> work;
        std::istreambuf_iterator<wchar_t> itBegin(ifs);
        std::istreambuf_iterator<wchar_t> itEnd;

        for (; itBegin != itEnd; itBegin++)
        {
            if (*itBegin != ',' && *itBegin != '\n')
            {
                buffComma += *itBegin;
                if (*itBegin == '"')
                {
                    if (!doubleQuoteMode)
                    {
                        doubleQuoteMode = true;
                    }
                    else
                    {
                        doubleQuoteMode = false;
                    }
                }
            }
            else if (*itBegin == ',')
            {
                if (!doubleQuoteMode)
                {
                    work.push_back(buffComma);
                    buffComma.clear();
                }
                else
                {
                    buffComma += *itBegin;
                }
            }
            else if (*itBegin == '\n')
            {
                if (!doubleQuoteMode)
                {
                    work.push_back(buffComma);
                    buffComma.clear();
                    csvData.push_back(work);
                    work.clear();
                }
                else
                {
                    buffComma += *itBegin;
                }
            }
        }

        return csvData;
    }

    static std::vector<std::vector<std::wstring> > ReadFromString(const std::wstring& text)
    {
        std::vector<std::vector<std::wstring> > csvData;

        // 「"」記号で囲まれているとセル内改行ができることに注意
        std::wstring buffComma;
        bool doubleQuoteMode = false;
        std::vector<std::wstring> work;
        std::wstring::const_iterator itBegin(text.cbegin());
        std::wstring::const_iterator itEnd(text.cend());;

        for (; itBegin != itEnd; itBegin++)
        {
            if (*itBegin != L',' && *itBegin != L'\n')
            {
                buffComma += *itBegin;
                if (*itBegin == L'"')
                {
                    if (!doubleQuoteMode)
                    {
                        doubleQuoteMode = true;
                    }
                    else
                    {
                        doubleQuoteMode = false;
                    }
                }
            }
            else if (*itBegin == L',')
            {
                if (!doubleQuoteMode)
                {
                    work.push_back(buffComma);
                    buffComma.clear();
                }
                else
                {
                    buffComma += *itBegin;
                }
            }
            else if (*itBegin == L'\n')
            {
                if (!doubleQuoteMode)
                {
                    work.push_back(buffComma);
                    buffComma.clear();
                    csvData.push_back(work);
                    work.clear();
                }
                else
                {
                    buffComma += *itBegin;
                }
            }
        }

        return csvData;
    }


    static void Write(const std::wstring& filepath, const std::vector<std::vector<std::wstring>>& csvData)
    {
        // ファイルパスをUTF-8に変換（std::ofstreamはstd::wstring非対応）
        int size = WideCharToMultiByte(CP_UTF8, 0, filepath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string narrowPath(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, filepath.c_str(), -1, &narrowPath[0], size, nullptr, nullptr);

        std::ofstream ofs(narrowPath, std::ios::binary);
        if (!ofs.is_open()) {
            throw std::runtime_error("Cannot open file: " + narrowPath);
        }

        // UTF-8 BOM を書き込み
        const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        ofs.write(reinterpret_cast<const char*>(bom), sizeof(bom));

        // CSV内容書き込み
        for (const auto& row : csvData) {
            for (size_t j = 0; j < row.size(); ++j) {
                std::wstring cell = row[j];
                bool needQuotes = cell.find_first_of(L",\"\n\r") != std::wstring::npos;

                if (needQuotes) {
                    cell.insert(0, L"\"");
                    for (size_t pos = 1; (pos = cell.find(L"\"", pos)) != std::wstring::npos; pos += 2) {
                        cell.insert(pos, L"\""); // " を "" にエスケープ
                    }
                    cell += L"\"";
                }

                ofs << WstringToUtf8(cell);
                if (j != row.size() - 1) {
                    ofs << ",";
                }
            }
            ofs << "\r\n";
        }
    }

    static void Write_old(const std::wstring& filepath, const std::vector<std::vector<std::wstring> >& csvData)
    {
        std::wofstream ofs(filepath);
        if (ofs.is_open() == false)
        {
            int size = WideCharToMultiByte(CP_UTF8, 0, filepath.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string result(size - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, filepath.c_str(), -1, &result[0], size, nullptr, nullptr);

            std::string work = "Cannot open " + result;
            throw std::exception(work.c_str());
        }

        for (std::size_t i = 0; i < csvData.size(); ++i)
        {
            for (std::size_t j = 0; j < csvData.at(i).size(); ++j)
            {
                ofs << csvData.at(i).at(j);
                if (j != csvData.at(i).size() - 1)
                {
                    ofs << ",";
                }
            }
            ofs << "\n";
        }
    }

private:
    csv();

    static void ltrim(std::wstring& s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            [](wchar_t ch)
            {
                return !std::isspace(ch);
            }
        ));
    }

    static void rtrim(std::wstring& s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(),
            [](wchar_t ch)
            {
                return !std::isspace(ch);
            }
        ).base(), s.end());
    }

    static void trim(std::wstring& s)
    {
        rtrim(s);
        ltrim(s);
    }
};
