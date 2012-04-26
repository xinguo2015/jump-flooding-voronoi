/*=================================================================================================
  Author: Renato Farias (renatomdf@gmail.com)
  Created on: April 13th, 2012
  Purpose: A CPU implementation of the Jump Flooding Algorithm from the paper "Jump Flooding in
   GPU With Applications to Voronoi Diagram and Distance Transform" [Rong 2006]. The result is
   a Voronoi diagram generated from a number of seeds which the user provides with mouse clicks.
=================================================================================================*/

/*=================================================================================================
  INCLUDES
=================================================================================================*/

#include <GL/glut.h>

#include <assert.h>
#include <stdio.h>
#include <vector>

using namespace std;

/*=================================================================================================
  DEFINES
=================================================================================================*/

// Initial window dimensions
#define INIT_WINDOW_WIDTH  1024
#define INIT_WINDOW_HEIGHT  768

// Initial screen position
#define INIT_WINDOW_POS_X 0
#define INIT_WINDOW_POS_Y 0

/*=================================================================================================
  STRUCTS
=================================================================================================*/

// Represents a pixel with position (x,y)
typedef struct {
	int x,y;
} Point;

/*=================================================================================================
  GLOBALS
=================================================================================================*/

// Current window dimensions
int WindowWidth  = INIT_WINDOW_WIDTH;
int WindowHeight = INIT_WINDOW_HEIGHT;

// List of seeds
vector<Point> Seeds;

// Buffers
Point* BufferA = NULL;
Point* BufferB = NULL;

// Which buffer do we read from?
bool UsingBufferA = true;

/*=================================================================================================
  FUNCTIONS
=================================================================================================*/

