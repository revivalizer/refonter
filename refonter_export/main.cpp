#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <iostream>
#include <fstream>
#include <windows.h>

// For GNU
//#define PACK_STRUCT( __Declaration__ ) __Declaration__ __attribute__((__packed__))
// For MSVC
#define PACK_STRUCT( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )

char buf[512];

struct refonter_im_point
{
	int x, y;
	char tag;
	unsigned int id;
};

unsigned int get_num_contours(FT_Outline& outline);
unsigned int get_num_points(FT_Outline& outline, unsigned int c);

enum
{
	kOrderCounterClockwise,
	kOrderClockwise,
};

enum
{
	kContourTypeOuter,
	kContourTypeInner,
};

enum
{
	kPointTypeOn,
	kPointTypeOffConic,
	kPointTypeOffCubic,
};

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

enum
{
	kStatusOk = 0,
	kStatusErrorFTInit = 1,
	kStatusErrorFTLoadFontFace = 2,
	kStatusErrorFTSetCharSize = 3,
	kStatusErrorFTGlyphNotFound = 4,
	kStatusErrorFTGlyphNotLoaded = 5,
	kStatusErrorFTGlyphOutlineNotAvailable = 6,
	kStatusError = 0xffffffff,
};

typedef unsigned int refonter_status;

typedef unsigned short refonter_count;
typedef unsigned short refonter_coord;
typedef unsigned char  refonter_point_info;
typedef unsigned char  refonter_char_info;
typedef unsigned char  refonter_contour_info;
typedef unsigned short refonter_font_info;
typedef char           refonter_char_type;

PACK_STRUCT(
struct refonter_point
{
	refonter_coord      x;
	refonter_coord      y;
	refonter_point_info flags;
});

PACK_STRUCT(
struct refonter_contour
{
	refonter_contour_info flags;
	refonter_count        num_points;
	refonter_point*       points;
});

PACK_STRUCT(
struct refonter_char
{
	refonter_char_type id;
	refonter_char_info flags;
	refonter_count     num_contours;
	refonter_contour*  contours;
});

PACK_STRUCT(
struct refonter_font
{
	refonter_font_info  flags;
	refonter_count      num_chars;
	refonter_char*      chars;
});

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

refonter_font* refonter_init_blob(unsigned char* blob)
{
	refonter_font* p_font = (refonter_font*)blob;

	p_font->chars = (refonter_char*)((uintptr_t)p_font->chars + (uintptr_t)p_font);

	for (int ch = 0; ch < p_font->num_chars; ch++)
	{
		refonter_char* p_char = &p_font->chars[ch];

		p_char->contours = (refonter_contour*)((uintptr_t)p_char->contours + (uintptr_t)p_font);

		for (int c = 0; c < p_char->num_contours; c++)
		{
			refonter_contour* p_contour = &(p_char->contours[c]);

			p_contour->points = (refonter_point*)((uintptr_t)p_contour->points + (uintptr_t)p_font);

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

int main()
{
	unsigned char* blob;
	unsigned int blob_size;

	refonter_create_font_blob(&blob, &blob_size, "..\\..\\..\\DreamMMA.ttf", "abcdefghijklmnopqrstuvwxyz", 16*64, 72);
	//refonter_create_font_blob(&blob, &blob_size, "..\\..\\..\\8thCargo.ttf", "abcdefghijklmnopqrstuvwxyz", 16*64, 72);
	//refonter_create_font_blob(&blob, &blob_size, "..\\..\\..\\EchinosParkScript.ttf", "abcdefghijklmnopqrstuvwxyz", 16*64, 72);
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


	p_font = refonter_init_blob(blob);



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
