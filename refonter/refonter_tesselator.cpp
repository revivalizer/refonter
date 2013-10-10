#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

#include "refonter.h"
#include "refonter_vertex.h"
#include "refonter_tesselator.h"

enum
{
	kTypeTriangle = 1, //GL_TRIANGLES,
	kTypeTriangleStrip = 2, //GL_TRIANGLE_STRIP,
	kTypeTriangleFan = 3, //GL_TRIANGLE_FAN,
};

/*struct refonter_tesselation_object
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
	
	t.num_storage  = 0;

	t.state_is_border = false;
	t.state_count = 0;

	t.glu_tess_obj = glu_tess_obj;
}*/

void refonter_tesselation_object_init(refonter_tesselation_object& t, GLUtesselator* glu_tess_obj)
{
	t.num_storage  = 0;

	t.glu_tess_obj = glu_tess_obj;
}

void refonter_tesselation_object_add_vertex(refonter_tesselation_object* tess_obj, refonter_vec3 pos, refonter_vec3 normal)
{
	refonter_vertex* v = &(tess_obj->storage[tess_obj->num_storage++]);
	v->pos = pos;
	v->normal = normal;

	gluTessVertex(tess_obj->glu_tess_obj, (GLdouble*)&pos, (void*)v);
}

double refonter_fabsd(const double x) 
{
	double r;
	_asm fld  qword ptr [x]; 
	_asm fabs;  
	_asm fstp qword ptr [r]; 
	return r;
}

void refonter_tesselation_object_add_bezier_recursive(refonter_tesselation_object* tess_obj, refonter_vec3 start, refonter_vec3 control1, refonter_vec3 control2, refonter_vec3 end)
{
	// This is based on http://antigrain.com/research/adaptive_bezier/
	// Basically the simplest algorithm, with the flatness criteria suggested by Just d' FAQs

	const double distance_tolerance_manhattan = 0.25*0.25*0.25;

	// Compute midpoints neccesary to split curve in two
	refonter_vec3 mid12 = refonter_vertex_mid(start, control1);
	refonter_vec3 mid23 = refonter_vertex_mid(control1, control2);
	refonter_vec3 mid34 = refonter_vertex_mid(control2, end);
	refonter_vec3 mid123 = refonter_vertex_mid(mid12, mid23);
	refonter_vec3 mid234 = refonter_vertex_mid(mid23, mid34);
	refonter_vec3 mid1234 = refonter_vertex_mid(mid123, mid234);

	// Add midpoint and return if flat
	if (
		(refonter_fabsd(start.x + control2.x - control1.x - control1.x) +
		refonter_fabsd(start.y + control2.y - control1.y - control1.y) +
		refonter_fabsd(control1.x + end.x - control2.x - control2.x) +
		refonter_fabsd(control1.y + end.y - control2.y - control2.y)) <= distance_tolerance_manhattan)
	{
		refonter_tesselation_object_add_vertex(tess_obj, mid23, refonter_vec3());  // See update 1 on URL for discussion about why we add point 23 and not 1234
		return;
	}

	// Else subdivide
	refonter_tesselation_object_add_bezier_recursive(tess_obj, start, mid12, mid123, mid1234);
	refonter_tesselation_object_add_bezier_recursive(tess_obj, mid1234, mid234, mid34, end);

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
	refonter_tesselation_object_add_bezier_recursive(tess_obj, start, control1, control2, end);
	//refonter_tesselation_object_add_vertex(tess_obj, end, refonter_vec3());
}

/*void refonter_tesselation_segment_begin(unsigned int type, void* data)
{
	refonter_tesselation_object* t = (refonter_tesselation_object*)data;

	t->state_type = type;
	t->state_count = 0;
}*/

refonter_vec3 refonter_vec_from_point(const refonter_point& p)
{
	refonter_vec3 res;
	res.x = double((short)p.x)/64.0;
	res.y = double((short)p.y)/64.0;
	res.z = 0;

	return res;
}

/*  a portion of init() */
/*  the callback routines registered by gluTessCallback() */

void __stdcall beginCallback(GLenum which)
{
   glBegin(which);
}

void __stdcall endCallback(void)
{
   glEnd();
}

void __stdcall errorCallback(GLenum errorCode)
{
   const GLubyte *estring;

   estring = gluErrorString(errorCode);
//   fprintf (stderr, "Tessellation Error: %s\n", estring);
   OutputDebugStringA((LPCSTR)estring);
   //exit (0);
}

