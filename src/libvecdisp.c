// libvecdisp.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "libvecdisp.h"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"

#if defined(OUT_METHOD_SPIDAC)
	#include <bcm2835.h>
#elif defined(OUT_METHOD_SIM)
	#define SDL_MAIN_HANDLED
	#include <SDL2/SDL.h>
#endif 

//#define NANOSVG_IMPLEMENTATION
//#include "nanosvg.h"

/* Raspberry Pi 2 Pins
LDAC		GPIO_02		03		Out		Latch DAC sync line
SPI_MOSI	GPIO_10     19  	Out		spi master-out line
SPI_SCLK    GPIO_11     23  	Out		spi clock line
SPI_CE0     GPIO_8      24  	Out		chip-select line for dac with x and y output
SPI_CE1     GPIO_7      26  	Out		chip-select line for dac with z output
*/

#define OUT_BUFFER_SIZE 18 // set x as OUT_BUFFER_SIZE = pow(2, x). 

#define DAC_DELAY_US 5	// datasheet: longest settling time ( 0V to 4.048V is ca. 7,3us). Adjust if processing time goes below that.


// LUT generation at compile time by defining the S1(i) macro function and then filling array of size <x> with  S<x>(i) and then undefining S1 again to enable reusing the LUT generation
#define S4(i)    S1((i)),   S1((i)+1),     S1((i)+2),     S1((i)+3)
#define S16(i)   S4((i)),   S4((i)+4),     S4((i)+8),     S4((i)+12)
#define S64(i)   S16((i)),  S16((i)+16),   S16((i)+32),   S16((i)+48)
#define S256(i)  S64((i)),  S64((i)+64),   S64((i)+128),  S64((i)+192)
#define S1024(i) S256((i)), S256((i)+256), S256((i)+512), S256((i)+768)

const double lut_sin256[256] =  {
	#define S1(i) sin( (2*M_PI * i) / 256)
	S256(0)
	#undef S1
};

const double lut_cos256[256] =  {
	#define S1(i) cos( (2*M_PI * i) / 256)
	S256(0)
	#undef S1
};

// opcodes for out_buffer->data
typedef enum {
	//OUT_OPCODE_CLEAR = 0x0,			// Clear screen. Used when there is no z axis available
	OUT_OPCODE_MOVE = 0x1,		// Move to next coordinates within a shape without any delays
	OUT_OPCODE_START = 0x2,		// unblank the beam, jump to start coordinates
	OUT_OPCODE_STOP = 0x3			// blank the beam
} OUT_OPCODE_T;

/* Output Buffer ( FIFO )
'->data' bit encoding:
data[0], data[1] 		X-coordinate data encoded in SPI DAC transmission format
data[2], data[3] 		Y-coordinate data encoded in SPI DAC transmission format
data[4], data[5]		Z-value data encoded in SPI DAC transmission format
data[6]					opcode for starting, moving, stopping,.. the electron beam */
typedef struct {
	uint8_t ** data;
	uint32_t size;
	uint32_t pos_r;
	uint32_t pos_w;
} out_buffer_t;

/* ################################
### internal function headers	###
#################################*/
VECDISP_T vecdisp_out_buffer_in( uint16_t x0, uint16_t y0, uint16_t z0, OUT_OPCODE_T OUT_OPCODE );
VECDISP_T vecdisp_out_buffer_init(void);
VECDISP_T vecdisp_out_buffer_end(void);
VECDISP_T vecdisp_out_buffer_out( uint8_t ** data );
void vecdisp_draw_line_alg(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t z0);
void vecdisp_out_setvals(uint8_t * data);
void vecdisp_prepare_data( uint16_t x0, uint16_t y0, uint16_t z0, OUT_OPCODE_T OUT_OPCODE, uint8_t data_elem[7] );

/* ########################
### global variables	###
#########################*/
static out_buffer_t out_buffer;
uint32_t out_buffer_size = (((uint32_t) 0x1) << OUT_BUFFER_SIZE); 
uint32_t out_buffer_mask = (((uint32_t) 0x1) << OUT_BUFFER_SIZE) - 1;


/* ################
### functions 	###
#################*/

VECDISP_T vecdisp_init(void) {
	VECDISP_T ret = vecdisp_out_buffer_init();
	return ret;
}

