// menu.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include<unistd.h> 
#include <sys/wait.h>
#include <sys/types.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "libvecdisp.h"

#define APPS_DIR "/opt/vecdisp/apps"

#define STARTUP_MS 5000

typedef enum {
	PROGRAM_STATE_STARTUP = 0,
	PROGRAM_STATE_MENU,
	PROGRAM_STATE_EXEC,
	PROGRAM_STATE_SHUTDOWN
} PROGRAM_STATE_T;

// function headers
void logic_handle_input( SDL_Event * event,
	SDL_GameController * controller_p0,
	SDL_GameController * controller_p1,
	PROGRAM_STATE_T * PROGRAM_STATE,
	unsigned int * element_sel,
	unsigned int ls_nelements
);
void draw_menu(uint32_t ticks_lag, char ** ls_elements, unsigned int ls_nelements, unsigned int element_sel);
void draw_startup(uint32_t ticks_lag);


int main(int argc, char * argv[]) {
	VECDISP_T ret;
	PROGRAM_STATE_T PROGRAM_STATE = PROGRAM_STATE_STARTUP;
	uint32_t ticks_current = 0, ticks_previous = 0, ticks_elapsed = 0, ticks_lag = 0;
	srand((unsigned) ticks_previous);

	ret = vecdisp_init();
	assert(ret == VECDISP_SUCCESS);
	ret = vecdisp_out_init();
	assert(ret == VECDISP_SUCCESS);

	SDL_GameController * controller_p0 = NULL, * controller_p1 = NULL;
	int controller_cnt = 0;

	SDL_Init( SDL_INIT_GAMECONTROLLER );
	SDL_GameControllerEventState(SDL_ENABLE);

	/* ###########
	### piping ###
	############*/
	FILE *ls_pipe;
	unsigned int ls_nbytes = 100;
	char * ls_stream;
	char ls_cmd[120], bin_cmd[100], apps_dir[90];
	char ** ls_elements = NULL, ** ls_elements_realloc = NULL;
	unsigned int ls_nelements = 0;

	sprintf(apps_dir, APPS_DIR);
	sprintf(ls_cmd, "ls %s/ | grep .bin", apps_dir);
	ls_pipe = popen(ls_cmd, "r");

	if ( ls_pipe == NULL ) {
		fprintf (stderr, "pipe opening 'ls' failed.\n");
		return EXIT_FAILURE;
	}

	ls_stream = (char *) malloc(ls_nbytes + 1);
	getdelim (&ls_stream, &ls_nbytes, 0x04, ls_pipe); 	// 0x04 is EOT character
	
	if (pclose(ls_pipe) != 0) {
		fprintf (stderr, "running 'ls' returned != 0. Error detected\n");
	}

	ls_elements = (char**) realloc( ls_elements, 1 * sizeof( char * ) );
	ls_elements[0] = strtok(ls_stream, "\n" );

	int i = 0;
	while(ls_elements[i] != NULL) {
		i++;
		ls_elements_realloc = (char **) realloc( ls_elements, (i + 1) * sizeof( char * ) );
		if( ls_elements_realloc == NULL ) {
			fprintf(stderr, "realloc() ls_elements failed");
			break;
		}
		ls_elements = ls_elements_realloc;
		ls_elements[i] = strtok(NULL, "\n");
	}
	ls_nelements = i;


	/* ##############
	### main loop ###
	############## */
	unsigned int element_sel = 0;
	SDL_Event event;
	bool quit = false;
	while( quit == false ) {
		ticks_current = SDL_GetTicks();
		ticks_elapsed = ticks_current - ticks_previous;
		ticks_lag += ticks_elapsed;
		ticks_previous = ticks_current;

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
				logic_handle_input( &event, controller_p0, controller_p1, &PROGRAM_STATE, &element_sel, ls_nelements);
				break;
			case SDL_CONTROLLERBUTTONUP:
				logic_handle_input( &event, controller_p0, controller_p1, &PROGRAM_STATE, &element_sel, ls_nelements);
				break;
			}
		}

		switch(PROGRAM_STATE) {
		case PROGRAM_STATE_STARTUP:
			draw_startup(ticks_lag);
			if(ticks_lag >= STARTUP_MS) {
				ticks_lag = 0;
				PROGRAM_STATE = PROGRAM_STATE_MENU;
			}
			break;
		case PROGRAM_STATE_MENU:
			draw_menu(ticks_lag, ls_elements, ls_nelements, element_sel);
			ticks_lag = 0;
			break;
		case PROGRAM_STATE_EXEC:
			// shutting down contexts
			SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
			controller_p0 = NULL;
			controller_p1 = NULL;
			vecdisp_draw_point(0,0,DRAW_BRTNS_NONE);
			vecdisp_draw_update();
			ret = vecdisp_out_end();
			assert(ret == VECDISP_SUCCESS);
			ret = vecdisp_end();
			assert(ret == VECDISP_SUCCESS);
			
			sprintf(bin_cmd, "%s/%s", apps_dir, ls_elements[element_sel]);
			pid_t pid = fork();
			switch ( pid ) {
			case -1:
				perror("fork()");
				break;
			case 0:
				execl(bin_cmd, ls_elements[element_sel], NULL);
				printf("this should not be reachable...\n");
				break;      
			default:
				if (waitpid (pid, NULL, 0) != pid) {
					perror("waitpid()");
					break;
				}
			}
			
			// starting contexts again
			SDL_Init( SDL_INIT_GAMECONTROLLER );
			SDL_GameControllerEventState(SDL_ENABLE);
			ret = vecdisp_init();
			assert(ret == VECDISP_SUCCESS);
			ret = vecdisp_out_init();
			assert(ret == VECDISP_SUCCESS);

			PROGRAM_STATE = PROGRAM_STATE_MENU;
			break; 
		case PROGRAM_STATE_SHUTDOWN:
			free(ls_elements);
			SDL_Quit();
			vecdisp_out_end();
			vecdisp_end();
			system("poweroff");
			return EXIT_SUCCESS;
		}

		//vecdisp_dbg_showfps(); 
		vecdisp_draw_update();
	}

	// cleaning up
	free(ls_elements);
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
	vecdisp_out_end();
	vecdisp_end();
	return EXIT_SUCCESS;
}

