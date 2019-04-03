#include "stdafx.h"
#include "D3D11Renderer.h"
#include <d3dcompiler.h>

using namespace Microsoft::WRL;
using namespace DirectX;

static inline constexpr float deg_to_rad(float degree)
{
	return degree * F_PI / 180.0f;
}

D3D11Renderer::D3D11Renderer()
{
}

D3D11Renderer::~D3D11Renderer()
{
	if(mpImmediateContext)
	{
		mpImmediateContext->ClearState();
	}
}

b8 D3D11Renderer::Initialize(int32_t width, int32_t height, HWND hWnd)
{
	mClientWidth = width;
	mClientHeight = height;

	if(!CreateDevice())
	{
		return false;
	}

	if(!GetFactory())
	{
		return false;
	}

	if(!CreateSwapChain(hWnd))
	{
		return false;
	}

	if(!CreateVertexBuffer())
	{
		return false;
	}

	if(!LoadShader(L"shaders.hlsl", "VS", "vs_5_0", mpVertexShaderBlob))
	{
		return false;
	}

	if(!CreateInputLayout())
	{
		return false;
	}

	if(!CreateVertexShader())
	{
		return false;
	}

	if(!CreateVSConstantBuffer())
	{
		return false;
	}

	if(!CreatePixelShader())
	{
		return false;
	}

	if(!CreateRenderTargetView())
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::Render()
{
	if(!SetupGraphicsPipeline())
	{
		return false;
	}

	mpImmediateContext->ClearRenderTargetView(mpRTV.Get(), mClearColor);

	mpImmediateContext->Draw(mVertexCount, mVertexLocation);

	mpSwapChain->Present(0, 0);

	return true;
}

b8 D3D11Renderer::CreateDevice()
{
	// フラグの設定
	u32 flags = 0;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// フィーチャーレベルの列挙
	D3D_FEATURE_LEVEL feature_levels[]
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	// デバイスおよびイメディエイトコンテキストの生成
	ComPtr<ID3D11Device> pDevice;
	ComPtr<ID3D11DeviceContext> pImmediateContext;
	HRESULT hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		feature_levels,
		static_cast<u32>(std::size(feature_levels)),
		D3D11_SDK_VERSION,
		&pDevice,
		&mFeatureLevel,
		&pImmediateContext
	);
	if(FAILED(hr))
	{
		return false;
	}

	// ID3D11DeviceをmpDeviceのインターフェースに合わせる
	if(FAILED(pDevice.As(&mpDevice)))
	{
		return false;
	}

	// ID3D11DeviceContextをmpImmediateContextのインターフェースに合わせる
	if(FAILED(pImmediateContext.As(&mpImmediateContext)))
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::GetFactory()
{
	// ID3D11DeviceからIDXGIDeviceを取得する
	ComPtr<IDXGIDevice> pDXGIDevice;
	if(FAILED(mpDevice.As(&pDXGIDevice)))
	{
		return false;
	}

	// IDXGIDeviceからIDXGIAdapterを取得する
	ComPtr<IDXGIAdapter> pDXGIAdapter;
	if(FAILED(pDXGIDevice->GetAdapter(&pDXGIAdapter)))
	{
		return false;
	}

	// IDXGIAdapterからIDXGIFactoryを取得する
	if(FAILED(pDXGIAdapter->GetParent(IID_PPV_ARGS(&mpFactory))))
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::CreateSwapChain(HWND hWnd)
{
	// スワップチェインの設定
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1 {};
    swap_chain_desc1.Width = mClientWidth;
    swap_chain_desc1.Height = mClientHeight;
    swap_chain_desc1.Format = mSwapChainBufferFormat;
    swap_chain_desc1.Stereo = FALSE;
    swap_chain_desc1.SampleDesc.Count = 1;
    swap_chain_desc1.SampleDesc.Quality = 0;
    swap_chain_desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc1.BufferCount = SwapChainBufferCount;
    swap_chain_desc1.Scaling = DXGI_SCALING_STRETCH;
    swap_chain_desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc1.Flags = 0;

	// スワップチェインの生成
	ComPtr<IDXGISwapChain1> pSwapChain;
	HRESULT hr = mpFactory->CreateSwapChainForHwnd(
		mpDevice.Get(),
		hWnd,
		&swap_chain_desc1,
		nullptr,
		nullptr,
		&pSwapChain
	);
	if(FAILED(hr))
	{
		return false;
	}

	// pSwapChainをmpSwapChainの型に合わせる
	if(FAILED(pSwapChain.As(&mpSwapChain)))
	{
		return false;
	}

	return true;
}
b8 D3D11Renderer::CreateVertexBuffer()
{
	// 頂点を表すデータ構造
	struct Vertex
	{
		float x, y, z;
		float r, g, b, a;
	};

	// 頂点のデータ
	Vertex vertices[]
	{
		{  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },
		{  1.0f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f },
		{ -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f },
	};

	// 頂点バッファの設定
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(vertices);
	buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = sizeof(vertices[0]);

	// 頂点バッファの初期データの設定
	D3D11_SUBRESOURCE_DATA subresource_data {};
    subresource_data.pSysMem = vertices;
    subresource_data.SysMemPitch = 0;
    subresource_data.SysMemSlicePitch = 0;

	// 頂点バッファの生成
	HRESULT hr = mpDevice->CreateBuffer(
		&buffer_desc,
		&subresource_data,
		&mpVertexBuffer
	);
	if(FAILED(hr))
	{
		return false;
	}

	// 頂点の描画に必要なデータを設定
	mVertexStride = sizeof(Vertex);
	mVertexOffset = 0;
	mVertexCount = static_cast<u32>(std::size(vertices));
	mVertexLocation = 0;

	return true;
}


b8 D3D11Renderer::LoadShader(
	const wchar_t * path_str,
	const char * entry_point_str,
	const char * target_str,
	Microsoft::WRL::ComPtr<ID3DBlob> & p_shader_blob
)
{
	u32 flags = 0;
#if defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG;
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> p_error_blob;
	HRESULT hr = D3DCompileFromFile(
		path_str,
		nullptr,
		nullptr,
		entry_point_str,
		target_str,
		flags,
		0,
		&p_shader_blob,
		&p_error_blob
	);
	if(FAILED(hr))
	{
		OutputDebugString(reinterpret_cast<const c8 *>(
			p_error_blob->GetBufferPointer()
		));
		return false;
	}

	return true;
}

b8 D3D11Renderer::CreateInputLayout()
{
	// インプットレイアウトの要素を設定
	D3D11_INPUT_ELEMENT_DESC input_element_descs[2]{};
	input_element_descs[0].SemanticName = "POSITION";
	input_element_descs[0].SemanticIndex = 0;
	input_element_descs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	input_element_descs[0].InputSlot = 0;
	input_element_descs[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	input_element_descs[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	input_element_descs[0].InstanceDataStepRate = 0;

	input_element_descs[1].SemanticName = "COLOR";
	input_element_descs[1].SemanticIndex = 0;
	input_element_descs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	input_element_descs[1].InputSlot = 0;
	input_element_descs[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	input_element_descs[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	input_element_descs[1].InstanceDataStepRate = 0;

	// インプットレイアウトを生成
	HRESULT hr = mpDevice->CreateInputLayout(
		input_element_descs,
		static_cast<u32>(std::size(input_element_descs)),
		mpVertexShaderBlob->GetBufferPointer(),
		mpVertexShaderBlob->GetBufferSize(),
		&mpInputLayout
	);
	if(FAILED(hr))
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::CreateVertexShader()
{
	// 頂点シェーダの生成
	HRESULT hr = mpDevice->CreateVertexShader(
		mpVertexShaderBlob->GetBufferPointer(),
		mpVertexShaderBlob->GetBufferSize(),
		nullptr,
		&mpVertexShader
	);
	if(FAILED(hr))
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::CreateVSConstantBuffer()
{
	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX view = XMMatrixLookAtRH(mEye, mAt, mUp);
	XMMATRIX projection = XMMatrixPerspectiveFovRH(
		deg_to_rad(90.0f), 
		static_cast<f32>(mClientWidth) / mClientHeight,
		0.1f,
		100.0f
	);

	XMMATRIX cbScene = XMMatrixTranspose(world * view * projection);

	D3D11_BUFFER_DESC buffer_desc {};
	buffer_desc.ByteWidth = sizeof(cbScene);
	buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = sizeof(cbScene);

	D3D11_SUBRESOURCE_DATA subresource_data {};
	subresource_data.pSysMem = &cbScene;
	subresource_data.SysMemPitch;
	subresource_data.SysMemSlicePitch;

	HRESULT hr = mpDevice->CreateBuffer(&buffer_desc, &subresource_data, &mpVSConstantBuffer);
	if(FAILED(hr))
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::CreatePixelShader()
{
	ComPtr<ID3DBlob> p_shader_blob;
	if(!LoadShader(L"shaders.hlsl", "PS", "ps_5_0", p_shader_blob))
	{
		return false;
	}

	HRESULT hr = mpDevice->CreatePixelShader(
		p_shader_blob->GetBufferPointer(),
		p_shader_blob->GetBufferSize(),
		nullptr,
		&mpPixelShader
	);
	if(FAILED(hr))
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::CreateRenderTargetView()
{
	// バックバッファの取得
	ComPtr<ID3D11Texture2D> pBackBuffer;
	if(FAILED(mpSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))))
	{
		return false;
	}

	// レンダーターゲットビューの設定
	D3D11_RENDER_TARGET_VIEW_DESC1 rtv_desc {};
    rtv_desc.Format = mSwapChainBufferFormat;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtv_desc.Texture2D.MipSlice = 0;
	rtv_desc.Texture2D.PlaneSlice = 0;

	// レンダーターゲットビューの生成
	HRESULT hr = mpDevice->CreateRenderTargetView1(
		pBackBuffer.Get(),
		&rtv_desc,
		&mpRTV
	);
	if(FAILED(hr))
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::SetupGraphicsPipeline()
{
	// Input Assembler (IA)
	mpImmediateContext->IASetVertexBuffers(0, 1, mpVertexBuffer.GetAddressOf(), &mVertexStride, &mVertexOffset);
	mpImmediateContext->IASetInputLayout(mpInputLayout.Get());
	mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Vertex Shader (VS)
	mpImmediateContext->VSSetShader(mpVertexShader.Get(), nullptr, 0);
	mpImmediateContext->VSSetConstantBuffers(0, 1, mpVSConstantBuffer.GetAddressOf());

	// Hull Shader (HS)

	// Domain Shader (DS)

	// Geometry Shader (GS)

	// Stream Output (SO)

	// Rasterizer Stage (RS)

	// ビューポートの設定
	D3D11_VIEWPORT viewport {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(mClientWidth);
	viewport.Height = static_cast<FLOAT>(mClientHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	mpImmediateContext->RSSetViewports(1, &viewport);

	// Pixel Shader (PS)
	mpImmediateContext->PSSetShader(mpPixelShader.Get(), nullptr, 0);

	// Output Merger (OM)
	ID3D11RenderTargetView * pp_RTVs[]{ mpRTV.Get() };
	mpImmediateContext->OMSetRenderTargets(1, pp_RTVs, nullptr);

	return true;
}