#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_OUTLINE_H

// These three lines are win/VC specific
#include <windows.h>
static char errorString[512];
#define REFONTER_ERROR(format, ...) sprintf(errorString, format, __VA_ARGS__); OutputDebugStringA(errorString); 

#include "refonter.h"
#include "refonter_export.h"

struct intermediate_point
{
	int x, y;
	char tag;
};

unsigned int get_num_contours(FT_Outline& outline)
{
	return outline.n_contours;
}

unsigned int get_contour_num_points(FT_Outline& outline, unsigned int contour)
{
	if (contour==0)
		return outline.contours[0] + 1;
	else
		return outline.contours[contour] - outline.contours[contour-1];
}


unsigned int get_contour_start_index(FT_Outline& outline, unsigned int contour)
{
	if (contour==0)
		return 0;
	else
		return outline.contours[contour-1]+1;
}

// Find the relative offset from start index to first point of type ON.
// This is neccesary because a contour may start with any type of point, but
// refonter assumes (for simplicity)  that the first point is type ON.
unsigned int get_contour_start_offset(FT_Outline& outline, unsigned int contour)
{
	// Determine start and end pos
	unsigned int start = get_contour_start_index(outline, contour);
	unsigned int stop = start + get_contour_num_points(outline, contour);

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

unsigned int get_point_index(FT_Outline& outline, unsigned int contour, unsigned int point)
{
	unsigned int start = get_contour_start_index(outline, contour);
	unsigned int offset = get_contour_start_offset(outline, contour);
	unsigned int num_points_in_contour = get_contour_num_points(outline, contour);

	unsigned int i = start + ((point + offset) % num_points_in_contour); // treat points in array as circular buffer

	return i;
}

intermediate_point get_point(FT_Outline& outline, unsigned int contour, unsigned int point)
{
	unsigned int i = get_point_index(outline, contour, point);

	intermediate_point res;
	res.x   = outline.points[i].x;
	res.y   = outline.points[i].y;
	res.tag = FT_CURVE_TAG(outline.tags[i]); // we're only interested in the type tag

	return res;
}

unsigned int get_contour_order(FT_Outline& outline, unsigned int contour)
{
	unsigned int n = get_contour_num_points(outline, contour);

	// Compute polygon area (or double polygon area, rather)
	signed int area = 0;

	for (unsigned int i = 0; i < n; i++)
	{
		intermediate_point p1 = get_point(outline, contour, i);
		intermediate_point p2 = get_point(outline, contour, (i+1) % n);

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

refonter_status load_char_outline(FT_Face& ftFace, refonter_char_type ch, FT_Outline* outline, int* width)
{
	// Init
	FT_Error error;

	// Get glyph index
	FT_UInt ftGlyphIndex = FT_Get_Char_Index(ftFace, ch);

	if (ftGlyphIndex==0)
	{
		REFONTER_ERROR("FT glyph not found: %d\n", ch);
		return kStatusErrorFTGlyphNotFound;
	}

	// Load glyph
	error = FT_Load_Glyph(ftFace, ftGlyphIndex, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
	if (error != FT_Err_Ok)
	{
		REFONTER_ERROR("Load glyph error: %d (glyph: %d)\n", error, ch);
		return kStatusErrorFTGlyphNotLoaded;
	}

	FT_GlyphSlot glyph = ftFace->glyph;

	*width = glyph->metrics.horiAdvance;

	// Check outline format
	if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
	{
		REFONTER_ERROR("ERRORL: Glyph format must be outline.");
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
		REFONTER_ERROR("FT init error: %d\n", ftError);
		return kStatusErrorFTInit;
	}

	// Load font face
	ftError = FT_New_Face(ftLibrary, path, 0, &ftFace);
	if (ftError != FT_Err_Ok)
	{
		REFONTER_ERROR("FT load face error: %d\n", ftError)
		FT_Done_FreeType(ftLibrary);
		return kStatusErrorFTLoadFontFace;
	}

	// Set char sizes
	ftError = FT_Set_Char_Size(ftFace, 0, point_size, resolution, resolution);
	if (ftError != FT_Err_Ok)
	{
		REFONTER_ERROR("FT set char size error: %d\n", ftError);
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
		int width;
		rfError = load_char_outline(ftFace, char_order[i], &outline, &width);

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

	// COmpute byte size and allocate blob
	unsigned int size_fonts = num_fonts*sizeof(refonter_font);
	unsigned int size_chars = num_chars*sizeof(refonter_char);
	unsigned int size_contours = num_contours*sizeof(refonter_contour);
	unsigned int size_points = num_points*sizeof(refonter_point);

	*blob_size = size_fonts + size_chars + size_contours + size_points;
	*blob = (unsigned char*)malloc(*blob_size);

	// Define pointers
	refonter_font*    cur_font    = (refonter_font*)   (*blob);
	refonter_char*    cur_char    = (refonter_char*)   (*blob + size_fonts);
	refonter_contour* cur_contour = (refonter_contour*)(*blob + size_fonts + size_chars);
	refonter_point*   cur_point   = (refonter_point*)  (*blob + size_fonts + size_chars + size_contours);

	// Append font to blob
	cur_font->flags     = 0;
	cur_font->num_chars = num_chars;
	cur_font->chars     = cur_char;

	// For each char
	for (unsigned int i = 0; i < num_chars; i++)
	{
		// Load char outline
		FT_Outline outline;
		int width;
		rfError = load_char_outline(ftFace, char_order[i], &outline, &width); // we don't check error here since it succeeded previously

		// Apppend char to blob
		unsigned int num_contours = get_num_contours(outline);

		cur_char->id           = char_order[i];
		cur_char->flags        = 0;
		cur_char->num_contours = num_contours;
		cur_char->contours     = cur_contour;
		cur_char->width        = width;
		cur_char++;

		// For each contour in char
		for (unsigned int c = 0; c < num_contours; c++)
		{
			// Append contour to blob
			unsigned int num_points = get_contour_num_points(outline, c);
			cur_contour->flags      = 0;
			cur_contour->num_points = get_contour_num_points(outline, c);
			cur_contour->points     = cur_point;
			cur_contour++;

			// For each point in contour 
			for (unsigned int p = 0; p < num_points; p++)
			{
				intermediate_point im_point = get_point(outline, c, p);

				// Append point to blob
				cur_point->x = im_point.x;
				cur_point->y = im_point.y;

				switch (im_point.tag)
				{
					case FT_CURVE_TAG_ON:
						cur_point->flags = kPointTypeOn;
						break;
					case FT_CURVE_TAG_CONIC:
						cur_point->flags = kPointTypeOffConic;
						break;
					case FT_CURVE_TAG_CUBIC:
						cur_point->flags = kPointTypeOffCubic;
						break;
				}

				cur_point++;
			}
		}
	}
	
	return kStatusOk;
}

void transform_pointers_to_offsets(refonter_font* p_font)
{
	// For each char
	for (unsigned int ch = 0; ch < p_font->num_chars; ch++)
	{
		refonter_char* p_char = &(p_font->chars[ch]);

		// For each contour in char
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
	// For each char
	for (unsigned int ch = 0; ch < p_font->num_chars; ch++)
	{
		refonter_char* p_char = &(p_font->chars[ch]);

		// For each contour in char
		for (int c = 0; c < p_char->num_contours; c++)
		{
			refonter_contour* p_contour = &(p_char->contours[c]);

			// Init delta encoding
			refonter_coord prev_x = 0, prev_y = 0;

			// For each point in contour
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

