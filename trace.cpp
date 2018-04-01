#include <cstdio>
#include <ctime>
#include <GL/glut.h>
#include <cmath>
#include "global.h"
#include "sphere.h"

//
// Global variables
//
extern int win_width;
extern int win_height;

extern GLfloat frame[WIN_HEIGHT][WIN_WIDTH][3];  

extern float image_width;
extern float image_height;

extern Point eye_pos;
extern float image_plane;
extern RGB_float background_clr;
extern RGB_float null_clr;

extern Spheres *scene;

// light 1 position and color
extern Point light1;
extern float light1_ambient[3];
extern float light1_diffuse[3];
extern float light1_specular[3];

// global ambient term
extern float global_ambient[3];

// light decay parameters
extern float decay_a;
extern float decay_b;
extern float decay_c;

extern int step_max;

extern bool shadow_on;
extern bool reflection_on;
extern bool refraction_on;
extern bool chess_board_pattern_on;
extern bool stochastic_ray_generation_on;
extern bool super_sampling_on;

/////////////////////////////////////////////////////////////////////

/*********************************************************************
 * Phong illumination - you need to implement this!
 *********************************************************************/
RGB_float phong(Point q, Vector v, Vector surf_norm, Spheres *sph) {
//
// do your thing here
//
  Vector lightVec = get_vec(q, light1);
  float d = vec_len(lightVec);
  normalize(&lightVec);

  RGB_float color{0,0,0};

  // change this for shadow
  if(shadow_on && intersect_shadow(q, lightVec, scene)){
    color.r = global_ambient[0] * sph->reflectance;
    color.g = global_ambient[1] * sph->reflectance;
    color.b = global_ambient[2] * sph->reflectance;
  }
  else{
    float coef = 1/(decay_a + decay_b*d + decay_c*d*d);
    float dot = vec_dot(surf_norm, lightVec);
    Vector reflectedRay = vec_minus(vec_scale(surf_norm, 2*fmax(dot, 0.0)), lightVec);
    normalize(&reflectedRay);
    float p = pow(vec_dot(reflectedRay, v), sph->mat_shineness);

    color.r += light1_ambient[0] * sph->mat_ambient[0];
    color.g += light1_ambient[1] * sph->mat_ambient[1];
    color.b += light1_ambient[2] * sph->mat_ambient[2];

    color.r += coef *
                (light1_diffuse[0] * sph->mat_diffuse[0] * dot +
                  light1_specular[0] * sph->mat_specular[0] * p);
    color.g += coef *
                (light1_diffuse[1] * sph->mat_diffuse[1] * dot +
                  light1_specular[1] * sph->mat_specular[1] * p);
    color.b += coef *
                (light1_diffuse[2] * sph->mat_diffuse[2] * dot +
                  light1_specular[2] * sph->mat_specular[2] * p);
  }
	return color;
}

void fix_color(RGB_float & color){
  if(color.r < 0.0) color.r = 0.0;
  else if(color.r > 1.0) color.r = 1.0;
  if(color.g < 0.0) color.g = 0.0;
  else if(color.g > 1.0) color.g = 1.0;
  if(color.b < 0.0) color.b = 0.0;
  else if(color.b > 1.0) color.b = 1.0;
}

/************************************************************************
 * This is the recursive ray tracer - you need to implement this!
 * You should decide what arguments to use.
 ************************************************************************/
const int NUM_RANDOM_RAYS = 5;
const float RANDOM_RAY_SCALE_VALUE = 0.1;

RGB_float recursive_ray_trace(Vector ray, int nSteps) {
//
// do your thing here
//
	RGB_float color = background_clr;

  Point hit;
  Spheres * closest = intersect_scene(eye_pos, ray, scene, &hit);

  if(closest){
    Vector eye_vec = get_vec(hit, eye_pos);
    normalize(&eye_vec);

    Vector surf_norm = sphere_normal(hit, closest);
    normalize(&surf_norm);

    Vector lightVec = get_vec(hit, eye_pos);
    normalize(&lightVec);

    color = phong(hit, eye_vec, surf_norm, closest);

    if(reflection_on && nSteps <= step_max){
      Vector reflectedRay = vec_minus(vec_scale(surf_norm, 2*fmax(vec_dot(surf_norm, lightVec), 0.0)), lightVec);
      normalize(&reflectedRay);

      RGB_float reflectedColor = recursive_ray_trace(reflectedRay, nSteps + 1);

      if(stochastic_ray_generation_on){
        for(int i=0; i<NUM_RANDOM_RAYS; i++){
          Vector randomRay = reflectedRay;
          randomRay.x += ((float) rand())/RAND_MAX;
          randomRay.y += ((float) rand())/RAND_MAX;
          randomRay.z += ((float) rand())/RAND_MAX;
          normalize(&randomRay);
          RGB_float randomReflectedColor = recursive_ray_trace(reflectedRay, nSteps + 1);
          reflectedColor = clr_add(reflectedColor, clr_scale(randomReflectedColor, RANDOM_RAY_SCALE_VALUE));
        }
        reflectedColor = clr_scale(reflectedColor, 1/( 1 + RANDOM_RAY_SCALE_VALUE*NUM_RANDOM_RAYS));
      }
    }
  }

  fix_color(color);
  return color;
}

/*********************************************************************
 * This function traverses all the pixels and cast rays. It calls the
 * recursive ray tracer and assign return color to frame
 *
 * You should not need to change it except for the call to the recursive
 * ray tracer. Feel free to change other parts of the function however,
 * if you must.
 *********************************************************************/
const int dx[] = {-1,-1, 1, 1};
const int dy[] = {-1, 1,-1, 1};

void ray_trace() {
  srand (time(NULL));
  int i, j;
  float x_grid_size = image_width / float(win_width);
  float y_grid_size = image_height / float(win_height);
  float x_start = -0.5 * image_width;
  float y_start = -0.5 * image_height;
  RGB_float ret_color;
  Point cur_pixel_pos;
  Vector ray;

  // ray is cast through center of pixel
  cur_pixel_pos.x = x_start + 0.5 * x_grid_size;
  cur_pixel_pos.y = y_start + 0.5 * y_grid_size;
  cur_pixel_pos.z = image_plane;

  for (i=0; i<win_height; i++) {
    for (j=0; j<win_width; j++) {
      ray = get_vec(eye_pos, cur_pixel_pos);
      normalize(&ray);

      ret_color = recursive_ray_trace(ray, step_max);

      if(super_sampling_on){
        for(int k=0;k<4;k++){
          Point pos = cur_pixel_pos;
          pos.x += dx[k] * x_grid_size * 0.5;
          pos.y += dy[k] * x_grid_size * 0.5;

          Vector new_ray = get_vec(eye_pos, pos);
          ret_color = clr_add(ret_color, recursive_ray_trace(new_ray, step_max));
        }
        ret_color = clr_scale(ret_color, 0.2);
      }

      frame[i][j][0] = GLfloat(ret_color.r);
      frame[i][j][1] = GLfloat(ret_color.g);
      frame[i][j][2] = GLfloat(ret_color.b);

      cur_pixel_pos.x += x_grid_size;
    }

    cur_pixel_pos.y += y_grid_size;
    cur_pixel_pos.x = x_start;
  }
}
