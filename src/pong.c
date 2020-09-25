// pong.c

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

#define MS_PER_UPDATE 8		// ses the time resolution of the game logic
#define PLATFORM_HEIGHT 200
#define PLATFORM_SPEED 4 		// Platform speed per logic update (px)
#define BALL_SIZE 10
#define BALL_SPEED 4			// Ball speed per logic update (px)

#define GOALS_WIN 5

/* box_t: axis aligned box type */
typedef struct {
	uint16_t min_x;
	uint16_t min_y;
	uint16_t max_x;
	uint16_t max_y;
} box_t;

/* aabb_t: axis aligned bounding box with position, velocity, acceleration */
typedef struct {
	box_t pos;
	int32_t velo_x;
	int32_t velo_y;
	int32_t accel_x;
	int32_t accel_y;
} aabb_t;

typedef enum {
	GAME_STATE_PLAYING = 0,
	GAME_STATE_GOAL,
	GAME_STATE_GAMEOVER
} GAME_STATE_T;

typedef struct {
	bool move_up;
	bool move_down;
	bool shoot;
} input_params_t;

// Function declarations
void logic_handle_input(
	SDL_Event * event,
	input_params_t * input_p0,
	input_params_t * input_p1,
	SDL_GameController * controller_p0,
	SDL_GameController * controller_p1,
	bool KI_enabled
	);
void logic_update( uint32_t timestep_ms, input_params_t * input_p0, input_params_t * input_p1 );
void logic_resetgame(void);
bool event_belongs_to_controller(SDL_Event * event, SDL_GameController * controller);
void draw_playing(uint32_t ticks_lag);
void draw_goal(uint32_t ticks_lag);
void draw_gameover(uint32_t ticks_lag);

/* vecdisp_colldet_aabb_aabb(): collision detection between to axis aligned bounding boxes. If htey overlap, a collision happened.
parameter: * a = pointer to first aabb_t, * b = pointer to second aabb_t
return value: true = collision happened, false = no collision */
bool vecdisp_colldet_aabb_aabb( aabb_t * a, aabb_t * b );

/* vecdisp_colldet_aabb_borderbox(): collision detection between axis aligned bounding box and border box. If aabb_t is going outside the border box, collision happened.
parameter: *a = point to aabb_t, * box = pointer to the border box
return value: true = collision happened, false = no collision */

bool vecdisp_colldet_aabb_borderbox( aabb_t * a, aabb_t * box );

// Static / Global Variables:
static SDL_Event event;
static bool quit = false;

static aabb_t platform_p0 = { { 0,0,0,0 }, 0,0,0,0 };
static aabb_t platform_p1 = {  { 0,0,0,0 }, 0,0,0,0 };
static aabb_t ball =  { {0,0,0,0}, 0,0,0,0 };
static aabb_t gamefield = { { 0, 0, DRAW_RES - 1, DRAW_RES - 1}, 0,0,0,0 };
static aabb_t goal_p0 = { { 0, 0, 1, DRAW_RES - 1}, 0,0,0,0 };
static aabb_t goal_p1 = { { DRAW_RES - 2, 0, DRAW_RES - 1, DRAW_RES - 1}, 0,0,0,0 };
static uint8_t goals_cnt_p0 = 0, goals_cnt_p1 = 0;
static GAME_STATE_T GAME_STATE = GAME_STATE_PLAYING;

