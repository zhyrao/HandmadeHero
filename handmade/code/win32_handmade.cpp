/**
 * @Author:      joe
 * @Version:     1.0
 * @DateTime:    2018-11-15 23:38:09
 * @Description: ...
 */
#include <windows.h>

#define internal static			// use for functions
#define local_presist static	// use for local vari
#define global_variable static	// use for global vari

//TODO: this is global now
global_variable bool Running = false;
global_variable BITMAPINFO BitmapInfo;
global_variable void* BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContent;

internal void
Win32ResizeDIBSection(int Width, int Height)
{
	if (BitmapHandle)
	{
		DeleteObject(BitmapHandle);
	}
	
	if (BitmapDeviceContent == NULL)
	{
		BitmapDeviceContent = CreateCompatibleDC(0);
	}

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	BitmapInfo.bmiHeader.biSizeImage = 0;
	BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
	BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
	BitmapInfo.bmiHeader.biClrUsed = 0;
	BitmapInfo.bmiHeader.biClrImportant = 0;

		
	BitmapHandle = CreateDIBSection(BitmapDeviceContent,
									&BitmapInfo,
									DIB_RGB_COLORS,
									&BitmapMemory,
									0,
									0);
}

internal void 
Win32UpdateWindow(HDC DeviceContent, int X, int Y, int Width, int Height)
{
	StretchDIBits(DeviceContent, 
				  X, Y, Width, Height,
				  X, Y, Width, Height,
				  BitmapMemory,
				  &BitmapInfo,
				  DIB_RGB_COLORS,
				  SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowProc(HWND   Window,
								UINT   Message,
								WPARAM WParam,
								LPARAM LParam)
{
	LRESULT Result = 0;
	switch(Message)
	{
		case WM_SIZE:
		{
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);

			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;

			Win32ResizeDIBSection(Width, Height);
			OutputDebugStringA("WM_SIZE\n");
		} break;
		case WM_DESTROY:
		{
			Running = false;
		} break;
		case WM_CLOSE:
		{
			Running = false;
		} break;
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContent = BeginPaint(Window, &Paint);

			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			Win32UpdateWindow(DeviceContent, X, Y, Width, Height);
			EndPaint(Window, &Paint);
		} break;

		default:
		{
			//OutputDebugStringA("default\n");
			Result = DefWindowProcA(Window, Message, WParam, LParam);
		} break;
	}

	return Result;
}


int CALLBACK 
WinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR     CommandLine,
	int       ShowCode)
{
	WNDCLASS WindowClass = {};
	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowProc;
	WindowClass.hInstance = Instance;
	//WindowClass.hIcon;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowExA(
			0,
			WindowClass.lpszClassName,
			"Handmade Hero",
  			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
  			CW_USEDEFAULT,
  			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,	
			Instance,
			0);

		if (WindowHandle)
		{
			Running = true;
			MSG Message;
			while(Running)
			{
				BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
				if (MessageResult != 0)
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				else
				{
					break;
				}
			}
			
		}
		else
		{
			//TODO: logging
		}
	}
	else
	{
		//TODO: logging
	}
	return 0;
}