VECDISP_T vecdisp_end(void) {
	return vecdisp_out_buffer_end();
}

VECDISP_T vecdisp_out_buffer_init(void) {
	out_buffer.size = out_buffer_size;
	out_buffer.pos_r = 0;
	out_buffer.pos_w = 0;
	out_buffer.data = (uint8_t **) calloc(out_buffer_size, sizeof(uint8_t* ));
	if(out_buffer.data == NULL ) return VECDISP_FAILURE;
	for(int i = 0; i < out_buffer_size; i++ ) {
		out_buffer.data[i] = (uint8_t *) calloc(7, sizeof(uint8_t ));
		if( out_buffer.data[i] == NULL ) {
			for(int j = i - 1; j >= 0; j--) {
				free(out_buffer.data[j] );
			}
			free(out_buffer.data);
			return VECDISP_FAILURE;
		}
	}
	return VECDISP_SUCCESS;
}

VECDISP_T vecdisp_out_buffer_end(void) {
	for(int i = out_buffer.size - 1; i >= 0; i--) {
		free(out_buffer.data[i]);
	}
	free(out_buffer.data);
	return VECDISP_SUCCESS;
}

VECDISP_T vecdisp_out_buffer_in( uint16_t x0, uint16_t y0, uint16_t z0, OUT_OPCODE_T OUT_OPCODE ) {
	x0 = (x0 * DRAW_SCALE) & 0x0FFF;
	y0 = (y0 * DRAW_SCALE) & 0x0FFF;
	z0 = (~z0) & 0x0FFF;
	OUT_OPCODE = OUT_OPCODE & 0xFF;

	if( ((out_buffer.pos_w + 1) & out_buffer_mask) == out_buffer.pos_r) {
		uint8_t ** data_tmp = (uint8_t **) realloc( out_buffer.data, (out_buffer_size << 1) * 7 * sizeof(uint8_t) );
		if(data_tmp == NULL) {
			return VECDISP_FAILURE;
		}
		for(int i = 0; i < (out_buffer_size << 1); i++ ) {
			data_tmp[i] = (uint8_t *) calloc(7, sizeof(uint8_t ));
			if( data_tmp[i] == NULL ) {
				for(int j = i - 1; j >= 0; j--) {
					free(data_tmp[j] );
				}
				free(data_tmp);
				return VECDISP_FAILURE;
			}
		}
		out_buffer.data = data_tmp;
		uint32_t delta = ( out_buffer_size - 1 - out_buffer.pos_r );
		for( int i = out_buffer.pos_r; i < out_buffer_size; i++ ) {
			memcpy( out_buffer.data[i], data_tmp[i + out_buffer_size], 7  * sizeof(uint8_t) );
		}
		out_buffer.pos_r += delta;
		out_buffer_size = (out_buffer_size << 1);
		out_buffer_mask = (out_buffer_size - 1);
	}
	vecdisp_prepare_data( x0, y0, z0, OUT_OPCODE, out_buffer.data[out_buffer.pos_w] );

	out_buffer.pos_w = (out_buffer.pos_w + 1) & out_buffer_mask;
	return VECDISP_SUCCESS;
}

VECDISP_T vecdisp_out_buffer_out( uint8_t ** data ) {
	if(out_buffer.pos_r == out_buffer.pos_w) return VECDISP_FAILURE;

	*data = out_buffer.data[out_buffer.pos_r];
	out_buffer.pos_r++;
	out_buffer.pos_r &= out_buffer_mask;
	return VECDISP_SUCCESS;
}

void vecdisp_prepare_data( uint16_t x0, uint16_t y0, uint16_t z0, OUT_OPCODE_T OUT_OPCODE, uint8_t data_elem[7] ) {
	data_elem[0] = 0x00 | ( x0 >> 8 ) | (OUT_GAIN << 5)  | (0x01 << 4);
	data_elem[1] = 0x00 | x0;
	data_elem[2] = 0x00 | ( y0 >> 8 ) | ( 0x01 << 7 ) | (OUT_GAIN << 5)  | (0x01 << 4);
	data_elem[3] = 0x00 | y0;
	data_elem[4] = 0x00 | ( z0 >> 8 ) | (OUT_GAIN << 5)  | (0x01 << 4);
	data_elem[5] = 0x00 | z0;
	data_elem[6] = 0x00 | OUT_OPCODE;
}