int main(int argc, char * argv[]) {
	VECDISP_T ret = 0;
	uint32_t ticks_current = 0, ticks_previous = 0, ticks_elapsed = 0, ticks_lag = 0;
	srand((unsigned) ticks_previous);

	ret = vecdisp_init();
	assert(ret == VECDISP_SUCCESS);
	ret = vecdisp_out_init();
	assert(ret == VECDISP_SUCCESS);

	SDL_Init( SDL_INIT_GAMECONTROLLER );
	SDL_GameControllerEventState(SDL_ENABLE);
	
	SDL_GameController * controller_p0 = NULL, * controller_p1 = NULL;
	int controller_cnt = 0;
	input_params_t input_p0 = {false, false, false}, input_p1 = {false, false, false};
	bool AI_enabled = true;
	
	logic_resetgame();

	/* ### GAME LOOP ### */
	while(quit == false) {
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
				printf("SDL controller added.\n");
				if(controller_p0 == NULL) {
					controller_p0 = SDL_GameControllerOpen(event.cdevice.which);
					printf("adding controller p0\n");
					controller_cnt++;
				}
				else if(controller_p1 == NULL) {
					controller_p1 = SDL_GameControllerOpen(event.cdevice.which);
					printf("adding controller p1\n");
					controller_cnt++;
				}
				else {
					printf("Only two controllers allowed.\n");
				}
				break;
			case SDL_CONTROLLERDEVICEREMOVED:
				printf("SDL controller removed.\n");
				if (event_belongs_to_controller(&event, controller_p0) == true ) {
					SDL_GameControllerClose(controller_p0);
					printf("freeing controller_p0\n");
					controller_p0 = NULL;
					controller_cnt--;
				}
				else if (event_belongs_to_controller(&event, controller_p1) == true ) {
					SDL_GameControllerClose(controller_p1);
					printf("freeing controller_p1\n");
					controller_p1 = NULL;
					controller_cnt--;
				}
				break;
			case SDL_CONTROLLERBUTTONDOWN:
				if(controller_cnt < 2) {
					AI_enabled = true;
				}
				if(controller_cnt > 0) {
					logic_handle_input( &event, &input_p0 ,&input_p1, controller_p0, controller_p1, AI_enabled);
				}
				break;
			case SDL_CONTROLLERBUTTONUP:
				if(controller_cnt > 0) {
					logic_handle_input( &event, &input_p0 ,&input_p1, controller_p0, controller_p1, AI_enabled);
				}
				break;
			}
		}

		switch(GAME_STATE) {
		case GAME_STATE_PLAYING:
			while ( ticks_lag >= MS_PER_UPDATE ) {
				logic_update(MS_PER_UPDATE, &input_p0, &input_p1);
				ticks_lag -= MS_PER_UPDATE;
			}
			draw_playing(ticks_lag);
			break;
		case GAME_STATE_GOAL:
			draw_goal(ticks_lag);
			if( ticks_lag >= 5000 ) {
				ticks_lag = 0;
				GAME_STATE = GAME_STATE_PLAYING;
			}
			break;
		case GAME_STATE_GAMEOVER:
			draw_gameover(ticks_lag);
			if( ticks_lag >= 5000 ) {
				ticks_lag = 0;
				GAME_STATE = GAME_STATE_PLAYING;
				goals_cnt_p0 = 0;
				goals_cnt_p1 = 0;
			}
			break; 
		}
        
		vecdisp_dbg_showfps();
		vecdisp_draw_update();
	}

	// cleaning up
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
	controller_p0 = NULL;
	controller_p1 = NULL;
	ret = vecdisp_out_end();
	assert(ret == VECDISP_SUCCESS);
	ret = vecdisp_end();
	assert(ret == VECDISP_SUCCESS);
	return EXIT_SUCCESS;
}

void logic_handle_input(
	SDL_Event * event,
	input_params_t * input_p0,
	input_params_t * input_p1,
	SDL_GameController * controller_p0,
	SDL_GameController * controller_p1,
	bool KI_enabled
	) {
	switch (event->cbutton.button) {
		case SDL_CONTROLLER_BUTTON_BACK:
			if(event->cbutton.state == 1) {
				SDL_Event user_quit_event;
				user_quit_event.type = SDL_QUIT;
				SDL_PushEvent( &user_quit_event );
			}
			break;
	}
	
	if(event_belongs_to_controller(event, controller_p0) == true ) {
		input_p0->move_up = false;
		input_p0->move_down = false;
		input_p0->shoot = false;
		switch (event->cbutton.button) {
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			if(event->cbutton.state == 1) {
				input_p0->move_up = true;
			}
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			if(event->cbutton.state == 1) {
				input_p0->move_down = true;
			}
			break;
		case SDL_CONTROLLER_BUTTON_B:	// On the SNES Controller labeled Button A!
			if(event->cbutton.state == 1) {
				input_p0->shoot = true;
			}
			break;
		}
	}
	if(event_belongs_to_controller(event, controller_p1) == true ) {
		input_p1->move_up = false;
		input_p1->move_down = false;
		input_p1->shoot = false;
		switch (event->cbutton.button) {
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			if(event->cbutton.state == 1) {
				input_p1->move_up = true;
			}
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			if(event->cbutton.state == 1) {
				input_p1->move_down = true;
			}
			break;
		case SDL_CONTROLLER_BUTTON_B:  // On the SNES Controller labeled Button A!
			if(event->cbutton.state == 1) {
				input_p1->shoot = true;
			}
			break;
		}
	}
}

