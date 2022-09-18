#pragma once
#include "..\Public\headers.h"
#include "..\Public\GameTime.h"

using namespace Microsoft::WRL;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool InitWindowsApp(HINSTANCE instanceHandle, int show);
int Run();

IDXGIAdapter1* GetSupportedAdapter(ComPtr<IDXGIFactory4>& dxgiFactory, const D3D_FEATURE_LEVEL featureLevel);
void EnableDebugLayer();
void CreateDxgiFactory();
void FindAdapter();
void CreateDevice();
void CreateFence();
void GetDescriptorSize();
void CheckMSAASupport();
void CreateCommandObject();
void CreateSwapChain();
void CreateDescriptorHeap();
void CreateRtv();
void CreateDsv();
void FlushCommandQueue();
void SetViewport();
void SetScissorRect();
bool InitD3D();
bool Init(HINSTANCE instanceHandle, int show);
void Draw();
