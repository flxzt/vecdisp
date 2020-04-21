// astroids.c

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

#define MS_PER_UPDATE 16		// ses the time resolution of the game logic
#define PLAYER_HITBOX_SIZE 100
#define ASTROID_INIT_SIZE 50
#define  ASTROID_RESIZE_EN 1    
#define ASTROID_RESIZE_TIME 2   // sec
#define COLLISION_EN 1
#define SHOW_HITBOX_EN 0
#define INF_SHOOTING_EN 1       // [TODO]
#define PLAYER_SPEED 4

typedef enum {
	GAME_STATE_PLAYING = 0,
    GAME_STATE_GOAL,
	GAME_STATE_GAMEOVER
} GAME_STATE_T;

typedef struct {
	bool move_left;
	bool move_right;
    bool move_up;
    bool move_down;
	bool shoot;
} input_params_t;

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

//void logic_update(uint32_t timestep_ms, input_params_t * input_p0, input_params_t * input_p1 );
void draw_playing(void);
void reset_game(void);
void update_game(input_params_t * input_p0 );
void draw_gameover(void);
void logic_handle_input(SDL_Event * event, input_params_t * input_p0, SDL_GameController * controller_p0);
bool event_belongs_to_controller(SDL_Event * event, SDL_GameController * controller);

bool vecdisp_colldet_aabb_aabb( aabb_t * a, aabb_t * b );

// Static / Global Variables:
static SDL_Event event;
static bool quit = false;

static aabb_t player = { { 0,0,0,0 }, 0,0,0,0 };
static aabb_t astroid = {  { 0,0,0,0 }, 0,0,0,0 };
static GAME_STATE_T GAME_STATE = GAME_STATE_PLAYING;

static uint16_t score_counter = 0;
static uint16_t astroid_size_update = 0;

int main(int argc, char * argv[]) {
	VECDISP_T ret = 0;
	uint32_t ticks_current = 0, ticks_previous = SDL_GetTicks(), ticks_elapsed = 0, ticks_lag = 0;
	srand((unsigned) ticks_previous);

	ret = vecdisp_init();
	assert(ret == VECDISP_SUCCESS);
	ret = vecdisp_out_init();
	assert(ret == VECDISP_SUCCESS);

    SDL_Init( SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER );
	SDL_GameControllerEventState(SDL_ENABLE);
	
	static SDL_GameController * controller_p0 = NULL;
	static int controller_cnt = 0;
	static input_params_t input_p0 = {false, false, false};

    reset_game();

    uint32_t time_start = time(0);
    uint32_t time_size_update = time_start + ASTROID_RESIZE_TIME;

    /* ### GAME LOOP ### */
	while(quit == false) {

        if(ASTROID_RESIZE_EN){
            if(time_size_update < time(0)){
                astroid_size_update = astroid_size_update  + 10; 
                time_size_update = time(0) + ASTROID_RESIZE_TIME;
            }
        }

        ticks_current = SDL_GetTicks();
		ticks_elapsed = ticks_current - ticks_previous;
		ticks_lag += ticks_elapsed;
		ticks_previous = ticks_current;

        SDL_PollEvent(&event);
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
            else {
                printf("Only one players allowed.\n");
            }
            break;

         case SDL_CONTROLLERDEVICEREMOVED:
            if (event_belongs_to_controller(&event, controller_p0) == true ) {
                SDL_GameControllerClose(controller_p0);
                printf("freeing controller_p0\n");
                controller_p0 = NULL;
                controller_cnt--;
            }
            break;
         case SDL_CONTROLLERBUTTONUP:       
            if(controller_cnt > 0) {
                printf("BUTTONUP");
                logic_handle_input( &event, &input_p0, controller_p0);
            }
            break;
        case SDL_CONTROLLERBUTTONDOWN:      
            if(controller_cnt > 0) {
                printf("BUTTONDOWN");
                logic_handle_input( &event, &input_p0, controller_p0);
            }
            break;

        }

        switch(GAME_STATE) {
		case GAME_STATE_PLAYING:
			while ( ticks_lag >= MS_PER_UPDATE ) {
				//logic_update(MS_PER_UPDATE, &input_p0, &input_p1); 
				ticks_lag -= MS_PER_UPDATE;
			}
			update_game(&input_p0);
            draw_playing();
            
			break;

        case GAME_STATE_GAMEOVER:

			draw_gameover();

             if( ticks_lag >= 5000 ) {
				ticks_lag = 0;
				GAME_STATE = GAME_STATE_PLAYING;
                reset_game();
			}

            break;
			      
        
		

        }

        vecdisp_dbg_showfps();
		vecdisp_draw_update();

    }

    // cleaning up
    SDL_QuitSubSystem(  SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER );
    ret = vecdisp_out_end();
	assert(ret == VECDISP_SUCCESS);
	ret = vecdisp_end();
	assert(ret == VECDISP_SUCCESS);
	return 0;
}

