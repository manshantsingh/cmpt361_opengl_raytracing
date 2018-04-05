/***********************************************************
 * CMPT 361, Spring 2018
 *
 *  raycast.cpp
 *      
 *  Render a simple scene using ray tracing
 * 
 *  NAME: Manshant Singh Kohli
 *  SFU ID: 301257171
 *
 *  Template code for drawing a scene using raycasting.
 *  Some portions of the code was originally written by 
 *  M. vandePanne - and then modified by R. Zhang & H. Li
***********************************************************/

#include "include/Angel.h"

#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <string>

#include "trace.h"
#include "global.h"
#include "sphere.h"
#include "image_util.h"
#include "scene.h"

//
// Global variables
//
// Here we avoid dynamic memory allocation as a convenience. You can
// change the resolution of your rendered image by changing the values
// of WIN_X_SIZE and WIN_Y_SIZE in "global.h", along with other
// global variables
//

int win_width = WIN_WIDTH;
int win_height = WIN_HEIGHT;

GLfloat frame[WIN_HEIGHT][WIN_WIDTH][3];   
// array for the final image 
// This gets displayed in glut window via texture mapping, 
// you can also save a copy as bitmap by pressing 's'

float image_width = IMAGE_WIDTH;
float image_height = (float(WIN_HEIGHT) / float(WIN_WIDTH)) * IMAGE_WIDTH;

// some colors
RGB_float background_clr; // background color
RGB_float null_clr = {0.0, 0.0, 0.0};   // NULL color

//
// these view parameters should be fixed
//
Point eye_pos = {0.0, 0.0, 0.0};  // eye position
float image_plane = -2;           // image plane position

//chessboard
Point chessboard_pos = {0,2.5,0};
Vector chessboard_normal = {0,15,4};

// 0 is black and 1 is white
RGB_float chessboard_ambient[2] = {{0, 0, 0}, {1, 1, 1}};
RGB_float chessboard_diffuse[2] = {{0, 0, 0}, {1, 1, 1}};
RGB_float chessboard_specular[2] = {{0, 0, 0}, {1, 1, 1}};
float chessboard_shineness = 1;
float chessboard_reflectance = 0.5;

// list of spheres in the scene
Spheres *scene = NULL;

// light 1 position and color
Point light1;
float light1_ambient[3];
float light1_diffuse[3];
float light1_specular[3];

// global ambient term
float global_ambient[3];

// light decay parameters
float decay_a;
float decay_b;
float decay_c;

// maximum level of recursions; you can use to control whether reflection
// is implemented and for how many levels
int step_max = 1;

// You can put your flags here
// a flag to indicate whether you want to have shadows
bool shadow_on = false,
	reflection_on = false,
	refraction_on = false,
	chess_board_pattern_on = false,
	stochastic_ray_generation_on = false,
	super_sampling_on = false;


// OpenGL
const int NumPoints = 6;

//----------------------------------------------------------------------------

void init()
{
	// Vertices of a square
	double ext = 1.0;
	vec4 points[NumPoints] = {
		vec4( -ext, -ext,  0, 1.0 ), //v1
		vec4(  ext, -ext,  0, 1.0 ), //v2
		vec4( -ext,  ext,  0, 1.0 ), //v3	
		vec4( -ext,  ext,  0, 1.0 ), //v3	
		vec4(  ext, -ext,  0, 1.0 ), //v2
		vec4(  ext,  ext,  0, 1.0 )  //v4
	};

	// Texture coordinates
	vec2 tex_coords[NumPoints] = {
		vec2( 0.0, 0.0 ),
		vec2( 1.0, 0.0 ),
		vec2( 0.0, 1.0 ),
		vec2( 0.0, 1.0 ),
		vec2( 1.0, 0.0 ),
		vec2( 1.0, 1.0 )
	};

	// Initialize texture objects
	GLuint texture;
	glGenTextures( 1, &texture );

	glBindTexture( GL_TEXTURE_2D, texture );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, WIN_WIDTH, WIN_HEIGHT, 0,
		GL_RGB, GL_FLOAT, frame );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glActiveTexture( GL_TEXTURE0 );

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers( 1, &buffer );
	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(tex_coords), NULL, GL_STATIC_DRAW );
	GLintptr offset = 0;
	glBufferSubData( GL_ARRAY_BUFFER, offset, sizeof(points), points );
	offset += sizeof(points);
	glBufferSubData( GL_ARRAY_BUFFER, offset, sizeof(tex_coords), tex_coords );

	// Load shaders and use the resulting shader program
	GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
	glUseProgram( program );

	// set up vertex arrays
	offset = 0;
	GLuint vPosition = glGetAttribLocation( program, "vPosition" );
	glEnableVertexAttribArray( vPosition );
	glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(offset) );

	offset += sizeof(points);
	GLuint vTexCoord = glGetAttribLocation( program, "vTexCoord" ); 
	glEnableVertexAttribArray( vTexCoord );
	glVertexAttribPointer( vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(offset) );

	glUniform1i( glGetUniformLocation(program, "texture"), 0 );

	glClearColor( 1.0, 1.0, 1.0, 1.0 );
}

/*********************************************************
 * This is the OpenGL display function. It is called by
 * the event handler to draw the scene after you have
 * rendered the image using ray tracing. Remember that
 * the pointer to the image memory is stored in 'frame'.
 *
 * There is no need to change this.
 **********************************************************/

void display( void )
{
	glClear( GL_COLOR_BUFFER_BIT );
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);

	glDrawArrays( GL_TRIANGLES, 0, NumPoints );

	glutSwapBuffers();
}

/*********************************************************
 * This function handles keypresses
 *
 *   s - save image
 *   q - quit
 *
 * DO NOT CHANGE
 *********************************************************/
void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 'q':case 'Q':
		free(scene);
		exit(0);
		break;
	case 's':case 'S':
		save_image();
		glutPostRedisplay();
		break;
	default:
		break;
	}
}



//----------------------------------------------------------------------------

int main( int argc, char **argv )
{
	// Parse the arguments
	if (argc < 3) {
		printf("Missing arguments ... use:\n");
		printf("./raycast [-u | -d] step_max <options>\n");
		return -1;
	}
	
	if (strcmp(argv[1], "-u") == 0) {  // user defined scene
		set_up_user_scene();
	} else { // default scene
		set_up_default_scene();
	}

	step_max = atoi(argv[2]); // maximum level of recursions

	// Optional arguments
	for(int i = 3; i < argc; i++)
	{
		std::string a = argv[i];
		if(a == "+s") shadow_on = true;
		else if(a == "+l") reflection_on = true;
		else if(a == "+r") refraction_on = true;
		else if(a == "+c") chess_board_pattern_on = true;
		else if(a == "+f") stochastic_ray_generation_on = true;
		else if(a == "+p") super_sampling_on = true;
	}

	//
	// ray trace the scene now
	//
	// we have used so many global variables and this function is
	// happy to carry no parameters
	//
	printf("Rendering scene using my fantastic ray tracer ...\n");
	normalize(&chessboard_normal);
	ray_trace();

	// we want to make sure that intensity values are normalized
	histogram_normalization();

	// Show the result in glut via texture mapping
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE );
	glutInitWindowSize( WIN_WIDTH, WIN_HEIGHT );
	glutCreateWindow( "Ray tracing" );
	glewInit();
	init();

	glutDisplayFunc( display );
	glutKeyboardFunc( keyboard );
	glutMainLoop();
	return 0;
}
