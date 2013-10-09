#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "refonter.h"
#include "refonter_export.h"

#include <windows.h>

struct refonter_im_point
{
	int x, y;
	char tag;
	unsigned int id;
};

unsigned int get_num_contours(FT_Outline& outline);
unsigned int get_num_points(FT_Outline& outline, unsigned int c);


char buf[512];

unsigned int get_contour_start_index(FT_Outline& outline, unsigned int c)
{
	if (c==0)
		return 0;
	else
		return outline.contours[c-1]+1;
}

unsigned int get_contour_start_offset(FT_Outline& outline, unsigned int c)
{
	// Determine start and end pos
	unsigned int start = get_contour_start_index(outline, c);
	unsigned int stop = start + get_num_points(outline, c);

	// Look for first FT_CURVE_TAG_ON point
	unsigned int i = start;

	while (i<stop && FT_CURVE_TAG(outline.tags[i])!=FT_CURVE_TAG_ON)
		i++;

	// Return
	if (i<stop)
		return i-start;
	else
		return -1;

}

unsigned int get_point_index(FT_Outline& outline, unsigned int c, unsigned int p)
{
	unsigned int start = get_contour_start_index(outline, c);
	unsigned int offset = get_contour_start_offset(outline, c);
	unsigned int num = get_num_points(outline, c);

	unsigned int i = start + ((p + offset) % num);

	return i;
}

refonter_im_point get_point(FT_Outline& outline, unsigned int c, unsigned int p)
{
	unsigned int i = get_point_index(outline, c, p);

	refonter_im_point res;
	res.x   = outline.points[i].x;
	res.y   = outline.points[i].y;
	res.tag = FT_CURVE_TAG(outline.tags[i]);
	res.id  = i;

	return res;
}

unsigned int get_num_contours(FT_Outline& outline)
{
	return outline.n_contours;
}

unsigned int get_num_points(FT_Outline& outline, unsigned int c)
{
	if (c==0)
		return outline.contours[0] + 1;
	else
		return outline.contours[c] - outline.contours[c-1];
}

unsigned int get_contour_order(FT_Outline& outline, unsigned int c)
{
	unsigned int n = get_num_points(outline, c);

	// Compute polygon area (or double)
	signed int area = 0;

	for (unsigned int i = 0; i < n; i++)
	{
		refonter_im_point p1 = get_point(outline, c, i);
		refonter_im_point p2 = get_point(outline, c, (i+1) % n);

		area += p1.x*p2.y - p2.x*p1.y;
	}

	// Determine winding order
	if (area > 0)
		return kOrderCounterClockwise;
	else
		return kOrderClockwise;
}

unsigned int get_contour_type(FT_Outline& outline, unsigned int c)
{
	FT_Orientation orientation = FT_Outline_Get_Orientation(&outline);
	unsigned int order = get_contour_order(outline, c);

	if      (orientation==FT_ORIENTATION_TRUETYPE   && order==kOrderCounterClockwise)
		return kContourTypeInner;
	else if (orientation==FT_ORIENTATION_TRUETYPE   && order==kOrderClockwise)
		return kContourTypeOuter;
	else if (orientation==FT_ORIENTATION_POSTSCRIPT && order==kOrderCounterClockwise)
		return kContourTypeOuter;
	else if (orientation==FT_ORIENTATION_POSTSCRIPT && order==kOrderClockwise)
		return kContourTypeInner;
	else
		return -1;
}

