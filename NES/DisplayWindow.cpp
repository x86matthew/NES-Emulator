#include "NES.h"

DWORD DisplayWindow_SetPixel(WORD wX, WORD wY, BYTE bRed, BYTE bGreen, BYTE bBlue)
{
	DWORD *pdwPixel = NULL;
	RGBQUAD *pPixelRGB = NULL;

	// validate X value
	if(wX >= DISPLAY_WINDOW_WIDTH)
	{
		return 1;
	}

	// validate Y value
	if(wY >= DISPLAY_WINDOW_HEIGHT)
	{
		return 1;
	}

	// set pixel data in bitmap
	pPixelRGB = &gSystem.DisplayWindow.CurrentFrameBuffer[(DISPLAY_WINDOW_WIDTH * wY) + wX];
	pPixelRGB->rgbRed = bRed;
	pPixelRGB->rgbGreen = bGreen;
	pPixelRGB->rgbBlue = bBlue;

	return 0;
}

DWORD DisplayWindow_UpdateTitle(DWORD dwFPS)
{
	char szTitle[512];

	// update window title
	memset(szTitle, 0, sizeof(szTitle));
	_snprintf(szTitle, sizeof(szTitle) - 1, "NES Emulator (FPS: %u)", dwFPS);
	SetWindowTextA(gSystem.DisplayWindow.hWnd, szTitle);

	return 0;
}

DWORD DisplayWindow_UpdateFps()
{
	DWORD dwCurrTime = 0;

	// get current time
	dwCurrTime = GetTickCount();

	// check if the FPS counter is ready
	if(gSystem.DisplayWindow.bFpsCounterReady == 0)
	{
		// set initial state
		gSystem.DisplayWindow.dwFpsPreviousTime = dwCurrTime;
		gSystem.DisplayWindow.bFpsCounterReady = 1;
	}
	else
	{
		// increase counter
		gSystem.DisplayWindow.dwFpsCounter++;

		// check if the duration since the last update is more than 1 second
		if((dwCurrTime - gSystem.DisplayWindow.dwFpsPreviousTime) >= 1000)
		{
			// update title bar
			DisplayWindow_UpdateTitle(gSystem.DisplayWindow.dwFpsCounter);

			// reset counter
			gSystem.DisplayWindow.dwFpsCounter = 0;
			gSystem.DisplayWindow.dwFpsPreviousTime = dwCurrTime;
		}
	}

	return 0;
}

DWORD DisplayWindow_FrameReady()
{
	// frame ready - copy to output bitmap
	EnterCriticalSection(&gSystem.DisplayWindow.CurrentFrameBufferCriticalSection);
	memcpy(gSystem.DisplayWindow.pBitmapPixelData, gSystem.DisplayWindow.CurrentFrameBuffer, sizeof(gSystem.DisplayWindow.CurrentFrameBuffer));
	gSystem.DisplayWindow.bCurrentFrameReady = 1;
	LeaveCriticalSection(&gSystem.DisplayWindow.CurrentFrameBufferCriticalSection);

	// update FPS
	DisplayWindow_UpdateFps();

	return 0;
}

