#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>

#include <D3DCompiler.h>
#include <DirectXMath.h>

#include <wrl.h>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace Microsoft;

// cpp
#include <exception>
#include <string.h>

#include <stdexcept>