void logic_handle_input( SDL_Event * event,
	SDL_GameController * controller_p0,
	SDL_GameController * controller_p1,
	PROGRAM_STATE_T * PROGRAM_STATE,
	unsigned int * element_sel,
	unsigned int ls_nelements
	) {
	switch (event->cbutton.button) {
	case SDL_CONTROLLER_BUTTON_BACK:
		if(event->cbutton.state == 1) {
			SDL_Event user_quit_event;
			user_quit_event.type = SDL_QUIT;
			SDL_PushEvent( &user_quit_event );
		}
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		if(event->cbutton.state == 1) {
			if( *element_sel > 0)
			(*element_sel)--;
		}
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		if(event->cbutton.state == 1) {
			if( *element_sel < ls_nelements -1)
			(*element_sel)++;
		}
		break;
	case SDL_CONTROLLER_BUTTON_B:	// On the SNES Controller labeled Button A!
		if(event->cbutton.state == 1) {
			*PROGRAM_STATE = PROGRAM_STATE_EXEC;
		}
		break;
	}
}

void draw_startup(uint32_t ticks_lag) {
	double startup_brtns = lround(0x0FFF * exp( (-1) * 0.7* (1 - ((double) ticks_lag) / STARTUP_MS )));
	vecdisp_shape_t * shape_svg = vecdisp_shape_import_svg("/opt/vecdisp/assets/vecdisp_logo.svg");
	if(shape_svg == NULL) {
		return;
	}
	uint16_t x0 = 0;
	uint16_t y0 = 200;
	uint16_t x1 = DRAW_RES - 1;
	uint16_t y1 = DRAW_RES - 201;
	vecdisp_draw_shape( shape_svg, x0, y0, x1, y1, startup_brtns );
}

void draw_menu(uint32_t ticks_lag, char ** ls_elements, unsigned int ls_nelements, unsigned int element_sel) {
	static int i = 0;
	i += 2;
	if(i >= 256) i = 0;
	int brightness = DRAW_BRTNS_BRIGHT;
	char header[100] = "", element[100] = "";

	sprintf(header, "Binaries:");
	uint16_t x0 = DRAW_CENTER - ( 50 * strlen(header) ) / 2;
	uint16_t y0 = DRAW_RES - 101;
	uint16_t x1 = DRAW_CENTER + ( 50 * strlen(header) ) / 2;
	uint16_t y1 = DRAW_RES - 21;
	vecdisp_draw_string(x0, y0 , x1, y1 , brightness, header , 15);
	
	
	x0 = 10;
	y0 = DRAW_RES - 201;
	y1 = DRAW_RES - 151;
	for( int j = 0; j < ls_nelements; j++ ) {
		if( element_sel == j ) {
			brightness = lround( 0x0FFF * (0.9 + 0.1 * lut_sin256[i]));
			sprintf(element, "./%s", ls_elements[j]);
		} else {
			brightness = DRAW_BRTNS_DARK;
			sprintf(element, "  ./%s", ls_elements[j]);
		}
		x1 = 10 + 30 * strlen(element);
		vecdisp_draw_string(x0, y0, x1, y1, brightness, element , 10);
		y0 -= 50;
		y1 -= 50;
	}
	
}
