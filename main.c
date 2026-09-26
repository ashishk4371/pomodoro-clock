#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <SDL2/SDL_mixer.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define DIGIT_WIDTH 120
#define DIGIT_HEIGHT 120
#define FOCUS_START_MIN 0
#define FOCUS_START_SEC 10
#define BREAK_START_MIN 0 
#define BREAK_START_SEC 5 

Mix_Music    *sound       = NULL;
SDL_Window   *window      = NULL;
SDL_Renderer *renderer    = NULL;
SDL_Cursor   *arrowCursor = NULL;
SDL_Cursor   *handCursor  = NULL;

typedef struct {
	SDL_Texture *texture;
	int width;
	int height;
	char *path;
} Image;

Image numbers[10];
Image start_img;
Image reset_img;
Image colon;

typedef struct {
    SDL_Rect rectangle;
    SDL_Color normal_color;
    SDL_Color hover_color;
    SDL_Color pressed_color;
    int is_hovered;
    int is_pressed;
	Image image;
} Button;

enum TimerType {
	FOCUS,
	BREAK
};

enum TimerState {
	COMPLETE,
	NOT_COMPLETE
};

typedef struct {
	SDL_Rect rectangle;
	SDL_Color color;
	Button button;
	Image *digits;
	bool started;
	int minutes;
	int seconds;
	Uint32 last_tick;
	enum TimerType type;
	enum TimerState state;
} Timer;

Timer timer;

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
	image.path = malloc(strlen(filepath) + 1);

	if(image.path == NULL) {}

	strcpy(image.path, filepath);

	SDL_FreeSurface(surface);

	return image;
}

void renderImage(SDL_Renderer *renderer, const Image *image, const SDL_Rect *destination) {
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
	free(image->path);
    image->path = NULL;
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
void renderButton(SDL_Renderer *renderer, const Button *button) {
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
	renderImage(renderer, &button->image, &button->rectangle);
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
			if(timer->type == FOCUS) {
				timer->type = BREAK;
				timer->minutes = BREAK_START_MIN;
				timer->seconds = BREAK_START_SEC;
			} else {
				timer->type = FOCUS;
				timer->minutes = FOCUS_START_MIN;
				timer->seconds = FOCUS_START_SEC;
			}
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

int appSetup() {
	window = SDL_CreateWindow(
							"Pomo Dhillon",
							SDL_WINDOWPOS_CENTERED,
							SDL_WINDOWPOS_CENTERED,
							WINDOW_WIDTH,
							WINDOW_HEIGHT,
							SDL_WINDOW_SHOWN
							);

	if(window == NULL) {
		fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 0;
	}

	SDL_RaiseWindow(window);

	renderer = SDL_CreateRenderer(
								window,
								-1,
								SDL_RENDERER_ACCELERATED
								);

	if(renderer == NULL) {
		fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 0;
	}

	sound = Mix_LoadMUS("sound/alarm.mp3");
	if(sound == NULL) {
		fprintf(stderr, "Failed to load the sound: %s\n", Mix_GetError());
		Mix_CloseAudio();
		SDL_Quit();
		return 0;
	}

	return 1;
}

int loadImages() {
	for(int i=0; i<10; i++) {
		char path[32];

		snprintf(path, sizeof(path), "image/%d.png", i);
		numbers[i] = loadImage(renderer, path);

		if(numbers[i].texture == NULL) {
			fprintf(stderr, "Failed to load image: %s [%s]\n", numbers[i].path, IMG_GetError()); 
			return 0;
		}
	}
	
	start_img = loadImage(renderer, "image/start.png");
	reset_img = loadImage(renderer, "image/reset.png");
	colon     = loadImage(renderer, "image/colon.png");

	if(start_img.texture == NULL ||
	   reset_img.texture == NULL ||
	   colon.texture     == NULL) {
		fprintf(stderr, "Failed to load button image: %s\n", IMG_GetError()); 
		return 0;
	}

	return 1;
}

int observeEvents(SDL_Event *event, Timer *timer) {
	while(SDL_PollEvent(event)) {
		if(event->type == SDL_QUIT) {
			return 0;
		} else if(event->type == SDL_MOUSEMOTION) {
			int mouse_x = event->motion.x;
			int mouse_y = event->motion.y;

			timer->button.is_hovered = SDL_PointInRect(
														&(SDL_Point) {mouse_x, mouse_y},
														&timer->button.rectangle);

			if(timer->button.is_hovered) {
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND));
			} else {
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
			}
		} else if(event->type == SDL_MOUSEBUTTONDOWN) {
			if(event->button.button == SDL_BUTTON_LEFT) {
				int mouse_x = event->button.x;
				int mouse_y = event->button.y;

				timer->button.is_pressed = SDL_PointInRect(&(SDL_Point) {mouse_x, mouse_y},
															&timer->button.rectangle);
			}
		} else if(event->type == SDL_MOUSEBUTTONUP) {
			if(event->button.button == SDL_BUTTON_LEFT) {
				int mouse_x = event->button.x;
				int mouse_y = event->button.y;
				timer->button.is_pressed = 0;

				if(SDL_PointInRect(&(SDL_Point) {mouse_x, mouse_y}, &timer->button.rectangle)) {

					if(timer->button.image.texture == reset_img.texture) {
						timer->button.image = start_img;
						timer->started = false;
						Mix_HaltMusic();
					} else {
						timer->button.image = reset_img;
						timer->started = true;
					}

					timer->last_tick = SDL_GetTicks();
				}
			}
		}
	}

	return 1;
}