LRESULT CALLBACK DisplayWindow_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hDC = NULL;
	RECT ClientRect;
	PAINTSTRUCT PaintInfo;
	DWORD dwWindowWidth = 0;
	DWORD dwWindowHeight = 0;

	if(uMsg == WM_PAINT)
	{
		hDC = BeginPaint(hWnd, &PaintInfo);
		if(hDC != NULL)
		{
			// stretch bitmap contents to the display window
			GetClientRect(hWnd, &ClientRect);
			dwWindowWidth = ClientRect.right - ClientRect.left;
			dwWindowHeight = ClientRect.bottom - ClientRect.top;

			// lock bitmap
			EnterCriticalSection(&gSystem.DisplayWindow.CurrentFrameBufferCriticalSection);

			// check if a full frame is ready to render
			if(gSystem.DisplayWindow.bCurrentFrameReady == 0)
			{
				// the current frame has not finished rendering.
				// copy the partially-complete frame instead, this is not ideal but usually looks better than dropping an entire frame.
				memcpy(gSystem.DisplayWindow.pBitmapPixelData, gSystem.DisplayWindow.CurrentFrameBuffer, sizeof(gSystem.DisplayWindow.CurrentFrameBuffer));
			}
			gSystem.DisplayWindow.bCurrentFrameReady = 0;

			// redraw bitmap - stretch to window size
			StretchBlt(hDC, 0, 0, dwWindowWidth, dwWindowHeight, gSystem.DisplayWindow.hBitmapDC, 0, 0, DISPLAY_WINDOW_WIDTH, DISPLAY_WINDOW_HEIGHT, SRCCOPY);

			// unlock bitmap
			LeaveCriticalSection(&gSystem.DisplayWindow.CurrentFrameBufferCriticalSection);

			EndPaint(hWnd, &PaintInfo);

			// handled message
			return 0;
		}
	}
	else if(uMsg == WM_REDRAW_BITMAP)
	{
		// force re-paint
		RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

		// handled message
		return 0;
	}
	else if(uMsg == WM_DESTROY)
	{
		// exit
		PostQuitMessage(0);

		// handled message
		return 0;
	}
	else if(uMsg == WM_SETCURSOR)
	{
		if(LOWORD(lParam) == HTCLIENT)
		{
			// use default cursor
			SetCursor(LoadCursor(NULL, IDC_ARROW));

			// handled message
			return 1;
		}
	}
	else if(uMsg == WM_KEYDOWN)
	{
		// key down
		UpdateKeyState((BYTE)wParam, 1);
	}
	else if(uMsg == WM_KEYUP)
	{
		// key up
		UpdateKeyState((BYTE)wParam, 0);
	}

	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI RedrawBitmapThread(LPVOID lpArg)
{
	BYTE bError = 0;
	HMODULE hDwmApiModule = NULL;
	HRESULT (WINAPI *pDwmFlush)() = NULL;
	BYTE bUseDwmFlush = 0;
	HMONITOR hMonitor = NULL;
	MONITORINFOEX MonitorInfo;
	DEVMODE DeviceMode;
	HANDLE hRedrawEvent = NULL;
	DWORD dwTimerID = 0;
	DWORD dwDelayInterval = 0;

	// load dwmapi module
	hDwmApiModule = LoadLibraryA("dwmapi.dll");
	if(hDwmApiModule != NULL)
	{
		// find DwmFlush function
		pDwmFlush = (HRESULT(WINAPI*)())GetProcAddress(hDwmApiModule, "DwmFlush");
		if(pDwmFlush != NULL)
		{
			// even if DwmFlush exists, various factors can cause it to fail
			// call the function and check the return value
			if(pDwmFlush() == 0)
			{
				// success
				bUseDwmFlush = 1;
			}
		}
	}

	// if DwmFlush can't be used, create a timer that is roughly in sync with the monitor refresh rate
	if(bUseDwmFlush == 0)
	{
		// find the monitor which contains the window
		hMonitor = MonitorFromWindow(gSystem.DisplayWindow.hWnd, MONITOR_DEFAULTTONEAREST);
		if(hMonitor == NULL)
		{
			ERROR_CLEAN_UP(1);
		}

		// get monitor info
		memset(&MonitorInfo, 0, sizeof(MonitorInfo));
		MonitorInfo.cbSize = sizeof(MonitorInfo);
		if(GetMonitorInfo(hMonitor, &MonitorInfo) == 0)
		{
			ERROR_CLEAN_UP(1);
		}

		// get display settings
		memset(&DeviceMode, 0, sizeof(DeviceMode));
		DeviceMode.dmSize = sizeof(DeviceMode);
		if(EnumDisplaySettings(MonitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &DeviceMode) == 0)
		{
			ERROR_CLEAN_UP(1);
		}

		// create redraw event
		hRedrawEvent = CreateEvent(NULL, 0, 0, NULL);
		if(hRedrawEvent == NULL)
		{
			ERROR_CLEAN_UP(1);
		}

		// calculate delay interval
		dwDelayInterval = 1000 / DeviceMode.dmDisplayFrequency;
		if(dwDelayInterval == 0)
		{
			// 1ms minimum
			dwDelayInterval = 1;
		}

		// create timer to trigger hRedrawEvent at the screen refresh rate
		dwTimerID = timeSetEvent(dwDelayInterval, 1, (LPTIMECALLBACK)hRedrawEvent, 0, TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
		if(dwTimerID == 0)
		{
			ERROR_CLEAN_UP(1);
		}
	}

	// display window ready - set event
	SetEvent((HANDLE)lpArg);

	// main redraw loop
	for(;;)
	{
		// check for shutdown event
		if(WaitForSingleObject(gSystem.hShutDownEvent, 0) == WAIT_OBJECT_0)
		{
			// caught shutdown event
			break;
		}

		// check mode
		if(bUseDwmFlush != 0)
		{
			// DwmFlush enabled - synchronise with next frame
			if(pDwmFlush() != 0)
			{
				// error
				ERROR_CLEAN_UP(1);
			}
		}
		else
		{
			// using timer - wait for event
			WaitForSingleObject(hRedrawEvent, INFINITE);
		}

		// redraw bitmap
		SendMessage(gSystem.DisplayWindow.hWnd, WM_REDRAW_BITMAP, 0, 0);
	}

	// clean up
CLEAN_UP:
	if(hDwmApiModule != NULL)
	{
		FreeLibrary(hDwmApiModule);
	}
	if(hRedrawEvent != NULL)
	{
		CloseHandle(hRedrawEvent);
	}
	if(dwTimerID != 0)
	{
		timeKillEvent(dwTimerID);
	}
	if(bError != 0)
	{
		return 1;
	}

	return 0;
}

DWORD WINAPI DisplayWindowThread(LPVOID lpArg)
{
	BYTE bError = 0;
	WNDCLASSEXA WndClass;
	DWORD dwDefaultWidth = 0;
	DWORD dwDefaultHeight = 0;
	RECT WindowRect;
	HBITMAP hBitmap = NULL;
	BITMAPINFO BitmapInfo;
	MSG Msg;

	// register window class
	memset(&WndClass, 0, sizeof(WndClass));
	WndClass.cbSize = sizeof(WndClass);
	WndClass.lpfnWndProc = DisplayWindow_WndProc;
	WndClass.hInstance = (HINSTANCE)GetModuleHandle(NULL);
	WndClass.lpszClassName = "NESDisplayWindowClass";
	RegisterClassExA(&WndClass);

	// calculate initial window size
	dwDefaultWidth = DISPLAY_WINDOW_WIDTH * 2;
	dwDefaultHeight = DISPLAY_WINDOW_HEIGHT * 2;
	memset(&WindowRect, 0, sizeof(WindowRect));
	WindowRect.left = 0;
	WindowRect.top = 0;
	WindowRect.right = dwDefaultWidth;
	WindowRect.bottom = dwDefaultHeight;
	AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, 0);

	// create window
	gSystem.DisplayWindow.hWnd = CreateWindowExA(0, WndClass.lpszClassName, "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, dwDefaultWidth, dwDefaultHeight, NULL, NULL, WndClass.hInstance, NULL);
	if(gSystem.DisplayWindow.hWnd == NULL)
	{
		ERROR_CLEAN_UP(1);
	}

	// create bitmap DC
	gSystem.DisplayWindow.hBitmapDC = CreateCompatibleDC(NULL);
	if(gSystem.DisplayWindow.hBitmapDC == NULL)
	{
		ERROR_CLEAN_UP(1);
	}

	// create bitmap
	memset(&BitmapInfo, 0, sizeof(BitmapInfo));
	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = DISPLAY_WINDOW_WIDTH;
	BitmapInfo.bmiHeader.biHeight = -DISPLAY_WINDOW_HEIGHT;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	hBitmap = CreateDIBSection(gSystem.DisplayWindow.hBitmapDC, &BitmapInfo, DIB_RGB_COLORS, &gSystem.DisplayWindow.pBitmapPixelData, NULL, 0);
	if(hBitmap == NULL)
	{
		ERROR_CLEAN_UP(1);
	}
	SelectObject(gSystem.DisplayWindow.hBitmapDC, hBitmap);

	// set initial window title
	DisplayWindow_UpdateTitle(0);

	// display window ready - set event
	SetEvent((HANDLE)lpArg);

	// show window
	ShowWindow(gSystem.DisplayWindow.hWnd, SW_SHOW);

	// message loop
	for(;;)
	{
		if(GetMessage(&Msg, NULL, 0, 0) == 0)
		{
			break;
		}
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	// window closed, set flag
	gSystem.DisplayWindow.bWindowClosed = 1;

	// wait for CPU to stop before cleaning up, otherwise the PPU might continue writing to the free'd bitmap image
	WaitForSingleObject(gSystem.hShutDownEvent, INFINITE);

	// clean up
CLEAN_UP:
	if(hBitmap != NULL)
	{
		DeleteObject(hBitmap);
	}
	if(bError != 0)
	{
		return 1;
	}

	return 0;
}

DWORD InitialiseDisplayWindow()
{
	BYTE bError = 0;
	HANDLE hWaitHandleList[2];
	HANDLE hReadyEvent = NULL;

	// create event
	hReadyEvent = CreateEvent(NULL, 0, 0, NULL);
	if(hReadyEvent == NULL)
	{
		ERROR_CLEAN_UP(1);
	}

	// create frame buffer critical section
	InitializeCriticalSection(&gSystem.DisplayWindow.CurrentFrameBufferCriticalSection);
	gSystem.DisplayWindow.bCurrentFrameBufferCriticalSectionReady = 1;

	// create display window thread
	gSystem.DisplayWindow.hThread = CreateThread(NULL, 0, DisplayWindowThread, hReadyEvent, 0, NULL);
	if(gSystem.DisplayWindow.hThread == NULL)
	{
		ERROR_CLEAN_UP(1);
	}

	// wait for confirmation from thread
	hWaitHandleList[0] = hReadyEvent;
	hWaitHandleList[1] = gSystem.DisplayWindow.hThread;
	if(WaitForMultipleObjects(2, hWaitHandleList, 0, INFINITE) != WAIT_OBJECT_0)
	{
		ERROR_CLEAN_UP(1);
	}

	// create background thread to periodically redraw the output bitmap in the display window
	gSystem.DisplayWindow.hRedrawBitmapThread = CreateThread(NULL, 0, RedrawBitmapThread, hReadyEvent, 0, NULL);
	if(gSystem.DisplayWindow.hRedrawBitmapThread == NULL)
	{
		ERROR_CLEAN_UP(1);
	}

	// wait for confirmation from thread
	hWaitHandleList[0] = hReadyEvent;
	hWaitHandleList[1] = gSystem.DisplayWindow.hRedrawBitmapThread;
	if(WaitForMultipleObjects(2, hWaitHandleList, 0, INFINITE) != WAIT_OBJECT_0)
	{
		ERROR_CLEAN_UP(1);
	}

CLEAN_UP:
	if(hReadyEvent != NULL)
	{
		CloseHandle(hReadyEvent);
	}
	if(bError != 0)
	{
		return 1;
	}

	return 0;
}
