#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>

#include <wrl.h>
using namespace Microsoft::WRL;
using namespace Microsoft;

#include <dxgi1_6.h>

#include <DirectXMath.h>
using namespace DirectX;

// for d3d12
#include <d3d12.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>

#include <d3dx12.h>

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

// cpp
#include <exception>

#define AY_WND_CLASS_NAME _T("AY_DX12_CLASS")
#define AY_WND_TITLE _T("AY_DX12")
#define THROW_IF_FAILED(hr)     \
    if (FAILED(hr))             \
    {                           \
        throw std::exception(); \
    }

// Draw a triangle

// Vertex
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

int main(int argc, char *argv[])
{
    // 获取当前模块的实例句柄（相当于WinMain中的hInstance）
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // 1. 定义变量
    const UINT nFrameBackBufCount = 3u;

    int iWidth = 800;
    int iHeight = 600;

    UINT nFrameIndex = 0u;
    UINT nFrame = 0u;

    UINT nDXGIFactoryFlags = 0u;
    UINT nRTVDescriptorSize = 0u;

    HWND hWnd = nullptr;

    MSG msg = {};

    float fAspectRatio = 3.0f;

    D3D12_VERTEX_BUFFER_VIEW stVertexBufferView = {};
    UINT64 nFenceValue = 0u;

    CD3DX12_VIEWPORT stViewPort(0.0f, 0.0f, static_cast<float>(iWidth), static_cast<float>(iHeight));
    CD3DX12_RECT stScissorRect(0, 0, iWidth, iHeight);

    ComPtr<IDXGIFactory5> pIDXGIFactory;
    ComPtr<IDXGIAdapter1> pIAdapter;
    ComPtr<ID3D12Device> pID3DDevice;
    ComPtr<ID3D12CommandQueue> pICommandQueue;
    ComPtr<IDXGISwapChain1> pISwapChain1;
    ComPtr<IDXGISwapChain3> pISwapChain3;
    ComPtr<ID3D12DescriptorHeap> pIRTVHeap;
    ComPtr<ID3D12Resource> pIARenderTargets[nFrameBackBufCount];
    ComPtr<ID3D12CommandAllocator> pICommandAllocator;
    ComPtr<ID3D12RootSignature> pIRootSignature;
    ComPtr<ID3D12PipelineState> pIPipelineState;
    ComPtr<ID3D12GraphicsCommandList> pICommandList;
    ComPtr<ID3D12Resource> pIVertexBuffer;
    ComPtr<ID3D12Fence> pIFence;

    // 2. 创建窗口
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_GLOBALCLASS;
    wcex.lpfnWndProc = DefWindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(GetStockObject(NULL_BRUSH));
    wcex.lpszClassName = AY_WND_CLASS_NAME;
    RegisterClassEx(&wcex);

    DWORD dwStyle = WS_OVERLAPPED | WS_SYSMENU;
    RECT rtWnd = {0, 0, iWidth, iHeight};
    AdjustWindowRect(&rtWnd, dwStyle, FALSE);

    // 计算窗口居中的屏幕坐标
    int posX = (GetSystemMetrics(SM_CXSCREEN) - rtWnd.right + rtWnd.left) / 2;
    int posY = (GetSystemMetrics(SM_CYSCREEN) - rtWnd.bottom + rtWnd.top) / 2;

    hWnd = CreateWindow(
        AY_WND_CLASS_NAME,
        AY_WND_TITLE,
        dwStyle,
        posX,
        posY,
        rtWnd.right - rtWnd.left,
        rtWnd.bottom - rtWnd.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // D3D12
    // 创建DXGI工厂
    /**
     * IDXGIFactory ==> 代表整个图形子系统
     * Factory创建 ---- 适配器 显示器 3D设备
     */
    CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(&pIDXGIFactory));

    // 创建D3D12设备
    // Factory -- 寻找适配器
    for (int adapterIndex = 0; pIDXGIFactory->EnumAdapters1(adapterIndex, &pIAdapter) != DXGI_ERROR_NOT_FOUND; adapterIndex++)
    {
        // 遍历所有适配器
        DXGI_ADAPTER_DESC1 desc = {};
        pIAdapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // 软件适配器 跳过
            continue;
        }

        // 传入nullptr -- 避免没创建成功时还得释放 - 这里只是检查Adapter是否支持D3D12 12_1
        if (SUCCEEDED(D3D12CreateDevice(pIAdapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

    THROW_IF_FAILED(D3D12CreateDevice(pIAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&pID3DDevice)));

    // 创建命令队列
    // OpenGL | DX11 ------ Context ------
    // DX12 -- 命令队列
    /**
     * CommandQueue 命令队列
     * 1. 3D设备
     * 2. Copy Engine
     * 3. Compute Engine
     * 4. Video Engine
     */
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // 啥都能干 - 图形渲染就用这个
    THROW_IF_FAILED(pID3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pICommandQueue)));

    // 创建交换链
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = nFrameBackBufCount;
    swapChainDesc.Width = iWidth;
    swapChainDesc.Height = iHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    THROW_IF_FAILED(pIDXGIFactory->CreateSwapChainForHwnd(
        pICommandQueue.Get(),
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &pISwapChain1));

    // 获取当前绘制的后缓冲序号 - 只有在SwapChain3版本可用
    THROW_IF_FAILED(pISwapChain1.As(&pISwapChain3));
    nFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

    // RTV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = nFrameBackBufCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    THROW_IF_FAILED(pID3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&pIRTVHeap)));
    nRTVDescriptorSize = pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pIRTVHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < nFrameBackBufCount; i++)
    {
        THROW_IF_FAILED(pISwapChain3->GetBuffer(i, IID_PPV_ARGS(&pIARenderTargets[i])));
        pID3DDevice->CreateRenderTargetView(pIARenderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, nRTVDescriptorSize);
    }

    // 根签名
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> pSignature;
    ComPtr<ID3DBlob> pError;

    THROW_IF_FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError));
    THROW_IF_FAILED(pID3DDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&pIRootSignature)));

    // 创建渲染管线对象
    // Pipeline State Object - PSO
    // PSO - Rasterizer State + Depth Stencil State + Blend State + Input Layout + Vertex Shader + Pixel Shader
    ComPtr<ID3DBlob> pVertexShader;
    ComPtr<ID3DBlob> pPixelShader;

    // DEBUG
    UINT nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    TCHAR pszShaderFileName[] = _T("../../Shaders/dx12-triangle.hlsl");

    THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr, "MainVS", "vs_5_0", nCompileFlags, 0, &pVertexShader, nullptr));
    THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr, "MainPS", "ps_5_0", nCompileFlags, 0, &pPixelShader, nullptr));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
    psoDesc.pRootSignature = pIRootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    THROW_IF_FAILED(pID3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pIPipelineState)));

    Vertex triangleVertices[] =
        {
            {{0.0f, 0.25f * fAspectRatio, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.25f * fAspectRatio, -0.25f * fAspectRatio, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{-0.25f * fAspectRatio, -0.25f * fAspectRatio, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

    const UINT nVertexBufferSize = sizeof(triangleVertices);

    auto HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto ResDesc = CD3DX12_RESOURCE_DESC::Buffer(nVertexBufferSize);
    THROW_IF_FAILED(pID3DDevice->CreateCommittedResource(
        &HeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &ResDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pIVertexBuffer)));

    UINT8 *pVertexDataBegin = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    THROW_IF_FAILED(pIVertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pVertexDataBegin)));
    memcpy(pVertexDataBegin, triangleVertices, nVertexBufferSize);
    pIVertexBuffer->Unmap(0, nullptr);

    stVertexBufferView.BufferLocation = pIVertexBuffer->GetGPUVirtualAddress();
    stVertexBufferView.StrideInBytes = sizeof(Vertex);
    stVertexBufferView.SizeInBytes = nVertexBufferSize;

    // 命令分配器
    THROW_IF_FAILED(pID3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pICommandAllocator)));
    THROW_IF_FAILED(pID3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pICommandAllocator.Get(), pIPipelineState.Get(), IID_PPV_ARGS(&pICommandList)));

    // 围栏
    THROW_IF_FAILED(pID3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pIFence)));
    nFenceValue = 1;
    auto hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (hFenceEvent == nullptr)
    {
        THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Render
    HANDLE phWait = CreateWaitableTimer(nullptr, FALSE, nullptr);
    LARGE_INTEGER liDueTime = {};

    liDueTime.QuadPart = -1i64;
    SetWaitableTimer(phWait, &liDueTime, 0, nullptr, nullptr, FALSE);

    DWORD dwRet = 0;
    BOOL bExit = FALSE;

    while (!bExit)
    {
        dwRet = ::MsgWaitForMultipleObjects(1, &phWait, FALSE, INFINITE, QS_ALLINPUT);
        switch (dwRet - WAIT_OBJECT_0)
        {
        case 0:
        case WAIT_TIMEOUT:
        {
            pICommandList->SetGraphicsRootSignature(pIRootSignature.Get());
            pICommandList->RSSetViewports(1, &stViewPort);
            pICommandList->RSSetScissorRects(1, &stScissorRect);

            {
                auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(pIARenderTargets[nFrameIndex].Get(),
                    D3D12_RESOURCE_STATE_PRESENT,
                    D3D12_RESOURCE_STATE_RENDER_TARGET);
                pICommandList->ResourceBarrier(1, &Barrier);
            }
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pIRTVHeap->GetCPUDescriptorHandleForHeapStart(), nFrameIndex, nRTVDescriptorSize);

            // 设置渲染目标
            pICommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
            // 继续记录命令 开始新一帧渲染

            const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
            pICommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
            pICommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            pICommandList->IASetVertexBuffers(0, 1, &stVertexBufferView);

            // Draw Call
            pICommandList->DrawInstanced(3, 1, 0, 0);

            {
                auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(pIARenderTargets[nFrameIndex].Get(),
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    D3D12_RESOURCE_STATE_PRESENT);
                pICommandList->ResourceBarrier(1, &Barrier);
            }

            // 结束命令记录
            THROW_IF_FAILED(pICommandList->Close());
            // 执行命令列表
            ID3D12CommandList *ppCommandLists[] = {pICommandList.Get()};
            pICommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

            THROW_IF_FAILED(pISwapChain3->Present(1, 0));

            const UINT64 fence = nFenceValue;
            THROW_IF_FAILED(pICommandQueue->Signal(pIFence.Get(), fence));
            nFenceValue++;

            if (pIFence->GetCompletedValue() < fence)
            {
                THROW_IF_FAILED(pIFence->SetEventOnCompletion(fence, hFenceEvent));
                WaitForSingleObject(hFenceEvent, INFINITE);
            }

            // 一帧渲染结束
            nFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();
            THROW_IF_FAILED(pICommandAllocator->Reset());
            THROW_IF_FAILED(pICommandList->Reset(pICommandAllocator.Get(), pIPipelineState.Get()));
        }
        break;
        case 1:
        {
            while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    bExit = TRUE;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        break;
        default:
            break;
        }
    };
}