#if defined (OUT_METHOD_ANALOG)
	void vecdisp_draw_line_alg(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t z0) {
		vecdisp_out_buffer_in(x0, y0, z0, OUT_OPCODE_MOVE);
		vecdisp_out_buffer_in(x1, y1, z0, OUT_OPCODE_MOVE);
	}
#elif defined (OUT_METHOD_DIGITAL)
	void vecdisp_draw_line_alg(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t z0) {
		int dx =  abs( x1 - x0 );
		int dy = (-1) * abs( y1 - y0 );
		int sx = x0 < x1 ? 1 : -1;
		int sy = y0 < y1 ? 1 : -1;
		int err = dx + dy;
		int e2; // error value e_xy
		while (1) {
			vecdisp_out_buffer_in(x0, y0, z0, OUT_OPCODE_MOVE);

			if ( ( x0 == x1 ) && ( y0 == y1 ) ) {
				break;
			}
			e2 = 2 * err;
			if (e2 > dy) { err += dy; x0 += sx; } // e_xy+e_x > 0
			if (e2 < dx) { err += dx; y0 += sy; } // e_xy+e_y < 0
		}
	}
#endif

void vecdisp_draw_move(uint16_t x0, uint16_t y0, uint16_t z0) {
	vecdisp_out_buffer_in(x0, y0, z0, OUT_OPCODE_MOVE);
}

void vecdisp_draw_point(uint16_t x0, uint16_t y0, uint16_t z0) {
	vecdisp_out_buffer_in(x0, y0, z0, OUT_OPCODE_START);
	vecdisp_out_buffer_in(x0, y0, z0, OUT_OPCODE_MOVE);
	vecdisp_out_buffer_in(x0, y0, 0, OUT_OPCODE_STOP);
}

void vecdisp_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t z0) {
	vecdisp_out_buffer_in(x0, y0, z0, OUT_OPCODE_START);
	vecdisp_draw_line_alg(x0, y0, x1, y1, z0);
	vecdisp_out_buffer_in(x1, y1, 0, OUT_OPCODE_STOP);
}

void vecdisp_draw_path(uint16_t path[][2], uint32_t len, uint16_t z0 ) {
	vecdisp_out_buffer_in(path[0][0], path[0][1], z0, OUT_OPCODE_START);
	for(int i = 1; i < len; i++) {
		vecdisp_draw_line_alg( path[i - 1][0], path[i - 1][1], path[i][0], path[i][1], z0);
	}
	vecdisp_out_buffer_in(path[len - 1][0], path[len - 1][1], 0, OUT_OPCODE_STOP);
}

void vecdisp_draw_rect_aa(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t z0) {
	vecdisp_out_buffer_in(x0, y0, z0, OUT_OPCODE_START);
	vecdisp_draw_line_alg( x0, y0, x0, y1, z0);
	vecdisp_draw_line_alg( x0, y1, x1, y1, z0);
	vecdisp_draw_line_alg( x1, y1, x1, y0, z0);
	vecdisp_draw_line_alg( x1, y0, x0, y0, z0);
	vecdisp_out_buffer_in(x0, y0, 0, OUT_OPCODE_STOP);
}

void vecdisp_draw_ellipse(uint16_t x0, uint16_t y0, uint16_t radius_x, uint16_t radius_y, uint16_t z0, uint8_t roundness) {
	int x1 = x0 + radius_x, y1 = y0, x2, y2;
	
	vecdisp_out_buffer_in(x1, y1, z0, OUT_OPCODE_START);
	for( int i = 0; i < 256; i += roundness ) {		// smaller steps => more corners of ellipse
		x2 = lround(x0 + radius_x * lut_cos256[i]);
		y2 = lround(y0 + radius_y * lut_sin256[i]);
		vecdisp_draw_line_alg( x1, y1, x2, y2, z0);
		x1 = x2;
		y1 = y2;
	}
	vecdisp_draw_line_alg(x2, y2, x0 + radius_x, y0, z0);
	vecdisp_out_buffer_in(x2, y2, 0, OUT_OPCODE_STOP);
}

