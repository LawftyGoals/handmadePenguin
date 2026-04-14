#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_gamepad.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <SDL3/SDL.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#define internal static
#define local_persist static
#define global_variable static

#define MAX_GAMEPADS 4



typedef struct {
	int Size;
	int WriteCursor;
	int PlayCursor;
	void *Data;
} sdl_audio_ring_buffer;

// pixels are always 32-bit and have the bytes of BGRX order
typedef struct {
	SDL_Texture *Texture;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
} sdl_offscreen_buffer;

typedef struct {
	int Width;
	int Height;
} sdl_window_dimension;

typedef struct {
	int SamplesPerSecond;
	int ToneHz;
	int16_t ToneVolume;
	uint32_t RunningSampleIndex;
	float WavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
	float tSine;
	int LatencySampleCount;
} sdl_sound_output;

//SCREEN BUFFER
global_variable sdl_offscreen_buffer GlobalBackbuffer;

sdl_window_dimension SDLGetWindowDimension(SDL_Window *Window){
	sdl_window_dimension Result;

	SDL_GetWindowSize(Window, &Result.Width, &Result.Height);

	return Result;
}

internal void RenderWeirdGradient(sdl_offscreen_buffer Buffer, int BlueOffset, int GreenOffset){
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


internal void SDLResizeTexture(sdl_offscreen_buffer *Buffer,SDL_Renderer *Renderer, int Width, int Height) {
	int BytesPerPixel = 4;
	if (Buffer->Memory) munmap(Buffer->Memory, Buffer->Height * Buffer->Width * BytesPerPixel);
	if (Buffer->Texture) SDL_DestroyTexture(Buffer->Texture);

	Buffer->Texture = SDL_CreateTexture(
			Renderer, 
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,
			Width,
			Height);

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->Pitch = Width * BytesPerPixel;

	Buffer->Memory = mmap(0,
			Height * Width * BytesPerPixel,
			PROT_READ|PROT_WRITE,
			MAP_ANONYMOUS|MAP_PRIVATE,
			-1,
			0);


}

internal void SDLUpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer, sdl_offscreen_buffer Buffer){
	SDL_UpdateTexture(Buffer.Texture,
			0,
			Buffer.Memory,
			Buffer.Pitch);

	SDL_RenderTexture(Renderer,
			Buffer.Texture,
			0,
			0);

	SDL_RenderPresent(Renderer);
}


bool HandleEvent(SDL_Event *Event, sdl_sound_output *SoundOutput){

	bool shouldContinue = true;

	switch (Event->type){
		case SDL_EVENT_QUIT:
			{
				//TODO: Handle this with a message to user?
				printf("SDL_QUIT\n");
				shouldContinue = false;
			} break;
		case SDL_EVENT_WINDOW_RESIZED:
			{
				SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
				SDL_Renderer *Renderer = SDL_GetRenderer(Window);
				printf("SDL_EVENT_WINDOW_RESIZED (%d, %d)\n", Event->window.data1, Event->window.data2);
				SDLResizeTexture(&GlobalBackbuffer, Renderer, Event->window.data1, Event->window.data2 );
			} break;
		case SDL_EVENT_WINDOW_EXPOSED:
			{

				SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
				SDL_Renderer *Renderer = SDL_GetRenderer(Window);	

				SDLUpdateWindow(Window, Renderer, GlobalBackbuffer);

			} break;
			//Key events
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			{
				SDL_Keycode key = Event->key.key;
				bool isDown = (Event->key.down);
				bool wasDown = false;
				if(!(Event->key.down)){
					wasDown = true;
				}
				if(Event->key.repeat){
					wasDown = true;
				}
				if(!(Event->key.repeat)){

					switch(key){
						case SDLK_W:
							{
								printf("W: ");
								if(isDown){
									//SoundOutput->ToneHz = 512 + (int)(256.0f * (1.0f/3000.0));
									//SoundOutput->WavePeriod = SoundOutput->SamplesPerSecond/SoundOutput->ToneHz;
									printf("SPS: %d\nTHz: %d\nTV: %d\nRSI: %d\nWP: %fd\nBPS: %d\nSBS: %d\ntS: %fLSC %d\n", SoundOutput->SamplesPerSecond, SoundOutput->ToneHz, SoundOutput->ToneVolume, SoundOutput->RunningSampleIndex, SoundOutput->WavePeriod, SoundOutput->BytesPerSample, SoundOutput->SecondaryBufferSize, SoundOutput->tSine, SoundOutput->LatencySampleCount);
								}
								if(wasDown){
									printf("was down\n");
								}
							} break;
					}
				}
			} break;
	}

	return shouldContinue;

}

SDL_Gamepad *GamepadHandles[MAX_GAMEPADS];

internal void SDLOpenGamepads(){
	int maxpads = MAX_GAMEPADS;
	SDL_JoystickID *GamepadIds = SDL_GetGamepads(&maxpads);

	for(int idx = 0; idx < MAX_GAMEPADS; idx++) GamepadHandles[idx] = nullptr;


	int count = 0;
	while(GamepadIds[count] != '\0') count++;

	for(int GamepadIndex = 0; GamepadIndex < count; ++GamepadIndex){
		if(SDL_IsGamepad(GamepadIds[GamepadIndex])){
			GamepadHandles[GamepadIndex] = SDL_OpenGamepad(GamepadIds[GamepadIndex]);
		}
	}
}


