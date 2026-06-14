#include "SlideShow.h"
#include <algorithm>
#include <sstream>
#include "HeaderOnlyCsv.hpp"
#include "CaesarCipher.h"

using namespace NSSlideShow;

bool SlideShow::m_fastMode = false;

namespace
{
Page::CharacterPosition ParseCharacterPosition(const std::wstring& rawValue)
{
    std::wstring value = rawValue;
    std::transform(value.begin(), value.end(), value.begin(), towlower);

    if (value == L"left")
    {
        return Page::CharacterPosition::Left;
    }

    if (value == L"center")
    {
        return Page::CharacterPosition::Center;
    }

    return Page::CharacterPosition::Right;
}

bool ParseBoolValue(const std::wstring& rawValue)
{
    std::wstring value = rawValue;
    std::transform(value.begin(), value.end(), value.begin(), towlower);

    if (value == L"1" || value == L"true" || value == L"yes" || value == L"y" || value == L"on")
    {
        return true;
    }

    return false;
}

float ParseScaleValue(const std::wstring& rawValue)
{
    if (rawValue.empty())
    {
        return 1.0f;
    }

    try
    {
        const float value = std::stof(rawValue);
        if (value > 0.0f)
        {
            return value;
        }
    }
    catch (...)
    {
    }

    return 1.0f;
}
}

static std::vector<std::wstring> split(const std::wstring& s, wchar_t delim)
{
    std::vector<std::wstring> result;
    std::wstringstream ss(s);
    std::wstring item;

    while (getline(ss, item, delim))
    {
        if (!item.empty() && item.back() == L'\r')
        {
            item.pop_back();
        }

        result.push_back(item);
    }

    return result;
}

void SlideShow::Init(
    IFont* font,
    ISoundEffect* SE,
    ISprite* sprTextBack,
    ISprite* sprFade,
    const std::vector<Page>& pageList,
    const bool bEnglish)
{
    m_font = font;
    m_SE = SE;
    m_sprTextBack = sprTextBack;
    m_sprFade = sprFade;
    m_pageList = pageList;
    m_isFadeIn = true;

    m_font->Init(bEnglish);
    m_SE->Init();

    InitConstValue();
}

void NSSlideShow::SlideShow::Init(IFont* font,
                                        ISoundEffect* SE,
                                        ISprite* sprTextBack,
                                        ISprite* sprFade,
                                        const std::wstring& csvFile,
                                        ISprite* sprImage,
                                        const bool encrypt,
                                        const bool bEnglish)
{
    m_font = font;
    m_SE = SE;
    m_sprTextBack = sprTextBack;
    m_sprFade = sprFade;
    m_sprImage = sprImage;

    m_font->Init(bEnglish);
    m_SE->Init();

    std::vector<std::vector<std::wstring> > vvs;
    if (encrypt == false)
    {
        vvs = csv::Read(csvFile);
    }
    else
    {
        auto workStr = CaesarCipher::DecryptFromFile(csvFile);
        vvs = csv::ReadFromString(workStr);
    }

    std::vector<Page> pageList;
    const bool hasResolutionColumn = (!vvs.empty() && vvs[0].size() >= 8);
    const int textCol = hasResolutionColumn ? 3 : 2;
    const int charCol = hasResolutionColumn ? 4 : 3;
    const int posCol = hasResolutionColumn ? 5 : 4;
    const int flipCol = hasResolutionColumn ? 6 : 5;
    const int scaleCol = hasResolutionColumn ? 7 : 6;

    Page page;
    int pageNum = 0;
    std::vector<std::vector<std::wstring>> textList;
    for (size_t i = 1; i < vvs.size(); ++i)
    {
        std::vector<std::wstring> line = vvs.at(i);
        int pageNumTemp = std::stoi(line.at(0));

        // 新しいページ
        if (pageNum != pageNumTemp)
        {
            // 古いページを登録
            {
                // １行目だったら古いページはない
                if (page.GetSprite() == nullptr)
                {
                    // Do nothing
                }
                else
                {
                    page.SetTextList(textList);
                    pageList.push_back(page);

                    page.SetSprite(nullptr);
                    page.SetForegroundSprite(nullptr);
                    page.SetBackgroundBaseResolution(0, 0);
                    textList.clear();
                }
            }
            pageNum = pageNumTemp;
            std::wstring imagePath = line.at(1);

            ISprite* sprite = sprImage->Create();
            sprite->Load(imagePath);
            page.SetSprite(sprite);

            if (hasResolutionColumn && line.size() > 2)
            {
                const std::wstring resStr = line.at(2);
                const size_t xPos = resStr.find(L'x');
                if (xPos != std::wstring::npos && xPos > 0 && xPos + 1 < resStr.size())
                {
                    try
                    {
                        const int w = std::stoi(resStr.substr(0, xPos));
                        const int h = std::stoi(resStr.substr(xPos + 1));
                        page.SetBackgroundBaseResolution(w, h);
                    }
                    catch (...)
                    {
                    }
                }
            }

            if (line.size() > charCol && !line.at(charCol).empty())
            {
                ISprite* foregroundSprite = sprImage->Create();
                foregroundSprite->Load(line.at(charCol));
                page.SetForegroundSprite(foregroundSprite);

                Page::ForegroundLayout layout;
                if (line.size() > posCol)
                {
                    layout.position = ParseCharacterPosition(line.at(posCol));
                }

                if (line.size() > flipCol)
                {
                    layout.flipX = ParseBoolValue(line.at(flipCol));
                }

                if (line.size() > scaleCol)
                {
                    layout.scale = ParseScaleValue(line.at(scaleCol));
                }

                page.SetForegroundLayout(layout);
            }
            else
            {
                page.SetForegroundSprite(nullptr);
                page.SetForegroundLayout(Page::ForegroundLayout());
            }

            std::vector<std::wstring> texts = split(line.at(textCol), L'\n');

            for (size_t j = 0; j < texts.size(); ++j)
            {
                texts.at(j).erase(std::remove(texts.at(j).begin(), texts.at(j).end(), L'"'),
                                  texts.at(j).end());
            }
            textList.push_back(texts);
        }
        else
        {
            std::vector<std::wstring> texts = split(line.at(textCol), L'\n');
            for (size_t j = 0; j < texts.size(); ++j)
            {
                texts.at(j).erase(std::remove(texts.at(j).begin(), texts.at(j).end(), L'"'),
                                  texts.at(j).end());
            }
            textList.push_back(texts);
        }
    }
    page.SetTextList(textList);
    pageList.push_back(page);

    m_pageList = pageList;

    m_isFadeIn = true;

    InitConstValue();
}

