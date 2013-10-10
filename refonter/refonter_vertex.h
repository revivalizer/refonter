#pragma once

// TODO: We really should be able to run in warning level 4 with no warnings.
#pragma warning(push) 
#pragma warning(disable: 4201)
struct refonter_vec3
{
	union {
		struct
		{
			double x;
			double y;
			double z;
		};
		double v[3];
	};
};
#pragma warning(pop) 

struct refonter_vertex
{
	refonter_vertex() {} // needs explict default dummy constructor, because it will otherwise generate memset reference :(

	refonter_vec3 pos;
	refonter_vec3 normal;
};

refonter_vec3 refonter_vertex_minus(const refonter_vec3& p1, const refonter_vec3& p2);
refonter_vec3 refonter_vertex_plus(const refonter_vec3& p1, const refonter_vec3& p2);
refonter_vec3 refonter_vertex_mid(const refonter_vec3& p1, const refonter_vec3& p2);
void refonter_vertex_zero(refonter_vec3& p);
double refonter_vertex_dist(const refonter_vec3& p1, const refonter_vec3& p2);

