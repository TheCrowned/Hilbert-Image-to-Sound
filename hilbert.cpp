#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h> //sleep function

//IMAGE PROCESS
//https://github.com/nothings/stb/blob/master/tests/image_test.c
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//SOUNDS PROCESS
#include "sounds.h"

//Hilbert algorithm expects a Black and White square, power of two image of any size. We expect a 256x256 for performance/sound reasons.
int width = 256;
int height = 256;
//To turn any image into a supported one, use imagemagick with the following command:
//convert test.jpg -resize 256x256! -colorspace Gray test2.jpg

//How many tones we want to use. We use notes from B7 (3951Hz) to A2 (110Hz), a total of 52 tones.
int tones_number = 52;

void rot(int , int& , int& , int , int );
int xy2d (int , int , int );
char* read_file (char* );
void print_matrix (uint8_t** );
uint8_t* read_image (const char* );
uint8_t** arrange_image_in_matrix(uint8_t* );
uint8_t* unroll_matrix_through_hilbert_curve(uint8_t** );
uint8_t* means_vector(uint8_t* );

int main( int argc, char *argv[] ) {
	const char* file_name = "test.jpg";
	uint8_t* image = read_image( file_name );
	uint8_t** matrix = arrange_image_in_matrix( image );
    
    printf( "\n- Assuming square, black and white, 256px input matrix -\n");
    sleep(2);
    
    printf( "\nAcquiring image matrix...\n");
    sleep(2);
	stbi_image_free ( image ); //free some memory
	printf("ACQUIRED IMAGE MATRIX!\n\n"); 
	
    printf( "\nUnrolling image matrix thrugh a Hilbert curve...\n");
    sleep(2);
	uint8_t* unrolled = unroll_matrix_through_hilbert_curve( matrix );

	//Free some more memory
	for( int i = 0; i < height; i++ ) {
		free( matrix[i] );
	}
	free( matrix );
    
	printf( "UNROLLED IMAGE THROUGH HILBERT CURVE!\n\n" );	
	
    printf( "\nCalculating means vector...\n");
    sleep(2);
	uint8_t* means = means_vector( unrolled );
    printf( "CALCULATED MEANS VECTOR!\n\n");

    printf( "\nWriting output audio file...\n");
    sleep(2);    
	output_sounds(means, tones_number);
    printf( "WRITTEN AUDIO FILE!\n\n");
	return 0;
}

/*
 * Reads the image top-to-bottom, row by row.
 * An unsigned short (aka uint8_t) holds values from 0 to 255. 256 is truncated to 0 already.
 * width and height then hold the dimension of the image, while bpp the number of channels in the input (1 for black and white).
 */ 
uint8_t* read_image( const char* file_name ) {
	int bpp;

    uint8_t* read_bn_image = stbi_load( file_name, &width, &height, &bpp, 1 ); //1 is for black and white

	return read_bn_image;
}

/*
 * Allocate and populate the matrix.
 * It just basically reads the vector breaking it into new lines every _height_ items.
 */
uint8_t** arrange_image_in_matrix( uint8_t* image_array ) {
	uint8_t** matrix = (uint8_t**) malloc( width * sizeof( uint8_t* ) );
	int pos, i, j; 
	for( i = 0; i < width; i++ ) {
		matrix[i] = (uint8_t*) malloc( height * sizeof( uint8_t ) );

		for( j = 0; j < height; j++ ) {
			pos = i * width  + j;
			matrix[i][j] = image_array[pos]; //populate
		}
		
	}

	return matrix;
}

/*
 * Cycle through matrix and map each point to an integer with H curve.
 * Hilbert functions expect entry (0,0) to be at bottom-left and (n-1, n-1) to be at top-right (x growing right, y upwards).
 * For that reason, we need first to flip the image: up goes down and down goes up.
 * This is not enough, because output appears flipped on main diagonal. On calling xy2d, we thus flip coordinates to flip through diagonal.
 */ 
uint8_t* unroll_matrix_through_hilbert_curve( uint8_t** matrix ) {
	uint8_t* unrolled = (uint8_t*) malloc( width*height * sizeof( uint8_t ) ); //output array

	for( int i = 0; i < width*height; i++ ) //assign all indexes to -1, so it's easier to spot if some indexes get non-written
		unrolled[i] = -1;

	int pos;
	for( unsigned int i = 0; i < width; i++ ) {
		for( unsigned int j = 0; j < height; j++ ) {
			int k = width - 1 - i; //flip image
			pos = xy2d( width, j, k ); //call will coordinates flip
			
			unrolled[pos] = matrix[i][j];
		}
	}

	return unrolled;
}

/*
 * Can't use a tone for each pixel, so we calculate averages for an appropriate number of pixels.
 * End up with a means array with a number of entries equal to tones_number.
 */ 
uint8_t* means_vector( uint8_t* unrolled_image ) {
	int size = width*height;
	int px_per_block = size/tones_number;

	uint8_t* means = (uint8_t*) malloc( tones_number * sizeof( uint8_t ) );
	int sum = 0, pos = 0;
	for( int i = 0; i < size; i++ ) {
		pos = i % px_per_block;
		
		if( pos == 0 )
			sum = 0; //reset mean sum

		sum += unrolled_image[i];

		if( pos == px_per_block - 1 )
			means[i/px_per_block] = sum/px_per_block;
	}

	return means;
}

/*
 * Print a matrix.
 */ 
void print_matrix( uint8_t** matrix ) {
	for( int i = 0; i < width; i++ ) {
		for( int j = 0; j < height; j++ ) {
			printf( "%d ", matrix[i][j] );
		}
		printf("\n");
	}
}

/*
 * Convert a 2D point (x,y) to its integet Hilbert distance.
 * Code from https://en.wikipedia.org/wiki/Hilbert_curve. One of the most ingenious pieces of code I have ever seen!
 * See https://marcin-chwedczuk.github.io/iterative-algorithm-for-drawing-hilbert-curve for go-through explanation.
 */ 
int xy2d ( int n, int x, int y ) {
    int rx, ry, s, d = 0;
    for( s = n/2; s > 0; s /= 2 ) {
        rx = ( x & s ) > 0;
        ry = ( y & s ) > 0;
        
        d += s * s * ( ( 3 * rx ) ^ ry );

        rot( s, x, y, rx, ry );
    }
    
    return d;
}


/*
 * Rotate/flip a quadrant appropriately.
 */ 
void rot(int n, int &x, int &y, int rx, int ry) {
    if (ry == 0) {
        if (rx == 1) {
            x = n-1 - x;
            y = n-1 - y;
            
        }

        //Swap x and y
        int t  = x;
        x = y;
        y = t;
    }
}