void SlideShow::Next()
{
    if (m_waitNextCount < WAIT_NEXT_FRAME)
    {
        return;
    }
    int textIndex = m_pageList.at(m_pageIndex).GetTextIndex();
    int textIndexMax = (int)m_pageList.at(m_pageIndex).GetTextList().size();
    if (textIndex < textIndexMax - 1)
    {
        textIndex++;
    }
    else
    {
        m_FadeOutCount = 0;
        m_isFadeOut = true;
    }
    m_pageList.at(m_pageIndex).SetTextIndex(textIndex);
    m_SE->PlayMove();
    m_waitNextCount = 0;
}

bool SlideShow::Update()
{
    InitConstValue();

    bool isFinish = false;
    if (m_isFadeIn)
    {
        if (m_FadeInCount < FADE_FRAME_MAX)
        {
            m_FadeInCount++;
        }
        else
        {
            m_isFadeIn = false;
            m_FadeInCount = 0;
        }
    }
    else if (m_isFadeOut)
    {
        if (m_FadeOutCount < FADE_FRAME_MAX)
        {
            m_FadeOutCount++;
        }
        else
        {
            m_isFadeOut = false;
            m_FadeOutCount = 0;
            m_isFadeIn = true;
            m_FadeInCount = 0;
            if (m_isSkip)
            {
                m_isSkip = false;
                isFinish = true;
            }
            else if (m_pageIndex <= (int)m_pageList.size() - 2)
            {
                m_pageIndex++;
                m_pageList.at(m_pageIndex).SetTextIndex(0);
            }
            else
            {
                isFinish = true;
            }
        }
    }
    else
    {
        m_waitNextCount++;
    }
    return isFinish;
}

void SlideShow::Render()
{
    const int characterCenterY = 432;
    int characterCenterX = 1248;

    const Page& currentPage = m_pageList.at(m_pageIndex);
    const int bgBaseW = currentPage.GetBackgroundBaseWidth();
    const int bgBaseH = currentPage.GetBackgroundBaseHeight();
    if (bgBaseW > 0 && bgBaseH > 0)
    {
        const float kBaseWidth = 1600.0f;
        float bgScale = kBaseWidth / static_cast<float>(bgBaseW);
        bgScale = std::ceil(bgScale * 1000.0f) / 1000.0f;
        currentPage.GetSprite()->DrawImageEx(0, 0, 255, false, bgScale);
    }
    else
    {
        currentPage.GetSprite()->DrawImage(0, 0);
    }

    if (currentPage.GetForegroundSprite() != nullptr)
    {
        const Page::ForegroundLayout layout = currentPage.GetForegroundLayout();
        if (layout.position == Page::CharacterPosition::Left)
        {
            characterCenterX = 352;
        }
        else if (layout.position == Page::CharacterPosition::Center)
        {
            characterCenterX = 800;
        }

        currentPage.GetForegroundSprite()->DrawImageEx(characterCenterX,
                                                       characterCenterY,
                                                       255,
                                                       layout.flipX,
                                                       layout.scale);
    }
    m_sprTextBack->DrawImageEx(0, 0, 255, false, 1.0f);
    std::vector<std::vector<std::wstring>> vss = currentPage.GetTextList();
    int textIndex = currentPage.GetTextIndex();
    if (vss.at(textIndex).size() >= 1)
    {
        m_font->DrawText_(vss.at(textIndex).at(0), 100, 730);
    }

    if (vss.at(textIndex).size() >= 2)
    {
        m_font->DrawText_(vss.at(textIndex).at(1), 100, 780);
    }

    if (vss.at(textIndex).size() >= 3)
    {
        m_font->DrawText_(vss.at(textIndex).at(2), 100, 830);
    }

    if (m_isFadeIn)
    {
        m_sprFade->DrawImage(0, 0, 255 - m_FadeInCount*255/FADE_FRAME_MAX);
    }
    if (m_isFadeOut)
    {
        m_sprFade->DrawImage(0, 0, m_FadeOutCount*255/FADE_FRAME_MAX);
    }
}

