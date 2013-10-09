/*double refonter_bezier_length(const refonter_vec3& start, const refonter_vec3& control_1, const refonter_vec3& control_2, const refonter_vec3& end)
{
	refonter_vec3 prev;
	double length = 0.0;

	for (double t = 0.0; t <= 1.0; t+=1.0/64.0)
	{
		if (t > 0.0)
		{
			length += refonter_vertex_dist(prev, refonter_bezier(t, start, control_1, control_2, end));
		}
	}

	return length;
}
*/



/*double quadratic_bezier(double t, double start, double control, double end)
{
    return              start * (1.0 - t) * (1.0 - t) 
           + 2.0 *    control * (1.0 - t) * t
           +              end * t         * t;
}

refonter_vec3 quadratic_bezier(double t, const refonter_vec3& start, const refonter_vec3& control, const refonter_vec3& end)
{
	refonter_vec3 res;

	for (int i = 0; i < 3; i++)
		res.v[i] = quadratic_bezier(t, start.v[i], control.v[i], end.v[i]);

	return res;
}*/

/*double refonter_quadratic_bezier_length(const refonter_vec3& start, const refonter_vec3& control, const refonter_vec3& end)
{
	refonter_vec3 prev, cur;
	double length = 0.0;

	for (double t = 0.0; t <= 1.0; t+=1.0/64.0)
	{
		if (t > 0.0)
		{
			length += refonter_vertex_dist(prev, cur = quadratic_bezier(t, start, control, end));
		}

		prev = cur;
	}

	return length;
}*/

#include "refonter.h"
#include "refonter_export.h"

#include <iomanip>
#include <iostream>
#include <fstream>



