/**
 * @Author:      joe
 * @Version:     1.0
 * @DateTime:    2018-11-15 23:38:09
 * @Description: ...
 */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>

#define internal static			// use for functions
#define local_presist static	// use for local vari
#define global_variable static	// use for global vari

#define Pi32 3.141592659

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;

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
global_variable bool32 GlobalRunning = false;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer; 

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// Direct Sound
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal void 
Win32LoadXInput()
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");

	if (!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput_9_1_0.dll");
	}

	if (!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

internal void 
Win32InitDSound(HWND Window, int32 SamplesPerSeccond, int32 BufferSize)
{
	// NOTE: Load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if (DSoundLibrary)
	{
		// Get a Directsound Object
		direct_sound_create* DirectSoundCreate= (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
		    WaveFormat.nSamplesPerSec = SamplesPerSeccond;
		    WaveFormat.wBitsPerSample = 16;
		    WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
		    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;		    
		    WaveFormat.cbSize = 0;
			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
				
				// Create a primary buffer
				LPDIRECTSOUNDBUFFER PrimaryBuffer; 
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{					
					HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
					if (SUCCEEDED(Error))
					{
						// NOTE: we have finally set the format
						OutputDebugStringA("Primary buffer Create as set:\n");
					}
					else
					{
						//TODO: Diagnostic
					}
				}			
			}
			else
			{
				// TODO: Diagnostic
			}
			// Create a secondary buffer
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize; 
			BufferDescription.lpwfxFormat = &WaveFormat;

			
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
			if (SUCCEEDED(Error))
			{					
				OutputDebugStringA("Primary buffer Create as set:\n");
			}						
			// Start it playing
		}
		else
		{
			//TODO: logging
		}
	}
	else
	{
		// TODO: Diagnostic
	}
}

internal win32_window_dimension
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
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Width * BytesPerPixel;
}

internal void 
Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer,
						   HDC DeviceContent,
						   int WindowWidth,
						   int WindowHeight)
{
	StretchDIBits(DeviceContent,
				  0, 0, WindowWidth, WindowHeight,
				  0, 0, Buffer->Width, Buffer->Height,
				  Buffer->Memory,
				  &Buffer->Info,
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

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = WParam;
			bool32 WasDown = ((LParam & (1 << 30)) != 0);
			bool32 IsDown = ((LParam & (1 << 31)) == 0);

			if (WasDown != IsDown) // Holding
			{
				if (VKCode == 'W'){

				}
				else if (VKCode == 'A'){

				}
				else if (VKCode == 'S'){

				}
				else if (VKCode == 'D'){

				}
				else if (VKCode == 'Q'){

				}
				else if (VKCode == 'E'){

				}
				else if (VKCode == VK_UP){

				}
				else if (VKCode == VK_LEFT){
					
				}
				else if (VKCode == VK_DOWN){
					
				}
				else if (VKCode == VK_RIGHT){
					
				}
				else if (VKCode == VK_ESCAPE){
					OutputDebugStringA("Escape:");
					if (WasDown)
					{
						OutputDebugStringA("WasDown");
					}
					if (IsDown)
					{
						OutputDebugStringA("IsDown");
					}
					OutputDebugStringA("\n");
				}
				else if (VKCode == VK_SPACE){
					
				}

				bool32 AltKeyWasDown = (LParam & (1 << 29));
				if ((VKCode == VK_F4) && AltKeyWasDown)
				{
					GlobalRunning = false;
				}
			}
			//
		}break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContent = BeginPaint(Window, &Paint);

			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContent, Dimension.Width, Dimension.Height);
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


struct win32_sount_output
{
	// Note: Sound Test
	int SamplesPerSeccond;
	int ToneHz;
	int16 ToneVolume;
	uint32 RunningSamepleIndex;
	int WavePeriod;
	//int HalfWavePeriod = WavePeriod/ 2;
	int BytesperSample;
	int SencondaryBufferSize;
	real32 tSine;
	int LatencySampleCount;
};

