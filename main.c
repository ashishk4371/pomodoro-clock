#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <SDL2/SDL_mixer.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define DIGIT_WIDTH 120
#define DIGIT_HEIGHT 120
#define START_MIN 0 
#define START_SEC 5 

Mix_Music *sound = NULL;

typedef struct {
	SDL_Texture *texture;
	int width;
	int height;
} Image;

typedef struct {
    SDL_Rect rectangle;
    SDL_Color normal_color;
    SDL_Color hover_color;
    SDL_Color pressed_color;
    int is_hovered;
    int is_pressed;
	Image image;
} Button;

typedef struct {
	SDL_Rect rectangle;
	SDL_Color color;
	Button button;
	Image *digits;
	bool started;
	int minutes;
	int seconds;
	Uint32 last_tick;
} Timer;

int validateInit() {
	if(SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 0;
	}

	if(!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
		fprintf(stderr, "SDL Image failed: %s\n", IMG_GetError());
		SDL_Quit();
		return 0;
	}

	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		fprintf(stderr, "SDL mixer failed: %s\n", Mix_GetError());
		return 0;
	}

	return 1;
}

Image loadImage(SDL_Renderer *renderer, const char *filepath) {

	Image image = {0};
	SDL_Surface *surface = IMG_Load(filepath);

	if(surface == NULL) {
		fprintf(stderr, "failed to load image: %s %s\n", filepath, IMG_GetError());
		return image;
	}

	SDL_Texture *image_texture = SDL_CreateTextureFromSurface(renderer, surface);
	if(image_texture == NULL) {
		fprintf(stderr, "failed to create image texture: %s\n", SDL_GetError());
		SDL_FreeSurface(surface);
		return image;
	}

	image.texture = image_texture;

	SDL_SetTextureBlendMode(image.texture, SDL_BLENDMODE_BLEND);

	image.width = surface->w;
	image.height = surface->h;

	SDL_FreeSurface(surface);

	return image;
}

void renderImage(SDL_Renderer *renderer, const Image *image, SDL_Rect *destination) {
	if(image == NULL || image->texture == NULL) {
		return;
	}
	SDL_RenderCopy(renderer,
					image->texture,
					NULL,
					destination
					);
}

void destroyImage(Image *image) {
	if(image->texture != NULL) {
		SDL_DestroyTexture(image->texture);
		image->texture = NULL;
	}
}

void renderTime(SDL_Renderer *renderer, Timer *timer, Image *colon) {

	int min_tens = timer->minutes / 10;
	int min_ones = timer->minutes % 10;

	int sec_tens = timer->seconds / 10;
	int sec_ones = timer->seconds % 10;

	SDL_Rect destination = {
		timer->rectangle.x, timer->rectangle.y, DIGIT_WIDTH, DIGIT_HEIGHT
	};

	// tenth min digit
	destination.x = timer->rectangle.x;
	renderImage(renderer, &timer->digits[min_tens], &destination);

	// oneth min digit
	destination.x = timer->rectangle.x + DIGIT_WIDTH;
	renderImage(renderer, &timer->digits[min_ones], &destination);

	destination.x = timer->rectangle.x + DIGIT_WIDTH * 2;
	renderImage(renderer, colon, &destination);

	// tenth sec digit
	destination.x = timer->rectangle.x + DIGIT_WIDTH * 3;
	renderImage(renderer, &timer->digits[sec_tens], &destination);

	// oneth sec digit
	destination.x = timer->rectangle.x + DIGIT_WIDTH * 4;
	renderImage(renderer, &timer->digits[sec_ones], &destination);
}

void renderButton(SDL_Renderer *renderer, Button *button) {
	SDL_Color color;

	if(button->is_pressed) {
		color = button->pressed_color;
	} else if(button->is_hovered) {
		color = button->hover_color;
	} else {
		color = button->normal_color;
	}

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &button->rectangle);

	SDL_RenderCopy(renderer, button->image.texture, NULL, &button->rectangle);
}

void updateTimer(Timer *timer, SDL_Renderer *renderer, Image *colon) {
	if(!timer->started) {
		timer->last_tick = SDL_GetTicks();
		return;
	}

	Uint32 current_tick = SDL_GetTicks();

	while(current_tick - timer->last_tick >= 1000) {
		timer->last_tick += 1000;

		if(timer->minutes == 0 && timer->seconds == 0) {
			if(timer->started) {
				Mix_PlayMusic(sound, 1);
			}
			timer->started = false;
			break;
		}

		if(timer->seconds > 0) {
			timer->seconds--;
		} else {
			timer->minutes--;
			timer->seconds = 59;
		}

		renderTime(renderer, timer, colon);
	}
}

