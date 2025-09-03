#pragma comment( lib, "d3d9.lib")
#if defined(DEBUG) || defined(_DEBUG)
#pragma comment( lib, "d3dx9d.lib")
#else
#pragma comment( lib, "d3dx9.lib")
#endif

#pragma comment (lib, "winmm.lib")

#pragma comment( lib, "SlideShow.lib")

#include "..\SlideShow\SlideShow.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <tchar.h>

using namespace NSStoryTelling;

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = NULL; } }

class Sprite : public ISprite
{
public:

    Sprite(LPDIRECT3DDEVICE9 dev)
        : m_pD3DDevice(dev)
    {
    }

    void DrawImage(const int x, const int y, const int transparency) override
    {
        D3DXVECTOR3 pos {(float)x, (float)y, 0.f};
        m_D3DSprite->Begin(D3DXSPRITE_ALPHABLEND);
        RECT rect = {
            0,
            0,
            static_cast<LONG>(m_width),
            static_cast<LONG>(m_height) };
        D3DXVECTOR3 center { 0, 0, 0 };
        m_D3DSprite->Draw(
            m_pD3DTexture,
            &rect,
            &center,
            &pos,
            D3DCOLOR_ARGB(transparency, 255, 255, 255));
        m_D3DSprite->End();

    }

    void Load(const std::wstring& filepath) override
    {
        LPD3DXSPRITE tempSprite { nullptr };
        if (FAILED(D3DXCreateSprite(m_pD3DDevice, &m_D3DSprite)))
        {
            throw std::exception("Failed to create a sprite.");
        }

        if (FAILED(D3DXCreateTextureFromFile(
            m_pD3DDevice,
            filepath.c_str(),
            &m_pD3DTexture)))
        {
            throw std::exception("Failed to create a texture.");
        }

        D3DSURFACE_DESC desc { };
        if (FAILED(m_pD3DTexture->GetLevelDesc(0, &desc)))
        {
            throw std::exception("Failed to create a texture.");
        }
        m_width = desc.Width;
        m_height = desc.Height;
    }

    ISprite* Create()
    {
        return new Sprite(m_pD3DDevice);
    }

    ~Sprite()
    {
        if (m_D3DSprite != nullptr)
        {
            m_D3DSprite->Release();
        }

        if (m_pD3DTexture != nullptr)
        {
            m_pD3DTexture->Release();
        }
    }

private:

    LPDIRECT3DDEVICE9 m_pD3DDevice = NULL;
    LPD3DXSPRITE m_D3DSprite = NULL;
    LPDIRECT3DTEXTURE9 m_pD3DTexture = NULL;
    UINT m_width { 0 };
    UINT m_height { 0 };
};

class Font : public IFont
{
public:

    Font(LPDIRECT3DDEVICE9 pD3DDevice)
        : m_pD3DDevice(pD3DDevice)
    {
    }

    void Init(const bool bEnglish)
    {
        if (!bEnglish)
        {
            HRESULT hr = D3DXCreateFont(m_pD3DDevice,
                                        24,
                                        0,
                                        FW_NORMAL,
                                        1,
                                        false,
                                        SHIFTJIS_CHARSET,
                                        OUT_TT_ONLY_PRECIS,
                                        ANTIALIASED_QUALITY,
                                        FF_DONTCARE,
                                        _T("ＭＳ 明朝"),
                                        &m_pFont);
        }
        else
        {
            HRESULT hr = D3DXCreateFont(m_pD3DDevice,
                                        24,
                                        0,
                                        FW_NORMAL,
                                        1,
                                        false,
                                        DEFAULT_CHARSET,
                                        OUT_TT_ONLY_PRECIS,
                                        CLEARTYPE_QUALITY,
                                        FF_DONTCARE,
                                        _T("Courier New"),
                                        &m_pFont);
        }
    }

    virtual void DrawText_(const std::wstring& msg, const int x, const int y)
    {
        RECT rect = { x, y, 0, 0 };
        m_pFont->DrawText(NULL, msg.c_str(), -1, &rect, DT_LEFT | DT_NOCLIP,
            D3DCOLOR_ARGB(255, 255, 255, 255));
    }

