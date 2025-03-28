#pragma once

#include <Basic/Basic.h>
#include <App/AppBase.h>

class AAppBase;

class AAppRunner
{
  public:
    static int Run(AAppBase *app, HINSTANCE hInstance);
    static HWND GetHwnd() { return m_hWnd; }

  protected:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

  private:
    static HWND m_hWnd;
};