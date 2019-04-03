#include "stdafx.h"
#include "D3D11Renderer.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

// �E�B���h�E�v���V�[�W���̑O���錾
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// �G���g���[�|�C���g
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nShowCmd)
{
	// �E�B���h�E�N���X�̓o�^
	WNDCLASSEX wcx {};
	wcx.cbSize = sizeof(wcx);
	wcx.style = 0;
	wcx.lpfnWndProc = WindowProc;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hInstance;
	wcx.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wcx.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wcx.lpszMenuName = nullptr;
	wcx.lpszClassName = "LearningD3D11";
	wcx.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);
	if(!RegisterClassEx(&wcx))
	{
		return 0;
	}

	// �E�B���h�E�X�^�C������ъg���E�B���h�E�X�^�C���̐ݒ�
	const DWORD window_style = WS_OVERLAPPEDWINDOW ^ WS_SIZEBOX;
	const DWORD window_style_ex = WS_EX_ACCEPTFILES;

	// ��ʃT�C�Y�̎擾
	const int32_t screen_width = GetSystemMetrics(SM_CXSCREEN);
	const int32_t screen_height = GetSystemMetrics(SM_CYSCREEN);

	// �N���C�A���g�̈�̃T�C�Y�ݒ�
	const int32_t client_width = 1280;
	const int32_t client_height = 720;

	// ��ʒ����ɔz�u�����ꍇ�̃N���C�A���g�̈�̍��W���v�Z
	RECT window_rect
	{
		(screen_width - client_width) / 2,
		(screen_height - client_height) / 2,
		(screen_width + client_width) / 2,
		(screen_height + client_height) / 2
	};
	// �E�B���h�E�̈�̍��W���v�Z
	AdjustWindowRectEx(&window_rect, window_style, FALSE, window_style_ex);

	// �E�B���h�E�T�C�Y�̌v�Z
	const int32_t window_width = window_rect.right - window_rect.left;
	const int32_t window_height = window_rect.bottom - window_rect.top;

	// �E�B���h�E�̐���
	HWND hWnd = CreateWindowEx(
		window_style_ex,
		wcx.lpszClassName,
		wcx.lpszClassName,
		window_style,
		window_rect.left,
		window_rect.top,
		window_width,
		window_height,
		nullptr,
		nullptr,
		wcx.hInstance,
		nullptr
	);
	if(hWnd == nullptr)
	{
		return 0;
	}

	D3D11Renderer render;
	if(!render.Initialize(client_width, client_height, hWnd))
	{
		return 0;
	}

	// �E�B���h�E�̕\��
	ShowWindow(hWnd, nShowCmd);

	// ���b�Z�[�W���[�v�J�n
	MSG msg {};
	while(true)
	{
		// ���b�Z�[�W�̗L�����m�F
		if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				break;
			}

			// ���b�Z�[�W���������̂ŁC�E�B���h�E�v���V�[�W���ɓ�����
			DispatchMessage(&msg);
		}

		render.Render();
	}

	return static_cast<int>(msg.wParam);
}

// �E�B���h�E�v���V�[�W��
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return 0;
}
