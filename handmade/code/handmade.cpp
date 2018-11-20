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

		tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
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
			uint8 Green = (X + BlueOffset);
			uint8 Blue = (Y + GreenOffset);
			*Pixel++ = (Green << 8 | Blue);			
		}
		PixelRow += Buffer->Pitch;
	}	
}

internal void GameUpdateAndRender(game_input *Input,
								  game_offscreen_buffer* Buffer,
								  game_sound_output_buffer* SoundBuffer)
{
	local_presist int BlueOffset = 0;
	local_presist int GreenOffset = 0;
	local_presist int ToneHz = 256;

	game_controller_input *Input0 = &Input->Controllers[0];
	if (Input0->IsAnalog)
	{
		//NOTE: use analog movement
		BlueOffset += (int) 4.0f * (Input0->EndX);
		ToneHz = 256 +	(int)(128.0f * (Input0->EndY));		
	}
	else
	{
		//NOTE: Use digital movement
	}

	if (Input0->Down.EndedDown)
	{
		OutputDebugStringA("A IS DOWN!\n");
		GreenOffset += 1;
	}

	GameOutputSound(SoundBuffer, ToneHz);
	RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}