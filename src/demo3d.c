// demo3d.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <gsl/gsl_math.h>

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

	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_GameControllerEventState(SDL_ENABLE);

	vecdisp_shape_t * shape_cube3d = vecdisp_shape_create(VECDISP_SHAPE_LINES, NULL, 0 );
	
	// format: {x, y, z}, x = horizontal, y = vertical, z = depth
	double cube3d[][3] = { 
		{ -1, -1, -1 },
		{ 1, -1, -1 },
		{ 1, -1, -1 },
		{ 1, 1, -1 },
		{ 1, 1, -1 },
		{ -1, 1, -1 },
		{ -1, 1, -1 },
		{ -1, -1, -1 },
		{ -1, -1, -1 },
		{ -1, -1, 1 },
		{ -1, 1, -1 },
		{ -1, 1, 1 },
		{ 1, 1, -1 },
		{ 1, 1, 1 },
		{ 1, -1, -1 },
		{ 1, -1, 1 },
		{ -1, -1, 1 },
		{ 1, -1, 1 },
		{ 1, -1, 1 },
		{ 1, 1, 1 },
		{ 1, 1, 1 },
		{ -1, 1, 1 },
		{ -1, 1, 1 },
		{ -1, -1, 1 },
	 };
	for(int i = 0; i < 24; i++) {
		uint16_t data[2] = {0,0};
		vecdisp_shape_data_add(shape_cube3d, &data, 1 );
		
	}
	printf("%i\n", shape_cube3d->data_len);
	
	double rotx = 0, roty = 0, rotz = 0;
	double sqrt_3 = sqrt(3);
	double sqrt_6 = sqrt(6);
	
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

		
		rotx += 0.00002;
		roty += 0.00001;
		rotz += 0.00001;
		if( rotx >= M_PI ) rotx -= M_PI;
		if( roty >= M_PI ) roty -= M_PI;
		if( rotz >= M_PI ) rotz -= M_PI;

		
		for(int i = 0; i < 24; i++) {
			double c[3];
			cube3d[i][0] = cos(rotx) * cube3d[i][0] - sin(rotx) * cube3d[i][1]; // Rotation around X Axis
			cube3d[i][1] = sin(rotx) * cube3d[i][0] + cos(rotx) * cube3d[i][1];
			cube3d[i][2] = 1 * cube3d[i][2];

			cube3d[i][0] = cos(roty) * cube3d[i][0] - sin(roty) * cube3d[i][2]; // Rotation around Y Axis
			cube3d[i][1] = 1 * cube3d[i][1];
			cube3d[i][2] = cos(roty) * cube3d[i][2] + sin(roty) * cube3d[i][0];

			cube3d[i][0] = 1 * cube3d[i][0]; // Rotation around Z Axis
			cube3d[i][1] = cos(rotz) * cube3d[i][1] - sin(rotz) * cube3d[i][2];
			cube3d[i][2] = cos(rotz) * cube3d[i][2] + sin(rotz) * cube3d[i][1];

			c[0] = ( sqrt(3) * cube3d[i][0] + (-1) * sqrt_3 * cube3d[i][2] ) / sqrt_6;
			c[1] = ( 1 * cube3d[i][0] + 2 * cube3d[i][1] + 1 * cube3d[i][2] ) / sqrt_6;
			
			shape_cube3d->data[i][0] =  (DRAW_RES / 4) * c[0] + (DRAW_RES / 2);
			shape_cube3d->data[i][1] = (DRAW_RES / 4) * c[1] + (DRAW_RES / 2);
		}
		
		vecdisp_draw_shape( shape_cube3d, 0, 0, DRAW_RES - 1, DRAW_RES - 1, DRAW_BRTNS_BRIGHT);
		vecdisp_dbg_showfps();
		vecdisp_draw_update();
	}

	// cleaning up
	vecdisp_shape_destroy(shape_cube3d);
	SDL_QuitSubSystem( SDL_INIT_EVERYTHING );
	vecdisp_out_end();
	vecdisp_end();
	return 0;
}
