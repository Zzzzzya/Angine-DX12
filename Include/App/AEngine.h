#pragma once

#include <App/AppBase.h>

class AAppRunner;
class AEngine : public AAppBase
{
  public:
    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

  public:
    explicit AEngine(UINT width, UINT height, std::wstring name);
    ~AEngine() {};

    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnDestroy() override;

  private:
    // 预备
    static const UINT nFrameBackBufCount = 3u;

    // DXGI
    ComPtr<IDXGISwapChain3> pSwapChain;

    // Render Needed
    CD3DX12_VIEWPORT mViewport;
    CD3DX12_RECT mScissorRect;

    // DX12 NEEDED
    ComPtr<ID3D12Device> pDevice;
    ComPtr<ID3D12CommandQueue> pCommandQueue;
    ComPtr<ID3D12CommandAllocator> pCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> pCommandList;
    ComPtr<ID3D12DescriptorHeap> pRtvHeap;
    ComPtr<ID3D12Resource> pRenderTargets[nFrameBackBufCount];
    UINT nRtvDescriptorSize;

    // Assets
    ComPtr<ID3D12RootSignature> pRootSignature;
    ComPtr<ID3D12PipelineState> pPipelineState;

    ComPtr<ID3D12Resource> pVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW stVertexBufferView;

    // Synchroization objects
    UINT nFrameIndex;
    HANDLE hFenceEvent;
    ComPtr<ID3D12Fence> pFence;
    UINT64 nFenceValue;

  private:
    // Init Steps
    void InitPipeline();
    void InitAssets();

    // Render
    void PopulateCommandList();

    // Sync
    void WaitForPreviousFrame();
};