refonter_status load_char_outline(FT_Face& ftFace, refonter_char_type ch, FT_Outline* outline)
{
	// Init
	FT_Error error;

	// Get glyph index
	FT_UInt ftGlyphIndex = FT_Get_Char_Index(ftFace, ch);

	if (ftGlyphIndex==0)
	{
		sprintf(buf, "FT glyph not found: %d\n", ch); OutputDebugStringA(buf); 
		return kStatusErrorFTGlyphNotFound;
	}

	// Load glyph
	error = FT_Load_Glyph(ftFace, ftGlyphIndex, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
	if (error != FT_Err_Ok)
	{
		sprintf(buf, "Load glyph error: %d (glyph: %d)\n", error, ch); OutputDebugStringA(buf);
		return kStatusErrorFTGlyphNotLoaded;
	}

	FT_GlyphSlot glyph = ftFace->glyph;

	// Check outline format
	if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
	{
		OutputDebugStringA("ERRORL: Glyph format must be outline.");
		return kStatusErrorFTGlyphOutlineNotAvailable;
	}

	// Return
	*outline = glyph->outline;
	return kStatusOk;
}

refonter_status refonter_create_font_blob(unsigned char** blob, unsigned int* blob_size, const char* path, const char* char_order, unsigned int point_size, unsigned int resolution)
{
	FT_Library ftLibrary;
	FT_Face    ftFace;

	FT_Error ftError;
	refonter_status rfError;

	// Open FT library
	ftError = FT_Init_FreeType(&ftLibrary);
	if (ftError != FT_Err_Ok)
	{
		sprintf(buf, "FT init error: %d\n", ftError); OutputDebugStringA(buf);
		return kStatusErrorFTInit;
	}

	// Load font face
	ftError = FT_New_Face(ftLibrary, path, 0, &ftFace);
	if (ftError != FT_Err_Ok)
	{
		sprintf(buf, "FT load face error: %d\n", ftError); OutputDebugStringA(buf);
		FT_Done_FreeType(ftLibrary);
		return kStatusErrorFTLoadFontFace;
	}

	// Set char sizes
	ftError = FT_Set_Char_Size(ftFace, 0, point_size, resolution, resolution);
	if (ftError != FT_Err_Ok)
	{
		sprintf(buf, "FT set char size error: %d\n", ftError); OutputDebugStringA(buf);
		FT_Done_Face(ftFace);
		FT_Done_FreeType(ftLibrary);
		return kStatusErrorFTSetCharSize;
	}

	// Get datastructure counts
	unsigned int num_fonts = 1;
	unsigned int num_chars = strlen(char_order);
	unsigned int num_contours = 0;
	unsigned int num_points = 0;

	for (unsigned int i = 0; i < num_chars; i++)
	{
		// Load char outline
		FT_Outline outline;
		rfError = load_char_outline(ftFace, char_order[i], &outline);

		if (rfError != kStatusOk)
		{
			FT_Done_Face(ftFace);
			FT_Done_FreeType(ftLibrary);
			return rfError;
		}

		// Update counts
		num_contours += outline.n_contours;
		num_points += outline.n_points;
	}

	// Allocate blob
	unsigned int size_fonts = num_fonts*sizeof(refonter_font);
	unsigned int size_chars = num_chars*sizeof(refonter_char);
	unsigned int size_contours = num_contours*sizeof(refonter_contour);
	unsigned int size_points = num_points*sizeof(refonter_point);

	*blob_size = size_fonts + size_chars + size_contours + size_points;
	*blob = (unsigned char*)malloc(*blob_size);

	// Define pointers
	refonter_font*    p_font    = (refonter_font*)   (*blob);
	refonter_char*    p_char    = (refonter_char*)   (*blob + size_fonts);
	refonter_contour* p_contour = (refonter_contour*)(*blob + size_fonts + size_chars);
	refonter_point*   p_point   = (refonter_point*)  (*blob + size_fonts + size_chars + size_contours);

	// Append font to blob
	p_font->flags = 0;
	p_font->num_chars = num_chars;
	p_font->chars = p_char;

	// Iterate chars
	for (unsigned int i = 0; i < num_chars; i++)
	{
		// Load char outline
		FT_Outline outline;
		rfError = load_char_outline(ftFace, char_order[i], &outline);

		// Apppend char
		unsigned int num_contours = get_num_contours(outline);

		p_char->id = char_order[i];
		p_char->flags = 0;
		p_char->num_contours = num_contours;
		p_char->contours = p_contour;
		p_char++;

		// Iterate contours
		for (unsigned int c = 0; c < num_contours; c++)
		{
			// Append contour
			unsigned int num_points = get_num_points(outline, c);
			p_contour->flags = 0;
			p_contour->num_points = get_num_points(outline, c);
			p_contour->points = p_point;
			p_contour++;

			// Iterate points
			for (unsigned int p = 0; p < num_points; p++)
			{
				refonter_im_point im_point = get_point(outline, c, p);

				p_point->x = im_point.x;
				p_point->y = im_point.y;

				switch (im_point.tag)
				{
					case FT_CURVE_TAG_ON:
						p_point->flags = kPointTypeOn;
						break;
					case FT_CURVE_TAG_CONIC:
						p_point->flags = kPointTypeOffConic;
						break;
					case FT_CURVE_TAG_CUBIC:
						p_point->flags = kPointTypeOffCubic;
						break;
				}

				p_point++;
			}
		}
	}

	
	return kStatusOk;
}

void transform_pointers_to_offsets(refonter_font* p_font)
{
	// Iterate over chars
	for (unsigned int ch = 0; ch < p_font->num_chars; ch++)
	{
		refonter_char* p_char = &(p_font->chars[ch]);

		// Iterate over contours
		for (int c = 0; c < p_char->num_contours; c++)
		{
			refonter_contour* p_contour = &(p_char->contours[c]);

			// Transform points pointer
			p_contour->points = (refonter_point*)((unsigned char*)p_contour->points - (unsigned char*)p_font);
		}

		// Transform contours pointer
		p_char->contours = (refonter_contour*)((unsigned char*)p_char->contours - (unsigned char*)p_font);
	}

	// Transform chars pointer
	p_font->chars = (refonter_char*)((unsigned char*)p_font->chars - (unsigned char*)p_font);
}

void delta_encode_points(refonter_font* p_font)
{
	// Iterate over chars
	for (unsigned int ch = 0; ch < p_font->num_chars; ch++)
	{
		refonter_char* p_char = &(p_font->chars[ch]);

		// Iterate over contours
		for (int c = 0; c < p_char->num_contours; c++)
		{
			refonter_contour* p_contour = &(p_char->contours[c]);

			refonter_coord prev_x = 0, prev_y = 0;

			// Iterate over points
			for (int p = 0; p < p_contour->num_points; p++)
			{
				refonter_point* p_point = &(p_contour->points[p]);

				// Delta encode
				refonter_coord cur_x = p_point->x, cur_y = p_point->y;
					
				p_point->x -= prev_x;
				p_point->y -= prev_y;

				prev_x = cur_x;
				prev_y = cur_y;
			}
		}
	}

}

