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

uint16_t pointcloud[][2] = {
	{DRAW_CENTER - 100,DRAW_CENTER - 100},
	{DRAW_CENTER - 50,DRAW_CENTER - 100},
	{DRAW_CENTER,DRAW_CENTER - 100},
	{DRAW_CENTER + 50,DRAW_CENTER - 100},
	{DRAW_CENTER + 100,DRAW_CENTER - 100},
	{DRAW_CENTER - 100,DRAW_CENTER - 50},
	{DRAW_CENTER - 50,DRAW_CENTER - 50},
	{DRAW_CENTER,DRAW_CENTER - 50},
	{DRAW_CENTER + 50,DRAW_CENTER - 50},
	{DRAW_CENTER + 100,DRAW_CENTER - 50},
	{DRAW_CENTER - 100,DRAW_CENTER - 0},
	{DRAW_CENTER - 50,DRAW_CENTER - 0},
	{DRAW_CENTER,DRAW_CENTER - 0},
	{DRAW_CENTER + 50,DRAW_CENTER - 0},
	{DRAW_CENTER + 100,DRAW_CENTER - 0},
	{DRAW_CENTER - 100,DRAW_CENTER + 50},
	{DRAW_CENTER - 50,DRAW_CENTER + 50},
	{DRAW_CENTER,DRAW_CENTER + 50},
	{DRAW_CENTER + 50,DRAW_CENTER + 50},
	{DRAW_CENTER + 100,DRAW_CENTER + 50},
	{DRAW_CENTER - 100,DRAW_CENTER + 100},
	{DRAW_CENTER - 50,DRAW_CENTER + 100},
	{DRAW_CENTER,DRAW_CENTER + 100},
	{DRAW_CENTER + 50,DRAW_CENTER + 100},
	{DRAW_CENTER + 100,DRAW_CENTER + 100}
};


int main(int argc, char * argv[]) {
	VECDISP_T ret;

	// Init
	ret = vecdisp_init();
	assert(ret == VECDISP_SUCCESS);
	ret = vecdisp_out_init();
	assert(ret == VECDISP_SUCCESS);

	SDL_GameController * controller_p0 = NULL, * controller_p1 = NULL;
	int controller_cnt = 0;

	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_GameControllerEventState(SDL_ENABLE);
	
	vecdisp_shape_t * shape_points = vecdisp_shape_create(VECDISP_SHAPE_TRIANGLES, pointcloud, sizeof(pointcloud) / sizeof(pointcloud[0]) );

	
	vecdisp_shape_t * shape_svg = vecdisp_shape_import_svg("C:\\Daten\\source\\C\\vecdisp\\assets\\test.svg");
	if(shape_svg == NULL) {
		printf("Failed to import svg\n");
		return -1;
	}
	
	
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
				case SDL_CONTROLLER_BUTTON_START:
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

		vecdisp_draw_shape( shape_svg, 0,0,DRAW_RES - 1, DRAW_RES - 1, DRAW_BRTNS_BRIGHT );
		//vecdisp_draw_shape( shape_points, 0, 0, DRAW_RES - 1, DRAW_RES - 1, DRAW_BRTNS_BRIGHT);
		vecdisp_dbg_showfps();
		vecdisp_draw_update();
	}

	// cleaning up
	vecdisp_shape_destroy(shape_svg);
	vecdisp_shape_destroy(shape_points);
	SDL_QuitSubSystem( SDL_INIT_EVERYTHING );
	vecdisp_out_end();
	vecdisp_end();
	return 0;
}

