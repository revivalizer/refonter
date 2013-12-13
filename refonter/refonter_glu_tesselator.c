#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

#include "refonter.h"
#include "refonter_vertex.h"
#include "refonter_glu_tesselator.h"

// The GLU tesselator doesn't provide a way to keep track of the current object, so we do it manually
refonter_tesselation_object* current_tesselator = NULL;

static void tessobj_init(refonter_tesselation_object* t, GLUtesselator* glu_tess_obj)
{
	t->num_contour_vertices  = 0;
	t->num_triangle_vertices = 0;

	t->glu_tess_obj = glu_tess_obj;
}

static refonter_vertex* tessobj_store_contour_vertex(refonter_tesselation_object* tess_obj, refonter_vec3 pos, refonter_vec3 normal)
{
	refonter_vertex* v = &(tess_obj->contour_vertices[tess_obj->num_contour_vertices++]);
	v->pos = pos;
	v->normal = normal;
	return v;
}

static void tessobj_add_contour_vertex(refonter_tesselation_object* tess_obj, refonter_vec3 pos, refonter_vec3 normal)
{
	refonter_vertex* v = tessobj_store_contour_vertex(tess_obj, pos, normal);

	gluTessVertex(tess_obj->glu_tess_obj, (GLdouble*)&pos, (void*)v);
}

static void current_tesselator_add_triangle_vertex(const refonter_vertex v)
{
	current_tesselator->triangle_vertices[current_tesselator->num_triangle_vertices++] = v;
}

static double fabsd(const double x) // We provide this function so we don't depend on libc
{
	if (x < 0.0)
		return -x;
	else
		return x;
}

static void tessobj_add_bezier_recursive(refonter_tesselation_object* tess_obj, refonter_vec3 start, refonter_vec3 control1, refonter_vec3 control2, refonter_vec3 end)
{
	// This is based on http://antigrain.com/research/adaptive_bezier/
	// Basically the simplest algorithm, with the flatness criteria suggested by Just d' FAQs

	double distance_tolerance_manhattan = tess_obj->flatness_tolerance;

	// Compute midpoints neccesary to split curve in two
	refonter_vec3 mid12 = refonter_vertex_mid(start, control1);
	refonter_vec3 mid23 = refonter_vertex_mid(control1, control2);
	refonter_vec3 mid34 = refonter_vertex_mid(control2, end);
	refonter_vec3 mid123 = refonter_vertex_mid(mid12, mid23);
	refonter_vec3 mid234 = refonter_vertex_mid(mid23, mid34);
	refonter_vec3 mid1234 = refonter_vertex_mid(mid123, mid234);

	// Evaluate flatness criteria
	int is_flat = 
		((fabsd(start.x + control2.x - control1.x - control1.x) +
		  fabsd(start.y + control2.y - control1.y - control1.y) +
		  fabsd(control1.x + end.x - control2.x - control2.x) +
		  fabsd(control1.y + end.y - control2.y - control2.y))     <= distance_tolerance_manhattan) ? 1 : 0;

	if (is_flat)
	{
		// Add midpoint and return (recursion complete)
		tessobj_add_contour_vertex(tess_obj, mid23, refonter_zero_vertex());  // See update 1 on URL for discussion about why we add point 23 and not 1234
		return;
	}
	else
	{
		// Subdivide
		tessobj_add_bezier_recursive(tess_obj, start, mid12, mid123, mid1234);
		tessobj_add_bezier_recursive(tess_obj, mid1234, mid234, mid34, end);
	}
}

static void tessobj_add_bezier(refonter_tesselation_object* tess_obj, refonter_vec3 start, refonter_vec3 control1, refonter_vec3 control2, refonter_vec3 end)
{
	// Add start point
	tessobj_add_contour_vertex(tess_obj, start, refonter_zero_vertex());

	// Start recursion
	tessobj_add_bezier_recursive(tess_obj, start, control1, control2, end);
}

static refonter_vec3 vec3_from_point(const refonter_point p)
{
	refonter_vec3 res;
	res.x = (double)((short)p.x)/64.0;
	res.y = (double)((short)p.y)/64.0;
	res.z = 0;

	return res;
}