void vecdisp_draw_cubebez( uint16_t a[2], uint16_t b[2], uint16_t c[2], uint16_t d[2], uint16_t z0 ) {
	inline uint16_t lerp(uint16_t a, uint16_t b, double t) {
		return lround(a + ( b - a ) * t);
	}
	uint16_t ab[2], bc[2], cd[2], abbc[2], bccd[2], dest[2], dest_prev[2];
	dest[0] = a[0];
	dest[1] = a[1];
	vecdisp_out_buffer_in(dest[0], dest[1], z0, OUT_OPCODE_START);
	
	double steps = 5;
	for(double t = 0; t <= 1.0; t += 1.0 / steps) {
		dest_prev[0] = dest[0];
		dest_prev[1] = dest[1];
		ab[0] = lerp(a[0], b[0], t);
		ab[1] = lerp(a[1], b[1], t);
		bc[0] = lerp(b[0], c[0], t);
		bc[1] = lerp(b[1], c[1], t);
		cd[0] = lerp(c[0], d[0], t);
		cd[1] = lerp(c[1], d[1], t);
		abbc[0] = lerp(ab[0], bc[0], t);
		abbc[1] = lerp(ab[1], bc[1], t);
		bccd[0] = lerp(bc[0], cd[0], t);
		bccd[1] = lerp(bc[1], cd[1], t);
		dest[0] = lerp(abbc[0], bccd[0], t);
		dest[1] = lerp(abbc[1], bccd[1], t);
		vecdisp_draw_line_alg(dest_prev[0], dest_prev[1], dest[0], dest[1], z0);
	}
	vecdisp_out_buffer_in(dest[0], dest[1], 0, OUT_OPCODE_STOP);
}

void vecdisp_draw_shape( vecdisp_shape_t * shape, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t z0 ) {
	double scalex = floor( ( ( (double) ( x1 - x0 )) / (DRAW_RES - 1 ) ) * 20) / 20;
	double scaley = floor( ( ( (double) ( y1 - y0 )) / (DRAW_RES - 1 ) ) * 20) / 20;

	switch(shape->TYPE) {
	case VECDISP_SHAPE_CUBEBEZ: {
		uint16_t a[2], b[2], c[2], d[2];
		
		for(uint32_t i = 0; i < shape->data_len - 3; i += 4) {
			a[0] = lround( scalex * shape->data[i][0] );
			a[1] = lround( scaley * shape->data[i][1] );
			b[0] = lround( scalex * shape->data[i + 1][0] );
			b[1] = lround( scaley * shape->data[i + 1][1] );
			c[0] = lround( scalex * shape->data[i + 2][0] );
			c[1] = lround( scaley * shape->data[i + 2][1] );
			d[0] = lround( scalex * shape->data[i + 3][0] );
			d[1] = lround( scaley * shape->data[i + 3][1] );
			vecdisp_draw_cubebez( a, b, c, d, z0 );
		}
		}
		break;
	case VECDISP_SHAPE_POINTS: {
		for(uint32_t i = 0; i < shape->data_len; i++) {
			uint16_t point_x0 = lround( scalex * shape->data[i][0] );
			uint16_t point_y0 = lround( scaley * shape->data[i][1] );
			vecdisp_draw_point( point_x0, point_y0, z0 );
		}
		}
		break;
	case VECDISP_SHAPE_LINES: {
		for(uint32_t i = 0; i < shape->data_len - 1; i += 2) {
			uint16_t line_x0 = lround( scalex * shape->data[i][0] );
			uint16_t line_y0 = lround( scalex * shape->data[i][1] );
			uint16_t line_x1 = lround( scalex * shape->data[i + 1][0] );
			uint16_t line_y1 = lround( scalex * shape->data[i + 1][1] );
			vecdisp_draw_line( line_x0, line_y0, line_x1, line_y1, z0 );
		}
		}
		break;
	}
}

vecdisp_shape_t * vecdisp_shape_create(VECDISP_SHAPE_TYPE_T TYPE, uint16_t data[][2], uint32_t data_len) {
	vecdisp_shape_t * shape = (vecdisp_shape_t*) calloc( 1, sizeof(vecdisp_shape_t) );
	shape->TYPE = TYPE;
	if(data == NULL || data_len == 0 ) return shape;
	vecdisp_shape_data_add(shape, data, data_len);
	return shape;
}

void vecdisp_shape_destroy(vecdisp_shape_t * shape ) {
	vecdisp_shape_data_clear( shape );
	free(shape);
	shape = NULL;
	return;
}

