/**
 * @Author:      joe
 * @Version:     1.0
 * @DateTime:    2018-11-15 23:38:09
 * @Description: ...
 */

#include <stdint.h>
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

#include "handmade.cpp"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <Xinput.h>
#include <dsound.h>

#include "win32_handmade.h"

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


internal debug_read_file_result
DEBUGPlatformReadEntireFile(char *FileName)
{
	debug_read_file_result Result = {};
	HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ,FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32 FileSize32 = SetTruncateUint64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
					(FileSize32 == BytesRead))
				{
					// NOTE: File read successfully
					Result.ContentsSize = FileSize32;
				}
				else
				{
					// TODO:Logging
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{
				// TODO:Logging
			}
		}
		else
		{
			// TODO:Logging
		}
		CloseHandle(FileHandle);
	}
	else
	{
		// TODO:Logging
	}

	return Result;
}
internal void
 DEBUGPlatformFreeFileMemory(void *Memory)
 {
 	if (Memory)
 	{
 		VirtualFree(Memory, 0, MEM_RELEASE);
 	}
 }

internal bool32 
DEBUGPlatformWriteEntireFile(char *FileName, uint32 MemorySize, void *Memory)
{
	bool32 Result = false;
	HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE,0, 0, CREATE_ALWAYS, 0, 0);

	if (FileHandle != INVALID_HANDLE_VALUE)
	{		
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			Result = (BytesWritten == MemorySize);
		}
		else
		{
			// TODO:Logging
			DEBUGPlatformFreeFileMemory(Memory);
		}
		CloseHandle(FileHandle);
	}
	else
	{
		// TODO:Logging
	}

	return Result;
}

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
		if(!XInputGetState) {XInputGetState = XInputGetStateStub;}
		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
		if(!XInputSetState) {XInputSetState = XInputSetStateStub;}
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
		direct_sound_create* DirectSoundCreate =
				(direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

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
			uint32 VKCode = (uint32)WParam;
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



internal void
Win32ClearBuffer(win32_sount_output* SoundOutput)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;
	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0,
											 SoundOutput->SencondaryBufferSize,
											 &Region1, &Region1Size,
											 &Region2, &Region2Size,
											 0)))
	{
		uint8* DestSample = (uint8*)Region1;
		for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ByteIndex ++)
		{		
			*DestSample++ = 0;
		}
		DestSample = (uint8*)Region2;
		for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ByteIndex ++)
		{		
			*DestSample++ = 0;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

void 
Win32FillSoundBuffer(win32_sount_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
					 game_sound_output_buffer* SourceBuffer)
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
		int16* DestSample = (int16*)Region1;
		int16* SourceSample = SourceBuffer->Samples;
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesperSample;
		for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex ++)
		{		
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSamepleIndex;
		}
		
		DestSample = (int16*)Region2;
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesperSample;
		for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex ++)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSamepleIndex;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void 
Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
								game_button_state *OldState, DWORD ButtonBit,
								game_button_state *NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

int CALLBACK 
WinMain(HINSTANCE Instance,
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

			win32_sount_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSamepleIndex = 0;
			SoundOutput.BytesperSample = sizeof(int16) * 2;
			SoundOutput.SencondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesperSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SencondaryBufferSize);
			Win32ClearBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;
			int16* Samples = (int16*)VirtualAlloc(0, SoundOutput.SencondaryBufferSize,
												  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
			LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);			
#else
			LPVOID BaseAddress = 0;
