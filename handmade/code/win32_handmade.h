/**
 * @Author:      joe
 * @Version:     1.0
 * @DateTime:    2018-11-20 11:15:19
 * @Description: ....
 */

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


struct win32_sount_output
{
	// Note: Sound Test
	int SamplesPerSecond;
	uint32 RunningSamepleIndex;
	int BytesperSample;
	int SencondaryBufferSize;
	real32 tSine;
	int LatencySampleCount;
};
