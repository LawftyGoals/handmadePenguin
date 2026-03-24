#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>

#define internal static
#define local_persist static
#define global_variable static

//TODO: THIS IS A GLOBAL... for now...
global_variable bool RUNNING = true;

global_variable SDL_Texture *Texture;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BytesPerPixel = 4;


internal void SDLResizeTexture(SDL_Renderer *Renderer, int Width, int Height) {

	if (BitmapMemory) free(BitmapMemory);
	if (Texture) SDL_DestroyTexture(Texture);

	Texture = SDL_CreateTexture(
			Renderer, 
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,
			Width,
			Height);
	
	BitmapWidth = Width;

	BitmapMemory = malloc(Width * Height * BytesPerPixel);

}

internal void SDLUpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer){
		SDL_UpdateTexture(Texture, 0, BitmapMemory, BitmapWidth * BytesPerPixel);

		SDL_RenderTexture(Renderer, Texture, 0, 0);

		SDL_RenderPresent(Renderer);
}


void HandleEvent(SDL_Event *Event){

	switch (Event->type){
		case SDL_EVENT_QUIT:
		{
			//TODO: Handle this with a message to user?
			printf("SDL_QUIT\n");
			RUNNING = false;
		} break;
		case SDL_EVENT_WINDOW_RESIZED:
		{
			SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
			SDL_Renderer *Renderer = SDL_GetRenderer(Window);
			printf("SDL_EVENT_WINDOW_RESIZED (%d, %d)\n", Event->window.data1, Event->window.data2);
			SDLResizeTexture(Renderer, Event->window.data1, Event->window.data2 );
		} break;
		case SDL_EVENT_WINDOW_EXPOSED:
		{
	
			SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
			SDL_Renderer *Renderer = SDL_GetRenderer(Window);	

			SDLUpdateWindow(Window, Renderer);

		}
	}

}



int main(int argc, char* argv[]){

	//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Handmade Hero", "This is handmade hero", 0);
	
	
	SDL_Window *Window;
	
	SDL_Renderer *Renderer;

		
	if(!SDL_Init(SDL_INIT_VIDEO)){
		printf("failed to init sdl");
		return 1;
	}

	if(!SDL_CreateWindowAndRenderer("Handmade Hero", 640, 480, SDL_WINDOW_RESIZABLE, &Window, &Renderer)){ 

		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
		return 1;
	}


	while (RUNNING){
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			HandleEvent(&event);
		}

	}


	SDL_DestroyWindow(Window);


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