void 
Win32FillSoundBuffer(win32_sount_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;
	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock,
											 BytesToWrite,
											 &Region1, &Region1Size,
											 &Region2, &Region2Size,
											 0)))
	{
		int16* SmapleOut = (int16*)Region1;
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesperSample;						
		for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex ++)
		{
			real32 SineValue = sinf(SoundOutput->tSine);
			int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
			*SmapleOut++ = SampleValue;
			*SmapleOut++ = SampleValue;

			SoundOutput->tSine += 2.0f * Pi32 * 1.0f / (real32)SoundOutput->WavePeriod;
			++SoundOutput->RunningSamepleIndex;
		}
		SmapleOut = (int16*)Region2;
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesperSample;
		for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex ++)
		{
			real32 SineValue = sinf(SoundOutput->tSine);
			int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
			*SmapleOut++ = SampleValue;
			*SmapleOut++ = SampleValue;

			SoundOutput->tSine += 2.0f * Pi32 * 1.0f / (real32)SoundOutput->WavePeriod;
			++SoundOutput->RunningSamepleIndex;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

int CALLBACK 
WinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR     CommandLine,
	int       ShowCode)
{
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

	Win32LoadXInput();

	WNDCLASS WindowClass = {};

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

			// Note: Graphics test
			int XOffset = 0;
			int YOffset = 0;

			win32_sount_output SoundOutput = {};
			SoundOutput.SamplesPerSeccond = 48000;
			SoundOutput.ToneHz = 256;
			SoundOutput.ToneVolume = 3000;
			SoundOutput.RunningSamepleIndex = 0;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSeccond / SoundOutput.ToneHz;
			SoundOutput.BytesperSample = sizeof(int16) * 2;
			SoundOutput.SencondaryBufferSize = SoundOutput.SamplesPerSeccond * SoundOutput.BytesperSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSeccond / 15;

			Win32InitDSound(Window, SoundOutput.SamplesPerSeccond, SoundOutput.SencondaryBufferSize);
			Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.LatencySampleCount * SoundOutput.BytesperSample);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;
	
			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);

			uint64 LastCycleCount = __rdtsc();
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

				// Input
				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						// NOTE: This controller is  connected
						XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

						bool32 Up = (Pad->wButtons && XINPUT_GAMEPAD_DPAD_UP);
						bool32 Down = (Pad->wButtons && XINPUT_GAMEPAD_DPAD_DOWN);
						bool32 Left = (Pad->wButtons && XINPUT_GAMEPAD_DPAD_LEFT);
						bool32 Right = (Pad->wButtons && XINPUT_GAMEPAD_DPAD_RIGHT);
						bool32 Start = (Pad->wButtons && XINPUT_GAMEPAD_START);
						bool32 Back = (Pad->wButtons && XINPUT_GAMEPAD_BACK);
						bool32 LeftShoulder = (Pad->wButtons && XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool32 RightShoulder = (Pad->wButtons && XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool32 AButton = (Pad->wButtons && XINPUT_GAMEPAD_A);
						bool32 BButton = (Pad->wButtons && XINPUT_GAMEPAD_B);
						bool32 XButton = (Pad->wButtons && XINPUT_GAMEPAD_X);
						bool32 YButton = (Pad->wButtons && XINPUT_GAMEPAD_Y);

						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;

						XOffset += StickX / 4096;
						YOffset += StickY / 4096;		
						SoundOutput.ToneHz = 512 + (256.0f * ((real32)StickY / 30000.0f));
						SoundOutput.WavePeriod = SoundOutput.SamplesPerSeccond / SoundOutput.ToneHz;									
					}
					else
					{
						// NOTE: The controller is not available
					}
				}
				RenderWeirdGradient(&GlobalBackBuffer, XOffset,YOffset);
				
				// Note: DirectSound Output test
				DWORD PlayCursor;
				DWORD WriteCursor;
				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					DWORD ByteToLock = (SoundOutput.RunningSamepleIndex * SoundOutput.BytesperSample) % SoundOutput.SencondaryBufferSize;

					DWORD TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesperSample)) % SoundOutput.SencondaryBufferSize;
					DWORD BytesToWrite;
					if (ByteToLock > TargetCursor)
					{
						BytesToWrite = (SoundOutput.SencondaryBufferSize - ByteToLock);
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - ByteToLock;
					}
					
					Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);									
				}

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContent, Dimension.Width, Dimension.Height);

				uint64 EndCycleCount = __rdtsc();

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);
				
				uint64 CycleElapsed = EndCycleCount - LastCycleCount;
				int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				real64 MSPerFrame = ((1000.0f * CounterElapsed) / (real64)PerfCountFrequency);
				real64 FPS = (real64)PerfCountFrequency / (real64)CounterElapsed;
				real64 MCPF = (real64)(CycleElapsed /(1000.0f * 1000.0f));

				char Buffer[256];
				sprintf(Buffer, "%.02fms/f, %.02ff/s, %.02fmc/f\n", MSPerFrame, FPS, MCPF);
				OutputDebugStringA(Buffer);

				LastCycleCount = EndCycleCount;
				LastCounter = EndCounter;
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
