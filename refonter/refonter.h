#pragma once

#include <stdint.h> 
#include "refonter_vertex.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: For GNU
//#define PACK_STRUCT( __Declaration__ ) __Declaration__ __attribute__((__packed__))
// For MSVC
#define PACK_STRUCT( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )

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
	kPointTypeOn = 1,
	kPointTypeOffConic = 2,
	kPointTypeOffCubic = 4,
};

typedef unsigned short refonter_count;
typedef unsigned short refonter_coord;
typedef unsigned char  refonter_point_info;
typedef unsigned char  refonter_char_info;
typedef unsigned char  refonter_contour_info;
typedef unsigned short refonter_font_info;
typedef char           refonter_char_type;

PACK_STRUCT(
typedef struct 
{
	refonter_coord      x;
	refonter_coord      y;
	refonter_point_info flags;
} refonter_point);

PACK_STRUCT(
typedef struct 
{
	refonter_contour_info flags;
	refonter_count        num_points;
	refonter_point*       points;
} refonter_contour);

PACK_STRUCT(
typedef struct 
{
	refonter_char_type id;
	refonter_char_info flags;
	refonter_coord     width;
	refonter_count     num_contours;
	refonter_contour*  contours;
} refonter_char);

PACK_STRUCT(
typedef struct 
{
	refonter_font_info  flags;
	refonter_count      num_chars;
	refonter_char*      chars;
} refonter_font);

refonter_font* refonter_init_blob(unsigned char* blob);

double refonter_bezier1(double t, double start, double control1, double control2, double end);
refonter_vec3 refonter_bezier3(double t, const refonter_vec3 start, const refonter_vec3 control1, const refonter_vec3 control2, const refonter_vec3 end);
refonter_vec3 refonter_quadratic_control_to_cubic(const refonter_vec3 p0, const refonter_vec3 p1);

static const uint32_t kRefonterSubdivision = 64;

#ifdef __cplusplus
}
#endif