void runApp() {
	int running = 1;
	SDL_Event event;

	while(running) {
		running = observeEvents(&event, &timer);
		if(!running) {
			break;
		}
		updateTimer(&timer, renderer, &colon);

		SDL_SetRenderDrawColor(renderer, 163, 79, 76, 255);
		SDL_RenderClear(renderer);

		renderButton(renderer, &timer.button);
		renderTime(renderer, &timer, &colon);
		SDL_RenderPresent(renderer);

		SDL_Delay(16);
	}
}

void configureTimer() {
	int clockWidth = DIGIT_WIDTH * 5;
    int clockHeight = DIGIT_HEIGHT;

    int x = (WINDOW_WIDTH - clockWidth) / 2;
    int y = (WINDOW_HEIGHT - clockHeight) / 2.5;

    timer = (Timer) {
        .rectangle = { x, y, clockWidth, clockHeight },
        .color = { 0, 0, 0, 0 },
        .button = {
			.rectangle = {
				x + (clockWidth - 200) / 2,
				y + DIGIT_HEIGHT,
				200,
				70
			},
			.normal_color = { 255, 255, 255, 255 },
			.hover_color = { 255, 255, 255, 255 },
			.pressed_color = { 255, 255, 120, 255 },
			.is_hovered = 0,
			.is_pressed = 0,
			.image = start_img
		},
        .digits = numbers,
        .started = false,
        .last_tick = SDL_GetTicks(),
		.type = FOCUS
    };

	if(timer.type == FOCUS) {
		timer.minutes = FOCUS_START_MIN;
		timer.seconds = FOCUS_START_SEC;
	} else {
		timer.minutes = BREAK_START_MIN;
		timer.seconds = BREAK_START_SEC;
	}

	printf("timer type: %d\n", timer.type);
}

void cleanupApp() {
	for(int i=0; i<10; i++) {
		destroyImage(&numbers[i]);
	}

	destroyImage(&colon);
	destroyImage(&start_img);
	destroyImage(&reset_img);

	if(sound != NULL) {
		Mix_FreeMusic(sound);
		sound = NULL;
	}

	Mix_CloseAudio();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	IMG_Quit();
	SDL_Quit();
}

int main() {

	if(!validateInit() ||
	   !appSetup()     ||
	   !loadImages()) {
		cleanupApp();
		return 0;
	}

	configureTimer();
	runApp();
	cleanupApp();

	return 1;
}
