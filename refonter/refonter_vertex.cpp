#include "refonter_vertex.h"

refonter_vec3 refonter_vertex_minus(const refonter_vec3& p1, const refonter_vec3& p2)
{
	refonter_vec3 res;
	res.x = p1.x - p2.x;
	res.y = p1.y - p2.y;
	res.z = p1.z - p2.z;
	return res;
}

refonter_vec3 refonter_vertex_plus(const refonter_vec3& p1, const refonter_vec3& p2)
{
	refonter_vec3 res;
	res.x = p1.x + p2.x;
	res.y = p1.y + p2.y;
	res.z = p1.z + p2.z;
	return res;
}

refonter_vec3 refonter_vertex_mid(const refonter_vec3& p1, const refonter_vec3& p2)
{
	refonter_vec3 res = refonter_vertex_plus(p1, p2);
	res.x /= 2;
	res.y /= 2;
	res.z /= 2;
	return res;
}

void refonter_vertex_zero(refonter_vec3& p)
{
	p.x = 0;
	p.y = 0;
	p.z = 0;
}

// TODO: Do we even use these two functions?
double refonter_sqrtd(const double x) { double r; _asm fld  qword ptr [x]; 
                                          _asm fsqrt; 
                                          _asm fstp qword ptr [r]; 
                                          return r; }

double refonter_vertex_dist(const refonter_vec3& p1, const refonter_vec3& p2)
{
	refonter_vec3 d = refonter_vertex_minus(p1, p2);

	return refonter_sqrtd(d.x*d.x + d.y*d.y + d.z*d.z);
}