    ~Font()
    {
        m_pFont->Release();
        m_pFont = nullptr;
    }

private:

    LPDIRECT3DDEVICE9 m_pD3DDevice = NULL;
    LPD3DXFONT m_pFont = NULL;
};


class SoundEffect : public ISoundEffect
{
    virtual void PlayMove() override
    {
        PlaySound(_T("cursor_move.wav"), NULL, SND_FILENAME | SND_ASYNC);
    }
    virtual void Init() override
    {

    }
};

LPDIRECT3D9 g_pD3D = NULL;
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
LPD3DXFONT g_pFont = NULL;
LPD3DXMESH pMesh = NULL;
D3DMATERIAL9* pMaterials = NULL;
LPDIRECT3DTEXTURE9* pTextures = NULL;
DWORD dwNumMaterials = 0;
LPD3DXEFFECT pEffect = NULL;
D3DXMATERIAL* d3dxMaterials = NULL;
float f = 0.0f;
bool bFinish = false;

SlideShow* story = nullptr;

void TextDraw(LPD3DXFONT pFont, wchar_t* text, int X, int Y)
{
    RECT rect = { X,Y,0,0 };
    pFont->DrawText(NULL, text, -1, &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255, 0, 0, 0));
}

HRESULT InitD3D(HWND hWnd)
{
    if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        return E_FAIL;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.BackBufferCount = 1;
    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality = 0;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.hDeviceWindow = hWnd;
    d3dpp.Flags = 0;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice)))
    {
        if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice)))
        {
            return(E_FAIL);
        }
    }

    HRESULT hr = D3DXCreateFont(
        g_pd3dDevice,
        20,
        0,
        FW_HEAVY,
        1,
        false,
        SHIFTJIS_CHARSET,
        OUT_TT_ONLY_PRECIS,
        ANTIALIASED_QUALITY,
        FF_DONTCARE,
        _T("ＭＳ ゴシック"),
        &g_pFont);
    if FAILED(hr)
    {
        return(E_FAIL);
    }

    LPD3DXBUFFER pD3DXMtrlBuffer = NULL;

    if (FAILED(D3DXLoadMeshFromX(_T("cube.x"), D3DXMESH_SYSTEMMEM,
        g_pd3dDevice, NULL, &pD3DXMtrlBuffer, NULL,
        &dwNumMaterials, &pMesh)))
    {
        MessageBox(NULL, _T("Xファイルの読み込みに失敗しました"), NULL, MB_OK);
        return E_FAIL;
    }
    d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    pMaterials = new D3DMATERIAL9[dwNumMaterials];
    pTextures = new LPDIRECT3DTEXTURE9[dwNumMaterials];

    for (DWORD i = 0; i < dwNumMaterials; i++)
    {
        pMaterials[i] = d3dxMaterials[i].MatD3D;
        pMaterials[i].Ambient = pMaterials[i].Diffuse;
        pTextures[i] = NULL;

        int len = MultiByteToWideChar(CP_UTF8, 0, d3dxMaterials[i].pTextureFilename, -1, nullptr, 0);
        std::wstring result(len - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, d3dxMaterials[i].pTextureFilename, -1, &result[0], len);

        if (!result.empty())
        {
            if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice, result.c_str(), &pTextures[i])))
            {
                MessageBox(NULL, _T("テクスチャの読み込みに失敗しました"), NULL, MB_OK);
            }
        }
    }
    pD3DXMtrlBuffer->Release();

    D3DXCreateEffectFromFile(
        g_pd3dDevice,
        _T("simple.fx"),
        NULL,
        NULL,
        D3DXSHADER_DEBUG,
        NULL,
        &pEffect,
        NULL
    );

    return S_OK;
}

