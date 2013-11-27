#include "refonter_vertex.h"

refonter_vec3 refonter_zero_vertex()
{
	refonter_vec3 res;
	res.x = 0;
	res.y = 0;
	res.z = 0;
	return res;
}

refonter_vec3 refonter_vertex_minus(const refonter_vec3 p1, const refonter_vec3 p2)
{
	refonter_vec3 res;
	res.x = p1.x - p2.x;
	res.y = p1.y - p2.y;
	res.z = p1.z - p2.z;
	return res;
}

refonter_vec3 refonter_vertex_plus(const refonter_vec3 p1, const refonter_vec3 p2)
{
	refonter_vec3 res;
	res.x = p1.x + p2.x;
	res.y = p1.y + p2.y;
	res.z = p1.z + p2.z;
	return res;
}

refonter_vec3 refonter_vertex_mid(const refonter_vec3 p1, const refonter_vec3 p2)
{
	refonter_vec3 res = refonter_vertex_plus(p1, p2);
	res.x /= 2;
	res.y /= 2;
	res.z /= 2;
	return res;
}