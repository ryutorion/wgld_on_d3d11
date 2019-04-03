#include "stdafx.h"
#include "D3D11Renderer.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ウィンドウプロシージャの前方宣言
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// エントリーポイント
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nShowCmd)
{
	// ウィンドウクラスの登録
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

	// ウィンドウスタイルおよび拡張ウィンドウスタイルの設定
	const DWORD window_style = WS_OVERLAPPEDWINDOW ^ WS_SIZEBOX;
	const DWORD window_style_ex = WS_EX_ACCEPTFILES;

	// 画面サイズの取得
	const int32_t screen_width = GetSystemMetrics(SM_CXSCREEN);
	const int32_t screen_height = GetSystemMetrics(SM_CYSCREEN);

	// クライアント領域のサイズ設定
	const int32_t client_width = 1280;
	const int32_t client_height = 720;

	// 画面中央に配置した場合のクライアント領域の座標を計算
	RECT window_rect
	{
		(screen_width - client_width) / 2,
		(screen_height - client_height) / 2,
		(screen_width + client_width) / 2,
		(screen_height + client_height) / 2
	};
	// ウィンドウ領域の座標を計算
	AdjustWindowRectEx(&window_rect, window_style, FALSE, window_style_ex);

	// ウィンドウサイズの計算
	const int32_t window_width = window_rect.right - window_rect.left;
	const int32_t window_height = window_rect.bottom - window_rect.top;

	// ウィンドウの生成
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

	// ウィンドウの表示
	ShowWindow(hWnd, nShowCmd);

	// メッセージループ開始
	MSG msg {};
	while(true)
	{
		// メッセージの有無を確認
		if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				break;
			}

			// メッセージがあったので，ウィンドウプロシージャに投げる
			DispatchMessage(&msg);
		}

		render.Render();
	}

	return static_cast<int>(msg.wParam);
}

// ウィンドウプロシージャ
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
