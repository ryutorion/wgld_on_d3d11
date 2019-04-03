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

	//! ������
	b8 Initialize(int32_t width, int32_t height, HWND hWnd);
	//! �X�V
	b8 Update(float delta);
	//! �`��
	b8 Render();

private:
	//! �f�o�C�X�̐���
	b8 CreateDevice();
	//! �t�@�N�g���̎擾
	b8 GetFactory();
	//! �X���b�v�`�F�C���̐���
	b8 CreateSwapChain(HWND hWnd);
	//! ���b�V���̐���
	b8 GenerateMesh();
	//! ���_�o�b�t�@�̐���
	b8 CreateVertexBuffer();
	//! �C���f�b�N�X�o�b�t�@�̐���
	b8 CreateIndexBuffer();
	//! �V�F�[�_�o�C�i���̓ǂݍ���
	b8 LoadShader(
		const wchar_t * path_str,
		const char * entry_point_str,
		const char * target_str,
		Microsoft::WRL::ComPtr<ID3DBlob> & p_shader_blob
	);
	//! �C���v�b�g���C�A�E�g�̐���
	b8 CreateInputLayout();
	//! ���_�V�F�[�_�̐���
	b8 CreateVertexShader();
	//! ���_�V�F�[�_�̒萔�o�b�t�@����
	b8 CreateVSConstantBuffers();

	//! ���X�^���C�U�[�X�e�[�g�̐���
	bool CreateRasterizerState();

	//! �s�N�Z���V�F�[�_�̐���
	b8 CreatePixelShader();

	//! �����_�[�^�[�Q�b�g�r���[�̐���
	b8 CreateRenderTargetView();

	//! �f�v�X�X�e���V���t�H�[�}�b�g
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	//! �f�v�X�X�e���V���e�N�X�`���̐���
	b8 CreateDepthStencilTexture();
	//! �f�v�X�X�e���V���r���[�̐���
	b8 CreateDepthStencilView();
	//! �f�v�X�X�e���V���X�e�[�g�̐���
	b8 CreateDepthStencilState();

	//! �O���t�B�b�N�X�p�C�v���C���̐ݒ�
	b8 SetupGraphicsPipeline();

private:
	//! �N���C�A���g�̈�̕�
	s32 mClientWidth = 0;
	//! �N���C�A���g�̈�̍���
	s32 mClientHeight = 0;

	//! �t�B�[�`���[���x��
	D3D_FEATURE_LEVEL mFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	//! �f�o�C�X�ւ̃|�C���^
	Microsoft::WRL::ComPtr<ID3D11Device5> mpDevice;
	//! �C���f�B�G�C�g�R���e�L�X�g�ւ̃|�C���^
	Microsoft::WRL::ComPtr<ID3D11DeviceContext4> mpImmediateContext;

	//! �t�@�N�g���ւ̃|�C���^
	Microsoft::WRL::ComPtr<IDXGIFactory5> mpFactory;

	//! �X���b�v�`�F�C���̃o�b�t�@�t�H�[�}�b�g
	DXGI_FORMAT mSwapChainBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	//! �X���b�v�`�F�C���̃o�b�t�@��
	static const uint32_t SwapChainBufferCount = 2;
	//! �X���b�v�`�F�C���ւ̃|�C���^
	Microsoft::WRL::ComPtr<IDXGISwapChain4> mpSwapChain;

	struct Vertex
	{
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR normal;
		DirectX::XMVECTOR color;
	};
	std::vector<Vertex> mVertices;
	std::vector<u32> mIndices;

	//! ���_�o�b�t�@�ւ̃|�C���^
	Microsoft::WRL::ComPtr<ID3D11Buffer> mpVertexBuffer;
	u32 mVertexStride = 0;
	u32 mVertexOffset = 0;
	u32 mVertexCount = 0;
	u32 mVertexLocation = 0;
	//! �C���f�b�N�X�o�b�t�@�ւ̃|�C���^
	Microsoft::WRL::ComPtr<ID3D11Buffer> mpIndexBuffer;
	DXGI_FORMAT mIndexFormat = DXGI_FORMAT_R32_UINT;
	u32 mIndexCount = 0;

	//! ���_�V�F�[�_�̃o�C�i��
	Microsoft::WRL::ComPtr<ID3DBlob> mpVertexShaderBlob;
	//! �C���v�b�g���C�A�E�g�ւ̃|�C���^
	Microsoft::WRL::ComPtr<ID3D11InputLayout> mpInputLayout;
	//! ���_�V�F�[�_�ւ̃|�C���^
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

	//! ���X�^���C�U�[�X�e�[�g�ւ̃|�C���^
	Microsoft::WRL::ComPtr<ID3D11RasterizerState2> mpRasterizerState;

	//! �s�N�Z���V�F�[�_�ւ̃|�C���^
	Microsoft::WRL::ComPtr<ID3D11PixelShader> mpPixelShader;

	//! �����_�[�^�[�Q�b�g�r���[�ւ̃|�C���^
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView1> mpRTV;
	//! �f�v�X�X�e���V���e�N�X�`���ւ̃|�C���^
	Microsoft::WRL::ComPtr<ID3D11Texture2D1> mpDepthStencilTexture;
	//! �f�v�X�X�e���V���r���[�ւ̃|�C���^
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mpDSV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mpDepthStencilState;

	//! ��ʂ�h��Ԃ��F(RGBA)
	f32 mClearColor[4]{ 0.0f, 0.0f, 0.0f, 1.0f };

	DirectX::XMVECTOR mEye = DirectX::XMVectorSet(0.0f, 0.0f, 20.0f, 1.0f);
	DirectX::XMVECTOR mAt = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR mUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	DirectX::XMVECTOR mLightDir = DirectX::XMVectorSet(-0.5f, 0.5f, 0.5f, 0.0f);
};

#endif // D3D11_RENDERER_H_INCLUDED