int main() {

	if(!validateInit()) {
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow(
										 "Pomo Dhillon",
										 SDL_WINDOWPOS_CENTERED,
										 SDL_WINDOWPOS_CENTERED,
										 800,
										 600,
										 SDL_WINDOW_SHOWN
										 );

	if(window == NULL) {
		fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_RaiseWindow(window);

	SDL_Renderer *renderer = SDL_CreateRenderer(
												window,
												-1,
												SDL_RENDERER_ACCELERATED
											   );

	if(renderer == NULL) {
		fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	sound = Mix_LoadMUS("sound/alarm.mp3");
	if(sound == NULL) {
		fprintf(stderr, "Failed to load the sound: %s\n", Mix_GetError());
		Mix_CloseAudio();
		SDL_Quit();
		return 1;
	}
	
	int running = 1;
	SDL_Event event;

	Image numbers[10] = {
		loadImage(renderer, "image/0.png"),
		loadImage(renderer, "image/1.png"),
		loadImage(renderer, "image/2.png"),
		loadImage(renderer, "image/3.png"),
		loadImage(renderer, "image/4.png"),
		loadImage(renderer, "image/5.png"),
		loadImage(renderer, "image/6.png"),
		loadImage(renderer, "image/7.png"),
		loadImage(renderer, "image/8.png"),
		loadImage(renderer, "image/9.png"),
	};

	Image start_img = loadImage(renderer, "image/start.png");
	Image reset_img = loadImage(renderer, "image/reset.png");
	Image colon = loadImage(renderer, "image/colon.png");

	int clockWidth = DIGIT_WIDTH * 5;
	int clockHeight = DIGIT_HEIGHT;

	int x = (WINDOW_WIDTH - clockWidth) / 2;
	int y = (WINDOW_HEIGHT - clockHeight) / 2.5;

	Button start_button = {
		.rectangle = { clockWidth - DIGIT_WIDTH, y + DIGIT_HEIGHT, 200, 70 },
		.normal_color = { 70, 70, 180, 255 },
		.hover_color = { 100, 100, 230, 255 },
		.pressed_color = { 40, 40, 120, 255 },
		.is_hovered = 0,
		.is_pressed = 0,
		.image = start_img
	};

	Timer timer = {
		.rectangle = {x, y, clockWidth, clockHeight},
		.color = {},
		.button = start_button,
		.digits = numbers,
		.started = false,
		.minutes = START_MIN,
		.seconds = START_SEC,
		.last_tick = SDL_GetTicks()
	};

	while(running) {
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				running = 0;
			} else if(event.type == SDL_MOUSEMOTION) {
				int mouse_x = event.motion.x;
				int mouse_y = event.motion.y;

				start_button.is_hovered = SDL_PointInRect(
														  &(SDL_Point) {mouse_x, mouse_y},
														  &start_button.rectangle);
			} else if(event.type == SDL_MOUSEBUTTONDOWN) {
				if(event.button.button == SDL_BUTTON_LEFT) {
					int mouse_x = event.button.x;
					int mouse_y = event.button.y;

					start_button.is_pressed = SDL_PointInRect(&(SDL_Point) {mouse_x, mouse_y},
															  &start_button.rectangle);
				}
			} else if(event.type == SDL_MOUSEBUTTONUP) {
				if(event.button.button == SDL_BUTTON_LEFT) {
					int mouse_x = event.button.x;
					int mouse_y = event.button.y;

					if(start_button.is_pressed && SDL_PointInRect(&(SDL_Point) {mouse_x, mouse_y}, &start_button.rectangle)) {
						start_button.is_pressed = 0;

						if(start_button.image.texture == reset_img.texture) {
							start_button.image = start_img;
							timer.started = false;
							Mix_HaltMusic();
						} else {
							start_button.image = reset_img;
							timer.started = true;
						}

						timer.minutes = START_MIN;
						timer.seconds = START_SEC;
						timer.last_tick = SDL_GetTicks();
					}
				}
			}
		}

		updateTimer(&timer, renderer, &colon);

		SDL_SetRenderDrawColor(renderer, 30, 30, 80, 255);
		SDL_RenderClear(renderer);
		renderButton(renderer, &start_button);
		renderTime(renderer, &timer, &colon);
		SDL_RenderPresent(renderer);

		SDL_Delay(16);
	}

	for(int i=0; i<=10; i++) {
		destroyImage(&numbers[i]);
	}

	destroyImage(&colon);
	destroyImage(&start_img);
	destroyImage(&reset_img);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	IMG_Quit();
	SDL_Quit();
	
	return 0;
}
