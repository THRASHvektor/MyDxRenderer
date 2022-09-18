#include "..\Public\Initialize.h"



//////////////////////////////////////////////WINDOW INITIALIZE//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HWND ghMainWnd = 0;
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:															//MOUSE LEFT CLICK
		MessageBox(0, L"HELLWORLD", L"Hello", MB_OK);
		return 0;
	case WM_KEYDOWN:																//NOT KEYBOARD
		if (wParam == VK_ESCAPE)
			DestroyWindow(ghMainWnd);
		return 0;
	case WM_DESTROY:																//WINDOW DESTROYED
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);								//OTHER MESSAGE
}

//INITIALIZE WINDOW
bool InitWindowsApp(HINSTANCE instanceHandle, int show)
{
	//FILL IN THE WNDCLASS STRUCT
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;														//WINDOW PROCESS FUNCTION
	wc.cbClsExtra = 0;																//EXTRA MEMORY SPACE
	wc.cbWndExtra = 0;																//EXTRA MEMORY SPACE
	wc.hInstance = instanceHandle;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);										//ICON OF THE WINDOW
	wc.hCursor = LoadCursor(0, IDC_ARROW);											//STYLE OF THE MOUSE
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);							//WHITE BACKGROUND
	wc.lpszMenuName = 0;															//NO MENU
	wc.lpszClassName = L"MainWnd";													//WINDOW'S NAME

	//REGISTER WC
	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"REGISTER FAILED", 0, 0);
		return false;
	}

	//CREATE A WINDOW AND RETURN ITS HANDLER
	ghMainWnd = CreateWindow(
		L"MainWnd",																	//USE wc.lpszClassName TO CREATE WINDOW
		L"DX12INITIALIZE",															//WINDOW'S NAME
		WS_OVERLAPPED,																//WINDOW'S STYLE
		CW_USEDEFAULT,																//X
		CW_USEDEFAULT,																//Y
		CW_USEDEFAULT,																//WIDTH
		CW_USEDEFAULT,																//HEIGHT
		0,																			//PARENT WINDOW HANDLER
		0,																			//MENU HANDLER
		instanceHandle,																//APPLICATION HANDLER
		0
	);
	if (!ghMainWnd)
	{
		MessageBox(0, L"CREATE WINDOW FAILED", 0, 0);
		return false;
	}

	//DISPLAY
	ShowWindow(ghMainWnd, show);
	UpdateWindow(ghMainWnd);
	return true;
}

//MESSAGE CIRCUMSTANCE
int Run()
{
	MSG msg = { 0 };
	while (msg.message!=WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))								//HAVE MESSAGE TO DEAL WITH
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Draw();
		}
	}
	return (int)msg.wParam;
}

//////////////////////////////////////////////WINDOW INITIALIZE////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int Width = 1280;
int Height = 720;
const int SwapChainBufferCount = 2;

ComPtr<ID3D12Device> md3dDevice;
ComPtr<IDXGIFactory4> mdxgiFactory;
ComPtr<ID3D12Fence> mfence;
ComPtr<ID3D12CommandQueue> mCommandQueue;
ComPtr<ID3D12CommandAllocator> mCommandAllocator;
ComPtr<ID3D12GraphicsCommandList> mCommandList;
ComPtr<IDXGISwapChain> mSwapChain;
ComPtr<ID3D12DescriptorHeap> mRtvHeap;
ComPtr<ID3D12DescriptorHeap> mDsvHeap;
ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
ComPtr<ID3D12Resource> mDepthStencilBuffer;

D3D12_VIEWPORT mViewport;
D3D12_RECT mScissorRect;

UINT mrtvDescriptorSize = 0;									//RENDERTARGET
UINT mdsvDescriptorSize = 0;									//DEPTH, STENCIL
UINT mCbvSrvUavDescriptorSize = 0;								//CBUFFER, SHADER RESOURCE, UAV
UINT mCurrentBackBuffer = 0;
UINT mCurrentFence = 0;

IDXGIAdapter1* adapter = nullptr;

////////////////////////////////////////////////////////////////////D3DINITIALIZE///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IDXGIAdapter1* GetSupportedAdapter(ComPtr<IDXGIFactory4>& dxgiFactory, const D3D_FEATURE_LEVEL featureLevel)
{
	IDXGIAdapter1* adapter = nullptr;
	for (std::uint32_t adapterIndex = 0U;; ++adapterIndex)
	{
		IDXGIAdapter1* currentAdapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == dxgiFactory->EnumAdapters1(adapterIndex, &currentAdapter))
			break;
		if (SUCCEEDED(D3D12CreateDevice(currentAdapter, featureLevel, _uuidof(ID3D12Device), nullptr)))
		{
			adapter = currentAdapter;
			break;
		}
		currentAdapter->Release();
	}
	return adapter;
}

//ENABLE DEBUG LAYER
void EnableDebugLayer()
{
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
#endif
}

//CREATE DXGI FACTORY
void CreateDxgiFactory()
{
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));
}

//FIND ADAPTERS
void FindAdapter()
{
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	for (std::uint32_t i = 0U; i < _countof(featureLevels); ++i)
	{
		adapter = GetSupportedAdapter(mdxgiFactory, featureLevels[i]);
		if (adapter != nullptr)	break;
	}
}

//CREATE DEVICE
void CreateDevice()
{
	ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&md3dDevice)));
}

//CREATE FENCE
void CreateFence()
{
	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mfence)));
}

