/**
 * @Author:      joe
 * @Version:     1.0
 * @DateTime:    2018-11-19 23:30:02
 * @Description: ...
 */

#include "handmade.h"

internal void 
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{
	local_presist real32 tSine;
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16* SampleOut = SoundBuffer->Samples;
	for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex ++)
	{
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += (real32)(2.0f * Pi32 * 1.0f / (real32)WavePeriod);
	}
}

internal void 
RenderWeirdGradient(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
{
	uint8* PixelRow = (uint8*)(Buffer->Memory);
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32* Pixel = (uint32*)(PixelRow);
		for (int X = 0; X < Buffer->Width; ++X)
		{
			uint8 Green = (uint8)(X + BlueOffset);
			uint8 Blue = (uint8)(Y + GreenOffset);
			*Pixel++ = (Green << 8 | Blue);			
		}
		PixelRow += Buffer->Pitch;
	}	
}

internal
void GameUpdateAndRender(game_memory *Memory,
						 game_input *Input,
						 game_offscreen_buffer* Buffer,
						 game_sound_output_buffer* SoundBuffer)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	
	game_state *GameState = (game_state*)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		char *FileName = __FILE__;
		
		debug_read_file_result File = DEBUGPlatformReadEntireFile(FileName);
		if (File.Contents)
		{
			DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
			DEBUGPlatformFreeFileMemory(Memory);
		}

		GameState->ToneHz = 256;
		GameState->GreenOffset = 0;
		GameState->BlueOffset = 0;

		Memory->IsInitialized = true;
	}
	game_controller_input *Input0 = &Input->Controllers[0];
	if (Input0->IsAnalog)
	{
		//NOTE: use analog movement
		GameState->BlueOffset += (int)(4.0f * Input0->EndX);
		GameState->ToneHz = 256 + (int)(128.0f * Input0->EndY);		
	}
	else
	{
		//NOTE: Use digital movement
	}

	if (Input0->Down.EndedDown)
	{
		GameState->GreenOffset += 1;
	}

	GameOutputSound(SoundBuffer, GameState->ToneHz);
	RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}