VECDISP_T vecdisp_shape_data_add(vecdisp_shape_t * shape, uint16_t data[][2], uint32_t data_len) {
	uint16_t ** new_data = (uint16_t**) realloc(shape->data, (shape->data_len + data_len) * sizeof(uint16_t*));
	if(new_data == NULL) {
		return VECDISP_FAILURE;
	}
	for(uint32_t i = 0; i < data_len; i++) {
		new_data[shape->data_len + i] = (uint16_t*) malloc(2 * sizeof(uint16_t));
		if( new_data[shape->data_len + i] == NULL ) {
			for(uint32_t j = i -1; j >= 0; j--) {
				free(new_data[shape->data_len + j]);
				return VECDISP_FAILURE;
			}
		}
		new_data[shape->data_len + i][0] = data[i][0];
		new_data[shape->data_len + i][1] = data[i][1];
	}
	
	shape->data = new_data;
	shape->data_len += data_len;
	return VECDISP_SUCCESS;
}

void vecdisp_shape_data_clear( vecdisp_shape_t * shape ) {
	if(shape->data != NULL) {
		for(uint32_t i = 0; i < shape->data_len - 1; i++ ) {
			free( shape->data[i] );
		}
		free(shape->data);
	}
	shape->data_len = 0;
}

vecdisp_shape_t * vecdisp_shape_import_svg( const char * filepath ) {
	vecdisp_shape_t * shape_svg = vecdisp_shape_create(VECDISP_SHAPE_CUBEBEZ, NULL, 0 );
	shape_svg->TYPE = VECDISP_SHAPE_CUBEBEZ;
	NSVGimage *image = NULL;

	image = nsvgParseFromFile(filepath, "px", 96.0 );
	if (image == NULL) {
		vecdisp_shape_destroy(shape_svg);
		return NULL;
	}

	double image_scale = 0;
	if(image->width > image->height) {
		image_scale = ((double) (DRAW_RES - 1)) / image->width;
	} else {
		image_scale = ((double) (DRAW_RES - 1)) / image->height;
	}
	image_scale = floor( image_scale * 20 ) / 20;

	for (NSVGshape * shape = image->shapes; shape != NULL; shape = shape->next) {
		for (NSVGpath* path = shape->paths; path != NULL; path = path->next) {
			for (int i = 0; i < path->npts-1; i += 3) {
				float* p = &path->pts[i*2];
				uint16_t bezierpoints[4][2];
				bezierpoints[0][0] = (uint16_t)(p[0] * image_scale);
				bezierpoints[0][1] = (uint16_t)(DRAW_RES - 1 - p[1] * image_scale);
				bezierpoints[1][0] = (uint16_t)(p[2] * image_scale);
				bezierpoints[1][1] = (uint16_t)(DRAW_RES - 1 - p[3] * image_scale);
				bezierpoints[2][0] = (uint16_t)(p[4] * image_scale);
				bezierpoints[2][1] = (uint16_t)(DRAW_RES - 1 - p[5] * image_scale);
				bezierpoints[3][0] = (uint16_t)(p[6] * image_scale);
				bezierpoints[3][1] = (uint16_t)(DRAW_RES - 1 - p[7] * image_scale);

				if( vecdisp_shape_data_add(shape_svg, bezierpoints, 4) == VECDISP_FAILURE) {
					vecdisp_shape_destroy(shape_svg);
					free(image);
					return NULL;
				}
			}
		}
	}
	free(image);
	return shape_svg;
}

void vecdisp_dbg_showfps(void) {
	static uint32_t fps_cnt = 0;
	static struct timespec clk_cur, clk_prev, clk_res;
	clock_getres(CLOCK_MONOTONIC, &clk_res);
	clock_gettime(CLOCK_MONOTONIC, &clk_cur);
	fps_cnt++;
	if( clk_cur.tv_sec - clk_prev.tv_sec >= 1  ) {
		clk_prev = clk_cur;
		printf("FPS: %d\n", fps_cnt);
		fps_cnt = 0;
	}
}

/* ########################
### OUT_METHOD_SPIDAC 	###
#########################*/
#if defined (OUT_METHOD_SPIDAC)

#define DAC_PIN_LDACN RPI_V2_GPIO_P1_03

