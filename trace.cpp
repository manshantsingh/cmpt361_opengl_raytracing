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


extern Point chessboard_pos;
extern Vector chessboard_normal;

extern RGB_float chessboard_ambient[];
extern RGB_float chessboard_diffuse[];
extern RGB_float chessboard_specular[];  
extern float chessboard_shineness;
extern float chessboard_reflectance;

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

Vector normalized(Vector v){
  normalize(&v);
  return v;
}



bool intersect_chessboard(Point o, Vector u, Point& hit){
  Vector toPlane = get_vec(chessboard_pos, o);
  float toPlaneDotChessBoardNormal = vec_dot(chessboard_normal, toPlane);
  float uDotChessBoardNormal = vec_dot(chessboard_normal, u);

  if(toPlaneDotChessBoardNormal==0 || uDotChessBoardNormal==0) return false;

  float r = toPlaneDotChessBoardNormal/uDotChessBoardNormal;
  if(r<=0) return false;

  Vector v = vec_scale(u, r);
  hit = get_point(o, v);
  return fabs(hit.x)<4.0 && hit.z <-2 && hit.z >= -10;
}

RGB_float phongChessboard(Point q, Vector v, int color_index){

  //AMBIENT GLOBAL
  RGB_float ambient = {
    chessboard_ambient[color_index].r *2*global_ambient[0]*chessboard_reflectance,
    chessboard_ambient[color_index].g *2*global_ambient[1]*chessboard_reflectance,
    chessboard_ambient[color_index].b *2*global_ambient[2]*chessboard_reflectance
  };

  Vector Shadow = get_vec(q, light1);
  if(shadow_on && intersect_shadow(q, Shadow, scene)) return ambient;

  Vector light = normalized(get_vec(q, light1));

  float decay = 1.0/( decay_a + decay_b * vec_len(light) + decay_c*pow(vec_len(light), 2));

  float lDotChessNormal = vec_dot(chessboard_normal, light);
  RGB_float diffuse = {
    light1_diffuse[0] * chessboard_diffuse[color_index].r * lDotChessNormal,
    light1_diffuse[1] * chessboard_diffuse[color_index].g * lDotChessNormal,
    light1_diffuse[2] * chessboard_diffuse[color_index].b * lDotChessNormal
  };

  float ref_scalar = 2.0*(vec_dot(chessboard_normal, light));
  Vector h = vec_plus(vec_scale(chessboard_normal, ref_scalar), vec_scale(light, -1.0));
  float s = pow(vec_dot(v, h), chessboard_shineness);
  RGB_float specular = {
    light1_specular[0]*chessboard_specular[color_index].r*s,
    light1_specular[1]*chessboard_specular[color_index].g*s,
    light1_specular[2]*chessboard_specular[color_index].b*s
  };

  RGB_float color = clr_add(diffuse, specular);
  color = clr_scale(color, decay);
  color = clr_add(color, ambient);

  return color;
}

RGB_float getBoardColor(Point hit){
  Vector eye_vec = normalized(get_vec(hit, eye_pos));
  if((((int)hit.x)%2 == 0 && ((int)hit.z)%2==0) || (((int)hit.x)%2!=0 && ((int)hit.z)%2!=0)){
    return phongChessboard(hit, eye_vec, hit.x>0?1:0);
  }
  return phongChessboard(hit, eye_vec, hit.x>0?0:1);
}



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
    color.r = global_ambient[0];
    color.g = global_ambient[1];
    color.b = global_ambient[2];
  }
  else{
    float coef = 1/(decay_a + decay_b*d + decay_c*d*d);
    float dot = vec_dot(surf_norm, lightVec);
    Vector reflectedRay = normalized(vec_minus(vec_scale(surf_norm, 2*fmax(dot, 0.0)), lightVec));
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

RGB_float recursive_ray_trace(Point o, Vector ray, int nSteps) {
//
// do your thing here
//
  nSteps--;
	RGB_float color = background_clr;

  Point hit;
  Spheres * closest = intersect_scene(eye_pos, ray, scene, &hit);

  if(closest){
    Vector surf_norm = normalized(sphere_normal(hit, closest));
    Vector lightVec = normalized(get_vec(hit, o));

    color = phong(hit, normalized(get_vec(hit, eye_pos)), surf_norm, closest);

    if(reflection_on && nSteps >= 0){
      Vector reflectedRay = normalized(vec_minus(vec_scale(surf_norm, 2*fmax(vec_dot(surf_norm, lightVec), 0.0)), lightVec));

      RGB_float reflectedColor = recursive_ray_trace(hit, reflectedRay, nSteps);

      if(stochastic_ray_generation_on){
        for(int i=0; i<NUM_RANDOM_RAYS; i++){
          Vector randomRay = reflectedRay;
          randomRay.x += ((float) rand())/RAND_MAX;
          randomRay.y += ((float) rand())/RAND_MAX;
          randomRay.z += ((float) rand())/RAND_MAX;
          normalize(&randomRay);
          RGB_float randomReflectedColor = recursive_ray_trace(hit, reflectedRay, nSteps);
          reflectedColor = clr_add(reflectedColor, clr_scale(randomReflectedColor, RANDOM_RAY_SCALE_VALUE));
        }
        reflectedColor = clr_scale(reflectedColor, 1/( 1 + RANDOM_RAY_SCALE_VALUE*NUM_RANDOM_RAYS));
      }

      color = clr_add(color, clr_scale(reflectedColor, closest->reflectance));
    }
  }
  else if(chess_board_pattern_on && intersect_chessboard(o, ray, hit)){
    Vector lightVec = normalized(get_vec(hit, o));

    color = getBoardColor(hit);

    if(reflection_on && nSteps >= 0){
      Vector reflectedRay = normalized(vec_minus(vec_scale(chessboard_normal, 2*fmax(vec_dot(chessboard_normal, lightVec), 0.0)), lightVec));

      RGB_float reflectedColor = recursive_ray_trace(hit, reflectedRay, nSteps);

      if(stochastic_ray_generation_on){
        for(int i=0; i<NUM_RANDOM_RAYS; i++){
          Vector randomRay = reflectedRay;
          randomRay.x += ((float) rand())/RAND_MAX;
          randomRay.y += ((float) rand())/RAND_MAX;
          randomRay.z += ((float) rand())/RAND_MAX;
          normalize(&randomRay);
          RGB_float randomReflectedColor = recursive_ray_trace(hit, reflectedRay, nSteps);
          reflectedColor = clr_add(reflectedColor, clr_scale(randomReflectedColor, RANDOM_RAY_SCALE_VALUE));
        }
        reflectedColor = clr_scale(reflectedColor, 1/( 1 + RANDOM_RAY_SCALE_VALUE*NUM_RANDOM_RAYS));
      }

      color = clr_add(color, clr_scale(reflectedColor, chessboard_reflectance));
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
      ray = normalized(get_vec(eye_pos, cur_pixel_pos));

      ret_color = recursive_ray_trace(eye_pos, ray, step_max);

      if(super_sampling_on){
        for(int k=0;k<4;k++){
          Point pos = cur_pixel_pos;
          pos.x += dx[k] * x_grid_size * 0.5;
          pos.y += dy[k] * x_grid_size * 0.5;

          Vector new_ray = get_vec(eye_pos, pos);
          ret_color = clr_add(ret_color, recursive_ray_trace(eye_pos, new_ray, step_max));
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
