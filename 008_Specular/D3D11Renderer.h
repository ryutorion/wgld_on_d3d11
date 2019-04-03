#pragma once
#ifndef D3D11_RENDERER_H_INCLUDED
#define D3D11_RENDERER_H_INCLUDED

#include <d3d11_4.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>

class D3D11Renderer
{
public:
	D3D11Renderer();
	~D3D11Renderer();

	//! 初期化
	b8 Initialize(int32_t width, int32_t height, HWND hWnd);
	//! 更新
	b8 Update(float delta);
	//! 描画
	b8 Render();

private:
	//! デバイスの生成
	b8 CreateDevice();
	//! ファクトリの取得
	b8 GetFactory();
	//! スワップチェインの生成
	b8 CreateSwapChain(HWND hWnd);
	//! メッシュの生成
	b8 GenerateMesh();
	//! 頂点バッファの生成
	b8 CreateVertexBuffer();
	//! インデックスバッファの生成
	b8 CreateIndexBuffer();
	//! シェーダバイナリの読み込み
	b8 LoadShader(
		const wchar_t * path_str,
		const char * entry_point_str,
		const char * target_str,
		Microsoft::WRL::ComPtr<ID3DBlob> & p_shader_blob
	);
	//! インプットレイアウトの生成
	b8 CreateInputLayout();
	//! 頂点シェーダの生成
	b8 CreateVertexShader();
	//! 頂点シェーダの定数バッファ生成
	b8 CreateVSConstantBuffers();

	//! ラスタライザーステートの生成
	bool CreateRasterizerState();

	//! ピクセルシェーダの生成
	b8 CreatePixelShader();

	//! レンダーターゲットビューの生成
	b8 CreateRenderTargetView();

	//! デプスステンシルフォーマット
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	//! デプスステンシルテクスチャの生成
	b8 CreateDepthStencilTexture();
	//! デプスステンシルビューの生成
	b8 CreateDepthStencilView();
	//! デプスステンシルステートの生成
	b8 CreateDepthStencilState();

	//! グラフィックスパイプラインの設定
	b8 SetupGraphicsPipeline();

private:
	//! クライアント領域の幅
	s32 mClientWidth = 0;
	//! クライアント領域の高さ
	s32 mClientHeight = 0;

	//! フィーチャーレベル
	D3D_FEATURE_LEVEL mFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	//! デバイスへのポインタ
	Microsoft::WRL::ComPtr<ID3D11Device5> mpDevice;
	//! イメディエイトコンテキストへのポインタ
	Microsoft::WRL::ComPtr<ID3D11DeviceContext4> mpImmediateContext;

	//! ファクトリへのポインタ
	Microsoft::WRL::ComPtr<IDXGIFactory5> mpFactory;

	//! スワップチェインのバッファフォーマット
	DXGI_FORMAT mSwapChainBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	//! スワップチェインのバッファ数
	static const uint32_t SwapChainBufferCount = 2;
	//! スワップチェインへのポインタ
	Microsoft::WRL::ComPtr<IDXGISwapChain4> mpSwapChain;

	struct Vertex
	{
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR normal;
		DirectX::XMVECTOR color;
	};
	std::vector<Vertex> mVertices;
	std::vector<u32> mIndices;

	//! 頂点バッファへのポインタ
	Microsoft::WRL::ComPtr<ID3D11Buffer> mpVertexBuffer;
	u32 mVertexStride = 0;
	u32 mVertexOffset = 0;
	u32 mVertexCount = 0;
	u32 mVertexLocation = 0;
	//! インデックスバッファへのポインタ
	Microsoft::WRL::ComPtr<ID3D11Buffer> mpIndexBuffer;
	DXGI_FORMAT mIndexFormat = DXGI_FORMAT_R32_UINT;
	u32 mIndexCount = 0;

	//! 頂点シェーダのバイナリ
	Microsoft::WRL::ComPtr<ID3DBlob> mpVertexShaderBlob;
	//! インプットレイアウトへのポインタ
	Microsoft::WRL::ComPtr<ID3D11InputLayout> mpInputLayout;
	//! 頂点シェーダへのポインタ
	Microsoft::WRL::ComPtr<ID3D11VertexShader> mpVertexShader;

	struct CBScene
	{
		DirectX::XMMATRIX VP;
		DirectX::XMVECTOR light_dir;
		DirectX::XMVECTOR ambient_color;
		DirectX::XMVECTOR eye_dir;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> mpVSCBScene;

	struct CBModel
	{
		DirectX::XMMATRIX W;
		DirectX::XMMATRIX IW;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> mpVSCBModels[1];

	//! ラスタライザーステートへのポインタ
	Microsoft::WRL::ComPtr<ID3D11RasterizerState2> mpRasterizerState;

	//! ピクセルシェーダへのポインタ
	Microsoft::WRL::ComPtr<ID3D11PixelShader> mpPixelShader;

	//! レンダーターゲットビューへのポインタ
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView1> mpRTV;
	//! デプスステンシルテクスチャへのポインタ
	Microsoft::WRL::ComPtr<ID3D11Texture2D1> mpDepthStencilTexture;
	//! デプスステンシルビューへのポインタ
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mpDSV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mpDepthStencilState;

	//! 画面を塗りつぶす色(RGBA)
	f32 mClearColor[4]{ 0.0f, 0.0f, 0.0f, 1.0f };

	DirectX::XMVECTOR mEye = DirectX::XMVectorSet(0.0f, 0.0f, 20.0f, 1.0f);
	DirectX::XMVECTOR mAt = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR mUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	DirectX::XMVECTOR mLightDir = DirectX::XMVectorSet(-0.5f, 0.5f, 0.5f, 0.0f);
};

#endif // D3D11_RENDERER_H_INCLUDED
