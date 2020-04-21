// clock.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "libvecdisp.h"

#define CLOCK_RAD ( (DRAW_CENTER ) - ( 0.1 * DRAW_CENTER ) )
#define SECHAND_LEN (double)0.9
#define MINHAND_LEN (double)0.7
#define HOURHAND_LEN (double)0.3
#define HOURMARKERS_LEN (double)0.95
#define MINMARKERS_LEN (double)0.99
#define DATEDISP_X0 lround(DRAW_CENTER - 0.4 * DRAW_CENTER)
#define DATEDISP_Y0 lround(DRAW_CENTER - 0.2 * DRAW_CENTER)
#define DATEDISP_X1 lround(DRAW_CENTER + 0.4 * DRAW_CENTER)
#define DATEDISP_Y1 lround(DRAW_CENTER - 0.1 * DRAW_CENTER)


int main(int argc, char * argv[]) {
	VECDISP_T ret;
	ret = vecdisp_init();
	assert(ret == VECDISP_SUCCESS);
	ret = vecdisp_out_init();
	assert(ret == VECDISP_SUCCESS);
	time_t rawtime;
	struct tm * time_now;
	int sec_now, min_now, hour_now, mday_now, month_now, year_now;
	SDL_Event event;
	char datedisp[20] = "";

	SDL_GameController * controller_p0 = NULL, * controller_p1 = NULL;
	int controller_cnt = 0;

	SDL_Init( SDL_INIT_GAMECONTROLLER );
	SDL_GameControllerEventState(SDL_ENABLE);
	
	int i = 0;
	double phi[60] = { 0 };
    long x_circlesection[60] = { 0 }, y_circlesection[60] = { 0 };
	long x_sec[60] = { 0 }, y_sec[60] = { 0 };
    long x_min[60] = { 0 }, y_min[60] = { 0 };
    long x_hour[60] = { 0 }, y_hour[60] = { 0 };
    long x_hourmarkers[60] = { 0 }, y_hourmarkers[60] = { 0 };
	long x_minmarkers[60] = { 0 }, y_minmarkers[60] = { 0 };
	for(i = 0; i < 60; i++) {
		phi[i] = ( i * 2 * M_PI )/ 60.0;
        x_circlesection[i] = lround(DRAW_CENTER + sin(phi[i]) * CLOCK_RAD);
		y_circlesection[i] = lround(DRAW_CENTER + cos(phi[i]) * CLOCK_RAD);
		x_sec[i] = lround(DRAW_CENTER + sin(phi[i]) * CLOCK_RAD * SECHAND_LEN);
		y_sec[i] = lround(DRAW_CENTER + cos(phi[i]) * CLOCK_RAD * SECHAND_LEN);
		x_min[i] = lround(DRAW_CENTER + sin(phi[i]) * CLOCK_RAD * MINHAND_LEN);
		y_min[i] = lround(DRAW_CENTER + cos(phi[i]) * CLOCK_RAD * MINHAND_LEN);
		x_hour[i] = lround(DRAW_CENTER + sin(phi[i]) * CLOCK_RAD * HOURHAND_LEN);
		y_hour[i] = lround(DRAW_CENTER + cos(phi[i]) * CLOCK_RAD * HOURHAND_LEN);
		x_minmarkers[i] = lround(DRAW_CENTER + sin(phi[i]) * CLOCK_RAD * MINMARKERS_LEN);
		y_minmarkers[i] = lround(DRAW_CENTER + cos(phi[i]) * CLOCK_RAD * MINMARKERS_LEN);
        x_hourmarkers[i] = lround(DRAW_CENTER + sin(phi[i]) * CLOCK_RAD * HOURMARKERS_LEN);
		y_hourmarkers[i] = lround(DRAW_CENTER + cos(phi[i]) * CLOCK_RAD * HOURMARKERS_LEN);
	}
	
	i = 0;
	bool quit = false;
	while(quit == false) {
		time(&rawtime);
		time_now = localtime(&rawtime);
		sec_now = time_now->tm_sec;
		min_now = time_now->tm_min;
		hour_now = time_now->tm_hour;
		if(hour_now >= 12) hour_now -= 12;
		mday_now = time_now->tm_mday;
		month_now = time_now->tm_mon + 1;
		year_now = time_now->tm_year + 1900;
		sprintf(datedisp, "%04u-%02u-%02u", year_now, month_now, mday_now);

		//vecdisp_draw_ellipse(DRAW_CENTER, DRAW_CENTER, CLOCK_RAD, CLOCK_RAD, DRAW_BRTNS_BRIGHT, 8); // Achtung langsam!
		vecdisp_draw_string(DATEDISP_X0, DATEDISP_Y0, DATEDISP_X1, DATEDISP_Y1, DRAW_BRTNS_DARK, datedisp , 10);
        
        for(int j = 0; j < 60; j += 5) {
            vecdisp_draw_line(x_hourmarkers[j], y_hourmarkers[j], x_circlesection[j], y_circlesection[j], DRAW_BRTNS_BRIGHT);
        }
		for(int j = 0; j < 60; j++) {
			vecdisp_draw_line(x_minmarkers[j], y_minmarkers[j], x_circlesection[j], y_circlesection[j], DRAW_BRTNS_BRIGHT);
		}
		vecdisp_draw_line(DRAW_CENTER, DRAW_CENTER, x_sec[sec_now], y_sec[sec_now], DRAW_BRTNS_BRIGHT);
		vecdisp_draw_line(DRAW_CENTER, DRAW_CENTER, x_min[min_now], y_min[min_now], DRAW_BRTNS_BRIGHT);
		vecdisp_draw_line(DRAW_CENTER, DRAW_CENTER, x_hour[ (hour_now * 5) + (min_now / 12) ], y_hour[ (hour_now * 5) + (min_now / 12) ], DRAW_BRTNS_BRIGHT);
		
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
				if ( event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_p0) ) ) {
					SDL_GameControllerClose(controller_p0);
					printf("freeing controller_p0\n");
					controller_p0 = NULL;
					controller_cnt--;
				}
				else if (event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_p1) ) ) {
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
				}
				break;
			}
		}
		vecdisp_dbg_showfps();
		vecdisp_draw_update();
	}

	// Cleaning up
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
	controller_p0 = NULL;
	controller_p1 = NULL;
	ret = vecdisp_out_end();
	assert(ret == VECDISP_SUCCESS);
	ret = vecdisp_end();
	assert(ret == VECDISP_SUCCESS);
	return 0;
}
