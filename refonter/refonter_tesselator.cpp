#include "refonter.h"
#include "refonter_vertex.h"
#include "refonter_tesselator.h"

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

static const unsigned int kMaxVertices = 8*1024*3;

enum
{
	kTypeTriangle = 1, //GL_TRIANGLES,
	kTypeTriangleStrip = 2, //GL_TRIANGLE_STRIP,
	kTypeTriangleFan = 3, //GL_TRIANGLE_FAN,
};

struct refonter_tesselation_object
{
	refonter_vertex front[kMaxVertices];
	refonter_vertex side[kMaxVertices];
	refonter_vertex back[kMaxVertices];

	refonter_vertex storage[kMaxVertices];

	unsigned int num_front;
	unsigned int num_side;
	unsigned int num_back;
	unsigned int num_storage;

	unsigned int state_type;
	unsigned int state_is_border;
	unsigned int state_count;

	refonter_vertex h1;
	refonter_vertex h2;

	GLUtesselator* glu_tess_obj;
};

void refonter_tesselation_object_init(refonter_tesselation_object& t, GLUtesselator* glu_tess_obj)
{
	t.num_front = 0;
	t.num_side  = 0;
	t.num_back  = 0;

	t.state_is_border = false;
	t.state_count = 0;

	t.glu_tess_obj = glu_tess_obj;
}

void refonter_tesselation_object_add_vertex(refonter_tesselation_object* tess_obj, refonter_vec3 pos, refonter_vec3 normal)
{
	refonter_vertex* v = &(tess_obj->storage[tess_obj->num_storage++]);
	v->pos = pos;
	v->normal = normal;

	gluTessVertex(tess_obj->glu_tess_obj, (GLdouble*)&pos, (void*)v);
}

void refonter_tesselation_object_add_bezier(refonter_tesselation_object* tess_obj, refonter_vec3 start, refonter_vec3 control1, refonter_vec3 control2, refonter_vec3 end)
{
	/*double length = refonter_bezier_length(start, control1, control2, end);

	unsigned int n = (unsigned int)length / 16; // four subdivisions per point

	for (unsigned int i = 0; i < n; i++)
	{
		double v = (double)i/(double)n;
		
		

	}*/
	end; control1; control2;
	refonter_tesselation_object_add_vertex(tess_obj, start, refonter_vec3());
}

void refonter_tesselation_segment_begin(unsigned int type, void* data)
{
	refonter_tesselation_object* t = (refonter_tesselation_object*)data;

	t->state_type = type;
	t->state_count = 0;
}

refonter_vec3 refonter_vec_from_point(const refonter_point& p)
{
	refonter_vec3 res;
	res.x = p.x;
	res.y = p.y;
	res.z = 0;

	return res;
}

void refonter_tesselate_test(unsigned char* blob)
{
	refonter_font* p_font = refonter_init_blob(blob);

	refonter_tesselation_object* t = (refonter_tesselation_object*)malloc(sizeof(refonter_tesselation_object)*p_font->num_chars);

	//GLUtesselator* tesselator = new gluNewTess();
	GLUtesselator *tess = gluNewTess();

	for (unsigned int ch = 0; ch < p_font->num_chars; ch++)
	{
		refonter_tesselation_object_init(t[ch], tess);

		gluTessBeginPolygon(tess, &t[ch]);

		refonter_char* p_char = &(p_font->chars[ch]);

		for (unsigned int c = 0; c < p_char->num_contours; c++)
		{
			gluTessBeginContour(tess);

			refonter_contour* p_contour = &(p_char->contours[c]);

			refonter_point* p_point_start = &(p_contour->points[0]);
			refonter_point* p_point = p_point_start;
			refonter_point* p_point_end = &(p_contour->points[p_contour->num_points]);

			// push start point

			while (p_point < p_point_end)
			{
				if (p_point[1].flags & kPointTypeOn)
				{
					refonter_vec3 pos = refonter_vec_from_point(*p_point);
					refonter_vec3 d = refonter_vertex_minus(refonter_vec_from_point(p_point[1]), refonter_vec_from_point(p_point[0]));
					refonter_vec3 normal;
					normal.x = d.y;
					normal.y = -d.x;
					normal.z = 0;

					refonter_tesselation_object_add_vertex(&t[ch], pos, normal);
					p_point++;
				}
				else if (p_point[1].flags & kPointTypeOffConic)
				{
					refonter_vec3 start_pos = refonter_vec_from_point(*p_point);

					while (p_point[2].flags & kPointTypeOffConic)
					{
						refonter_vec3 end_pos = refonter_vertex_mid(refonter_vec_from_point(p_point[1]), refonter_vec_from_point(p_point[2]));
						refonter_vec3 control = refonter_vec_from_point(p_point[1]);

						refonter_tesselation_object_add_bezier(&t[ch], start_pos, refonter_quadratic_control_to_cubic(start_pos, control), refonter_quadratic_control_to_cubic(end_pos, control), end_pos);

						start_pos = end_pos;
						p_point++;
					}

					refonter_vec3 end_pos = refonter_vec_from_point(p_point[2]);
					refonter_vec3 control = refonter_vec_from_point(p_point[1]);

					refonter_tesselation_object_add_bezier(&t[ch], start_pos, refonter_quadratic_control_to_cubic(start_pos, control), refonter_quadratic_control_to_cubic(end_pos, control), end_pos);

					start_pos = end_pos;
					p_point+=2;
				}
				else if (p_point[1].flags & kPointTypeOffCubic)
				{
					refonter_tesselation_object_add_bezier(&t[ch], refonter_vec_from_point(p_point[0]), refonter_vec_from_point(p_point[1]), refonter_vec_from_point(p_point[2]), refonter_vec_from_point(p_point[3]));
					p_point+=4;
				}

			}

			gluTessEndContour(tess);
		}

		gluTessEndPolygon(tess);
	}

	gluDeleteTess(tess);
}
