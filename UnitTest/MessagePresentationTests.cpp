#include "../SlideShow/SlideShow.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace NSSlideShow;

namespace
{
std::vector<std::wstring> g_texts;
std::vector<std::wstring> g_images;
std::map<std::wstring, int> g_loads;
std::map<std::wstring, int> g_lost;
std::map<std::wstring, int> g_reset;
int g_alive = 0;

void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

class Sprite : public ISprite
{
public:
    Sprite() { ++g_alive; }
    ~Sprite() override { --g_alive; }
    void DrawImage(int, int, int) override { g_images.push_back(m_path); }
    void DrawImageEx(int, int, int, bool, float) override { g_images.push_back(m_path); }
    void Load(const std::wstring& path) override { m_path = path; ++g_loads[path]; }
    ISprite* Create() override { return new Sprite(); }
    void GetImageSize(int& width, int& height) const override { width = 400; height = 600; }
    void OnDeviceLost() override { ++g_lost[m_path]; }
    void OnDeviceReset() override { ++g_reset[m_path]; }
private:
    std::wstring m_path;
};

class Font : public IFont
{
public:
    void DrawText_(const std::wstring& text, int, int) override { g_texts.push_back(text); }
    void Init(bool) override {}
    void OnDeviceLost() override {}
    void OnDeviceReset() override {}
};

class Sound : public ISoundEffect
{
public:
    void PlayMove() override {}
    void Init() override {}
};

void Settle(SlideShow& show)
{
    for (int i = 0; i < 35; ++i)
    {
        Require(!show.Update(), "Finished before the last message");
    }
}

void CheckRender(SlideShow& show, const std::vector<std::wstring>& texts,
                 const std::vector<std::wstring>& images)
{
    g_texts.clear();
    g_images.clear();
    show.Render();
    Require(g_texts == texts, "Incorrect message lines");
    Require(g_images == images, "Incorrect portrait or unexpected fade");
}

void TestMessageTransitions()
{
    const char* path = "message-presentation-test.csv";
    std::ofstream csv(path, std::ios::binary);
    csv << "PageNo,Image,Resolution,Message,CharLeft,CharLeftResolution,CharLeftFlip,CharLeftScale,"
           "CharCenter,CharCenterResolution,CharCenterFlip,CharCenterScale,"
           "CharRight,CharRightResolution,CharRightFlip,CharRightScale\r\n"
           "1,bg,1600x900,One,left,,,1,,,,,normal,,,1\r\n"
           "1,bg,1600x900,\"Two A\r\nTwo B\",,,,,,,,,surprised,,,1\r\n"
           "1,bg,1600x900,\"Three A\r\nThree B\r\nThree C\",,,,,,,,,normal,,,1\r\n"
           "2,next-bg,1600x900,End,,,,,,,,,,,,\r\n";
    csv.close();

    SlideShow show;
    show.Init(new Font(), new Sound(), new Sprite(), new Sprite(),
              L"message-presentation-test.csv", new Sprite(), false, false);
    Settle(show);
    CheckRender(show, { L"One" }, { L"bg", L"left", L"normal", L"" });
    show.Next();
    // Same background: the next text and expression must appear immediately, with no fade.
    CheckRender(show, { L"Two A", L"Two B" }, { L"bg", L"surprised", L"" });
    Settle(show);
    show.Next();
    CheckRender(show, { L"Three A", L"Three B", L"Three C" }, { L"bg", L"normal", L"" });
    Require(g_loads[L"normal"] == 1, "Repeated portrait was not cached");
    show.OnDeviceLost();
    show.OnDeviceReset();
    Require(g_lost[L"normal"] == 1 && g_reset[L"normal"] == 1,
            "Shared portrait received duplicate device events");
    Require(g_lost[L"surprised"] == 1 && g_reset[L"surprised"] == 1,
            "Inactive portrait missed device events");
    Settle(show);
    show.Next();
    for (int i = 0; i < 55; ++i)
    {
        Require(!show.Update(), "Background transition ended the slideshow");
    }
    CheckRender(show, { L"End" }, { L"next-bg", L"" });
    show.Next();
    bool finished = false;
    for (int i = 0; i < 25 && !finished; ++i)
    {
        finished = show.Update();
    }
    Require(finished, "Last message did not finish");
    show.Finalize();
    Require(g_alive == 0, "Sprites were not released");
    Require(std::remove(path) == 0, "Cannot remove test fixture");
}
}

int main()
{
    TestMessageTransitions();
    std::cout << "Message presentation tests passed (1/2/3 lines, expressions, exits, caching, device reset, finish).\n";
}