void logic_update( uint32_t timestep_ms, input_params_t * input_p0, input_params_t * input_p1 ) {

	// FIRST: Update Objects to Input
	if (input_p0->move_up == true ) {
		platform_p0.velo_y = PLATFORM_SPEED;
	} else if ( input_p0->move_down == true ) {
		platform_p0.velo_y = (-1) * PLATFORM_SPEED;
	} else {
		platform_p0.velo_y = 0;
	}
	if (input_p1->move_up == true ) {
		platform_p1.velo_y = PLATFORM_SPEED;
	} else if ( input_p1->move_down == true ) {
		platform_p1.velo_y = (-1) * PLATFORM_SPEED;
	} else {
		platform_p1.velo_y = 0;
	}

	// Collision Detection
	if ( vecdisp_colldet_aabb_borderbox( &platform_p0, &gamefield ) == true ) {
		platform_p0.velo_y = 0;
	}
	if ( vecdisp_colldet_aabb_borderbox( &platform_p1, &gamefield ) == true ) {
		platform_p1.velo_y = 0;
	}
	if ( vecdisp_colldet_aabb_aabb( &ball, &platform_p0 ) == true ) {
		ball.velo_x *= (-1);
		uint16_t dy = platform_p0.pos.max_y - ball.pos.min_y;
		uint16_t platform_height = platform_p0.pos.max_y - platform_p0.pos.min_y;
		if ( dy < (1 * platform_height) / 6 ) {
			ball.velo_y += (2);
		} else if ( (1 * platform_height) / 6 <= dy && dy <= (2 * platform_height) / 6 ) {
			ball.velo_y += (1);
		} else if ( (4 * platform_height) / 6 <= dy && dy <= (5 * platform_height) / 6 ) {
			ball.velo_y += (-1);
		} else if ( (5 * platform_height) / 6 <= dy ) {
			ball.velo_y += (-2);
		}
	} 
	if ( vecdisp_colldet_aabb_aabb( &ball, &platform_p1 ) == true) {
		ball.velo_x *= (-1);
		uint16_t dy = platform_p1.pos.max_y - ball.pos.min_y;
		uint16_t platform_height = platform_p1.pos.max_y - platform_p1.pos.min_y;
		if ( dy < (1 * platform_height) / 6 ) {
			ball.velo_y += (2);
		} else if ( (1 * platform_height) / 6 <= dy && dy <= (2 * platform_height) / 6 ) {
			ball.velo_y += (1);
		} else if ( (4 * platform_height) / 6 <= dy && dy <= (5 * platform_height) / 6 ) {
			ball.velo_y += (-1);
		} else if ( (5 * platform_height) / 6 <= dy ) {
			ball.velo_y += (-2);
		}
	}
	
	if ( vecdisp_colldet_aabb_aabb( &ball, &goal_p0) == true ) {
		goals_cnt_p1++;
		printf("goals p1: %d\n", goals_cnt_p1);
		logic_resetgame();
	}

	if ( vecdisp_colldet_aabb_aabb( &ball, &goal_p1) == true ) {
		goals_cnt_p0++;
		logic_resetgame();
		printf("goals p0: %d\n", goals_cnt_p0);
	}

	if ( vecdisp_colldet_aabb_borderbox( &ball, &gamefield ) == true ) {
		ball.velo_y *= (-1);
	}

	// LAST: Update Objects Physics for the Timestep
	platform_p0.pos.min_y += platform_p0.velo_y;
	platform_p0.pos.max_y += platform_p0.velo_y;
	platform_p1.pos.min_y += platform_p1.velo_y;
	platform_p1.pos.max_y += platform_p1.velo_y;
	ball.pos.min_x += ball.velo_x;
	ball.pos.max_x += ball.velo_x;
	ball.pos.min_y += ball.velo_y;
	ball.pos.max_y += ball.velo_y;

}

void logic_resetgame(void) {
	ball.pos.min_x = DRAW_CENTER;
	ball.pos.min_y = DRAW_CENTER;
	ball.pos.max_x = ball.pos.min_x + BALL_SIZE;
	ball.pos.max_y = ball.pos.min_y + BALL_SIZE;
	if ( goals_cnt_p0 >= GOALS_WIN || goals_cnt_p1 >= GOALS_WIN ) {
		ball.velo_x = BALL_SPEED;
		GAME_STATE = GAME_STATE_GAMEOVER;
	} else {
		ball.velo_x = BALL_SPEED + goals_cnt_p0 + goals_cnt_p1;
		GAME_STATE = GAME_STATE_GOAL;
	}
	ball.velo_y = 0;
	platform_p0.pos.min_x = 10;
	platform_p0.pos.min_y = (DRAW_CENTER - 10);
	platform_p0.pos.max_x = 10 + 15;
	platform_p0.pos.max_y = (DRAW_CENTER - 10) + PLATFORM_HEIGHT;

	platform_p1.pos.min_x = DRAW_RES - 26;
	platform_p1.pos.min_y = (DRAW_CENTER + 10) - PLATFORM_HEIGHT;
	platform_p1.pos.max_x = DRAW_RES - 11;
	platform_p1.pos.max_y = DRAW_CENTER + 10;
}

