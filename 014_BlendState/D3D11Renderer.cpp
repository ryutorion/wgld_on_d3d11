#include "stdafx.h"
#include "D3D11Renderer.h"
#include <d3dcompiler.h>

using namespace std;
using namespace Microsoft::WRL;
using namespace DirectX;

static inline constexpr float deg_to_rad(float degree)
{
	return degree * F_PI / 180.0f;
}

D3D11Renderer::D3D11Renderer()
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	CoCreateInstance(
		CLSID_WICImagingFactory2,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&mpImagingFactory)
	);
}

D3D11Renderer::~D3D11Renderer()
{
	if(mpImmediateContext)
	{
		mpImmediateContext->ClearState();
	}

	mpImagingFactory.Reset();

	CoUninitialize();
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

	if(!GenerateMesh())
	{
		return false;
	}

	if(!CreateVertexBuffer())
	{
		return false;
	}

	if(!CreateIndexBuffer())
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

	if(!CreateVSConstantBuffers())
	{
		return false;
	}

	if(!CreateRasterizerState())
	{
		return false;
	}

	if(!CreatePixelShader())
	{
		return false;
	}

	if(!CreatePSConstantBuffers())
	{
		return false;
	}

	if(!LoadTexture(L"texture.png", mpTextures[0], mpShaderResourceViews[0]))
	{
		return false;
	}

	if(!CreateSamplerState())
	{
		return false;
	}

	if(!CreateRenderTargetView())
	{
		return false;
	}

	if(!CreateDepthStencilTexture())
	{
		return false;
	}

	if(!CreateDepthStencilView())
	{
		return false;
	}

	if(!CreateBlendStates())
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::Update(float delta)
{
	static f32 total = 0.0f;
	static const f32 dt = 1.0f / 30.0f;
	static u64 tick = 0;

	total += delta;
	if(total >= dt)
	{
		total -= dt;
		++tick;
	}

	const f32 rad = (tick % 360) * F_PI / 180.0f;

	XMVECTOR d;
	CBModelVS worlds_vs[2]
	{
		XMMatrixTranslation(0.25f, 0.25f, -0.25f),
		XMMatrixTranslation(-0.25f, -0.25f, 0.25f),
	};
	CBModelPS worlds_ps[2] {};

	XMVECTOR axes[2]
	{
		XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
	};

	for(size_t i = 0; i < size(worlds_vs); ++i)
	{
		worlds_vs[i].W = XMMatrixRotationAxis(axes[i], rad) * worlds_vs[i].W;
		worlds_vs[i].W = XMMatrixTranspose(worlds_vs[i].W);
	}

	worlds_ps[0].IW = XMMatrixIdentity();
	worlds_ps[1].IW = XMMatrixIdentity();
	worlds_ps[1].IW.r[0] = XMVectorSet(0.5f, 0.0f, 0.0f, 0.0f);

	for(auto i = 0; i < size(worlds_vs); ++i)
	{
		D3D11_MAPPED_SUBRESOURCE mapped_subresource;
		HRESULT hr = mpImmediateContext->Map(mpVSCBModels[i].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
		if(FAILED(hr))
		{
			return false;
		}

		memcpy(mapped_subresource.pData, &worlds_vs[i], sizeof(worlds_vs[i]));

		mpImmediateContext->Unmap(mpVSCBModels[i].Get(), 0);
	}

	for(auto i = 0; i < size(worlds_ps); ++i)
	{
		D3D11_MAPPED_SUBRESOURCE mapped_subresource;
		HRESULT hr = mpImmediateContext->Map(mpPSCBModels[i].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
		if(FAILED(hr))
		{
			return false;
		}

		memcpy(mapped_subresource.pData, &worlds_ps[i], sizeof(worlds_ps[i]));

		mpImmediateContext->Unmap(mpPSCBModels[i].Get(), 0);
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
	mpImmediateContext->ClearDepthStencilView(mpDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	ID3D11ShaderResourceView* pp_SRVs[][1]{ { mpShaderResourceViews[0].Get() }, {nullptr} };

	for(auto i = 0; i < size(mpVSCBModels); ++i)
	{
		mpImmediateContext->VSSetConstantBuffers(1, 1, mpVSCBModels[i].GetAddressOf());
		mpImmediateContext->PSSetConstantBuffers(0, 1, mpPSCBModels[i].GetAddressOf());
		mpImmediateContext->PSSetShaderResources(0, static_cast<u32>(size(pp_SRVs[i])), pp_SRVs[i]);
		mpImmediateContext->PSSetSamplers(0, 1, mpSamplerStates[0].GetAddressOf());
		mpImmediateContext->OMSetBlendState(mpBlendStates[i].Get(), nullptr, 0xFFFFFFFF);

		mpImmediateContext->DrawIndexed(
			mIndexCounts[0],
			mStartIndexLocations[0],
			mBaseVertexLocations[0]
		);

	}

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

static void hsva_to_rgba(f32 h, f32 s, f32 v, f32 a, XMVECTOR & c)
{
	if(s > 1.0f || v > 1.0f || a > 1.0f)
	{
		return;
	}

	f32 th = fmod(h, 360.0f);
	s32 i = static_cast<s32>(floor(th / 60.0f));
	f32 f = th / 60.0f - i;
	f32 m = v * (1.0f - s);
	f32 n = v * (1 - s * f);
	f32 k = v * (1 - s * (1 - f));
	if(!(s > 0) && !(s < 0))
	{
		c = XMVectorSet(v, v, v, a);
	}
	else
	{
		f32 r[]{ v, n, m, m, k, v };
		f32 g[]{ k, v, v, n, m, m };
		f32 b[]{ m, m, k, v, v, n };

		c = XMVectorSet(r[i], g[i], b[i], a);
	}
}

static void GenerateTorus(
	const s32 row,
	const s32 column,
	const f32 irad,
	const f32 orad, 
	const XMVECTOR * p_color,
	vector<Vertex> & vertices,
	vector<u32> & indices
)
{
	for(s32 i = 0; i <= row; ++i)
	{
		f32 r = F_PI * 2.0f / row * i;
		f32 rr = cos(r);
		f32 ry = sin(r);

		for(s32 ii = 0; ii <= column; ++ii)
		{
			f32 tr = F_PI * 2.0f / column * ii;
			f32 tx = (rr * irad + orad) * cos(tr);
			f32 ty = ry * irad;
			f32 tz = (rr * irad + orad) * sin(tr);
			f32 rx = rr * cos(tr);
			f32 rz = rr * sin(tr);

			Vertex v;
			v.position = XMVectorSet(tx, ty, tz, 1.0f);
			v.normal = XMVectorSet(rx, ry, rz, 0.0f);
			if(p_color)
			{
				v.color = *p_color;
			}
			else
			{
				hsva_to_rgba(360.0f / column * ii, 1.0f, 1.0f, 1.0f, v.color);
			}
			vertices.emplace_back(v);
		}
	}

	for(s32 i = 0; i < row; ++i)
	{
		for(s32 ii = 0; ii < column; ++ii)
		{
			s32 r = (column + 1) * i + ii;
			indices.push_back(r);
			indices.push_back(r + column + 1);
			indices.push_back(r + 1);
			indices.push_back(r + column + 1);
			indices.push_back(r + column + 2);
			indices.push_back(r + 1);
		}
	}
}

static void GenerateSphere(
	const s32 row, 
	const s32 column, 
	const f32 rad,
	const XMVECTOR * p_color,
	vector<Vertex> & vertices,
	vector<u32> & indices
)
{
	for(auto i = 0; i <= row; ++i)
	{
		f32 r = F_PI * i / row;
		f32 ry = cos(r);
		f32 rr = sin(r);

		for(auto ii = 0; ii <= column; ++ii)
		{
			f32 tr = F_PI * 2.0f * ii / column;
			f32 tx = rr * rad * cos(tr);
			f32 ty = ry * rad;
			f32 tz = rr * rad * sin(tr);
			f32 rx = rr * cos(tr);
			f32 rz = rr * sin(tr);

			Vertex v;
			v.position = XMVectorSet(tx, ty, tz, 1.0f);
			v.normal = XMVectorSet(rx, ry, rz, 0.0f);
			if(p_color)
			{
				v.color = *p_color;
			}
			else
			{
				hsva_to_rgba(360.0f * i / row, 1.0f, 1.0f, 1.0f, v.color);
			}

			vertices.emplace_back(v);
		}
	}

	for(auto i = 0; i < row; ++i)
	{
		for(auto ii = 0; ii < column; ++ii)
		{
			u32 r = (column + 1) * i + ii;
			indices.emplace_back(r);
			indices.emplace_back(r + 1);
			indices.emplace_back(r + column + 2);
			indices.emplace_back(r);
			indices.emplace_back(r + column + 2);
			indices.emplace_back(r + column + 1);
		}
	}
}

b8 D3D11Renderer::GenerateMesh()
{
	mStartIndexLocations[0] = 0;
	mBaseVertexLocations[0] = 0;

	Vertex vertices[4];
	vertices[0].position = XMVectorSet(-1.0f,  1.0f, 0.0f, 1.0f);
	vertices[0].uv = XMFLOAT2(0.0f, 0.0f);
	vertices[0].color = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
	vertices[1].position = XMVectorSet( 1.0f,  1.0f, 0.0f, 1.0f);
	vertices[1].uv = XMFLOAT2(1.0f, 0.0f);
	vertices[1].color = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	vertices[2].position = XMVectorSet(-1.0f, -1.0f, 0.0f, 1.0f);
	vertices[2].uv = XMFLOAT2(0.0f, 1.0f);
	vertices[2].color = XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
	vertices[3].position = XMVectorSet( 1.0f, -1.0f, 0.0f, 1.0f);
	vertices[3].uv = XMFLOAT2(1.0f, 1.0f);
	vertices[3].color = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);

	for(auto v : vertices)
	{
		mVertices.emplace_back(v);
	}

	mIndices.emplace_back(0);
	mIndices.emplace_back(1);
	mIndices.emplace_back(2);
	mIndices.emplace_back(3);
	mIndices.emplace_back(2);
	mIndices.emplace_back(1);

	mIndexCounts[0] = static_cast<u32>(mIndices.size());

	return true;
}

b8 D3D11Renderer::CreateVertexBuffer()
{
	// 頂点バッファの設定
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = static_cast<u32>(sizeof(mVertices[0]) * mVertices.size());
	buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = sizeof(mVertices[0]);

	// 頂点バッファの初期データの設定
	D3D11_SUBRESOURCE_DATA subresource_data {};
	subresource_data.pSysMem = mVertices.data();
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
	mVertexCount = static_cast<u32>(mVertices.size());
	mVertexLocation = 0;

	return true;
}

b8 D3D11Renderer::CreateIndexBuffer()
{
	// インデックスバッファの設定
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = static_cast<u32>(sizeof(mIndices[0]) * mIndices.size());
	buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = sizeof(mIndices[0]);

	// インデックスバッファの初期データの設定
	D3D11_SUBRESOURCE_DATA subresource_data {};
	subresource_data.pSysMem = mIndices.data();
	subresource_data.SysMemPitch = 0;
	subresource_data.SysMemSlicePitch = 0;

	// インデックスバッファの生成
	HRESULT hr = mpDevice->CreateBuffer(
		&buffer_desc,
		&subresource_data,
		&mpIndexBuffer
	);
	if(FAILED(hr))
	{
		return false;
	}

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
	D3D11_INPUT_ELEMENT_DESC input_element_descs[4]{};
	input_element_descs[0].SemanticName = "POSITION";
	input_element_descs[0].SemanticIndex = 0;
	input_element_descs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	input_element_descs[0].InputSlot = 0;
	input_element_descs[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	input_element_descs[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	input_element_descs[0].InstanceDataStepRate = 0;

	input_element_descs[1].SemanticName = "NORMAL";
	input_element_descs[1].SemanticIndex = 0;
	input_element_descs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	input_element_descs[1].InputSlot = 0;
	input_element_descs[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	input_element_descs[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	input_element_descs[1].InstanceDataStepRate = 0;

	input_element_descs[2].SemanticName = "COLOR";
	input_element_descs[2].SemanticIndex = 0;
	input_element_descs[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	input_element_descs[2].InputSlot = 0;
	input_element_descs[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	input_element_descs[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	input_element_descs[2].InstanceDataStepRate = 0;

	input_element_descs[3].SemanticName = "TEXCOORD";
	input_element_descs[3].SemanticIndex = 0;
	input_element_descs[3].Format = DXGI_FORMAT_R32G32_FLOAT;
	input_element_descs[3].InputSlot = 0;
	input_element_descs[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	input_element_descs[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	input_element_descs[3].InstanceDataStepRate = 0;

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

b8 D3D11Renderer::CreateVSConstantBuffers()
{
	XMMATRIX view = XMMatrixLookAtRH(mEye, mAt, mUp);
	XMMATRIX projection = XMMatrixPerspectiveFovRH(
		deg_to_rad(45.0f), 
		static_cast<f32>(mClientWidth) / mClientHeight,
		0.1f,
		100.0f
	);

	CBScene cbScene {
		XMMatrixTranspose(view * projection),
	};

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

	HRESULT hr = mpDevice->CreateBuffer(&buffer_desc, &subresource_data, &mpVSCBScene);
	if(FAILED(hr))
	{
		return false;
	}

	CBModelVS cbModels[]
	{
		{ XMMatrixIdentity() },
		{ XMMatrixIdentity() },
	};

	for(auto i = 0; i < size(cbModels); ++i)
	{
		buffer_desc.ByteWidth = sizeof(cbModels[i]);
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = sizeof(cbModels[i]);

		subresource_data.pSysMem = &cbModels[i];
		subresource_data.SysMemPitch = 0;
		subresource_data.SysMemSlicePitch = 0;

		hr = mpDevice->CreateBuffer(&buffer_desc, &subresource_data, &mpVSCBModels[i]);
		if(FAILED(hr))
		{
			return false;
		}
	}

	return true;
}

bool D3D11Renderer::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC2 rasterizer_desc {};
	rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	rasterizer_desc.CullMode = D3D11_CULL_NONE;
	rasterizer_desc.FrontCounterClockwise = TRUE;
	rasterizer_desc.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
	rasterizer_desc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizer_desc.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizer_desc.DepthClipEnable = TRUE;
	rasterizer_desc.ScissorEnable = FALSE;
	rasterizer_desc.AntialiasedLineEnable = FALSE;
	rasterizer_desc.ForcedSampleCount = 0;
	rasterizer_desc.ConservativeRaster = D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	HRESULT hr = mpDevice->CreateRasterizerState2(&rasterizer_desc, &mpRasterizerState);
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

b8 D3D11Renderer::CreatePSConstantBuffers()
{
	CBLight cbLight {
		mLightPos,
		XMVectorSet(0.1f, 0.1f, 0.1f, 1.0f),
		mEye
	};

	D3D11_BUFFER_DESC buffer_desc {};
	buffer_desc.ByteWidth = sizeof(cbLight);
	buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = sizeof(cbLight);

	D3D11_SUBRESOURCE_DATA subresource_data {};
	subresource_data.pSysMem = &cbLight;
	subresource_data.SysMemPitch;
	subresource_data.SysMemSlicePitch;

	HRESULT hr = mpDevice->CreateBuffer(&buffer_desc, &subresource_data, &mpPSCBLight);
	if(FAILED(hr))
	{
		return false;
	}

	CBModelPS cbModels[]
	{
		{ XMMatrixIdentity() },
		{ XMMatrixIdentity() },
	};

	for(auto i = 0; i < size(cbModels); ++i)
	{
		buffer_desc.ByteWidth = sizeof(cbModels[i]);
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = sizeof(cbModels[i]);

		subresource_data.pSysMem = &cbModels[i];
		subresource_data.SysMemPitch = 0;
		subresource_data.SysMemSlicePitch = 0;

		hr = mpDevice->CreateBuffer(&buffer_desc, &subresource_data, &mpPSCBModels[i]);
		if(FAILED(hr))
		{
			return false;
		}
	}

	return true;
}

b8 D3D11Renderer::LoadTexture(
	const wchar_t * path_str,
	Microsoft::WRL::ComPtr<ID3D11Texture2D1>& p_texture,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView1>& p_shader_resource_view
)
{
	ComPtr<IWICBitmapDecoder> p_bitmap_decoder;
	HRESULT hr = mpImagingFactory->CreateDecoderFromFilename(
		path_str,
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnDemand,
		&p_bitmap_decoder
	);
	if(FAILED(hr))
	{
		return false;
	}

	ComPtr<IWICBitmapFrameDecode> p_bitmap_source;
	hr = p_bitmap_decoder->GetFrame(0, &p_bitmap_source);
	if(FAILED(hr))
	{
		return false;
	}

	D3D11_TEXTURE2D_DESC1 texture2d_desc {};
	p_bitmap_source->GetSize(&texture2d_desc.Width, &texture2d_desc.Height);
	texture2d_desc.MipLevels = 1;
	texture2d_desc.ArraySize = 1;

	WICPixelFormatGUID pixel_format;
	if(FAILED(p_bitmap_source->GetPixelFormat(&pixel_format)))
	{
		return false;
	}

	u32 stride = 0;
	if(pixel_format == GUID_WICPixelFormat24bppBGR)
	{
		stride = texture2d_desc.Width * 3;
		texture2d_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	}
	else if(pixel_format == GUID_WICPixelFormat24bppRGB)
	{
		stride = texture2d_desc.Width * 3;
		texture2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else
	{
		return false;
	}

	vector<u8> tmp(stride * texture2d_desc.Height);
	WICRect rect
	{
		0,
		0,
		static_cast<s32>(texture2d_desc.Width),
		static_cast<s32>(texture2d_desc.Height)
	};
	hr = p_bitmap_source->CopyPixels(&rect, stride, static_cast<u32>(tmp.size()), tmp.data());
	if(FAILED(hr))
	{
		return false;
	}

	vector<u8> buffer(texture2d_desc.Width * texture2d_desc.Height * 4);
	switch(texture2d_desc.Format)
	{
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		for(auto r = 0u; r < texture2d_desc.Height; ++r)
		{
			auto dst_base = texture2d_desc.Width * 4 * r;
			auto src_base = texture2d_desc.Width * 3 * r;
			for(auto c = 0u; c < texture2d_desc.Width; ++c)
			{
				buffer[dst_base + c * 4 + 0] = tmp[src_base + c * 3 + 2];
				buffer[dst_base + c * 4 + 1] = tmp[src_base + c * 3 + 1];
				buffer[dst_base + c * 4 + 2] = tmp[src_base + c * 3 + 0];
				buffer[dst_base + c * 4 + 3] = 255;
			}
		}
		break;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		for(auto r = 0u; r < texture2d_desc.Height; ++r)
		{
			auto dst_base = texture2d_desc.Width * 4 * r;
			auto src_base = texture2d_desc.Width * 3 * r;
			for(auto c = 0u; c < texture2d_desc.Width; ++c)
			{
				buffer[dst_base + c * 4 + 0] = tmp[src_base + c * 3 + 0];
				buffer[dst_base + c * 4 + 1] = tmp[src_base + c * 3 + 1];
				buffer[dst_base + c * 4 + 2] = tmp[src_base + c * 3 + 2];
				buffer[dst_base + c * 4 + 3] = 255;
			}
		}
		break;
	}

	texture2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texture2d_desc.SampleDesc.Count = 1;
	texture2d_desc.SampleDesc.Quality = 0;
	texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
	texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texture2d_desc.CPUAccessFlags = 0;
	texture2d_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	texture2d_desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

	D3D11_SUBRESOURCE_DATA subresource_data {};
	subresource_data.pSysMem = buffer.data();
	subresource_data.SysMemPitch = texture2d_desc.Width * 4;
	subresource_data.SysMemSlicePitch = static_cast<u32>(buffer.size());

	hr = mpDevice->CreateTexture2D1(&texture2d_desc, &subresource_data, &p_texture);
	if(FAILED(hr))
	{
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC1 shader_resource_view_desc {};
	shader_resource_view_desc.Format = texture2d_desc.Format;
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_view_desc.Texture2D.MostDetailedMip = 0;
	shader_resource_view_desc.Texture2D.MipLevels = 1;
	shader_resource_view_desc.Texture2D.PlaneSlice = 0;

	hr = mpDevice->CreateShaderResourceView1(
		p_texture.Get(),
		&shader_resource_view_desc,
		&p_shader_resource_view
	);
	if(FAILED(hr))
	{
		return false;
	}

	mpImmediateContext->GenerateMips(p_shader_resource_view.Get());

	return true;
}

b8 D3D11Renderer::CreateSamplerState()
{
	D3D11_FILTER filters[]
	{
		D3D11_FILTER_MIN_MAG_MIP_POINT,
	};

	D3D11_TEXTURE_ADDRESS_MODE address_modes[]
	{
		D3D11_TEXTURE_ADDRESS_CLAMP,
	};

	for(size_t i = 0; i < size(mpSamplerStates); ++i)
	{
		D3D11_SAMPLER_DESC sampler_desc{};
		sampler_desc.Filter = filters[i];
		sampler_desc.AddressU = address_modes[i];
		sampler_desc.AddressV = address_modes[i];
		sampler_desc.AddressW = address_modes[i];
		sampler_desc.MipLODBias = 0.0f;
		sampler_desc.MaxAnisotropy = 1;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampler_desc.BorderColor[0] = 1.0f;
		sampler_desc.BorderColor[1] = 1.0f;
		sampler_desc.BorderColor[2] = 1.0f;
		sampler_desc.BorderColor[3] = 1.0f;
		sampler_desc.MinLOD = -FLT_MAX;
		sampler_desc.MaxLOD = FLT_MAX;

		if(FAILED(mpDevice->CreateSamplerState(&sampler_desc, &mpSamplerStates[i])))
		{
			return false;
		}
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

b8 D3D11Renderer::CreateDepthStencilTexture()
{
	D3D11_TEXTURE2D_DESC1 texture2d_desc {};
	texture2d_desc.Width = mClientWidth;
	texture2d_desc.Height = mClientHeight;
	texture2d_desc.MipLevels = 1;
	texture2d_desc.ArraySize = 1;
	texture2d_desc.Format = mDepthStencilFormat;
	texture2d_desc.SampleDesc.Count = 1;
	texture2d_desc.SampleDesc.Quality = 0;
	texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
	texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	texture2d_desc.CPUAccessFlags = 0;
	texture2d_desc.MiscFlags = 0;
	texture2d_desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

	HRESULT hr = mpDevice->CreateTexture2D1(&texture2d_desc, nullptr, &mpDepthStencilTexture);
	if(FAILED(hr))
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::CreateDepthStencilView()
{
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc {};
	depth_stencil_view_desc.Format = mDepthStencilFormat;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_view_desc.Flags = 0;
	depth_stencil_view_desc.Texture2D.MipSlice = 0;

	HRESULT hr = mpDevice->CreateDepthStencilView(
		mpDepthStencilTexture.Get(),
		&depth_stencil_view_desc,
		&mpDSV
	);
	if(FAILED(hr))
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::CreateDepthStencilState()
{
	D3D11_DEPTH_STENCIL_DESC depth_stencil_desc {};
	depth_stencil_desc.DepthEnable = TRUE;
	depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
	depth_stencil_desc.StencilEnable = FALSE;
	depth_stencil_desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depth_stencil_desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	HRESULT hr = mpDevice->CreateDepthStencilState(&depth_stencil_desc, &mpDepthStencilState);
	if(FAILED(hr))
	{
		return false;
	}

	return true;
}

b8 D3D11Renderer::CreateBlendStates()
{
	D3D11_BLEND_DESC1 blend_descs[2] {};
	blend_descs[0].AlphaToCoverageEnable = FALSE;
	blend_descs[0].IndependentBlendEnable = FALSE;
	blend_descs[0].RenderTarget[0].BlendEnable = FALSE;
	blend_descs[0].RenderTarget[0].LogicOpEnable = FALSE;
	blend_descs[0].RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blend_descs[0].RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blend_descs[0].RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend_descs[0].RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blend_descs[0].RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blend_descs[0].RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blend_descs[0].RenderTarget[0].LogicOp = D3D11_LOGIC_OP_NOOP;
	blend_descs[0].RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	blend_descs[1] = blend_descs[0];
	blend_descs[1].RenderTarget[0].BlendEnable = TRUE;
	blend_descs[1].RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blend_descs[1].RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

	for(size_t i = 0; i < size(blend_descs); ++i)
	{
		HRESULT hr = mpDevice->CreateBlendState1(&blend_descs[i], &mpBlendStates[i]);
		if(FAILED(hr))
		{
			return false;
		}
	}

	return true;
}

b8 D3D11Renderer::SetupGraphicsPipeline()
{
	// Input Assembler (IA)
	mpImmediateContext->IASetVertexBuffers(0, 1, mpVertexBuffer.GetAddressOf(), &mVertexStride, &mVertexOffset);
	mpImmediateContext->IASetIndexBuffer(mpIndexBuffer.Get(), mIndexFormat, 0);
	mpImmediateContext->IASetInputLayout(mpInputLayout.Get());
	mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Vertex Shader (VS)
	mpImmediateContext->VSSetShader(mpVertexShader.Get(), nullptr, 0);
	mpImmediateContext->VSSetConstantBuffers(0, 1, mpVSCBScene.GetAddressOf());

	// Hull Shader (HS)

	// Domain Shader (DS)

	// Geometry Shader (GS)

	// Stream Output (SO)

	// Rasterizer Stage (RS)
	mpImmediateContext->RSSetState(mpRasterizerState.Get());

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
	mpImmediateContext->PSSetConstantBuffers(0, 1, mpPSCBLight.GetAddressOf());

	// Output Merger (OM)
	ID3D11RenderTargetView * pp_RTVs[]{ mpRTV.Get() };
	mpImmediateContext->OMSetRenderTargets(1, pp_RTVs, mpDSV.Get());
	mpImmediateContext->OMSetDepthStencilState(mpDepthStencilState.Get(), 0);

	return true;
}