void InitStory()
{
    // newはライブラリの使用者がするが、deleteはライブラリ内で行われる。
    // ちょっと良くないけど・・・まぁよし！
    ISoundEffect* pSE = new SoundEffect();

    Sprite* sprTextBack = new Sprite(g_pd3dDevice);
    sprTextBack->Load(_T("textBack.png"));

    Sprite* sprFade = new Sprite(g_pd3dDevice);
    sprFade->Load(_T("black.png"));

    IFont* pFont = new Font(g_pd3dDevice);

    // csvファイルから読むようにしたほうがいいような
    // 別に必要ないような、微妙なところ。
    // 巨大なゲームを作るわけじゃないし。
    // 追記：必要だった。

    if (_T("csv mode"))
    {
        Sprite* sprite = new Sprite(g_pd3dDevice);
        story->Init(pFont, pSE, sprTextBack, sprFade, _T("..\\Sample\\sample.csv"), sprite, false, true);
    }
    else
    {
        std::vector<Page> pageList;
        {
            Page page;
            Sprite* sprite = new Sprite(g_pd3dDevice);
            sprite->Load(_T("opening01.png"));
            page.SetSprite(sprite);
            std::vector<std::vector<std::wstring> > vss;
            std::vector<std::wstring> vs;
            vs.push_back(_T("サンプルテキスト１"));
            vs.push_back(_T("サンプルテキスト２"));
            vs.push_back(_T("サンプルテキスト３"));
            vss.push_back(vs);
            vs.clear();
            vs.push_back(_T("サンプルテキスト４サンプルテキスト４サンプルテキスト４"));
            vs.push_back(_T("サンプルテキスト５サンプルテキスト５サンプルテキスト５"));
            vs.push_back(_T("サンプルテキスト６サンプルテキスト６サンプルテキスト６"));
            vss.push_back(vs);
            vs.clear();
            vs.push_back(_T("サンプルテキスト７サンプルテキスト７サンプルテキスト７サンプルテキスト７サンプルテキスト７"));
            vs.push_back(_T("サンプルテキスト８サンプルテキスト８サンプルテキスト８サンプルテキスト８サンプルテキスト８"));
            vs.push_back(_T("サンプルテキスト９サンプルテキスト９サンプルテキスト９サンプルテキスト９サンプルテキスト９"));
            vss.push_back(vs);
            page.SetTextList(vss);
            pageList.push_back(page);
        }
        {
            Page page;
            Sprite* sprite = new Sprite(g_pd3dDevice);
            sprite->Load(_T("opening02.png"));
            page.SetSprite(sprite);
            std::vector<std::vector<std::wstring> > vss;
            std::vector<std::wstring> vs;
            vs.push_back(_T("サンプルテキストＡ"));
            vs.push_back(_T("サンプルテキストＢ"));
            vs.push_back(_T("サンプルテキストＣ"));
            vss.push_back(vs);
            vs.clear();
            vs.push_back(_T("サンプルテキストＤサンプルテキストＤサンプルテキストＤ"));
            vs.push_back(_T("サンプルテキストＥサンプルテキストＥ"));
            vs.push_back(_T("サンプルテキストＦ"));
            vss.push_back(vs);
            page.SetTextList(vss);
            pageList.push_back(page);
        }
        {
            Page page;
            Sprite* sprite = new Sprite(g_pd3dDevice);
            sprite->Load(_T("opening03.png"));
            page.SetSprite(sprite);
            std::vector<std::vector<std::wstring> > vss;
            std::vector<std::wstring> vs;
            vs.push_back(_T("１１１１１１１１１１１"));
            vs.push_back(_T("２２２２２２２２２２２２２"));
            vs.push_back(_T("３３３３３３３３３３３３３３３３３"));
            vss.push_back(vs);
            vs.clear();
            vs.push_back(_T("４４４４４４４４４４４４４４４４４４４４４４４４４４４４４４４４４"));
            vs.push_back(_T(""));
            vss.push_back(vs);
            vs.clear();
            vs.push_back(_T("５５５５５５５５５５５５５５５５５"));
            vss.push_back(vs);
            page.SetTextList(vss);
            pageList.push_back(page);
        }

        story->Init(pFont, pSE, sprTextBack, sprFade, pageList, true);
    }
}