int main()
{
	unsigned char* blob;
	unsigned int blob_size;

	//refonter_create_font_blob(&blob, &blob_size, "..\\..\\..\\DreamMMA.ttf", "abcdefghijklmnopqrstuvwxyz", 16*64, 72);
	//refonter_create_font_blob(&blob, &blob_size, "..\\..\\..\\8thCargo.ttf", "abcdefghijklmnopqrstuvwxyz", 16*64, 72);
	refonter_create_font_blob(&blob, &blob_size, "..\\..\\..\\EchinosParkScript.ttf", "abcdefghijklmnopqrstuvwxyz", 16*64, 72);
	//refonter_create_font_blob(&blob, &blob_size, "..\\..\\..\\Feathergraphy2.ttf", "abcdefghijklmnopqrstuvwxyz", 16*64, 72);
	//refonter_create_font_blob(&blob, &blob_size, "..\\..\\..\\MotorwerkOblique.ttf", "abcdefghijklmnopqrstuvwxyz", 16*64, 72);
	//refonter_create_font_blob(&blob, &blob_size, "..\\..\\..\\setbackt.ttf", "abcdefghijklmnopqrstuvwxyz", 16*64, 72);
	//refonter_create_font_blob(&blob, &blob_size, "..\\..\\..\\BirchStd.otf", "abcdefghijklmnopqrstuvwxyz", 16*64, 72);

	refonter_font* p_font = (refonter_font*)blob;

	delta_encode_points(p_font);
	transform_pointers_to_offsets(p_font);

	std::ofstream file;
	file.open("test.bin", std::ios::binary | std::ios::trunc);
	file.write((const char*)blob, blob_size);
	file.close();

	file.open("test.h", std::ios::trunc);

	file << "const char font[] = {";
	//file.write((const char*)blob, blob_size);
	for (uint32_t i = 0; i<blob_size; i++)
	{
		if ((i & 0xF) == 0x0)
			file << std::endl << "\t";

		//file << "0x" << std::hex << int(blob[i]) << ", ";
		file << "0x" << std::hex << std::setw(2) << std::setfill('0') << int(blob[i]) << ", ";

	}
	file << std::endl << "};";
	file.close();



	/*FT_Library ftLibrary;
	FT_Face    ftFace;
	FT_UInt    ftGlyphIndex;

	FT_Error error;

	error = FT_Init_FreeType(&ftLibrary);
	sprintf(buf, "Init error: %d\n", error); OutputDebugStringA(buf);

	error = FT_New_Face(ftLibrary, "..\\..\\..\\Dream MMA.ttf", 0, &ftFace);
	sprintf(buf, "Load face error: %d\n", error); OutputDebugStringA(buf);
	
	error = FT_Set_Char_Size(ftFace, 0, 16*64, 72, 72);
	sprintf(buf, "Set char size error: %d\n", error); OutputDebugStringA(buf);

	ftGlyphIndex = FT_Get_Char_Index(ftFace, 'o');
	sprintf(buf, "Glyph index: %d\n", ftGlyphIndex); OutputDebugStringA(buf); 

	error = FT_Load_Glyph(ftFace, ftGlyphIndex, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
	sprintf(buf, "Load glyph error: %d\n", error); OutputDebugStringA(buf);

	FT_GlyphSlot glyph = ftFace->glyph;

	if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
	{
		OutputDebugStringA("Fatal error: Glyph format must be outline.");
		return -1;
	}

	FT_Outline outline = glyph->outline;
	
	sprintf(buf, "Num contours: %d\n", outline.n_contours); OutputDebugStringA(buf);
	sprintf(buf, "Num points: %d\n", outline.n_points); OutputDebugStringA(buf);

	unsigned int num_contours = get_num_contours(outline);

	for (unsigned int c = 0; c < num_contours; c++)
	{
		sprintf(buf, "Traversing contour %d\n", c); OutputDebugStringA(buf);
		sprintf(buf, "Contour order %s\n", get_contour_order(outline, c)==kOrderClockwise ? "clockwise" : "counter clockwise"); OutputDebugStringA(buf);
		sprintf(buf, "Contour type %s\n", get_contour_type(outline, c)==kContourTypeInner ? "inner" : "outer"); OutputDebugStringA(buf);


		unsigned int num_points = get_num_points(outline, c);

		for (unsigned int p = 0; p < num_points; p++)
		{
			refonter_im_point point = get_point(outline, c, p);

			const char* curveTag;

			switch (point.tag)
			{
				case FT_CURVE_TAG_ON:
					curveTag = "TAG_ON";
					break;
				case FT_CURVE_TAG_CONIC:
					curveTag = "TAG_CONIC";
					break;
				case FT_CURVE_TAG_CUBIC:
					curveTag = "TAG_CUBIC";
					break;
			}

			sprintf(buf, "Point %d %d, (%d, %d) type: %s\n", p, point.id, point.x, point.y, curveTag); OutputDebugStringA(buf);

		}

	}*/

	/*// Iterate over contours
	for (int c = 0; c < outline.n_contours; c++)
	{
		sprintf(buf, "Traversing contour %d\n", c); OutputDebugStringA(buf);

		// Determine start and end points
		unsigned int pEnd = outline.contours[c];
		unsigned int pStart;

		if (c == 0)
			pStart = 0;
		else
			pStart = outline.contours[c-1]+1; // Starts one point after last point on previous contour

		// Iterate over points
		for (unsigned int p = pStart; p <= pEnd; p++)
		{
			char tag = FT_CURVE_TAG(outline.tags[p]);
			const char* curveTag;

			switch (tag)
			{
				case FT_CURVE_TAG_ON:
					curveTag = "TAG_ON";
					break;
				case FT_CURVE_TAG_CONIC:
					curveTag = "TAG_CONIC";
					break;
				case FT_CURVE_TAG_CUBIC:
					curveTag = "TAG_CUBIC";
					break;
			}

			sprintf(buf, "Point %d, (%d, %d) type: %s\n", p, outline.points[p].x, outline.points[p].y, curveTag); OutputDebugStringA(buf);
		}
	}*/
}