// Jump Flooding Algorithm
void ExecuteJumpFlooding( void ) {

	printf( "Executing the Jump Flooding Algorithm...\n" );

	assert( WindowWidth == INIT_WINDOW_WIDTH && WindowHeight == INIT_WINDOW_HEIGHT );

	// If the buffers already exist, delete them
	if( BufferA != NULL ) {
		free( BufferA );
		BufferA = NULL;
	}
	if( BufferB != NULL ) {
		free( BufferB );
		BufferB = NULL;
	}

	// Allocate memory for the two buffers
	BufferA = (Point*)malloc( sizeof( Point ) * INIT_WINDOW_WIDTH * INIT_WINDOW_HEIGHT );
	BufferB = (Point*)malloc( sizeof( Point ) * INIT_WINDOW_WIDTH * INIT_WINDOW_HEIGHT );

	assert( BufferA != NULL && BufferB != NULL );

	// Initialize BufferA with (-1,-1), indicating an invalid closest seed.
	// We don't need to initialize BufferB because it will be written to in the first round.
	for( int j = 0; j < INIT_WINDOW_HEIGHT; ++j ) {
		for( int i = 0; i < INIT_WINDOW_WIDTH; ++i ) {
			int idx = j*INIT_WINDOW_WIDTH + i;
			BufferA[ idx ].x = -1;
			BufferA[ idx ].y = -1;
		}
	}

	// Put the seeds into the first buffer
	for( int i = 0; i < Seeds.size(); ++i ) {
		Point p = Seeds[i];
		BufferA[ p.y*INIT_WINDOW_WIDTH + p.x ] = p;
	}

	// Initial step length is half the image's size. If the image isn't square,
	// we use the largest dimension.
	int step;
	if( INIT_WINDOW_WIDTH >= INIT_WINDOW_HEIGHT )
		step = INIT_WINDOW_WIDTH/2;
	else
		step = INIT_WINDOW_HEIGHT/2;

	// We use this boolean to know which buffer we are reading from
	UsingBufferA = true;

	// We read from the RBuffer and write into the WBuffer
	Point* RBuffer;
	Point* WBuffer;

	// Carry out the rounds of Jump Flooding
	while( step >= 1 ) {

		printf( "Jump Flooding with Step %i.", step );

		// Set which buffers we'll be using
		if( UsingBufferA == true ) {
			printf( " Reading from BufferA and writing to BufferB.\n" );
			RBuffer = BufferA;
			WBuffer = BufferB;
		}
		else {
			printf( " Reading from BufferB and writing to BufferA.\n" );
			RBuffer = BufferB;
			WBuffer = BufferA;
		}

		// Iterate over each pixel to find its closest seed
		for( int j = 0; j < INIT_WINDOW_HEIGHT; ++j ) {
			for( int i = 0; i < INIT_WINDOW_WIDTH; ++i ) {

				// The pixel's absolute index
				int idx = j*INIT_WINDOW_WIDTH + i;

				// The pixel's current closest seed (if any)
				Point& p = RBuffer[ idx ];

				// Go ahead and write our current closest seed, if any. If we don't do this
				// we might lose this information if we don't update our seed this round.
				WBuffer[ idx ] = p;

				// This is a seed, so skip this pixel
				if( p.x == i && p.y == j )
					continue;

				// dist will be used to judge which seed is closest
				float dist;

				if( p.x == -1 || p.y == -1 )
					dist = -1; // No closest seed has been found yet
				else
					dist = (p.x-i)*(p.x-i) + (p.y-j)*(p.y-j); // Current closest seed's distance

				// To find each pixel's closest seed, we look in its 8 neighbors thusly:
				//   (x-step,y-step) (x,y-step) (x+step,y-step)
				//   (x-step,y     ) (x,y     ) (x+step,y     )
				//   (x-step,y+step) (x,y+step) (x+step,y+step)

				for( int kj = -1; kj <= 1; ++kj ) {
					for( int ki = -1; ki <= 1; ++ki ) {

						// Calculate neighbor's row and column
						int nj = j + kj * step;
						int ni = i + ki * step;

						// If the neighbor is outside the bounds of the image, skip it
						if( ni < 0 || ni >= INIT_WINDOW_WIDTH || nj < 0 || nj >= INIT_WINDOW_HEIGHT )
							continue;

						// Calculate neighbor's absolute index
						int nidx = nj*INIT_WINDOW_WIDTH + ni;

						// Retrieve the neighbor
						Point& pk = RBuffer[ nidx ];

						// If the neighbor doesn't have a closest seed yet, skip it
						if( pk.x == -1 || pk.y == -1 )
							continue;

						// Calculate the distance from us to the neighbor's closest seed
						float newDist = (pk.x-i)*(pk.x-i) + (pk.y-j)*(pk.y-j);

						// If dist is -1, it means we have no closest seed, so we might as well take this one
						// Otherwise, only adopt this new seed if it's closer than our current closest seed
						if( dist == -1 || newDist < dist ) {
							WBuffer[ idx ] = pk;
							dist = newDist;
						}

					}
				}

			}
		}

		// Halve the step.
		step /= 2;

		// Swap the buffers for the next round
		UsingBufferA = !UsingBufferA;

	}

	// Lets us know which buffer we end up reading from
	if( UsingBufferA == true )
		printf( "Reading from BufferA.\n" );
	else
		printf( "Reading from BufferB.\n" );

}

// Renders the next frame and puts it on the display
void DisplayFunc( void ) {

	// Clear the window.
	glClear( GL_COLOR_BUFFER_BIT );

	// Draw the buffer, if possible
	Point* Buffer = UsingBufferA == true ? BufferA : BufferB;
	if( Buffer != NULL ) {

		glPointSize( 1 );
		glBegin( GL_POINTS );
			for( int j = 0; j < INIT_WINDOW_HEIGHT; ++j ) {
				for( int i = 0; i < INIT_WINDOW_WIDTH; ++i ) {
					int idx = j*INIT_WINDOW_WIDTH + i;
					glColor3f( (float)Buffer[idx].x/INIT_WINDOW_WIDTH,
					           (float)Buffer[idx].y/INIT_WINDOW_HEIGHT,
					           //(float)Buffer[idx].y/INIT_WINDOW_HEIGHT );
					           0.0f );
					glVertex2i( i, j );
				}
			}
		glEnd();

	}

	// Draw the seeds
	glColor3f( 0.0f, 0.0f, 1.0f );
	glPointSize( 8 );
	glBegin( GL_POINTS );
		for( int i = 0; i < Seeds.size(); ++i ) {
			glVertex2i( Seeds[i].x, Seeds[i].y );
		}
	glEnd();

	// Swap the buffers, flushing to screen.
	glutSwapBuffers();

}

