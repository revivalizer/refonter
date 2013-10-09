#include "refonter.h"

refonter_font* refonter_init_blob(unsigned char* blob)
{
	// Transform char pointer
	refonter_font* p_font = (refonter_font*)blob;
	p_font->chars = (refonter_char*)((uintptr_t)p_font->chars + (uintptr_t)p_font);

	for (int ch = 0; ch < p_font->num_chars; ch++)
	{
		// Transform contour pointers
		refonter_char* p_char = &p_font->chars[ch];
		p_char->contours = (refonter_contour*)((uintptr_t)p_char->contours + (uintptr_t)p_font);

		for (int c = 0; c < p_char->num_contours; c++)
		{
			// Transform point pointers
			refonter_contour* p_contour = &(p_char->contours[c]);
			p_contour->points = (refonter_point*)((uintptr_t)p_contour->points + (uintptr_t)p_font);

			// Delta decode point coords
			refonter_coord cur_x = 0, cur_y = 0;

			for (int p = 0; p < p_contour->num_points; p++)
			{
				cur_x += p_contour->points[p].x;
				cur_y += p_contour->points[p].y;

				p_contour->points[p].x = cur_x;
				p_contour->points[p].y = cur_y;
			}

		}
	}

	return p_font;
}

double refonter_bezier(double t, double start, double control1, double control2, double end)
{
    return              start * (1.0 - t) * (1.0 - t)  * (1.0 - t) 
           + 3.0 *   control1 * (1.0 - t) * (1.0 - t)  * t 
           + 3.0 *   control2 * (1.0 - t) * t          * t
           +              end * t         * t          * t;
}

refonter_vec3 refonter_bezier(double t, const refonter_vec3& start, const refonter_vec3& control1, const refonter_vec3& control2, const refonter_vec3& end)
{
	refonter_vec3 res;

	for (int i = 0; i < 3; i++)
		res.v[i] = refonter_bezier(t, start.v[i], control1.v[i], control2.v[i], end.v[i]);

	return res;
}

refonter_vec3 refonter_quadratic_control_to_cubic(const refonter_vec3& p0, const refonter_vec3& p1)
{
	refonter_vec3 res = p0;
	res = refonter_vertex_plus(res, p1);
	res = refonter_vertex_plus(res, p1);
	res.x /= 3;
	res.y /= 3;
	res.z /= 3;
	return res;
}

