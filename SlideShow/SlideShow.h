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
    virtual void DrawImageEx(const int x,
                             const int y,
                             const int transparency,
                             const bool flipX,
                             const float scale) = 0;
    virtual void Load(const std::wstring& filepath) = 0;
    virtual ISprite* Create() = 0;
    virtual void GetImageSize(int& width, int& height) const { width = 0; height = 0; }
    virtual ~ISprite() {};
    virtual void OnDeviceLost() = 0;
    virtual void OnDeviceReset() = 0;
};

class IFont
{
public:
    virtual void DrawText_(const std::wstring& msg, const int x, const int y) = 0;
    virtual void Init(const bool bEnglish) = 0;
    virtual ~IFont() {};
    virtual void OnDeviceLost() = 0;
    virtual void OnDeviceReset() = 0;
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
    enum class CharacterPosition
    {
        Left,
        Center,
        Right
    };

    struct ForegroundLayout
    {
        CharacterPosition position = CharacterPosition::Right;
        bool flipX = false;
        float scale = 1.0f;
        int characterBaseWidth = 0;
        int characterBaseHeight = 0;
    };

    ISprite* GetSprite() const;
    void SetSprite(ISprite* sprite);
    ISprite* GetForegroundSprite() const;
    void SetForegroundSprite(ISprite* sprite);
    ForegroundLayout GetForegroundLayout() const;
    void SetForegroundLayout(const ForegroundLayout& layout);
    void SetBackgroundBaseResolution(int width, int height);
    int GetBackgroundBaseWidth() const;
    int GetBackgroundBaseHeight() const;

    std::vector<std::vector<std::wstring>> GetTextList() const;
    void SetTextList(const std::vector<std::vector<std::wstring>>& textList);

    int GetTextIndex() const;
    void SetTextIndex(const int index);

private:

    ISprite* m_sprite = nullptr;
    ISprite* m_foregroundSprite = nullptr;
    ForegroundLayout m_foregroundLayout;
    int m_backgroundBaseWidth = 0;
    int m_backgroundBaseHeight = 0;
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
    void Skip();
    void SetScreenSize(const int width, const int height);

    static void SetFastMode(const bool arg);
    
    void OnDeviceLost();
    void OnDeviceReset();

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
    int m_screenWidth = 1600;
    int m_screenHeight = 900;
    const int FADE_FRAME_MAX = 20;
    bool m_isFadeIn = false;
    int m_FadeInCount = 0;
    bool m_isFadeOut = false;
    int m_FadeOutCount = 0;

    const int WAIT_NEXT_FRAME = 10;
    int m_waitNextCount = 0;

    bool m_isSkip = false;
};
}

