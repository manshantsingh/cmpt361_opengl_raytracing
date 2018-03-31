#include "sphere.h"
#include <cstdlib>
#include <cmath>
#include <cfloat>

/**********************************************************************
 * This function intersects a ray with a given sphere 'sph'. You should
 * use the parametric representation of a line and do the intersection.
 * The function should return the parameter value for the intersection, 
 * which will be compared with others to determine which intersection
 * is closest. The value -1.0 is returned if there is no intersection
 *
 * If there is an intersection, the point of intersection should be
 * stored in the "hit" variable
 **********************************************************************/
float intersect_sphere(Point o, Vector u, Spheres *sph, Point *hit) {
  Vector dist = get_vec(sph->center, o);

  float a = vec_dot(u,u);
  float b = 2 * vec_dot(u, dist);
  float c = vec_dot(dist, dist) - (sph->radius * sph->radius);

  float d = b*b - 4*a*c;

  if(d >= 0.0){
    float t = (-b - sqrt(d))/(2*a);
    if(t>0){
      *hit = get_point(o, vec_scale(u, t));
      return t;
    }
  }
	return -1.0;
}

bool intersect_shadow(Point o, Vector u, Spheres * sph){
  normalize(&u);
  while(sph){
    Vector dist = get_vec(sph->center, o);
    float a = vec_dot(u,u);
    float b = 2 * vec_dot(u, dist);
    float c = vec_dot(dist, dist) - (sph->radius * sph->radius);
    float d = b*b - 4*a*c;
    if(d >= 0.0 && (-b - sqrt(d))/(2*a) > 0.0 && (-b + sqrt(d))/(2*a) > 0.0){
      return true;
    }
    sph = sph->next;
  }
  return false;
}

/*********************************************************************
 * This function returns a pointer to the sphere object that the
 * ray intersects first; NULL if no intersection. You should decide
 * which arguments to use for the function. For exmaple, note that you
 * should return the point of intersection to the calling function.
 **********************************************************************/
Spheres *intersect_scene(Point o, Vector u, Spheres *sph, Point *hit) {
  Spheres * closest = nullptr;
  float min_dist = FLT_MAX;
  while(sph){
    float dist = intersect_sphere(o,u,sph,hit);
    if(dist != -1.0 && dist < min_dist){
      closest=sph;
      min_dist=dist;
    }
    sph = sph->next;
  }
	return closest;
}

/*****************************************************
 * This function adds a sphere into the sphere list
 *
 * You need not change this.
 *****************************************************/
Spheres *add_sphere(Spheres *slist, Point ctr, float rad, float amb[],
		    float dif[], float spe[], float shine, 
		    float refl, int sindex) {
  Spheres *new_sphere;

  new_sphere = (Spheres *)malloc(sizeof(Spheres));
  new_sphere->index = sindex;
  new_sphere->center = ctr;
  new_sphere->radius = rad;
  (new_sphere->mat_ambient)[0] = amb[0];
  (new_sphere->mat_ambient)[1] = amb[1];
  (new_sphere->mat_ambient)[2] = amb[2];
  (new_sphere->mat_diffuse)[0] = dif[0];
  (new_sphere->mat_diffuse)[1] = dif[1];
  (new_sphere->mat_diffuse)[2] = dif[2];
  (new_sphere->mat_specular)[0] = spe[0];
  (new_sphere->mat_specular)[1] = spe[1];
  (new_sphere->mat_specular)[2] = spe[2];
  new_sphere->mat_shineness = shine;
  new_sphere->reflectance = refl;
  new_sphere->next = NULL;

  if (slist == NULL) { // first object
    slist = new_sphere;
  } else { // insert at the beginning
    new_sphere->next = slist;
    slist = new_sphere;
  }

  return slist;
}

/******************************************
 * computes a sphere normal - done for you
 ******************************************/
Vector sphere_normal(Point q, Spheres *sph) {
  Vector rc;

  rc = get_vec(sph->center, q);
  normalize(&rc);
  return rc;
}