// Called when there is nothing to do
void IdleFunc( void ) {

	// Uncomment the following to repeatedly update the window
	glutPostRedisplay();

}

// Makes the necessary adjustments if the window is resized
void ReshapeFunc( int width, int height ) {

	// Adjust the viewport
	glViewport( 0, 0, (GLsizei)width, (GLsizei)height );

	// Update current window dimensions
	WindowWidth  = width;
	WindowHeight = height;

}

// Called when a (ASCII) keyboard key is pressed
void KeyboardFunc( unsigned char key, int x, int y ) {  

	switch( key ) {
		// ESC exits the program
		case 27:
			exit(0);

		// Switch from which buffer we're reading from
		// In effect, this lets us see the second-to-last iteration
		case 'b':
			UsingBufferA = !UsingBufferA;
			if( UsingBufferA == true )
				printf( "Reading from BufferA.\n" );
			else
				printf( "Reading from BufferB.\n" );
			break;

		// Clear Seeds and buffers
		case 'c':
			Seeds.clear();

			if( BufferA != NULL ) {
				free( BufferA );
				BufferA = NULL;
			}

			if( BufferB != NULL ) {
				free( BufferB );
				BufferB = NULL;
			}

			printf( "Clear.\n" );
			break;

		// Execute the algorithm
		case 'e':
			ExecuteJumpFlooding();
			break;
	}
}

// Called when a mouse button is pressed or released
void MouseFunc( int button, int state, int x, int y ) {

	// Key 0: left button
	// Key 1: middle button
	// Key 2: right button
	// Key 3: scroll up
	// Key 4: scroll down

	if( button == 0 && state == GLUT_DOWN ) {

		Point* Buffer = UsingBufferA == true ? BufferA : BufferB;

		if( Buffer != NULL ) {

			Point& p = Buffer[ y*INIT_WINDOW_WIDTH + x ];

			printf( "The closest seed to pixel (%i,%i) is located at (%i,%i).\n", x, y, p.x, p.y );

		}
		else {

			printf( "Creating new seed at (%i,%i).", x, y );

			Point p = { x, y };
			Seeds.push_back( p );

			printf( " %zi seeds total.\n", Seeds.size() );

		}

	}

}

// Initializes variables and OpenGL settings
void Initialize( void ) {

	// Set the background color to white
	glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );

	// Set up orthogonal projection with (0,0) at the top-left corner of the window
	glOrtho( 0, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, 0, -1 ,1 );

}

// Where it all begins...
int main( int argc, char **argv ) {

	// Initialize GLUT and window properties
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
	glutInitWindowSize( INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT );
	glutInitWindowPosition( INIT_WINDOW_POS_X, INIT_WINDOW_POS_Y );

	// Create main OpenGL window
	glutCreateWindow( "Jump Flooding Voronoi" );

	// Initialize variables and settings
	Initialize();

	// Register callback functions
	glutDisplayFunc( DisplayFunc );
	glutIdleFunc( IdleFunc );
	glutReshapeFunc( ReshapeFunc );
	glutKeyboardFunc( KeyboardFunc );
	glutMouseFunc( MouseFunc );

	// Enter the main loop
	glutMainLoop();

	return 0;

}
