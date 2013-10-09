#pragma once

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

refonter_status refonter_create_font_blob(unsigned char** blob, unsigned int* blob_size, const char* path, const char* char_order, unsigned int point_size, unsigned int resolution);
void transform_pointers_to_offsets(refonter_font* p_font);
void delta_encode_points(refonter_font* p_font);