void draw_playing (uint32_t ticks_lag) {
	char text_0[60], text_1[60];
	sprintf(text_0, "P0: %d", goals_cnt_p0);
	sprintf(text_1, "P1: %d", goals_cnt_p1);

	//vecdisp_draw_rect_aa( gamefield.pos.min_x, gamefield.pos.min_y, gamefield.pos.max_x, gamefield.pos.max_y, DRAW_BRTNS_BRIGHT );
	vecdisp_draw_rect_aa( platform_p0.pos.min_x, platform_p0.pos.min_y, platform_p0.pos.max_x, platform_p0.pos.max_y, DRAW_BRTNS_BRIGHT );
	vecdisp_draw_rect_aa( platform_p1.pos.min_x, platform_p1.pos.min_y, platform_p1.pos.max_x, platform_p1.pos.max_y, DRAW_BRTNS_BRIGHT);
	vecdisp_draw_ellipse(ball.pos.min_x + ( (ball.pos.max_x - ball.pos.min_x) >> 1),
		ball.pos.min_y + ( (ball.pos.max_y - ball.pos.min_y) >> 1),
		( (ball.pos.max_x - ball.pos.min_x) >> 1),
		( (ball.pos.max_x - ball.pos.min_x) >> 1),
		DRAW_BRTNS_BRIGHT, 32);
	
	
	vecdisp_draw_string(30, DRAW_RES - 90, 230, DRAW_RES - 30, DRAW_BRTNS_DARK, text_0 , 4);
	vecdisp_draw_string(DRAW_RES - 231, DRAW_RES - 90, DRAW_RES - 31, DRAW_RES - 30, DRAW_BRTNS_DARK, text_1 , 4);
}

bool event_belongs_to_controller(SDL_Event * event, SDL_GameController * controller) {
	if ( event->cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller) ) ) {
		return true;
	}
	return false;
}

void draw_goal(uint32_t ticks_lag) {
	int counter = 5000 - ticks_lag;
	char text_0[60], text_1[60], text_2[60];
	sprintf(text_0, "PLAYER 0: %d GOALS", goals_cnt_p0);
	sprintf(text_1, "PLAYER 1: %d GOALS", goals_cnt_p1);
	sprintf(text_2, "RESTART IN: %04d", counter);
	vecdisp_draw_string(DRAW_CENTER - 400, DRAW_CENTER + 100, DRAW_CENTER + 400, DRAW_CENTER + 170, DRAW_BRTNS_BRIGHT, text_0 , 20);
	vecdisp_draw_string(DRAW_CENTER - 400, DRAW_CENTER -100, DRAW_CENTER + 400, DRAW_CENTER - 30, DRAW_BRTNS_BRIGHT, text_1 , 20);
	vecdisp_draw_string(DRAW_CENTER - 300, DRAW_CENTER -400, DRAW_CENTER + 300, DRAW_CENTER -300, DRAW_BRTNS_BRIGHT, text_2 , 20);
}

void draw_gameover(uint32_t ticks_lag) {
	int counter = 5000 - ticks_lag;
	char text_0[60], text_1[60], text_2[60];

	sprintf(text_0, "///GAME OVER///");
	if( goals_cnt_p0 >= GOALS_WIN ) {
		sprintf(text_1, "WINNER: PLAYER 0");
	} else if ( goals_cnt_p1 >= GOALS_WIN ) {
		sprintf(text_1, "WINNER: PLAYER 1");
	}
	sprintf(text_2, "RESTART IN: %04d", counter);

	vecdisp_draw_string(DRAW_CENTER - 400, DRAW_CENTER + 100, DRAW_CENTER + 400, DRAW_CENTER + 170, DRAW_BRTNS_BRIGHT, text_0 , 20);
	vecdisp_draw_string(DRAW_CENTER - 400, DRAW_CENTER -100, DRAW_CENTER + 400, DRAW_CENTER - 30, DRAW_BRTNS_BRIGHT, text_1 , 20);
	vecdisp_draw_string(DRAW_CENTER - 300, DRAW_CENTER -400, DRAW_CENTER + 300, DRAW_CENTER -300, DRAW_BRTNS_BRIGHT, text_2 , 20);
}

bool vecdisp_colldet_aabb_aabb( aabb_t * a, aabb_t * b ) {
	if ( a->pos.min_x + a->velo_x <= b->pos.max_x &&
		a->pos.max_x + a->velo_x >= b->pos.min_x &&
		a->pos.min_y + a->velo_y <= b->pos.max_y &&
		a->pos.max_y + a->velo_y >= b->pos.min_y ) {
		return true;
	}
	return false;
}

bool vecdisp_colldet_aabb_borderbox( aabb_t * a, aabb_t * box ) {
	if ( a->pos.min_x + a->velo_x <= box->pos.min_x ||
		a->pos.max_x + a->velo_x >= box->pos.max_x ||
		a->pos.min_y + a->velo_y <= box->pos.min_y ||
		a->pos.max_y + a->velo_y >= box->pos.max_y ) {
			return true;
		}
	return false;
}
