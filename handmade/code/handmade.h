#if !defined(HANDMADE_H)
/**
 * @Author:      joe
 * @Version:     1.0
 * @DateTime:    2018-11-19 23:00:39
 * @Description: ...
 */

/*
	Note: Services that platform layer provides to the game
 */

/*
	Note: Services that the game provides to the platform layer.
 */

// Four things - timing, controller/keyboard input, bigmap buffer to use, sound buffer to use

struct game_offscreen_buffer
{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};


void GameUpdateAndRender(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset);

#define HANDMADE_H
#endif