bool event_belongs_to_controller(SDL_Event * event, SDL_GameController * controller) {
	if ( event->cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller) ) ) {
		return true;
	}
	return false;
}
void logic_handle_input(SDL_Event * event, input_params_t * input_p0, SDL_GameController * controller_p0) {
	
    if(event_belongs_to_controller(event, controller_p0) == true ) {
		input_p0->move_left = false;
		input_p0->move_right = false;
        input_p0->move_up = false;
		input_p0->move_down = false;

		input_p0->shoot = false;
		switch (event->cbutton.button) {
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            printf("DPAD_LEFT");
			if(event->cbutton.state == 1) {
				input_p0->move_left = true;
			}
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            printf("DPAD_RIGHT");
			if(event->cbutton.state == 1) {
				input_p0->move_right = true;
			}
			break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            printf("DPAD_UP");
			if(event->cbutton.state == 1) {
				input_p0->move_up = true;
			}
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            printf("DPAD_DOWN");
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
}


void reset_game(void){

    player.pos.min_y = DRAW_CENTER - 300;
    player.pos.min_x = DRAW_CENTER - PLAYER_HITBOX_SIZE/2;
    player.pos.max_y = (DRAW_CENTER - 300) + PLAYER_HITBOX_SIZE;
    player.pos.max_x = DRAW_CENTER + PLAYER_HITBOX_SIZE/2;

    astroid.pos.min_x = 0 ;
    astroid.pos.max_x = ASTROID_INIT_SIZE;
    astroid.pos.min_y = DRAW_RES;
    astroid.pos.max_y = DRAW_RES + ASTROID_INIT_SIZE;

    score_counter = 0;
    astroid_size_update = 0;
}

void update_game(input_params_t * input_p0 ){

    // FIRST: Update Objects to Input
	if (input_p0->move_left == true ) {
		player.velo_x = (-1) * PLAYER_SPEED;
	} else if ( input_p0->move_right == true ) {
		player.velo_x =  PLAYER_SPEED;
	} else if ( input_p0->move_up == true ) {
		player.velo_y =  PLAYER_SPEED;
	} else if ( input_p0->move_down == true ) {
		player.velo_y =  (-1) * PLAYER_SPEED;
	} else {
		player.velo_x = 0;
        player.velo_y = 0;
	}

    player.pos.min_x += player.velo_x;  // update pos player
    player.pos.max_x += player.velo_x;
    player.pos.min_y += player.velo_y;
    player.pos.max_y += player.velo_y;

    if(player.pos.max_x == DRAW_RES) GAME_STATE = GAME_STATE_GAMEOVER;
    if(player.pos.min_x == 0) GAME_STATE = GAME_STATE_GAMEOVER;
    if(player.pos.max_y == DRAW_RES) GAME_STATE = GAME_STATE_GAMEOVER;
    if(player.pos.min_y == 0) GAME_STATE = GAME_STATE_GAMEOVER;

    if(astroid.pos.min_x == 0){
        //srand(time(NULL));
        int r = rand() % (2 * DRAW_CENTER);
        
        printf("Rand: %d\n", r);

        astroid.pos.min_x = 0 + r;
        astroid.pos.max_x = ASTROID_INIT_SIZE + astroid_size_update + r;

        score_counter++;
    }
    
    astroid.pos.min_y = astroid.pos.min_y - 1;
    astroid.pos.max_y = astroid.pos.max_y - 1;

    if(astroid.pos.min_y == 0){
        astroid.pos.min_x = 0 ;
        astroid.pos.max_x = ASTROID_INIT_SIZE + astroid_size_update;
        astroid.pos.min_y = DRAW_RES;
        astroid.pos.max_y = DRAW_RES + ASTROID_INIT_SIZE + astroid_size_update;
    }

    if(COLLISION_EN){
        if ( vecdisp_colldet_aabb_aabb( &astroid, &player ) == true){
            printf("Game Over!\n");   // dbg
            GAME_STATE = GAME_STATE_GAMEOVER;
        }
    }

}

void draw_playing(void){
    
    uint16_t astroid_draw_corr;

    if( astroid.pos.max_y > DRAW_RES) vecdisp_draw_rect_aa(astroid.pos.min_x, astroid.pos.min_y, astroid.pos.max_x, DRAW_RES-1, DRAW_BRTNS_BRIGHT);
    else if(astroid.pos.max_x > DRAW_RES) vecdisp_draw_rect_aa(astroid.pos.min_x, astroid.pos.min_y, DRAW_RES-1, astroid.pos.max_y , DRAW_BRTNS_BRIGHT);
    else vecdisp_draw_rect_aa(astroid.pos.min_x, astroid.pos.min_y, astroid.pos.max_x, astroid.pos.max_y, DRAW_BRTNS_BRIGHT);


    // Draw Player:
     if(SHOW_HITBOX_EN) vecdisp_draw_rect_aa(player.pos.min_x, player.pos.min_y, player.pos.max_x, player.pos.max_y, DRAW_BRTNS_DARK); 

    vecdisp_draw_line(player.pos.min_x + 0.33 * PLAYER_HITBOX_SIZE, player.pos.min_y, player.pos.min_x + 0.66 * PLAYER_HITBOX_SIZE, player.pos.min_y,DRAW_BRTNS_BRIGHT);
    vecdisp_draw_line(player.pos.min_x + 0.33 * PLAYER_HITBOX_SIZE, player.pos.min_y, player.pos.min_x + 0.33 * PLAYER_HITBOX_SIZE, player.pos.min_y + 0.17 * PLAYER_HITBOX_SIZE, DRAW_BRTNS_BRIGHT);
    vecdisp_draw_line(player.pos.min_x + 0.66* PLAYER_HITBOX_SIZE, player.pos.min_y, player.pos.min_x + 0.66 * PLAYER_HITBOX_SIZE, player.pos.min_y + 0.17 * PLAYER_HITBOX_SIZE,DRAW_BRTNS_BRIGHT);
    vecdisp_draw_line(player.pos.min_x + 0.33* PLAYER_HITBOX_SIZE, player.pos.min_y + 0.17* PLAYER_HITBOX_SIZE, player.pos.min_x, player.pos.min_y + 0.17* PLAYER_HITBOX_SIZE,DRAW_BRTNS_BRIGHT);
    vecdisp_draw_line(player.pos.min_x + 0.66* PLAYER_HITBOX_SIZE, player.pos.min_y + 0.17* PLAYER_HITBOX_SIZE, player.pos.max_x, player.pos.min_y + 0.17* PLAYER_HITBOX_SIZE,DRAW_BRTNS_BRIGHT);

    vecdisp_draw_line(player.pos.min_x, player.pos.min_y + 0.17* PLAYER_HITBOX_SIZE, player.pos.min_x, player.pos.min_y + 0.33* PLAYER_HITBOX_SIZE,DRAW_BRTNS_BRIGHT);
    vecdisp_draw_line(player.pos.max_x, player.pos.min_y + 0.17* PLAYER_HITBOX_SIZE, player.pos.max_x, player.pos.min_y + 0.33* PLAYER_HITBOX_SIZE,DRAW_BRTNS_BRIGHT);

    vecdisp_draw_line(player.pos.min_x, player.pos.min_y + 0.33* PLAYER_HITBOX_SIZE, player.pos.min_x + 0.33 * PLAYER_HITBOX_SIZE, player.pos.min_y + 0.5* PLAYER_HITBOX_SIZE,DRAW_BRTNS_BRIGHT);
    vecdisp_draw_line(player.pos.max_x, player.pos.min_y + 0.33* PLAYER_HITBOX_SIZE, player.pos.min_x + 0.66 * PLAYER_HITBOX_SIZE, player.pos.min_y + 0.5* PLAYER_HITBOX_SIZE,DRAW_BRTNS_BRIGHT);
    
    vecdisp_draw_line(player.pos.min_x + 0.33 * PLAYER_HITBOX_SIZE, player.pos.min_y + 0.5* PLAYER_HITBOX_SIZE, player.pos.min_x + 0.33 * PLAYER_HITBOX_SIZE, player.pos.min_y + 0.83 * PLAYER_HITBOX_SIZE,DRAW_BRTNS_BRIGHT);
    vecdisp_draw_line(player.pos.min_x + 0.66 * PLAYER_HITBOX_SIZE, player.pos.min_y + 0.5* PLAYER_HITBOX_SIZE, player.pos.min_x + 0.66 * PLAYER_HITBOX_SIZE, player.pos.min_y + 0.83 * PLAYER_HITBOX_SIZE,DRAW_BRTNS_BRIGHT);

    vecdisp_draw_line(player.pos.min_x + 0.33 * PLAYER_HITBOX_SIZE, player.pos.min_y + 0.83 * PLAYER_HITBOX_SIZE, player.pos.min_x + 0.5 * PLAYER_HITBOX_SIZE, player.pos.min_y + PLAYER_HITBOX_SIZE,DRAW_BRTNS_BRIGHT);
    vecdisp_draw_line(player.pos.min_x + 0.66 * PLAYER_HITBOX_SIZE, player.pos.min_y + 0.83 * PLAYER_HITBOX_SIZE, player.pos.min_x + 0.5 * PLAYER_HITBOX_SIZE, player.pos.min_y + PLAYER_HITBOX_SIZE,DRAW_BRTNS_BRIGHT);

    // vecdisp_draw_rect_aa(player.pos.min_x, player.pos.min_y, player.pos.max_x, player.pos.max_y, DRAW_BRTNS_BRIGHT);
    
    // Field:

    vecdisp_draw_rect_aa(0, 0, DRAW_RES-1, DRAW_RES-1, DRAW_BRTNS_BRIGHT);

}

void draw_gameover(void) {

    char score[10];
    sprintf(score, "%.4d", score_counter);

	vecdisp_draw_string(DRAW_CENTER - 200, DRAW_CENTER -50, DRAW_CENTER + 200, DRAW_CENTER +50, DRAW_BRTNS_BRIGHT, score , 20);
    
    // Field:

    vecdisp_draw_rect_aa(0, 0, DRAW_RES-1, DRAW_RES-1, DRAW_BRTNS_BRIGHT);

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