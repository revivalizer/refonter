project "refonter"
	kind "StaticLib"

	-- disable warnings, too many
	warnings "Off"

	files { "../../refonter_vertex.*" }
	files { "../../refonter_glu_tesselator.*" }
	files { "../../refonter.*" }

	filter "Debug"
		defines { "DEBUG" }
		flags { "Symbols", "Maps" }
		optimize "Off"

	filter "Release"
		defines { "NDEBUG" }
		optimize "Size"
