#pragma once

static const unsigned int kMaxTesselatorVertices = 8*1024;

struct refonter_tesselation_object
{
	refonter_vertex storage[kMaxTesselatorVertices];

	unsigned int num_storage;

	uint32_t glID;

	GLUtesselator* glu_tess_obj;
};

refonter_tesselation_object* refonter_tesselate(refonter_font* p_font);
