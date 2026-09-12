#include "SlideShow.h"
#include <algorithm>
#include <sstream>
#include "HeaderOnlyCsv.hpp"
#include "CaesarCipher.h"

using namespace NSSlideShow;

bool SlideShow::m_fastMode = false;

namespace
{
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

void ParseResolutionValue(const std::wstring& resStr, int& outW, int& outH)
{
    outW = 0;
    outH = 0;
    if (resStr.empty())
    {
        return;
    }

    const size_t xPos = resStr.find(L'x');
    if (xPos == std::wstring::npos || xPos == 0 || xPos + 1 >= resStr.size())
    {
        return;
    }

    try
    {
        outW = std::stoi(resStr.substr(0, xPos));
        outH = std::stoi(resStr.substr(xPos + 1));
    }
    catch (...)
    {
    }
}

void ParseForegroundLayoutMulti(const std::vector<std::wstring>& line, const int baseCol, Page::ForegroundLayout& layout)
{
    const int resCol = baseCol;
    const int flipCol = baseCol + 1;
    const int scaleCol = baseCol + 2;

    if (line.size() > resCol)
    {
        ParseResolutionValue(line.at(resCol), layout.characterBaseWidth, layout.characterBaseHeight);
    }
    if (line.size() > flipCol)
    {
        layout.flipX = ParseBoolValue(line.at(flipCol));
    }
    if (line.size() > scaleCol)
    {
        layout.scale = ParseScaleValue(line.at(scaleCol));
    }
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
    const bool hasCharacterResolutionColumn = (!vvs.empty() && vvs[0].size() >= 9);
    const bool isMultiSlotFormat = (!vvs.empty() && vvs[0].size() >= 16);
    const int textCol = hasResolutionColumn ? 3 : 2;

    Page page;
    int pageNum = 0;
    std::vector<std::vector<std::wstring>> textList;
    std::vector<ForegroundState> foregroundStates;
    ForegroundState foregroundState;
    for (size_t i = 1; i < vvs.size(); ++i)
    {
        std::vector<std::wstring> line = vvs.at(i);
        int pageNumTemp = std::stoi(line.at(0));

        if (pageNum != pageNumTemp)
        {
            if (page.GetSprite() != nullptr)
            {
                page.SetTextList(textList);
                pageList.push_back(page);
                m_foregroundStates.push_back(foregroundStates);
                foregroundStates.clear();
                foregroundState = ForegroundState();
                page.SetSprite(nullptr);
                page.ClearForeground();
                page.SetBackgroundBaseResolution(0, 0);
                textList.clear();
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
                    catch (...) {}
                }
            }
        }

        if (isMultiSlotFormat)
        {
            // 3スロット形式は各行が完全な指定。空欄のキャラクターは表示しない。
            foregroundState = ForegroundState();
            for (int slot = 0; slot < 3; ++slot)
            {
                const int column = 4 + slot * 4;
                if (line.size() > column && !line.at(column).empty())
                {
                    foregroundState.sprites.at(slot) = LoadForegroundSprite(line.at(column));
                    ParseForegroundLayoutMulti(line, column + 1, foregroundState.layouts.at(slot));
                }
            }
        }
        else
        {
            const int charCol = hasResolutionColumn ? 4 : 3;
            const int charResCol = hasCharacterResolutionColumn ? 5 : -1;
            const int posCol = hasCharacterResolutionColumn ? 6 : (hasResolutionColumn ? 5 : 4);
            const int flipCol = hasCharacterResolutionColumn ? 7 : (hasResolutionColumn ? 6 : 5);
            const int scaleCol = hasCharacterResolutionColumn ? 8 : (hasResolutionColumn ? 7 : 6);

            if (line.size() > charCol && !line.at(charCol).empty())
            {
                ISprite* foregroundSprite = LoadForegroundSprite(line.at(charCol));

                Page::ForegroundLayout layout;
                if (line.size() > flipCol)
                {
                    layout.flipX = ParseBoolValue(line.at(flipCol));
                }
                if (line.size() > scaleCol)
                {
                    layout.scale = ParseScaleValue(line.at(scaleCol));
                }
                if (charResCol >= 0 && line.size() > charResCol)
                {
                    ParseResolutionValue(line.at(charResCol), layout.characterBaseWidth, layout.characterBaseHeight);
                }

                std::wstring posValue = L"right";
                if (line.size() > posCol)
                {
                    posValue = line.at(posCol);
                }
                std::transform(posValue.begin(), posValue.end(), posValue.begin(), towlower);
                if (posValue == L"left")
                {
                    foregroundState.sprites.at(0) = foregroundSprite;
                    foregroundState.layouts.at(0) = layout;
                }
                else if (posValue == L"center")
                {
                    foregroundState.sprites.at(1) = foregroundSprite;
                    foregroundState.layouts.at(1) = layout;
                }
                else
                {
                    foregroundState.sprites.at(2) = foregroundSprite;
                    foregroundState.layouts.at(2) = layout;
                }
            }
        }

        std::vector<std::wstring> texts = split(line.at(textCol), L'\n');
        for (std::wstring& text : texts)
        {
            text.erase(std::remove(text.begin(), text.end(), L'"'), text.end());
        }
        textList.push_back(texts);
        foregroundStates.push_back(foregroundState);
    }
    page.SetTextList(textList);
    pageList.push_back(page);
    m_foregroundStates.push_back(foregroundStates);

