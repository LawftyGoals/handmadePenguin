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

global_variable sdl_offscreen_buffer GlobalBackbuffer;

sdl_window_dimension SDLGetWindowDimension(SDL_Window *Window){
	sdl_window_dimension Result;

	SDL_GetWindowSize(Window, &Result.Width, &Result.Height);

	return Result;
}

internal void RenderWeirdGradient(sdl_offscreen_buffer Buffer, int BlueOffset, int GreenOffset){
	uint8_t *Row = (uint8_t *)Buffer.Memory;
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

					*Pixel++ =  ((Green << 8) | Blue);
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
			SDL_PIXELFORMAT_XRGB8888,
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





int main(int argc, char* argv[]){

	//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Handmade Hero", "This is handmade hero", 0);
	SDL_Window *Window;	
	SDL_Renderer *Renderer;

		
	if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)){
		printf("failed to init sdl");
		return 1;
	}

	if(SDL_HasGamepad()) SDLOpenGamepads();

	if(!SDL_CreateWindowAndRenderer("Handmade Hero", 640, 480, SDL_WINDOW_RESIZABLE, &Window, &Renderer)){ 

		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
		return 1;
	}

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
		RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);
		SDLUpdateWindow(Window, Renderer, GlobalBackbuffer);

		//++XOffset;
		//YOffset += 2;

		if(SDL_HasGamepad()){
				
				for (int GamepadIndex = 0; GamepadIndex < MAX_GAMEPADS; GamepadIndex++){
						Gamepad *gamepad = GamepadHandles[GamepadIndex];
						if(gamepad != 0 && SDL_GamepadConnected(gamepad)){

							
						}
				}

		}


	SDL_DestroyWindow(Window);

	SDLCloseGamepads();
	SDL_Quit();

	return(0);
}


/*
 *
 
			const double now = ((double)SDL_GetTicks()) / 1000.0;

			const float red = (float) (0.5 + 0.5 * SDL_sin(now));
			const float green = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
			const float blue = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 /  3));
			SDL_SetRenderDrawColorFloat(Renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);

			SDL_RenderClear(Renderer);

			SDL_RenderPresent(Renderer);
*/
