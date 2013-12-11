#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

#include "refonter.h"
#include "refonter_export.h"

#include <iomanip>
#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
	// Default values for args
	char* font_path = nullptr;
	char* chars = nullptr;
	int point_size = 16;
	int resolution = 72;
	
	bool args_ok = false;

	if (argc > 2) // Must have at least two args excluding path
	{
		uint32_t opt_argc = argc-3; // First argument exe path, last two char set and font path
		char** opt_argv = argv+1;

		args_ok = true;

		uint32_t i = 0;

		// Loop over optional args
		while (i<opt_argc && args_ok)
		{
			if (strcmp(opt_argv[i], "--res")==0 && i<(opt_argc-1))
			{
				resolution = atoi(opt_argv[i+1]);
				i += 2;
			}
			else if (strcmp(opt_argv[i], "--size")==0 && i<(opt_argc-1))
			{
				point_size = atoi(opt_argv[i+1]);
				i += 2;
			}
			else
			{
				args_ok = false;
			}
		}

		// Sanity check arguments
		args_ok &= (resolution>= 1 && resolution<4800);
		args_ok &= (point_size>= 1 && point_size<600);
	}

	// Print help and exit if arguments failed to parse arguments
	if (!args_ok)
	{
		printf("Couldn't understand arguments. Usage: refonter_export [--res <resolution>] [--size <point_size>] <charset> <font path>");
		return -1;
	}

	// Get remaining charset and font path arguments
	chars     = argv[argc-2];
	font_path = argv[argc-1];

	unsigned char* blob;
	unsigned int blob_size;

	// Create font blob and prepare for export
	refonter_status status = refonter_create_font_blob(&blob, &blob_size, font_path, chars, point_size*kRefonterSubdivision, resolution);

	if (status==kStatusOk)
	{
		refonter_font* p_font = (refonter_font*)blob;

		delta_encode_points(p_font);
		transform_pointers_to_offsets(p_font);

		// Append extensions to path
		char* extended_path = new char[strlen(font_path+10)];
		strcpy(extended_path, font_path);

		// Write binary file
		strcat(extended_path, ".refonter.bin");

		std::ofstream file;
		file.open(extended_path, std::ios::binary | std::ios::trunc);
		file.write((const char*)blob, blob_size);
		file.close();

		// Write header file
		strcat(extended_path, ".h");

		file.open(extended_path, std::ios::trunc);

		file << "unsigned char font[] = {";
		for (uint32_t i = 0; i<blob_size; i++)
		{
			if ((i & 0xF) == 0x0)
				file << std::endl << "\t";

			file << "0x" << std::hex << std::setw(2) << std::setfill('0') << int(blob[i]) << ", ";

		}
		file << std::endl << "};";
		file.close();

		return 0;
	}
	else
	{
		switch (status)
		{
			case kStatusErrorFTInit:
				printf("Couldn't initiaize freetype library.\n");
				break;
			case kStatusErrorFTLoadFontFace:
				printf("Couldn't load font!\n");
				break;
			case kStatusErrorFTSetCharSize:
				printf("Couldn't set char size.\n");
				break;
			case kStatusErrorFTGlyphNotFound:
				printf("Glyph not found.\n");
				break;
			case kStatusErrorFTGlyphNotLoaded:
				printf("Couldn't load glyph.");
				break;
			case kStatusErrorFTGlyphOutlineNotAvailable:
				printf("No outline available for glyph.");
				break;
		}

		return status;
	}
}
