#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
	double x;
	double y;
	double z;
} refonter_vec3;

typedef struct 
{
	refonter_vec3 pos;
	refonter_vec3 normal;
} refonter_vertex;

refonter_vec3 refonter_zero_vertex(void);
refonter_vec3 refonter_vertex_minus(const refonter_vec3 p1, const refonter_vec3 p2);
refonter_vec3 refonter_vertex_plus(const refonter_vec3 p1, const refonter_vec3 p2);
refonter_vec3 refonter_vertex_mid(const refonter_vec3 p1, const refonter_vec3 p2);

#ifdef __cplusplus
}
#endif
