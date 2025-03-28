#include "App/AEngine.h"
#include "App/AppRunner.h"

AEngine::AEngine(UINT width, UINT height, std::wstring name)
    : AAppBase(width, height, name), //
      mViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
      mScissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
{
}

// Init Steps
void AEngine::OnInit()
{
    InitPipeline();
    InitAssets();
}

//
void AEngine::InitPipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    /**
     * ID3D12Device
     * @brief D3D12 Device是dx12整个图形API的核心接口。代表了GPU的逻辑抽象，充当所有其他DX12对象的创建者。
     * 代表了一个显卡的编程接口
     * @param 需要IDXGI Factory --> 适配器 --> 设备
     */

    /**
     * DXGI DirectX Graphics Infrastructure
     * @brief DXGI是DirectX的一个组成部分，提供了一组用于管理图形设备的API。 同一为dx10 11 12提供基础设施
     * 给出一个硬件交互层
     * 主要功能：
     * 1. 找到物理显卡硬件
     * 2. 管理交换链
     */

    /**
     * 所以第一步 为了拿到一个D3D12 Device，我们需要先创建一个DXGI Factory
     */

    ComPtr<IDXGIFactory5> pIDXGIFactory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pIDXGIFactory)));

    // D3D12
    ComPtr<IDXGIAdapter1> pIAdapter;
    for (int adapterIndex = 0; pIDXGIFactory->EnumAdapters1(adapterIndex, &pIAdapter) != DXGI_ERROR_NOT_FOUND; adapterIndex++)
    {
        DXGI_ADAPTER_DESC1 desc = {};
        pIAdapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        if (SUCCEEDED(D3D12CreateDevice(pIAdapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

    ThrowIfFailed(D3D12CreateDevice(pIAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&pDevice)));

    // 接下来我们的渲染管线需要初始化
    // 需要我们的命令队列
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue)));

    // 创建交换链
    // 交换链是一个用于前后缓冲交换的对象
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = nFrameBackBufCount;
    swapChainDesc.Width = GetWidth();
    swapChainDesc.Height = GetHeight();
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> SwapChain;
    ThrowIfFailed(pIDXGIFactory->CreateSwapChainForHwnd(
        pCommandQueue.Get(),
        AAppRunner::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &SwapChain));

    ThrowIfFailed(pIDXGIFactory->MakeWindowAssociation(AAppRunner::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(SwapChain.As(&pSwapChain));
    nFrameIndex = pSwapChain->GetCurrentBackBufferIndex();

    // 描述堆符
    // 描述堆符是一个用于存储描述符的对象
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = nFrameBackBufCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&pRtvHeap)));
    nRtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (UINT n = 0; n < nFrameBackBufCount; n++)
    {
        ThrowIfFailed(pSwapChain->GetBuffer(n, IID_PPV_ARGS(&pRenderTargets[n])));
        pDevice->CreateRenderTargetView(pRenderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, nRtvDescriptorSize);
    }
}

void AEngine::InitAssets()
{
    // 根签名
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> pSignature;
    ComPtr<ID3DBlob> pError;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError));
    ThrowIfFailed(pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));

    ComPtr<ID3DBlob> pVertexShader;
    ComPtr<ID3DBlob> pPixelShader;

#if defined(_DEBUG)
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ThrowIfFailed(D3DCompileFromFile(L"../../Shaders/HelloTriangle.hlsl", nullptr, nullptr, "MainVS", "vs_5_0", compileFlags, 0, &pVertexShader, nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"../../Shaders/HelloTriangle.hlsl", nullptr, nullptr, "MainPS", "ps_5_0", compileFlags, 0, &pPixelShader, nullptr));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
    psoDesc.pRootSignature = pRootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    ThrowIfFailed(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPipelineState)));

    ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator)));
    ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator.Get(), pPipelineState.Get(), IID_PPV_ARGS(&pCommandList)));
    ThrowIfFailed(pCommandList->Close());

    // Create Vertex Buffer
    Vertex triangleVertices[] =
        {
            {{0.0f, 0.25f * GetAspectRatio(), 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.25f * GetAspectRatio(), -0.25f * GetAspectRatio(), 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{-0.25f * GetAspectRatio(), -0.25f * GetAspectRatio(), 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}} //
        };

    const UINT vertexBufferSize = sizeof(triangleVertices);
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pVertexBuffer)));

    UINT8 *pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(pVertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pVertexDataBegin)));
    memcpy(pVertexDataBegin, triangleVertices, vertexBufferSize);
    pVertexBuffer->Unmap(0, nullptr);

    stVertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
    stVertexBufferView.StrideInBytes = sizeof(Vertex);
    stVertexBufferView.SizeInBytes = vertexBufferSize;

    ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
    nFenceValue = 1;
    hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (hFenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    WaitForPreviousFrame();
}

void AEngine::OnUpdate()
{
}

void AEngine::OnRender()
{
    PopulateCommandList();

    ID3D12CommandList *ppCommandLists[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    ThrowIfFailed(pSwapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void AEngine::PopulateCommandList()
{
    // Command Reset - 必须是等CommandQueue执行完毕后才能Reset  所以帧结束需要Wait来同步
    ThrowIfFailed(pCommandAllocator->Reset());
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), pPipelineState.Get()));

    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
    pCommandList->RSSetViewports(1, &mViewport);
    pCommandList->RSSetScissorRects(1, &mScissorRect);

    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargets[nFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRtvHeap->GetCPUDescriptorHandleForHeapStart(), nFrameIndex, nRtvDescriptorSize);
    pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
    pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &stVertexBufferView);
    pCommandList->DrawInstanced(3, 1, 0, 0);

    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargets[nFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(pCommandList->Close());
}

void AEngine::OnDestroy()
{
    WaitForPreviousFrame();
    CloseHandle(hFenceEvent);
}

void AEngine::WaitForPreviousFrame()
{
    const UINT64 fence = nFenceValue;
    ThrowIfFailed(pCommandQueue->Signal(pFence.Get(), fence));
    nFenceValue++;

    if (pFence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(pFence->SetEventOnCompletion(fence, hFenceEvent));
        WaitForSingleObject(hFenceEvent, INFINITE);
    }

    nFrameIndex = pSwapChain->GetCurrentBackBufferIndex();
}