#include "App/AppRunner.h"
#include "App/AEngine.h"

int main()
{
    auto hInstance = GetModuleHandle(NULL);
    AEngine app(800, 600, L"AY_DX12");
    AAppRunner::Run(&app, hInstance);
    return 0;
}