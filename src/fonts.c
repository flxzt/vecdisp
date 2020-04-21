// fonts.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>

#include "libvecdisp.h"

void vecdisp_draw_char(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t z0, char x ) {
	uint16_t width = x1 - x0;
	uint16_t height = y1 - y0;
	uint16_t descender = y0;
	uint16_t baseline = descender + lround(height * 0.2);
	uint16_t median =  descender + lround(height * 0.6);
	uint16_t ascender = descender + height;

	/*
	//Displaying descender, baseline, median and ascender for development
	vecdisp_draw_line(x0, descender, x0 + width, descender, DRAW_BRTNS_DARK);
	vecdisp_draw_line(x0, baseline, x0 + width, baseline, DRAW_BRTNS_DARK);
	vecdisp_draw_line(x0, median, x0 + width, median, DRAW_BRTNS_DARK);
	vecdisp_draw_line(x0, ascender, x0 + width, ascender, DRAW_BRTNS_DARK);
	*/

	switch (x) {
		case 'a': {

			}
			break;
		case 'b': {

			}
			break;
		case 'c': {

			}
			break;
		case 'd': {

			}
			break;
		case 'e': {

			}
			break;
		case 'f': {

			}
			break;
		case 'g': {

			}
			break;
		case 'h': {

			}
			break;
		case 'i': {

			}
			break;
		case 'j': {

			}
			break;
		case 'k': {

			}
			break;
		case 'l': {

			}
			break;
		case 'm': {

			}
			break;
		case 'n': {

			}
			break;
		case 'o': {

			}
			break;
		case 'p': {

			}
			break;
		case 'q': {

			}
			break;
		case 'r': {

			}
			break;
		case 's': {

			}
			break;
		case 't': {

			}
			break;
		case 'u': {

			}
			break;
		case 'v': {

			}
			break;
		case 'w': {

			}
			break;
		case 'x': {

			}
			break;
		case 'y': {

			}
			break;
		case 'z': {

			}
			break;
		case 'A': {
			uint16_t lines[][2] = { { x0, baseline },
									{ x0 + (width / 2), ascender },
									{ x0 + width, baseline } };
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			vecdisp_draw_line(x0 + 1 * (width / 4), median, x0 + 3 * (width / 4), median, z0);
			}
			break;
		case 'B': {
			uint16_t lines[][2] = { { x0, baseline },
									{ x0, ascender },
									{ x0 + 2 * ( width/3 ), ascender},
									{ x0 + width, median + (ascender - median) / 2 },
									{ x0 + 2 * ( width/3 ), median },
									{ x0 + width, baseline + ( median - baseline ) / 2},
									{ x0 + 2 * ( width/3 ), baseline},
									{x0, baseline}
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			vecdisp_draw_line( x0, median, x0 + 2 * ( width/3 ), median, z0);
			}
			break;
		case 'C': {
			uint16_t lines[][2] = { { x0 + width, baseline },
									{ x0 + 1 * (width/3), baseline },
									{ x0, baseline + (median - baseline)/2 },
									{ x0, median + (ascender - median)/2 },
									{ x0 + 1 * (width/3), ascender},
									{x0 + width, ascender}
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case 'D': {
			uint16_t lines[][2] = { { x0, baseline},
									{ x0, ascender },
									{ x0 + 2 * (width/3), ascender },
									{ x0 + width, median + (ascender - median)/2 },
									{ x0 + width, baseline + (median-baseline)/2 },
									{ x0 + 2 * (width/3), baseline },
									{ x0, baseline}
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case 'E': {
			uint16_t lines[][2] = { { x0 + width, baseline },
									{ x0, baseline },
									{ x0, ascender},
									{ x0 + width, ascender }
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			vecdisp_draw_line( x0, median, x0 + width, median, z0 );
			}
			break;
		case 'F': {
			uint16_t lines[][2] = { { x0, baseline },
									{ x0, ascender},
									{ x0 + width, ascender }
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			vecdisp_draw_line( x0, median, x0 + width, median, z0 );
		 	}
			break;
		case 'G': {
				uint16_t lines[][2] = { { x0 + width, ascender },
										{ x0 + 1 * (width / 3), ascender },
										{ x0, median + (ascender - median)/2 },
										{ x0, baseline + (median - baseline)/2 },
										{ x0 + 1 * (width / 3), baseline },
										{ x0 + 2 * (width / 3), baseline },
										{ x0 + width, baseline + 1 * ( (median - baseline)/ 3) },
										{ x0 + width, median },
										{ x0 + width / 2, median}
										};
				vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case 'H': {
			vecdisp_draw_line( x0, baseline, x0, ascender, z0 );
			vecdisp_draw_line( x0 + width, baseline, x0 + width, ascender, z0 );
			vecdisp_draw_line( x0, median, x0 +width, median, z0 );
			}
			break;
		case 'I': {
			vecdisp_draw_line( x0 + width / 2, baseline, x0 + width / 2, ascender, z0 );
			}
			break;
		case 'J': {
			uint16_t lines[][2] = { { x0, baseline + 1 * ( ( median - baseline ) / 3 ) },
									{ x0 + 2 * (width/3), baseline },
									{ x0 + 2 * (width/3), baseline },
									{ x0 + width, baseline + 1 * ( ( median - baseline ) / 3 ) },
									{ x0 + width, ascender},
									{x0, ascender}
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case 'K': {
			uint16_t lines[][2] = { { x0 + width, ascender },
									{ x0, median },
									{ x0 + width, baseline}
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			vecdisp_draw_line( x0, ascender, x0, baseline, z0 );
			}
			break;
		case 'L': {
			uint16_t lines[][2] = { { x0, ascender },
									{ x0, baseline},
									{ x0 + width, baseline}
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case 'M': {
			uint16_t lines[][2] = { { x0, baseline },
									{ x0, ascender },
									{ x0 + width/2, baseline },
									{ x0 + width, ascender },
									{ x0 + width, baseline }
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case 'N': {
			uint16_t lines[][2] = { { x0, baseline },
									{ x0, ascender },
									{ x0 + width, baseline},
									{ x0 + width, ascender }
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case 'O': { 
			vecdisp_draw_ellipse( x0 + width / 2, baseline + (ascender - baseline) / 2, width / 2,  (ascender - baseline) / 2, z0, 24);
			}
			break;
		case 'P': {
			uint16_t lines[][2] = {	{ x0, ascender },
									{ x0 + 2 * ( width / 3 ), ascender },
									{ x0 + width, median + 2 * ( ( ascender - median ) / 3 ) },
									{ x0 + width, median + 1 * ( ( ascender - median ) / 3 ) },
									{ x0 + 2 * ( width / 3 ), median },
									{ x0, median },
									{ x0, baseline }
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			vecdisp_draw_line( x0, ascender, x0, median, z0);
			}
			break;
		case 'Q': {
			vecdisp_draw_ellipse( x0 + width / 2, baseline + (ascender - baseline) / 2, width / 2,  (ascender - baseline) / 2, z0, 24);
			vecdisp_draw_line( x0 + 2 * ( width/3 ), baseline + 1 * ( (median - baseline)/2 ), x0 + width, baseline, z0);
			}
			break;
		case 'R': {
			uint16_t lines[][2] = { { x0, baseline },
									{ x0, ascender },
									{ x0 + 2 * ( width / 3 ), ascender },
									{ x0 + width, median + 2 * ( (ascender - median)/3 ) },
									{ x0 + width, median + 1 * ( (ascender - median)/3 ) },
									{ x0 + 2* ( width / 3 ), median },
									{ x0, median },
									{ x0 + 2* ( width / 3 ), median },
									{ x0 + width, baseline}
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
		}
			break;
		case 'S': {
			uint16_t lines[][2] = { { x0, baseline },
									{ x0 + 2 * ( width / 3 ), baseline },
									{ x0 + width, baseline + 1 * ( (median - baseline) / 3 ) },
									{ x0 + width, baseline + 2 * ( (median - baseline) / 3 ) },
									{ x0, median + 1 * ( ( ascender - median ) / 3 ) },
									{ x0, median + 2 * ( ( ascender - median ) / 3 ) },
									{ x0 + 2 * ( width / 3 ), ascender },
									{ x0 + width, ascender }
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case 'T': { 
			vecdisp_draw_line(x0, ascender, x0 + width, ascender, z0);
			vecdisp_draw_line(x0 + width / 2, ascender, x0 + width/2, baseline, z0);
			}
			break;
		case 'U': {
			uint16_t lines[][2] = { { x0, ascender },
									{ x0, baseline + 1 * ( ( median - baseline ) / 3 ) },
									{ x0 + 1 * ( width / 3 ), baseline },
									{ x0 + 2 * ( width / 3 ), baseline },
									{ x0 + width , baseline + 1 * ( ( median - baseline ) / 3 ) },
									{ x0 + width, ascender}
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case 'V': {
			uint16_t lines[][2] = { { x0, ascender },
									{ x0 + width / 2, baseline },
									{ x0 + width, ascender}
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case 'W': {
			uint16_t lines[][2] = { { x0, ascender },
									{ x0 + 1 * ( width / 4 ), baseline },
									{ x0 + 2 * ( width / 4 ), baseline + 2 * (( median-baseline )/3) },
									{ x0 + 3 * ( width / 4 ), baseline },
									{ x0 + width, ascender }
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case 'X': {
			vecdisp_draw_line(x0, ascender, x0 + width, baseline, z0);
			vecdisp_draw_line(x0, baseline, x0 + width, ascender, z0);
			}
			break;
		case 'Y': {
			uint16_t lines[][2] = { { x0, ascender },
									{ x0 + width / 2, median + (ascender-median)/4 },
									{ x0 + width, ascender }
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			vecdisp_draw_line(x0 + width / 2, median + (ascender-median)/4, x0 + width / 2, baseline, z0);
			}
			break;
		case 'Z': {
			uint16_t lines[][2] = { { x0, ascender },
									{ x0 + width, ascender },
									{ x0, baseline }, 
									{ x0 + width, baseline }
									};
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case '0': { 
			vecdisp_draw_ellipse(x0 + width / 2, baseline + (ascender - baseline) / 2, width / 2, (ascender - baseline) / 2, z0, 32);
			}
			vecdisp_draw_line(x0 + width / 3, baseline + (ascender - baseline) / 3, x0 + 2 * ( width / 3), baseline + 2 * ( (ascender - baseline) / 3 ), z0);
			break;
		case '1': {
			uint16_t lines[][2] = { {x0 + width / 4, median + ( ascender - median ) / 2},
									{ x0 + 3 * (width / 4), ascender },
									{ x0 + 3 * (width / 4), baseline } };
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case '2': {
			uint16_t lines[][2] = { { x0, median + (ascender - median) / 2 }, 
								{ x0 + width / 3, ascender },
								{ x0 + 2 * (width / 3), ascender },
								{ x0 + width, median + (ascender - median) / 2 },
								{ x0, baseline },
								{ x0 + width, baseline } };
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case '3': {
			uint16_t lines[][2] = { { x0, ascender },
									{ x0 + 2 * (width / 3), ascender },
									{ x0 + width, median + (ascender - median ) / 2 },
									{ x0 + 2 * (width / 3), median },
									{ x0 + width, baseline + (median - baseline) / 2 },
									{ x0 + 2 * (width / 3), baseline },
									{ x0, baseline } };
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case '4': {
			uint16_t lines[][2] = { { x0 + 2 * (width / 3), baseline },
									{ x0 + 2 * (width / 3), ascender },
									{ x0, baseline + ( 2 * ((median - baseline)) / 3) },
									{ x0 + width, baseline + ( 2 * (median - baseline)) / 3 } };
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case '5': { 
			uint16_t lines[][2] = { { x0, baseline },
									{ x0 + 2 * (width / 3), baseline },
									{ x0 + width, baseline + (median - baseline) / 2 },
									{ x0 + 2 * (width / 3), median },
									{ x0, median },
									{ x0, ascender},
									{ x0 + width, ascender } };
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case '6': {
			uint16_t lines[][2] = { { x0, baseline + 2 * ((median - baseline) / 3) },
									{ x0 + width / 2, median },
									{ x0 + width, baseline + 2 * ((median - baseline) / 3) },
									{ x0 + width, baseline + 1 * ((median - baseline) / 3) },
									{ x0 + width / 2, baseline},
									{ x0, baseline + 1 * ((median - baseline) / 3)},
									{ x0, baseline + 2 * ((median - baseline) / 3) },
									{ x0 + width / 2, ascender } };
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case '7': {
				uint16_t lines[][2] = { { x0, ascender },
									{ x0 + 2 * (width/3), ascender },
									{ x0 + width / 2, baseline } };
				vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
				vecdisp_draw_line(x0 + width / 3, median, x0 + 2 * (width / 3), median, z0);
			}
			break;
		case '8': {
				vecdisp_draw_ellipse( x0 + width / 2, baseline + (median - baseline) / 2, width / 2, (median - baseline) / 2, z0, 32 );
				vecdisp_draw_ellipse( x0 + width / 2, median + (ascender - median) / 2, width / 2, (ascender - median) / 2, z0, 32 );
			}
			break;
		case '9': {
			uint16_t lines[][2] = { { x0 + width, median + 1 * (ascender - median) / 3 },
									{ x0 + width, median + 2 * (ascender - median) / 3 },
									{ x0 + width / 2, ascender },
									{ x0, median + 2 * (ascender - median) / 3 },
									{ x0, median + 1 * (ascender - median) / 3 },
									{ x0 + width / 2, median },
									{ x0 + width, median + 1 * (ascender - median) / 3 },
									{ x0 + width / 2, baseline } };
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case ' ': {  }
			break;
		case '.': {
				vecdisp_draw_ellipse(x0 + width / 4, baseline + width / 8, width / 8, width / 8, z0, 48);
			}
			break;
		case ',': {
			uint16_t lines[][2] = { { x0 + (width / 4) + (width / 8), baseline + width / 8 },
									{  x0 + (width / 4), baseline + 2 * (width / 8) },
									{  x0 + (width / 4) - (width / 8), baseline + 1 * (width / 8) },
									{  x0 + (width / 4), baseline },
									{ x0 + (width / 4) + (width / 8), baseline + width / 8 },
									{ x0 + (width / 4), descender + 2* ( (baseline - descender) / 3 ) } };
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			}
			break;
		case '-': {
				vecdisp_draw_line(x0 + width / 4, median, x0 + 3 * ( width / 4 ), median, z0);
			}
			break;
		case '_': {
				vecdisp_draw_line(x0, baseline, x0 + width, baseline, z0);
			}
			break;
		case '/': {
				vecdisp_draw_line(x0 + width / 4, baseline, x0 + 3 * ( width / 4 ), ascender, z0);
			}
			break;
		case '\\': {
			vecdisp_draw_line(x0 + width / 4, ascender, x0 + 3 * ( width / 4 ), baseline, z0);
		}
		break;
		case ':': {
			vecdisp_draw_ellipse(x0 + width / 4, baseline + width / 8, width / 8, width / 8, z0, 48);
			vecdisp_draw_ellipse(x0 + width / 4, median + width / 8, width / 8, width / 8, z0, 48);
		}
		break;
		case '!': {
				vecdisp_draw_line(x0 + width / 2, ascender, x0 + width / 2, baseline + (median - baseline) / 2, z0);
				vecdisp_draw_ellipse(x0 + width / 2, baseline + width / 8, width / 8, width / 8, z0, 48);
			}
			break;
		case '?': {
			uint16_t lines[][2] = { { x0 + width / 2, ascender },
									{ x0 + width, median + 2 * ((ascender - median) / 3) },
									{ x0 + width, median + 1 * ((ascender - median) / 3) },
									{ x0 + width / 2, median },
									{ x0 + width / 2, baseline + 2 * ( ( median - baseline ) / 3 ) } };
			vecdisp_draw_path(lines, sizeof( lines ) / sizeof( lines[0] ), z0);
			vecdisp_draw_ellipse(x0 + width / 2, baseline + width / 8, width / 8, width / 8, z0, 48);
			}
			break;
	}
}

void vecdisp_draw_string(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t z0, char * str, uint16_t margin ) {
	int str_len = strlen(str);
	uint16_t char_width = ( ( x1 - x0 ) / str_len ) - margin;
	char char_upper;
	
	for(int i = 0; i < str_len; i++) {
		x1 = x0 + char_width;
		char_upper = toupper(str[i]);
		vecdisp_draw_char(x0, y0, x1, y1, z0, char_upper);
		x0 = x1 + margin;
	}
}