VECDISP_T vecdisp_out_init(void) {
	int ret = bcm2835_init();
	if(ret != 1) return VECDISP_FAILURE;
	ret = bcm2835_spi_begin();
	if(ret != 1) return VECDISP_FAILURE;
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);        // Set SPI Bit Order
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                     // Set SPI Data Mode
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);		// Set clock divider. spi_clock_speed = apb_clock_speed / divider. Minimum value for MCP4822: divider = 8
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);        // Set SPI CS Line 0 active on low
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);		// Set SPI CS Line 1 active on low
	bcm2835_gpio_fsel(DAC_PIN_LDACN, BCM2835_GPIO_FSEL_OUTP);
 
	bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                        // Set SPI Chip Select Line
	bcm2835_gpio_write(DAC_PIN_LDACN, HIGH);						// LDAC Line of the DAC. Output voltages are updated with SPI register contents when the LDAC Line is low
	
	return VECDISP_SUCCESS;
}

VECDISP_T vecdisp_out_end(void) {
	bcm2835_spi_end();
	bcm2835_close();
	return VECDISP_SUCCESS;
}

/* optimized gpio_write() function from bcm2835.h */
void gpio_write_optim(uint8_t pin, uint8_t on) {
	uint8_t shift = pin % 32;
	volatile uint32_t* paddr;
	if(on) {
		paddr = bcm2835_gpio + BCM2835_GPSET0/4 + pin/32;
	} else {
		paddr = bcm2835_gpio + BCM2835_GPCLR0/4 + pin/32;
	}
	bcm2835_peri_write_nb(paddr, 1 << shift);
}


/* optimized bcm2835_writenb() function from bcm2835.h for faster transmission */
void spi_writenb_optim(const char* tbuf, uint32_t len)
{
	static uint8_t bcm2835_spi_bit_order = BCM2835_SPI_BIT_ORDER_MSBFIRST;
	static uint8_t bcm2835_byte_reverse_table[] = 
	{
		0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
		0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
		0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
		0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
		0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
		0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
		0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
		0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
		0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
		0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
		0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
		0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
		0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
		0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
		0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
		0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
		0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
		0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
		0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
		0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
		0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
		0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
		0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
		0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
		0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
		0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
		0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
		0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
		0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
		0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
		0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
		0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
	};
	inline uint8_t bcm2835_correct_order(uint8_t b) {
		if (bcm2835_spi_bit_order == BCM2835_SPI_BIT_ORDER_LSBFIRST)
		return bcm2835_byte_reverse_table[b];
		else
		return b;
	}
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    volatile uint32_t* fifo = bcm2835_spi0 + BCM2835_SPI0_FIFO/4;
    uint32_t i;

    /* Clear TX and RX fifos */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);

    /* Set TA = 1 */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);

    for (i = 0; i < len; i++) {
	
	/* Write to FIFO, no barrier */
	bcm2835_peri_write_nb(fifo, bcm2835_correct_order(tbuf[i]));
	
	/* Read from FIFO to prevent stalling */
	while (bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_RXD)
	    (void) bcm2835_peri_read_nb(fifo);
    }
    
    /* Wait for DONE to be set */
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE)) {
    }

    /* Set TA = 0, and also set the barrier */
    bcm2835_peri_set_bits(paddr, 0, BCM2835_SPI0_CS_TA);
}


