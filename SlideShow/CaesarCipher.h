#pragma once
#include <string>
#include <fstream>

class CaesarCipher
{
public:
    static std::wstring Encrypt(const std::wstring& text)
    {
        std::wstring result;
        wchar_t* work = nullptr;
        std::size_t len = text.length();
        work = new wchar_t[len + 1];
        wcscpy_s(work, len + 1, text.c_str());
        for (std::size_t i = 0; i < len + 1; ++i)
        {
            work[i] = work[i] + 10;
        }
        // work[i]を+10したときに0になってしまうことがある。
        // そのためworkの中には0x00が含まれることを考慮する
        result = std::wstring(&work[0], &work[len + 1]);
        delete[] work;
        return result;
    }

    static void EncryptToFile(const std::wstring& text, const std::wstring& filename)
    {
        std::wstring buff = Encrypt(text);

        std::wofstream ofs(filename);

        ofs.write(buff.data(), buff.size()); // ignore NULL
    }

    static std::string Decrypt(const std::string& text)
    {
        std::string result;
        char* work = nullptr;
        std::size_t len = text.length();
        work = new char[len]; // +1しない。暗号化時にヌル文字を+10した記号が末尾にあるから。

        // 途中にnull文字があっても指定バイト数までコピーする
        memcpy(work, text.c_str(), len);
        for (std::size_t i = 0; i < len; ++i)
        {
            work[i] = work[i] - 10;
        }
        result = std::string(work);
        delete[] work;
        return result;
    }

    static std::wstring Utf8ToWstring(const std::string& utf8)
    {
        if (utf8.empty()) return std::wstring();

        int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (len == 0) throw std::runtime_error("UTF-8 to UTF-16 conversion failed.");

        std::wstring result(len - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], len);
        return result;
    }

    static std::wstring DecryptFromFile(const std::wstring& text)
    {
        int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string narrowPath(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &narrowPath[0], size, nullptr, nullptr);

        std::ifstream ifs(narrowPath);
        if (!ifs)
        {
            return std::wstring();
        }
        std::istreambuf_iterator<char> itBegin(ifs);
        std::istreambuf_iterator<char> itEnd;
        std::string work(itBegin, itEnd);

        work = Decrypt(work);

        std::wstring work2 = Utf8ToWstring(work);

        return work2;
    }
};