static void __stdcall callback_begin_primitive(GLenum prim_type)
{
	current_tesselator->cur_prim_type = prim_type;
	current_tesselator->cur_prim_count = 0;
}

static void __stdcall callback_end_primitive(void)
{
	// Do nothing here at the moment
}

static void __stdcall callback_add_vertex(GLdouble* vertices)
{
	refonter_vertex v;
	v.pos.x = vertices[0];
	v.pos.y = vertices[1];
	v.pos.z = vertices[2];
	v.normal.x = 0.0;
	v.normal.y = 0.0;
	v.normal.z = 1.0;

	if (current_tesselator->cur_prim_type==GL_TRIANGLES)
	{
		current_tesselator_add_triangle_vertex(v);
	}
	else if (current_tesselator->cur_prim_type==GL_TRIANGLE_FAN)
	{
		if (current_tesselator->cur_prim_count >= 2)
		{
			// Output triangle comprised of center vertex, last vertex and current vertex
			current_tesselator_add_triangle_vertex(current_tesselator->cur_prim_history[0]);
			current_tesselator_add_triangle_vertex(current_tesselator->cur_prim_history[1]);
			current_tesselator_add_triangle_vertex(v);
		}

		// Update history
		current_tesselator->cur_prim_history[1] = v;

		if (current_tesselator->cur_prim_count==0)
			current_tesselator->cur_prim_history[0] = v;

	}
	else if (current_tesselator->cur_prim_type==GL_TRIANGLE_STRIP)
	{
		if (current_tesselator->cur_prim_count >= 2)
		{
			// Out triangle comprised of last three vertices, switching winding order for every triangle
			current_tesselator_add_triangle_vertex(current_tesselator->cur_prim_history[0]);
			current_tesselator_add_triangle_vertex(current_tesselator->cur_prim_history[1]);
			current_tesselator_add_triangle_vertex(v);
		}

		// Update history
		// We switch between updating history 0 and 1. This is neccesary to ensure correct winding order.
		// Take a look at an illustration of the triangle strip to understand why
		current_tesselator->cur_prim_history[(current_tesselator->cur_prim_count & 1)] = v;
	}

	current_tesselator->cur_prim_count++;
}

static void __stdcall callback_error(GLenum errorCode)
{
	// I am not sure what to do here yet :)
	const GLubyte *estring;

	estring = gluErrorString(errorCode);
	OutputDebugStringA((LPCSTR)estring);
}

static void __stdcall callback_combine(GLdouble coords[3], 
                                      GLdouble *vertex_data[4],
                                      GLfloat weight[4], GLdouble **dataOut )
{
	// NOTE: I am a little miffed that it is neccesary to check for null pointers in dataOut and vertex_data here.
	refonter_vertex* v = tessobj_store_contour_vertex(current_tesselator, refonter_zero_vertex(), refonter_zero_vertex());

	v->pos.x = coords[0];
	v->pos.y = coords[1];
	v->pos.z = coords[2];

	if (dataOut)
	{
		uint32_t i;

		double* coord = &(v->normal.x);

		for (i = 3; i <= 6; i++)
		{
			*coord++    = (vertex_data[0] ? weight[0] * vertex_data[0][i] : 0.0)
			            + (vertex_data[1] ? weight[1] * vertex_data[1][i] : 0.0)
			            + (vertex_data[2] ? weight[2] * vertex_data[2][i] : 0.0)
			            + (vertex_data[3] ? weight[3] * vertex_data[3][i] : 0.0);
		}

		*dataOut = (GLdouble*)v;
	}
}

// Just a simple function to wrap the point index around the contour
static const refonter_point get_point(refonter_contour* contour, uint32_t i) { return (contour->points[i % contour->num_points]); }