//GET DESCRIPTOR SIZE
void GetDescriptorSize()
{
	mrtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);									//RENDERTARGET
	mdsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);									//DEPTH, STENCIL
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);					//CBUFFER, SHADER RESOURCE, UAV
}

//CHECK MSAA SUPPORT AND SET
void CheckMSAASupport()
{
	DXGI_FORMAT bufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = bufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));		//IF SUPPORT, RETURN msQualityLevels
	assert(msQualityLevels.NumQualityLevels > 0);
}

//CREATE COMMAND QUEUE, COMMAND ALLOCATOR AND COMMAND LIST
void CreateCommandObject()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
	ThrowIfFailed(md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
}

//CREATE SWAP CHAIN
void CreateSwapChain()
{
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	mSwapChain.Reset();
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = Width;
	sd.BufferDesc.Height = Height;
	sd.BufferDesc.RefreshRate.Numerator = 60;																	//REFRESHRATE = NUMERATOR/DENOMINATOR
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = backBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;										
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;																					//MSAA SETTING
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;																							//DOUBLE BUFFER, ONE FRONT ONE BACK
	sd.OutputWindow = ghMainWnd;
	sd.Windowed = true;																							//WHEN FALSE, RUN IN FULL SCREEN
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(mdxgiFactory->CreateSwapChain(mCommandQueue.Get(), &sd, &mSwapChain));
}

//CREATE BUFFER DESCRIPTOR HEAP
void CreateDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;

	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));
}

//CREATE RENDER TARGET VIEW
void CreateRtv()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer->Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mrtvDescriptorSize);
	}
	
}

//CREATE DEPTH/STENCIL BUFFER AND VIEW
void CreateDsv()
{
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;				
	depthStencilDesc.Alignment = 0;	
	depthStencilDesc.Width = Width;
	depthStencilDesc.Height = Height;
	depthStencilDesc.DepthOrArraySize = 1;													//TEXTURE ARRAY SIZE
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1;													//MSAA SETTING
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;									
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	//CREATE RESOURCE AND PUT IT INTO HEAP, WHOSE NATURE IS GPU BLOCK
	D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(md3dDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(&mDepthStencilBuffer)));
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	//CONVERT THE RESOURCE FROM COMMON STATE TO DEPTH BUFFER
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);		//PREVENT IT BEING CLEAR
	mCommandList->ResourceBarrier(1, &barrier);
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* mCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(mCommandLists), mCommandLists);
}

//FLUSHING THE COMMAND QUEUE, TO SYNCHRONIZE CPU AND GPU
void FlushCommandQueue()
{
	mCurrentFence++;
	//WAIT FOR GPU EXECUTE ALL THE COMMAND IN THE QUEUE
	//SET NEW FENCE
	mCommandQueue->Signal(mfence.Get(), mCurrentFence);
	//CPU WAIT FOR GPU TILL THE COMMAND BEFORE THE FENCE BE EXECUTED
	if (mfence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenseSetDone");
		//IF FENCE HIT
		mfence->SetEventOnCompletion(mCurrentFence, eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

}

//SET VIEWPORT
void SetViewport()
{
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width = Width;
	mViewport.Height = Height;
	mViewport.MaxDepth = 1.0f;
	mViewport.MinDepth = 0.0f;
}

//SET SCISSOR RECT
void SetScissorRect()
{
	mScissorRect.left = 0;
	mScissorRect.top = 0;
	mScissorRect.right = Width;
	mScissorRect.bottom = Height;
}

bool InitD3D()
{
	EnableDebugLayer();
	CreateDxgiFactory();
	FindAdapter();
	CreateDevice();
	CreateFence();
	GetDescriptorSize();
	CheckMSAASupport();
	CreateCommandObject();
	CreateSwapChain();
	CreateDescriptorHeap();
	CreateRtv();
	CreateDsv();
	FlushCommandQueue();
	SetViewport();
	SetScissorRect();
	return true;
}

////////////////////////////////////////////////////////////////////D3DINITIALIZE///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Init(HINSTANCE instanceHandle, int show)
{
	if (!InitWindowsApp(instanceHandle, show))
		return false;
	else if (!InitD3D())
		return false;
	else
		return true;
}


void Draw()
{
	//REUSE COMMAND ALLOCATOR AND COMMAND LIST
	ThrowIfFailed(mCommandAllocator->Reset());
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));
	//CONVERT TO RENDER TARGET TO GET READY TO BE RENDERED
	UINT& ref_mCurrentBackBuffer = mCurrentBackBuffer;
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mCommandList->ResourceBarrier(1, &barrier);
	//SET VIEWPORT AND SCISSORRECT
	mCommandList->RSSetViewports(1, &mViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	//GET HANDLE THEN CLEAR
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		ref_mCurrentBackBuffer, mrtvDescriptorSize);
	mCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::DeepPink, 0, nullptr);										//CLEAR THE WHOLE RENDER TARGET INSTEAD OF JUST A RECT
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr);
	//SET RENDER HANDLE
	mCommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle); 
	//CONVERT TO SHOW MODE,READY TO BE PUSH INTO FRONT BUFFER
	D3D12_RESOURCE_BARRIER barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCommandList->ResourceBarrier(1, &barrier1);
	
	ThrowIfFailed(mCommandList->Close());
	
	ID3D12CommandList* mCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(mCommandLists), mCommandLists);
	//SWAP FRONT BACK BUFFER
	ThrowIfFailed(mSwapChain->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;
	FlushCommandQueue();

}