void __stdcall combineCallback(GLdouble coords[3], 
                     GLdouble *vertex_data[4],
                     GLfloat weight[4], GLdouble **dataOut )
{
	//refonter_vertex* v = &(tess_obj->storage[tess_obj->num_storage++]);
	refonter_vertex* v = new refonter_vertex();

	v->pos.x = coords[0];
	v->pos.y = coords[1];
	v->pos.z = coords[2];

	for (uint32_t i = 3; i <= 6; i++)
		v->normal.v[i-3]	= weight[0] * vertex_data[0][i]
							+ weight[1] * vertex_data[1][i]
							+ weight[2] * vertex_data[2][i] 
							+ weight[3] * vertex_data[3][i];
	*dataOut = (GLdouble*)v;
}

const refonter_point& get_point(refonter_contour* contour, uint32_t i) { return contour->points[i % contour->num_points]; }

refonter_tesselation_object* refonter_tesselate(refonter_font* p_font)
{
	//refonter_tesselation_object* t = (refonter_tesselation_object*)malloc(sizeof(refonter_tesselation_object)*p_font->num_chars);
	refonter_tesselation_object* t = new refonter_tesselation_object[p_font->num_chars];

	//GLUtesselator* tesselator = new gluNewTess();
	GLUtesselator *tess = gluNewTess();

	gluTessNormal(tess, 0.0, 0.0, 1.0);

	gluTessCallback(tess, GLU_TESS_VERTEX, (GLvoid (__stdcall *) ()) &glVertex3dv);
	gluTessCallback(tess, GLU_TESS_BEGIN, (GLvoid (__stdcall *) ()) &beginCallback);
	gluTessCallback(tess, GLU_TESS_END, (GLvoid (__stdcall *) ()) &endCallback);
	gluTessCallback(tess, GLU_TESS_ERROR, (GLvoid (__stdcall *) ()) &errorCallback);
	gluTessCallback(tess, GLU_TESS_COMBINE, (GLvoid (__stdcall *) ()) &combineCallback);

	for (unsigned int ch = 0; ch < p_font->num_chars; ch++)
	//for (unsigned int ch = 0; ch < 12; ch++)
	{
		refonter_tesselation_object_init(t[ch], tess);
		t[ch].glID = glGenLists(1);
		glNewList(t[ch].glID, GL_COMPILE);

		gluTessBeginPolygon(tess, &t[ch]);

		refonter_char* p_char = &(p_font->chars[ch]);

		for (unsigned int c = 0; c < p_char->num_contours; c++)
		{
			gluTessBeginContour(tess);

			refonter_contour* contour = &(p_char->contours[c]);

			uint32_t numPoints = contour->num_points;
			uint32_t i = 0;

			while (i < numPoints)
			{
				if (get_point(contour, i+1).flags & kPointTypeOn)
				{
					// On - on (straight line)
					refonter_tesselation_object_add_vertex(&t[ch], refonter_vec_from_point(get_point(contour, i)), refonter_vec3());
					i += 1;
				}
				else if (get_point(contour, i+1).flags & kPointTypeOffConic)
				{
					refonter_vec3 curPoint = refonter_vec_from_point(get_point(contour, i+0));

					// On - Off conic series
					while ((i < numPoints-1) && (get_point(contour, i+2).flags & kPointTypeOffConic))
					{
						refonter_vec3 endPoint = refonter_vertex_mid(refonter_vec_from_point(get_point(contour, i+1)), refonter_vec_from_point(get_point(contour, i+2)));
						refonter_vec3 control = refonter_vec_from_point(get_point(contour, i+1));

						refonter_tesselation_object_add_bezier(&t[ch], curPoint, refonter_quadratic_control_to_cubic(curPoint, control), refonter_quadratic_control_to_cubic(endPoint, control), endPoint);

						curPoint = endPoint;

						i += 1;
					}

					// On - Off conic - on
					refonter_vec3 control = refonter_vec_from_point(get_point(contour, i+1));
					refonter_vec3 endPoint = refonter_vec_from_point(get_point(contour, i+2));
					refonter_tesselation_object_add_bezier(&t[ch], curPoint, refonter_quadratic_control_to_cubic(curPoint, control), refonter_quadratic_control_to_cubic(endPoint, control), endPoint);
					i += 2;
				}
				else if (get_point(contour, i+1).flags & kPointTypeOffCubic)
				{
					// On - Off cubic - Off cubic - on
					refonter_tesselation_object_add_bezier(&t[ch], refonter_vec_from_point(get_point(contour, i+0)), refonter_vec_from_point(get_point(contour, i+1)), refonter_vec_from_point(get_point(contour, i+2)), refonter_vec_from_point(get_point(contour, i+3)));
					i += 4;
				}
			}

			gluTessEndContour(tess);
		}

		gluTessEndPolygon(tess);

		glEndList();
	}

	gluDeleteTess(tess);

	return t;
}
