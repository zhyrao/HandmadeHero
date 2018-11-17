/**
 * @Author:      joe
 * @Version:     1.0
 * @DateTime:    2018-11-15 23:38:09
 * @Description: ...
 */
#include <windows.h>
#include <stdint.h>

#define internal static			// use for functions
#define local_presist static	// use for local vari
#define global_variable static	// use for global vari

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;


struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

//TODO: this is global now
global_variable bool GlobalRunning = false;
global_variable win32_offscreen_buffer GlobalBackBuffer;

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT WindowRect;
	GetWindowRect(Window, &WindowRect);

	Result.Width = WindowRect.right - WindowRect.left;
	Result.Height = WindowRect.bottom - WindowRect.top;

	return Result;
}

internal void 
RenderWeirdGradient(win32_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
{
	uint8* PixelRow = (uint8*)(Buffer->Memory);
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32* Pixel = (uint32*)(PixelRow);
		for (int X = 0; X < Buffer->Width; ++X)
		{
			uint8 Green = (X + BlueOffset);
			uint8 Blue = (Y + GreenOffset);
			*Pixel++ = (Green << 8 | Blue);			
		}
		PixelRow += Buffer->Pitch;
	}	
}


internal void
Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)
{
	if (Buffer->Memory != 0)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	int BytesPerPixel = 4;

	// Note: when the biHeight is negative means image is top-down 
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Width * BytesPerPixel;
}

internal void 
Win32DisplayBufferInWindow(HDC DeviceContent,
						   int WindowWidth,
						   int WindowHeight,
						   win32_offscreen_buffer Buffer)
{
	StretchDIBits(DeviceContent, 
				  /*X, Y, Width, Height,
				  X, Y, Width, Height,*/
				  0, 0, WindowWidth, WindowHeight,
				  0, 0, Buffer.Width, Buffer.Height,
				  Buffer.Memory,
				  &Buffer.Info,
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

		} break;
		case WM_DESTROY:
		{
			GlobalRunning = false;
		} break;
		case WM_CLOSE:
		{
			GlobalRunning = false;
		} break;
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContent = BeginPaint(Window, &Paint);

			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferInWindow(DeviceContent, Dimension.Width, Dimension.Height, GlobalBackBuffer);
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

	//win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowProc;
	WindowClass.hInstance = Instance;
	//WindowClass.hIcon;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowExA(
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

		if (Window)
		{
			HDC DeviceContent = GetDC(Window);

			GlobalRunning = true;

			int XOffset = 0;
			int YOffset = 0;

			while(GlobalRunning)
			{
				MSG Message;
				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						GlobalRunning=false;
					}
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				RenderWeirdGradient(&GlobalBackBuffer, XOffset,YOffset);
				

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(DeviceContent, Dimension.Width, Dimension.Height, GlobalBackBuffer);
				ReleaseDC(Window, DeviceContent);

				++XOffset;
				YOffset += 2;
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
