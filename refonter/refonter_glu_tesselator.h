#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// We can't know beforehand how many vertices we'll need per character, so we hope this is enough. 
// We could use a growable array, but that would incur some code size and complexity
#define kMaxTesselatorVertices 8192

typedef struct 
{
	// Vertex storage
	refonter_vertex contour_vertices[kMaxTesselatorVertices]; // outline vertices input to tesselator
	refonter_vertex triangle_vertices[kMaxTesselatorVertices]; // triangle vertices, output from tesselator

	unsigned int num_contour_vertices;
	unsigned int num_triangle_vertices;

	// GLU tesselator instance
	GLUtesselator* glu_tess_obj;

	// Current primitive (triangle, strip, fan) info
	refonter_vertex cur_prim_history[2]; // used to convert strips and fans to triangles
	uint32_t cur_prim_type;
	uint32_t cur_prim_count; // number of primitives output for current primitive type

	// Other data
	double flatness_tolerance; // Criteria for ending bezier subdivision

} refonter_tesselation_object;

refonter_tesselation_object* refonter_glu_tesselate(refonter_font* p_font, refonter_tesselation_object* tess_objs, double flatness_tolerance);

#ifdef __cplusplus
}
#endif