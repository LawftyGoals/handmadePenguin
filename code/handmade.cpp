#include "handmade.h"


internal void GameOutputSound(game_sound_output_buffer *SoundBuffer){
	local_persist float tSine;
	int16_t ToneVolume = 3000;
	int ToneHz = 256;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16_t *SampleOut = SoundBuffer->Samples;
	
	for(int SampleIndex = 0;
			SampleIndex < SoundBuffer->SampleCount;
			++SampleIndex) {
		float SineValue = SDL_sin(tSine);
		int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		//if((tSine += 2.0f * SDL_PI_F / (float)WavePeriod) > 2.0f * SDL_PI_F){
		//	tSine -= 3.0f * SDL_PI_F;
		//	}

		tSine += (2.0f*SDL_PI_F)/((float)WavePeriod);
	}

}


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

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer){
	//TODO(law): Allow Sample offsets here for more robust platform options
	local_persist size_t XOffset = 0;
	local_persist size_t YOffset = 0;
	GameOutputSound(SoundBuffer);
	RenderWeirdGradient(*Buffer, XOffset, YOffset);

}