void vecdisp_out_setvals(uint8_t * data) {
	inline void setval_x0y0(uint8_t * data) {
		gpio_write_optim(DAC_PIN_LDACN, HIGH);
		bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
		spi_writenb_optim((char *) (data + 0), 2);
		spi_writenb_optim((char *) (data + 2), 2);
		gpio_write_optim(DAC_PIN_LDACN, LOW);
	}
	inline void setval_z0(uint8_t * data) {
		gpio_write_optim(DAC_PIN_LDACN, HIGH);
		bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
		spi_writenb_optim((char *) (data + 4), 2);
		gpio_write_optim(DAC_PIN_LDACN, LOW);
	}

	#if defined (OUT_METHOD_ANALOG)
		switch ( data[6]) {
		case OUT_OPCODE_START:
			setval_x0y0(data);
			//bcm2835_delayMicroseconds(DAC_DELAY_US);
			setval_z0(data);
			bcm2835_delayMicroseconds(DAC_DELAY_US);
			break;
		case OUT_OPCODE_MOVE:
			setval_x0y0(data);
			bcm2835_delayMicroseconds(DAC_DELAY_US);
			break;
		case OUT_OPCODE_STOP:
			setval_z0(data);
			bcm2835_delayMicroseconds(DAC_DELAY_US);
			break;
		}
	#elif defined (OUT_METHOD_DIGITAL)
		switch ( data[6]) {
		case OUT_OPCODE_START:
			setval_x0y0(data);
			bcm2835_delayMicroseconds(DAC_DELAY_US);
			setval_z0(data);
			bcm2835_delayMicroseconds(DAC_DELAY_US);
			break;
		case OUT_OPCODE_MOVE:
			setval_x0y0(data);
			break;
		case OUT_OPCODE_STOP:
			setval_z0(data);
			bcm2835_delayMicroseconds(DAC_DELAY_US);
			break;
		}
	#endif
}

VECDISP_T vecdisp_draw_update(void) {
	uint8_t * data = NULL;
	VECDISP_T ret = vecdisp_out_buffer_out( &data );
	while(ret == VECDISP_SUCCESS) {
		vecdisp_out_setvals( data );
		ret = vecdisp_out_buffer_out( &data );
	}
	return VECDISP_SUCCESS;
}

#endif

/* ####################
### OUT_METHOD_SIM 	###
#####################*/
#if defined (OUT_METHOD_SIM)

#define OUT_SIM_RES 1024

#define OUT_SIM_MASK ( 0x0000 & (DRAW_RES - 1) )

static SDL_Window * window;
static SDL_Renderer * renderer;
static SDL_Texture * texture;
static uint32_t pixelbuf[DRAW_RES * DRAW_RES];

VECDISP_T vecdisp_out_init(void) {
	int ret = SDL_Init(SDL_INIT_VIDEO);
	if ( ret != 0) return VECDISP_FAILURE;
	
	window = SDL_CreateWindow(
		"libvecdisp XY-Simulator",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		OUT_SIM_RES,
		OUT_SIM_RES,
		SDL_WINDOW_SHOWN
	);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	SDL_RenderSetLogicalSize(renderer, DRAW_RES, DRAW_RES);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	SDL_ShowCursor( SDL_DISABLE );

	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STATIC,
		DRAW_RES,
		DRAW_RES
		);

	memset(pixelbuf, 0, DRAW_RES * DRAW_RES * sizeof(uint32_t));
	return VECDISP_SUCCESS;
}

VECDISP_T vecdisp_out_end(void) {
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
	return VECDISP_SUCCESS;
}
void vecdisp_out_setvals(uint8_t * data) {
	uint16_t x0 = ( ( 0x0FFF & ( ( (uint16_t) data[0] ) << 8 ) ) | ( 0x00FF & data[1] ) ) / DRAW_SCALE;
	uint16_t y0 = ( ( 0x0FFF & ( ( (uint16_t) data[2] ) << 8 ) ) | ( 0x00FF & data[3] ) ) / DRAW_SCALE;
	uint16_t z0 = 0x0FFF & (~( 0x0F00 & ( ( (uint16_t) data[4] ) << 8 ) ) | ( 0x00FF & data[5] ) );
	y0 = (DRAW_RES - 1) - y0;
	uint32_t pxvalue = 0 | ( z0 >> 4 ) << 8;
	
	switch ( data[6]) {
	case OUT_OPCODE_MOVE:
		pixelbuf[ y0 * DRAW_RES + x0 ] = pxvalue;
		break;
	case OUT_OPCODE_START:
		break;
	case OUT_OPCODE_STOP:
		break;
	}
}

VECDISP_T vecdisp_draw_update(void) {
	uint8_t * data = NULL;
	VECDISP_T ret = vecdisp_out_buffer_out( &data );
	while(ret == VECDISP_SUCCESS) {
		vecdisp_out_setvals( data );
		ret = vecdisp_out_buffer_out( &data );
	}
	SDL_UpdateTexture(texture, NULL, pixelbuf, DRAW_RES * sizeof(uint32_t));
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	memset(pixelbuf, 0, DRAW_RES * DRAW_RES * sizeof(uint32_t));
	
	return VECDISP_SUCCESS;
}

#endif

