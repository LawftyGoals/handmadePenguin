#ifndef SDL_HANDMADE_H


/*
 * TODO:(law): services tha the platform provides to the game.
*/


/*
 * NOTE(law): Services that the game provides to the platform layer.
*/

// FOURE THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use


// pixels are always 32-bit and have the bytes of BGRX order
typedef struct {
	void *Memory;
	int Width;
	int Height;
	int Pitch;
} game_offscreen_buffer;

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset);

#define SDL_HANDMADE_H
#endif
