
#include "handmade.h"

internal void RenderWeirdGradient(game_offscreen_buffer Buffer, int BlueOffset, int GreenOffset){
	uint8_t *Row = (uint8_t *)Buffer.Memory;

	uint8_t Alpha = 255;

	for(int Y = 0;
			Y < Buffer.Height;
			++Y)
	{
		uint32_t *Pixel = (uint32_t *)Row;
		for(int X = 0;
				X < Buffer.Width;
				++X)
		{
			uint8_t Blue = (X + BlueOffset);
			uint8_t Green = (Y + GreenOffset);
			uint8_t Red = 255 -((((Blue + Green)*100)/((Green + 255)))*255)/100;
			*Pixel++ =  ((Alpha << 24) | (Red << 16) | (Green << 8) | Blue);
		}

		Row += Buffer.Pitch;
	}
}

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset){
	RenderWeirdGradient(*Buffer, XOffset, YOffset);
	

}
