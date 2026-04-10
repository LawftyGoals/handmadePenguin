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


 // AUDIO


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


bool HandleEvent(SDL_Event *Event){

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
									printf("is down\n");
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

/*
internal void SDLAudioCallback(void *UserData,SDL_AudioStream *Stream, int additional_amount, int total_amount){
	memset(Stream, 0, additional_amount);
}
*/

internal void SDLCALL SDLCustomAudioCallback(void *userdata, SDL_AudioStream *astream, int additional_amount, int total_amount) {

	local_persist int32_t RunningSampleIndex = 0;
	
	int16_t ToneVolume = 10000;
	int ToneHz = 256;
	int SamplesPerSecond = 48000;

	int BytesPerSample = sizeof(int16_t) * 2;

	int SquareWavePeriod = SamplesPerSecond / ToneHz;
	int HalfSquareWavePeriod = SquareWavePeriod / 2;

	void *SoundBuffer = malloc(additional_amount);
	int16_t *SampleOut = (int16_t *)SoundBuffer;
	int SampleCount = additional_amount / BytesPerSample;

	for (int SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex){
		int16_t SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
	}

	SDL_PutAudioStreamData(astream, SoundBuffer, additional_amount);

	free(SoundBuffer);
	
}


internal SDL_AudioStream *SDLInitAudio(uint8_t channels, int32_t SamplesFrequency) {

	SDL_AudioSpec spec = { SDL_AUDIO_S16, channels, SamplesFrequency};
	// THE AUDIOCALLBACK FUNCTION DOES NOT SEEM TO BE NECESSARY ON SDL3
	// use putaudiostreams and getaudiostreams.
	//SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, SDLAudioCallback, NULL);

	SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, SDLCustomAudioCallback, NULL);
	if(!stream){
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create audio stream: %s", SDL_GetError());
		return NULL;
	}

	return stream;
}




int main(int argc, char* argv[]){

	SDL_Window *Window;	
	SDL_Renderer *Renderer;
	SDL_AudioStream *AudioStream = NULL;

	if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)){
		return 1;
	}

	if(SDL_HasGamepad()) SDLOpenGamepads();

	if(!SDL_CreateWindowAndRenderer("Handmade Penguin", 640, 480, SDL_WINDOW_RESIZABLE, &Window, &Renderer)){ 

		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
		return 1;
	}

	int SamplesPerSecond = 48000;	
	/*int ToneHz = 256;
	int16_t ToneVolume = 6000;
	uint32_t RunningSampleIndex = 0;
	int SquareWavePeriod = SamplesPerSecond / ToneHz;
	int HalfSquareWavePeriod = SquareWavePeriod / 2;
	int BytesPerSample = sizeof(int16_t) * 2;
	int BytesToWrite = 800 * BytesPerSample;
	*/
	if((AudioStream = SDLInitAudio(2, SamplesPerSecond)) == NULL){
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not initialize audio.\n");
		return 1;
	}

	SDL_ResumeAudioStreamDevice(AudioStream);

	bool RUNNING = true;

	int XOffset = 0;
	int YOffset = 0;

	SDLGetWindowDimension(Window);
	sdl_window_dimension Dimension = SDLGetWindowDimension(Window);
	SDLResizeTexture(&GlobalBackbuffer, Renderer, Dimension.Width, Dimension.Height);


	while (RUNNING){
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			RUNNING = HandleEvent(&event);
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
/*
		void *SoundBuffer = malloc(BytesToWrite);
		int16_t *SampleOut = (int16_t *)SoundBuffer;
		int SampleCount = BytesToWrite/BytesPerSample;

		for(int SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex){
			int16_t SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;
		}
		SDL_PutAudioStreamData(AudioStream, SoundBuffer,  BytesToWrite);
		free(SoundBuffer);
*/

		SDLUpdateWindow(Window, Renderer, GlobalBackbuffer);
	}

	SDL_DestroyWindow(Window);
	SDLCloseGamepads();
	SDL_Quit();

	return(0);
}