    m_pageList = pageList;

    m_isFadeIn = true;

    InitConstValue();
}

ISprite* SlideShow::LoadForegroundSprite(const std::wstring& path)
{
    const auto found = m_foregroundSprites.find(path);
    if (found != m_foregroundSprites.end())
    {
        return found->second.get();
    }
    std::unique_ptr<ISprite> sprite(m_sprImage->Create());
    sprite->Load(path);
    ISprite* result = sprite.get();
    m_foregroundSprites.emplace(path, std::move(sprite));
    return result;
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

static void DrawForegroundSprite(ISprite& sprite,
                                  const Page::ForegroundLayout& layout,
                                  const int characterCenterY,
                                  const int screenWidth,
                                  const int screenHeight,
                                  const int slot)
{
    float effectiveScale = layout.scale;

    int charWidth = 0;
    int charHeight = 0;
    sprite.GetImageSize(charWidth, charHeight);
    if (charWidth <= 0 || charHeight <= 0)
    {
        return;
    }

    float fitScale = effectiveScale;
    if (layout.characterBaseWidth > 0)
    {
        fitScale = (std::min)(fitScale,
                              static_cast<float>(layout.characterBaseWidth) /
                              static_cast<float>(charWidth));
    }
    if (layout.characterBaseHeight > 0)
    {
        fitScale = (std::min)(fitScale,
                              static_cast<float>(layout.characterBaseHeight) /
                              static_cast<float>(charHeight));
    }
    effectiveScale = fitScale;

    const int renderedWidth = static_cast<int>(static_cast<float>(charWidth) * effectiveScale);
    const int renderedHeight = static_cast<int>(static_cast<float>(charHeight) * effectiveScale);

    int characterCenterX = 0;
    if (slot == 0)
    {
        characterCenterX = renderedWidth / 2;
    }
    else if (slot == 1)
    {
        characterCenterX = screenWidth / 2;
    }
    else
    {
        characterCenterX = screenWidth - renderedWidth / 2;
    }

    int fittedCharacterCenterY = characterCenterY;
    if (renderedHeight <= screenHeight)
    {
        if (fittedCharacterCenterY - renderedHeight / 2 < 0)
        {
            fittedCharacterCenterY = renderedHeight / 2;
        }
        if (fittedCharacterCenterY + renderedHeight / 2 > screenHeight)
        {
            fittedCharacterCenterY = screenHeight - renderedHeight / 2;
        }
    }

    sprite.DrawImageEx(characterCenterX,
                       fittedCharacterCenterY,
                       255,
                       layout.flipX,
                       effectiveScale);
}

void SlideShow::Render()
{
    // 立ち絵を従来位置から70px下げ、頭部と台詞欄の間の構図を整える。
    const int characterCenterY = 532;

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

    ForegroundState foreground;
    if (!m_foregroundStates.empty())
    {
        foreground = m_foregroundStates.at(m_pageIndex).at(currentPage.GetTextIndex());
    }
    else
    {
        // Pageを直接渡す既存APIも引き続き利用できる。
        foreground.sprites = { currentPage.GetForegroundLeft(),
                               currentPage.GetForegroundCenter(),
                               currentPage.GetForegroundRight() };
        foreground.layouts = { currentPage.GetForegroundLayoutLeft(),
                               currentPage.GetForegroundLayoutCenter(),
                               currentPage.GetForegroundLayoutRight() };
    }
    for (int slot = 0; slot < 3; ++slot)
    {
        ISprite* sprite = foreground.sprites.at(slot);
        if (sprite != nullptr)
        {
            DrawForegroundSprite(*sprite,
                                 foreground.layouts.at(slot),
                                 characterCenterY,
                                 m_screenWidth,
                                 m_screenHeight,
                                 slot);
        }
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
        delete m_pageList.at(i).GetForegroundLeft();
        delete m_pageList.at(i).GetForegroundCenter();
        delete m_pageList.at(i).GetForegroundRight();
        m_pageList.at(i).ClearForeground();
    }
    m_pageList.clear();
    m_foregroundStates.clear();
    m_foregroundSprites.clear();
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

    for (auto& item : m_foregroundSprites)
    {
        item.second->OnDeviceLost();
    }

    for (auto& item : m_pageList)
    {
        item.GetSprite()->OnDeviceLost();
        if (item.GetForegroundLeft() != nullptr) item.GetForegroundLeft()->OnDeviceLost();
        if (item.GetForegroundCenter() != nullptr) item.GetForegroundCenter()->OnDeviceLost();
        if (item.GetForegroundRight() != nullptr) item.GetForegroundRight()->OnDeviceLost();
    }
}

void NSSlideShow::SlideShow::OnDeviceReset()
{
    m_sprFade->OnDeviceReset();
    m_sprImage->OnDeviceReset();
    m_sprTextBack->OnDeviceReset();
    m_font->OnDeviceReset();

    for (auto& item : m_foregroundSprites)
    {
        item.second->OnDeviceReset();
    }

    for (auto& item : m_pageList)
    {
        item.GetSprite()->OnDeviceReset();
        if (item.GetForegroundLeft() != nullptr) item.GetForegroundLeft()->OnDeviceReset();
        if (item.GetForegroundCenter() != nullptr) item.GetForegroundCenter()->OnDeviceReset();
        if (item.GetForegroundRight() != nullptr) item.GetForegroundRight()->OnDeviceReset();
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

ISprite* Page::GetForegroundLeft() const { return m_foregroundLeft; }
void Page::SetForegroundLeft(ISprite* sprite, const ForegroundLayout& layout) { delete m_foregroundLeft; m_foregroundLeft = sprite; m_layoutLeft = layout; }
ISprite* Page::GetForegroundCenter() const { return m_foregroundCenter; }
void Page::SetForegroundCenter(ISprite* sprite, const ForegroundLayout& layout) { delete m_foregroundCenter; m_foregroundCenter = sprite; m_layoutCenter = layout; }
ISprite* Page::GetForegroundRight() const { return m_foregroundRight; }
void Page::SetForegroundRight(ISprite* sprite, const ForegroundLayout& layout) { delete m_foregroundRight; m_foregroundRight = sprite; m_layoutRight = layout; }
Page::ForegroundLayout Page::GetForegroundLayoutLeft() const { return m_layoutLeft; }
Page::ForegroundLayout Page::GetForegroundLayoutCenter() const { return m_layoutCenter; }
Page::ForegroundLayout Page::GetForegroundLayoutRight() const { return m_layoutRight; }
void Page::ClearForeground() { m_foregroundLeft = nullptr; m_foregroundCenter = nullptr; m_foregroundRight = nullptr; }

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