VOID Cleanup()
{
    SAFE_RELEASE(pMesh);
    SAFE_RELEASE(g_pFont);
    delete[] pMaterials;
    pMaterials = nullptr;
    delete[] pTextures;
    pTextures = nullptr;
    SAFE_RELEASE(pEffect);
    SAFE_RELEASE(g_pd3dDevice);
    SAFE_RELEASE(g_pD3D);
}

VOID Render()
{
    if (NULL == g_pd3dDevice)
    {
        return;
    }
    f += 0.010f;

    D3DXMATRIX mat;
    D3DXMATRIX View, Proj;
    D3DXMatrixPerspectiveFovLH(&Proj, D3DXToRadian(45), 1600.0f / 900.0f, 1.0f, 10000.0f);
    D3DXVECTOR3 vec1(3 * sinf(f), 3, -3 * cosf(f));
    D3DXVECTOR3 vec2(0, 0, 0);
    D3DXVECTOR3 vec3(0, 1, 0);
    D3DXMatrixLookAtLH(&View, &vec1, &vec2, &vec3);
    D3DXMatrixIdentity(&mat);
    mat = mat * View * Proj;
    pEffect->SetMatrix("matWorldViewProj", &mat);

    if (story != nullptr)
    {
        bFinish = story->Update();
        if (bFinish)
        {
            story->Finalize();
            delete story;
            story = nullptr;
        }
    }

    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
        D3DCOLOR_XRGB(100, 100, 100), 1.0f, 0);

    if (SUCCEEDED(g_pd3dDevice->BeginScene()))
    {
        wchar_t msg[128];
        wcscpy_s(msg, 128, _T("Ｍキーで紙芝居開始"));
        TextDraw(g_pFont, msg, 0, 0);

        pEffect->SetTechnique("BasicTec");
        UINT numPass;
        pEffect->Begin(&numPass, 0);
        pEffect->BeginPass(0);
        for (DWORD i = 0; i < dwNumMaterials; i++)
        {
            pEffect->SetTexture("texture1", pTextures[i]);
            pMesh->DrawSubset(i);
        }
        if (story != nullptr)
        {
            story->Render();
        }
        pEffect->EndPass();
        pEffect->End();
        g_pd3dDevice->EndScene();
    }

    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        Cleanup();
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        Render();
        return 0;
    case WM_SIZE:
        InvalidateRect(hWnd, NULL, true);
        return 0;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case 'M':
        {
            if (story != nullptr)
            {
                story->Finalize();
                delete story;
            }
            story = new SlideShow();
            InitStory();
            break;
        }
        case 'F':
        {
            SlideShow::SetFastMode(true);
            break;
        }
        case 'G':
        {
            SlideShow::SetFastMode(false);
            break;
        }
        case VK_RETURN:
        {
            if (story != nullptr)
            {
                story->Next();
            }
            break;
        }
        case VK_ESCAPE:
            PostQuitMessage(0);
            break;
        }
        break;
    case WM_LBUTTONDOWN:
    {
        if (story != nullptr)
        {
            story->Next();
        }
        break;
    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

INT WINAPI wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ INT)
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      _T("Window1"), NULL };
    RegisterClassEx(&wc);

    RECT rect;
    SetRect(&rect, 0, 0, 1600, 900);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    rect.right = rect.right - rect.left;
    rect.bottom = rect.bottom - rect.top;
    rect.top = 0;
    rect.left = 0;

    HWND hWnd = CreateWindow(_T("Window1"), _T("Hello DirectX9 World !!"),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right, rect.bottom,
        NULL, NULL, wc.hInstance, NULL);

    if (SUCCEEDED(InitD3D(hWnd)))
    {
        ShowWindow(hWnd, SW_SHOWDEFAULT);
        UpdateWindow(hWnd);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UnregisterClass(_T("Window1"), wc.hInstance);
    Cleanup();
    _CrtDumpMemoryLeaks();
    return 0;
}