internal void SDLCloseGamepads(){
	for(int GamepadIndex = 0; GamepadIndex < MAX_GAMEPADS; ++GamepadIndex){
		if(GamepadHandles[GamepadIndex]){
			SDL_CloseGamepad(GamepadHandles[GamepadIndex]);
		}
	}
}

internal SDL_AudioStream *SDLInitAudio(uint8_t channels, int32_t SamplesFrequency) {

	SDL_AudioSpec spec = { SDL_AUDIO_S16LE, channels, SamplesFrequency};

	SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if(!stream){
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create audio stream: %s", SDL_GetError());
		return NULL;
	}

	return stream;
}

internal void SDLFillSoundBuffer(SDL_AudioStream *AudioStream, sdl_sound_output *SoundOutput, int ByteToLock, int BytesToWrite){
	int SampleCount = BytesToWrite/SoundOutput->BytesPerSample;
	void *AudioBuffer = malloc(BytesToWrite);
	int16_t *SampleOut = (int16_t*)AudioBuffer;
	
	for(int SampleIndex = 0;
			SampleIndex < SampleCount;
			++SampleIndex) {
		float SineValue = SDL_sin(SoundOutput->tSine);
		int16_t SampleValue = (int16_t)(SineValue * SoundOutput->ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		if((SoundOutput->tSine += 2.0f * SDL_PI_F / (float)SoundOutput->WavePeriod) > 2.0f * SDL_PI_F){
			SoundOutput->tSine -= 3.0f * SDL_PI_F;
			}

		//SoundOutput->tSine += (2.0f*SDL_PI_F)/((float)SoundOutput->WavePeriod);
		++SoundOutput->RunningSampleIndex;
	}

	SDL_PutAudioStreamData(AudioStream, AudioBuffer, BytesToWrite);
	
	free(AudioBuffer);
}



int main(int argc, char* argv[]){

	SDL_Window *Window;	
	SDL_Renderer *Renderer;
	SDL_AudioStream *AudioStream = NULL;
	uint64_t PerformanceCounterFrequency = SDL_GetPerformanceFrequency();

	if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)){
		return 1;
	}

	if(SDL_HasGamepad()) SDLOpenGamepads();

	if(!SDL_CreateWindowAndRenderer("Handmade Penguin", 640, 480, SDL_WINDOW_RESIZABLE, &Window, &Renderer)){ 

		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
		return 1;
	}

	sdl_sound_output SoundOutput = {};
	SoundOutput.SamplesPerSecond = 48000;
	SoundOutput.ToneHz = 256;
	SoundOutput.ToneVolume = 3000;
	SoundOutput.WavePeriod = (float)SoundOutput.SamplesPerSecond / (float)SoundOutput.ToneHz;
	SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
	SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

	if((AudioStream = SDLInitAudio(2, SoundOutput.SamplesPerSecond)) == NULL){
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not initialize audio.\n");
		return 1;
	}

	SDLFillSoundBuffer(AudioStream, &SoundOutput, 0, SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample);
	SDL_ResumeAudioStreamDevice(AudioStream);
	

	bool RUNNING = true;
	int XOffset = 0;
	int YOffset = 0;

	SDLGetWindowDimension(Window);
	sdl_window_dimension Dimension = SDLGetWindowDimension(Window);
	SDLResizeTexture(&GlobalBackbuffer, Renderer, Dimension.Width, Dimension.Height);


	uint64_t PreviousCounter = SDL_GetPerformanceCounter();
	
	while (RUNNING){


		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			RUNNING = HandleEvent(&event, &SoundOutput);
		}

		//++XOffset; 
		//YOffset += 2;

		if(SDL_HasGamepad()){

			for (int GamepadIndex = 0; GamepadIndex < MAX_GAMEPADS; GamepadIndex++){
				SDL_Gamepad *gamepad = GamepadHandles[GamepadIndex];
				if(gamepad != 0 && SDL_GamepadConnected(gamepad)){
					// NOTE: We have a gamepad with index GamepadIndex.
					//bool Up = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP);
					//bool Down = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
					//bool Left = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
					//bool Right = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
					//bool Start = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START);
					//bool Back = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK);
					//bool LeftShoulder = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
					//bool RightShoulder = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
					bool AButton = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
					bool BButton = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST);
					//bool XButton = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST);
					//bool YButton = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH);	
					//int16_t StickX = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
					//int16_t StickY = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);	

					if(AButton){
						YOffset+=2;
					}
					if(BButton){
						++XOffset;
					}
				}
				else {
					//TODO: SOMETHING SOMETHING DARKSIDE
				}
			}

		}

		RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);

		int TargetQueueBytes = SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample;
		int RequiredBytes = TargetQueueBytes - SDL_GetAudioStreamQueued(AudioStream);
		if(RequiredBytes) SDLFillSoundBuffer(AudioStream, &SoundOutput, 0, RequiredBytes);


		SDLUpdateWindow(Window, Renderer, GlobalBackbuffer);

		uint64_t EndCounter = SDL_GetPerformanceCounter();

		uint64_t CounterElapsed = EndCounter - PreviousCounter;

		double MSPerFrame = (((1000.0f * (double)CounterElapsed) / (double)PerformanceCounterFrequency));
		double FPS = (double)PerformanceCounterFrequency / (double)CounterElapsed;
		printf("%.02f ms/f, %.02ff\n", MSPerFrame, FPS);

		//TODO display value;

		PreviousCounter = EndCounter;

	}

	SDL_DestroyWindow(Window);
	SDLCloseGamepads();
	SDL_Quit();

	return(0);
}


