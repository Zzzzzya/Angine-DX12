#pragma once

#include <Basic/Basic.h>
#include <Basic/Utils.h>
class AAppRunner;
class AAppBase
{
  public:
    AAppBase(UINT width, UINT height, std::wstring name);
    virtual ~AAppBase();

    virtual void OnInit() {};
    virtual void OnUpdate() {};
    virtual void OnRender() {};
    virtual void OnDestroy() {};

    virtual void OnKeyDown(UINT8 key) {};
    virtual void OnKeyUp(UINT8 key) {};

    // Accessors. 所有的存取器
    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }
    UINT GetAspectRatio() const { return m_aspectRatio; }
    const WCHAR *GetTitle() const { return m_title.c_str(); }

  protected:
    // Viewport dimensions.
    UINT m_width;
    UINT m_height;
    float m_aspectRatio;

    void GetHardwareAdapter(IDXGIFactory2 *pFactory, IDXGIAdapter1 **ppAdapter);

  private:
    // Window title.
    std::wstring m_title;
};