#endif
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(1);
			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize,
												  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			GameMemory.TransientStorage = ((uint8*)GameMemory.PermanentStorage
					 + GameMemory.PermanentStorageSize);

			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				game_input Input[2] = {};
				game_input *OldInput = &Input[0];
				game_input *NewInput = &Input[1];

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
					DWORD MaxControllerCount = XUSER_MAX_COUNT;
					// if (MaxControllerCount > ArrayCount(NewInput->Controllers))
					// {
					// 	MaxControllerCount = ArrayCount(NewInput->Controllers);
					// }
					for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
					{
						game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
						game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];

						XINPUT_STATE ControllerState;
						if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
						{
							// NOTE: This controller is  connected
							XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

							bool32 Up = (Pad->wButtons && XINPUT_GAMEPAD_DPAD_UP);
							bool32 Down = (Pad->wButtons && XINPUT_GAMEPAD_DPAD_DOWN);
							bool32 Left = (Pad->wButtons && XINPUT_GAMEPAD_DPAD_LEFT);
							bool32 Right = (Pad->wButtons && XINPUT_GAMEPAD_DPAD_RIGHT);

							NewController->IsAnalog = true;
	                        NewController->StartX = OldController->EndX;
	                        NewController->StartY = OldController->EndY;

							real32 X;
							if (Pad->sThumbLX < 0)
							{
								X = (real32)Pad->sThumbLX / 32768.0f;
							}
							else
							{
								X = (real32)Pad->sThumbLX / 32767.0f;
							}
							NewController->MinX = NewController->MaxX = NewController->EndX = X;

							real32 Y;
							if (Pad->sThumbLY < 0)
							{
								Y = (real32)Pad->sThumbLY / 32768.0f;
							}
							else
							{
								Y = (real32)Pad->sThumbLY / 32767.0f;
							}
							NewController->MinY = NewController->MaxY = NewController->EndY = Y;

							// int16 StickX = (int16)Pad->sThumbLX;
							// int16 StickY = (int16)Pad->sThumbLY;

							Win32ProcessXInputDigitalButton(Pad->wButtons,
															&OldController->Down, XINPUT_GAMEPAD_A,
															&NewController->Down);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
															&OldController->Right, XINPUT_GAMEPAD_B,
															&NewController->Right);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
															&OldController->Left, XINPUT_GAMEPAD_X,
															&NewController->Left);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
															&OldController->Up, XINPUT_GAMEPAD_Y,
															&NewController->Up);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
															&OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
															&NewController->LeftShoulder);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
															&OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
															&NewController->RightShoulder);

							//bool32 Start = (Pad->wButtons && XINPUT_GAMEPAD_START);
							//bool32 Back = (Pad->wButtons && XINPUT_GAMEPAD_BACK);														
						}
						else
						{
							// NOTE: The controller is not available
						}
					}

					
					//RenderWeirdGradient(&GlobalBackBuffer, XOffset,YOffset);
					
					// Note: DirectSound Output test
					DWORD ByteToLock = 0;
					DWORD TargetCursor = 0;
					DWORD BytesToWrite = 0;
					DWORD PlayCursor = 0;
					DWORD WriteCursor = 0;
					bool32 SoundIsValid = false;
					if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
					{
						ByteToLock = (SoundOutput.RunningSamepleIndex * SoundOutput.BytesperSample)
									 % SoundOutput.SencondaryBufferSize;

						TargetCursor = (PlayCursor + 
									   (SoundOutput.LatencySampleCount * SoundOutput.BytesperSample))
									   % SoundOutput.SencondaryBufferSize;
						
						if (ByteToLock > TargetCursor)
						{
							BytesToWrite = (SoundOutput.SencondaryBufferSize - ByteToLock);
							BytesToWrite += TargetCursor;
						}
						else
						{
							BytesToWrite = TargetCursor - ByteToLock;
						}

						SoundIsValid = true;
					}

					//int16* Samples = (int16*)_alloca(48000 * 2);
					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesperSample;
					SoundBuffer.Samples = Samples;



					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBackBuffer.Memory;
					Buffer.Width = GlobalBackBuffer.Width;
					Buffer.Height = GlobalBackBuffer.Height;
					Buffer.Pitch = GlobalBackBuffer.Pitch;
					GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);

					if (SoundIsValid)
					{
						Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);									
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

	#if 0
					char Buffer[256];
					sprintf(Buffer, "%.02fms/f, %.02ff/s, %.02fmc/f\n", MSPerFrame, FPS, MCPF);
					OutputDebugStringA(Buffer);
	#endif 
					LastCycleCount = EndCycleCount;
					LastCounter = EndCounter;

					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;
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
