// test.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "libvecdisp.h"

int main(int argc, char * argv[]) {
	VECDISP_T ret;

	// Init
	ret = vecdisp_init();
	assert(ret == VECDISP_SUCCESS);
	ret = vecdisp_out_init();
	assert(ret == VECDISP_SUCCESS);

	SDL_GameController * controller_p0 = NULL, * controller_p1 = NULL;
	int controller_cnt = 0;

	SDL_Init( SDL_INIT_GAMECONTROLLER );
	SDL_GameControllerEventState(SDL_ENABLE);
	
	/*
	vecdisp_shape_t * shape_svg = vecdisp_shape_import_svg("/opt/vecdisp/assets/image.svg");
	if(shape_svg == NULL) {
		printf("Failed to import svg\n");
		return -1;
	}
	*/
	char fps_str[50], controller_str[50];
	int i = DRAW_CENTER, dir = 1;
	
	// main loop
	SDL_Event event;
	bool quit = false;
	while( quit == false ) {
		while( SDL_PollEvent(&event) != 0 ) {
			switch (event.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_CONTROLLERDEVICEADDED:
				if(controller_p0 == NULL) {
					controller_p0 = SDL_GameControllerOpen(event.cdevice.which);
					printf("adding Controller p0\n");
					controller_cnt++;
				}
				else if(controller_p1 == NULL) {
					controller_p1 = SDL_GameControllerOpen(event.cdevice.which);
					printf("adding Controller p1\n");
					controller_cnt++;
				}
				else {
					printf("Only two controllers allowed.\n");
				}
				break;
			case SDL_CONTROLLERDEVICEREMOVED:
				if (event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_p0) ) )  {
					SDL_GameControllerClose(controller_p0);
					printf("freeing controller_p0\n");
					controller_p0 = NULL;
					controller_cnt--;
				}
				else if (event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_p1) ) )  {
					SDL_GameControllerClose(controller_p1);
					printf("freeing controller_p1\n");
					controller_p1 = NULL;
					controller_cnt--;
				}
				break;
			case SDL_CONTROLLERBUTTONDOWN:
				switch (event.cbutton.button) {
				case SDL_CONTROLLER_BUTTON_BACK:
					if(event.cbutton.state == 1) {
						SDL_Event user_quit_event;
						user_quit_event.type = SDL_QUIT;
						SDL_PushEvent( &user_quit_event );
					}
					break;
				}
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_q: {
					SDL_Event user_quit_event;
					user_quit_event.type = SDL_QUIT;
					SDL_PushEvent( &user_quit_event );
					break;
					}
				}
				break;
			}
		}

		static uint32_t fps_cnt = 0;
		static struct timespec clk_cur, clk_prev, clk_res;
		clock_getres(CLOCK_MONOTONIC, &clk_res);
		clock_gettime(CLOCK_MONOTONIC, &clk_cur);
		fps_cnt++;
		if( clk_cur.tv_sec - clk_prev.tv_sec >= 1  ) {
			clk_prev = clk_cur;
			sprintf(fps_str, "FPS:           %02d", fps_cnt);
			fps_cnt = 0;
		}

		sprintf(controller_str, "input devices: %d", controller_cnt);

		//vecdisp_draw_shape( shape_svg, 0,0,DRAW_RES - 1, DRAW_RES - 1, DRAW_BRTNS_BRIGHT );
		vecdisp_draw_rect_aa(0,0, DRAW_RES - 1, DRAW_RES -1, DRAW_BRTNS_BRIGHT);
		vecdisp_draw_string(DRAW_CENTER - 400, DRAW_CENTER + 350, DRAW_CENTER + 0, DRAW_CENTER + 450, DRAW_BRTNS_BRIGHT, "bright  ", 10);
		vecdisp_draw_string(DRAW_CENTER + 0, DRAW_CENTER + 300, DRAW_CENTER + 400, DRAW_CENTER + 400, DRAW_BRTNS_DARK, "dark    ", 10);
		vecdisp_draw_string(DRAW_CENTER - 400, DRAW_CENTER + 200, DRAW_CENTER + 400, DRAW_CENTER + 300, DRAW_BRTNS_BRIGHT, fps_str, 10);
		vecdisp_draw_string(DRAW_CENTER - 400, DRAW_CENTER + 50, DRAW_CENTER + 400, DRAW_CENTER + 150, DRAW_BRTNS_BRIGHT, controller_str, 10);
		vecdisp_draw_line(DRAW_CENTER - 20, DRAW_CENTER, DRAW_CENTER + 20, DRAW_CENTER, DRAW_BRTNS_BRIGHT);
		vecdisp_draw_line(DRAW_CENTER, DRAW_CENTER - 20, DRAW_CENTER, DRAW_CENTER + 20, DRAW_BRTNS_BRIGHT);
		vecdisp_draw_ellipse(i + 25, DRAW_CENTER - 425, 25, 25, DRAW_BRTNS_BRIGHT, 16);

		i += dir * 5;
		if(i >= DRAW_RES - 56 || i <= 6) {
			dir *= -1;
		}

		vecdisp_dbg_showfps();
		vecdisp_draw_update();
	}

	// cleaning up
	//vecdisp_shape_destroy(shape_svg);
	SDL_QuitSubSystem( SDL_INIT_GAMECONTROLLER );
	vecdisp_out_end();
	vecdisp_end();
	return 0;
}