refonter_tesselation_object* refonter_glu_tesselate(refonter_font* cur_font, refonter_tesselation_object* tess_objects, double flatness_tolerance)
{
	unsigned int character, contour;
	refonter_char* cur_char;
	refonter_contour* cur_contour;

	uint32_t numPoints, i;
	refonter_vec3 curPoint, endPoint, control;

	// Allocate glu tesselator
	GLUtesselator *glu_tess = gluNewTess();

	// Our normal is always +Z
	gluTessNormal(glu_tess, 0.0, 0.0, 1.0);

	// Setup callbacks for tesselator
	gluTessCallback(glu_tess, GLU_TESS_VERTEX,  (GLvoid (__stdcall *) ()) &callback_add_vertex);
	gluTessCallback(glu_tess, GLU_TESS_BEGIN,   (GLvoid (__stdcall *) ()) &callback_begin_primitive);
	gluTessCallback(glu_tess, GLU_TESS_END,     (GLvoid (__stdcall *) ()) &callback_end_primitive); // currently not used
	gluTessCallback(glu_tess, GLU_TESS_ERROR,   (GLvoid (__stdcall *) ()) &callback_error);
	gluTessCallback(glu_tess, GLU_TESS_COMBINE, (GLvoid (__stdcall *) ()) &callback_combine);

	// Iterate over all characters in font
	for (character = 0; character < cur_font->num_chars; character++)
	{
		cur_char = &(cur_font->chars[character]);

		// Init tesselator object for char
		tessobj_init(&(tess_objects[character]), glu_tess);
		current_tesselator = &tess_objects[character];
		current_tesselator->flatness_tolerance = flatness_tolerance;

		gluTessBeginPolygon(glu_tess, &tess_objects[character]);

		// Iterate over contours in char
		for (contour = 0; contour < cur_char->num_contours; contour++)
		{
			cur_contour = &(cur_char->contours[contour]);

			gluTessBeginContour(glu_tess);

			numPoints = cur_contour->num_points;

			// Iterate over points in contour
			// See the freetype2 docs for outline curve decomposition: http://www.freetype.org/freetype2/docs/glyphs/glyphs-6.html
			// Our invariant inside the while is that at the top of the loop, the point type is always ON, so we're
			// always starting a new curve section at the top of the loop.
			i = 0;

			while (i < numPoints)
			{
				if (get_point(cur_contour, i+1).flags & kPointTypeOn)
				{
					// On - On (straight line)
					tessobj_add_contour_vertex(current_tesselator, vec3_from_point(get_point(cur_contour, i)), refonter_zero_vertex());
					i += 1;
				}
				else if (get_point(cur_contour, i+1).flags & kPointTypeOffConic)
				{
					curPoint = vec3_from_point(get_point(cur_contour, i+0));

					// On - Off conic series
					while ((i < numPoints-1) && (get_point(cur_contour, i+2).flags & kPointTypeOffConic))
					{
						endPoint = refonter_vertex_mid(vec3_from_point(get_point(cur_contour, i+1)), vec3_from_point(get_point(cur_contour, i+2)));
						control = vec3_from_point(get_point(cur_contour, i+1));

						tessobj_add_bezier(current_tesselator, curPoint, refonter_quadratic_control_to_cubic(curPoint, control), refonter_quadratic_control_to_cubic(endPoint, control), endPoint);

						curPoint = endPoint;

						i += 1;
					}

					// On - Off conic - On
					control = vec3_from_point(get_point(cur_contour, i+1));
					endPoint = vec3_from_point(get_point(cur_contour, i+2));
					tessobj_add_bezier(current_tesselator, curPoint, refonter_quadratic_control_to_cubic(curPoint, control), refonter_quadratic_control_to_cubic(endPoint, control), endPoint);
					i += 2;
				}
				else if (get_point(cur_contour, i+1).flags & kPointTypeOffCubic)
				{
					// On - Off cubic - Off cubic - On
					tessobj_add_bezier(current_tesselator, vec3_from_point(get_point(cur_contour, i+0)), vec3_from_point(get_point(cur_contour, i+1)), vec3_from_point(get_point(cur_contour, i+2)), vec3_from_point(get_point(cur_contour, i+3)));
					i += 4;
				}
			}

			gluTessEndContour(glu_tess);
		}

		gluTessEndPolygon(glu_tess);
	}

	gluDeleteTess(glu_tess);

	return tess_objects;
}