void SlideShow::Finalize()
{
    delete m_sprTextBack;
    m_sprTextBack = nullptr;
    delete m_sprFade;
    m_sprFade = nullptr;
    delete m_font;
    m_font = nullptr;
    delete m_SE;
    m_SE = nullptr;
    for (std::size_t i = 0; i < m_pageList.size(); ++i)
    {
        delete m_pageList.at(i).GetSprite();
        m_pageList.at(i).SetSprite(nullptr);
        delete m_pageList.at(i).GetForegroundSprite();
        m_pageList.at(i).SetForegroundSprite(nullptr);
    }
    delete m_sprImage;
    m_sprImage = nullptr;
}

void NSSlideShow::SlideShow::Skip()
{
    m_isSkip = true;
    m_isFadeIn = false;
    m_FadeInCount = 0;
    m_isFadeOut = true;
    m_FadeOutCount = 0;
}

void NSSlideShow::SlideShow::SetScreenSize(const int width, const int height)
{
    m_screenWidth = width;
    m_screenHeight = height;
}

void NSSlideShow::SlideShow::SetFastMode(const bool arg)
{
    m_fastMode = arg;
}

void NSSlideShow::SlideShow::OnDeviceLost()
{
    m_sprFade->OnDeviceLost();
    m_sprImage->OnDeviceLost();
    m_sprTextBack->OnDeviceLost();
    m_font->OnDeviceLost();

    for (auto& item : m_pageList)
    {
        item.GetSprite()->OnDeviceLost();
        if (item.GetForegroundSprite() != nullptr)
        {
            item.GetForegroundSprite()->OnDeviceLost();
        }
    }
}

void NSSlideShow::SlideShow::OnDeviceReset()
{
    m_sprFade->OnDeviceReset();
    m_sprImage->OnDeviceReset();
    m_sprTextBack->OnDeviceReset();
    m_font->OnDeviceReset();

    for (auto& item : m_pageList)
    {
        item.GetSprite()->OnDeviceReset();
        if (item.GetForegroundSprite() != nullptr)
        {
            item.GetForegroundSprite()->OnDeviceReset();
        }
    }
}

void NSSlideShow::SlideShow::InitConstValue()
{
    int& fade_frame_max = const_cast<int&>(FADE_FRAME_MAX);
    int& wait_next_frame = const_cast<int&>(WAIT_NEXT_FRAME);

    if (m_fastMode)
    {
        fade_frame_max = 1;
        wait_next_frame = 1;
    }
    else
    {
        fade_frame_max = 20;
        wait_next_frame = 10;
    }
}

ISprite* Page::GetSprite() const
{
    return m_sprite;
}

void Page::SetSprite(ISprite* sprite)
{
    m_sprite = sprite;
}

ISprite* Page::GetForegroundSprite() const
{
    return m_foregroundSprite;
}

void Page::SetForegroundSprite(ISprite* sprite)
{
    m_foregroundSprite = sprite;
}

Page::ForegroundLayout Page::GetForegroundLayout() const
{
    return m_foregroundLayout;
}

void Page::SetForegroundLayout(const ForegroundLayout& layout)
{
    m_foregroundLayout = layout;
}

std::vector<std::vector<std::wstring>> Page::GetTextList() const
{
    return m_textList;
}

void Page::SetTextList(const std::vector<std::vector<std::wstring>>& textList)
{
    m_textList = textList;
}

int Page::GetTextIndex() const
{
    return m_textIndex;
}

void Page::SetTextIndex(const int index)
{
    m_textIndex = index;
}

void Page::SetBackgroundBaseResolution(const int width, const int height)
{
    m_backgroundBaseWidth = width;
    m_backgroundBaseHeight = height;
}

int Page::GetBackgroundBaseWidth() const
{
    return m_backgroundBaseWidth;
}

int Page::GetBackgroundBaseHeight() const
{
    return m_backgroundBaseHeight;
}

