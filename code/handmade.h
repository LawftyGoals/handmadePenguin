#ifndef SDL_HANDMADE_H


/*
 * TODO:(law): services tha the platform provides to the game.
*/


/*
 * NOTE(law): Services that the game provides to the platform layer.
*/

// FOURE THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use

typedef struct {
	int SamplesPerSecond;
	int SampleCount;
	int16_t *Samples;
} game_sound_output_buffer;

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer);

// pixels are always 32-bit and have the bytes of BGRX order
typedef struct {
	void *Memory;
	int Width;
	int Height;
	int Pitch;
} game_offscreen_buffer;

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer);

#define SDL_HANDMADE_H
#endif
