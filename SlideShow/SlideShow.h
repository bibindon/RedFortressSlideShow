// CSVファイル読み込みは保留。このクラスを使う機会が3回くらいしかないなら、作っても意味ない。
#pragma once
#include <string>
#include <vector>

namespace NSSlideShow
{
class ISprite
{
public:
    virtual void DrawImage(const int x, const int y, const int transparency = 255) = 0;
    virtual void Load(const std::wstring& filepath) = 0;
    virtual ISprite* Create() = 0;
    virtual ~ISprite() {};
};

class IFont
{
public:
    virtual void DrawText_(const std::wstring& msg, const int x, const int y) = 0;
    virtual void Init(const bool bEnglish) = 0;
    virtual ~IFont() {};
};

class ISoundEffect
{
public:
    virtual void PlayMove() = 0;
    virtual void Init() = 0;
    virtual ~ISoundEffect() {};
};

class Page
{
public:
    ISprite* GetSprite() const;
    void SetSprite(ISprite* sprite);

    std::vector<std::vector<std::wstring>> GetTextList() const;
    void SetTextList(const std::vector<std::vector<std::wstring>>& textList);

    int GetTextIndex() const;
    void SetTextIndex(const int index);

private:

    ISprite* m_sprite = nullptr;
    std::vector<std::vector<std::wstring>> m_textList;
    int m_textIndex = 0;
};

class SlideShow
{
public:

    void Init(IFont* font,
              ISoundEffect* SE,
              ISprite* sprTextBack,
              ISprite* sprFade,
              const std::vector<Page>& pageList,
              const bool bEnglish);

    void Init(IFont* font,
              ISoundEffect* SE,
              ISprite* sprTextBack,
              ISprite* sprFade,
              const std::wstring& csvFile,
              ISprite* sprImage,
              const bool encrypt,
              const bool bEnglish);

    void Next();
    bool Update();
    void Render();

    void Finalize();

    static void SetFastMode(const bool arg);

private:

    static bool m_fastMode;

    void InitConstValue();

    ISprite* m_sprTextBack;
    IFont* m_font;
    ISoundEffect* m_SE;
    std::vector<Page> m_pageList;
    int m_pageIndex = 0;

    ISprite* m_sprFade;
    ISprite* m_sprImage;
    const int FADE_FRAME_MAX = 20;
    bool m_isFadeIn = false;
    int m_FadeInCount = 0;
    bool m_isFadeOut = false;
    int m_FadeOutCount = 0;

    const int WAIT_NEXT_FRAME = 10;
    int m_waitNextCount = 0;
};
}

