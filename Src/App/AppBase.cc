#include "App/AppBase.h"

AAppBase::AAppBase(UINT width, UINT height, std::wstring name)
    : m_width(width), m_height(height), m_title(name)
{
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

AAppBase::~AAppBase()
{
}

void AAppBase::GetHardwareAdapter(IDXGIFactory2 *pFactory, IDXGIAdapter1 **ppAdapter)
{
}