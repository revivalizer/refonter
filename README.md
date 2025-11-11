What is refonter?
------------------

refonter lets you use truetype fonts in your 3D applications with very few dependencies. Currently, refonter only depends on OpenGL (for the GLU tesselator).

Why would you want that?
------------------

Because the executable file becomes very small. Or because you can't live with the freetype license. Maybe you can think of other reasons.
 
What are the tradeoffs?
------------------

Of course there are some tradeoffs, primarily loss of flexibility with regards to size/resolution. The refonter output only supports ONE font instance, one rendering of the font. Sticking with freetype, you might get different renderings depending on the point size. There are also a few other tradeoffs, but they're not inherent in the approach, they can be solved with further development.

How does it work?
------------------

refonter has three parts:

1. refonter_export application

   The exporter is a commandline application which converts a truetype font to a simpler, smaller file format.  

   The exporter uses freetype to get information about a font (given a point size and resolution), and exports contour/outline information for a set of characters you specify. This contour information consists of straight lines and conic and cubib beziers. This is the only information exported, so the font file becomes a lot smaller than the original truetype font. However, you also lose some information, for instance about how to best render the font at different sizes.

2. refonter tesselation library

   This library loads the simple font file format, and uses OpenGL/GLU to tesselate it, in essence transforming it to a set of triangles. Your application must link with this library, which is written in pretty clean C (we can clean this up some more along the way) and only depends on OpenGL.

3. The missing part: drawing code  

   Refonter currently has no mechanism for drawing the triangles, you must provide that yourself. However, it's not complicated to do. refonter doesn't provide a mechanism for it, because there are many different ways to do it, and it must fit into your drawing pipeline. An example for OpenGL 3.3 is provided here: https://gist.github.com/revivalizer/7937396


Who wrote this?
-----------------
Hi, I am Ralph Brorsen aka revivalizer. In the demoscene I am also known as revival/fnuque.

Limitations
------------------
* Not very well tested.  
refonter is not very well tested, has only been tried on a handful of fonts, and only used in one released application so far. However, so far it seems to work pretty well. But you may run into some issues.
* Only builds with Visual Studio 2012.  
I haven't had time to create more builds. However, this is not hard to do, and I wrote up a recipe below. Please send some pull requests my way.
* Only works for basic ASCII characters  
Works needs to be done to extend refonter to support other encodings. 
* No kerning data at the moment  
Kerning information is available in truetype fonts, but refonter doesn't support it at the moment.
* No 3D text at this time  
refonter currently only outputs 2D polygons. For some applications it's desirable to have a 3D representation of the font. I haven't had time to write the code to extrude the 2D characters to 3D, maybe someone else will step in.
* Loss of information  
truetype fonts are adaptable to resolution/size (hinting). This information is lost with refonter, which basically only supports a single rendering of the font. 
* For some fonts it might not work well.  
There are some types of visual information that is much better represented in the truetype format than in refonter. This is because truetype is programmable, meaning you can give compact implicit representations for some things, while refonter will explicitly store every contour in a character.

Building
------------------
Add the freetype source files to the project (see section below). Open project file. Go.

Building on other platforms
------------------
1. You need to build 2 libraries, libs/freetype and refonter. You probably want to build static libraries. Put your appropriate build files in a new directories under libs/freetype/build/ and refonter/build/ respectively.
2. The freetype build needs to include the following C files:
   * libs/
      * freetype/
         * src/
            * autofit/autofit.c
            * base/
               * ftbase.c
               * ftbbox.c
               * ftbitmap.c
               * ftdebug.c
               * ftfstype.c
               * ftgasp.c
               * ftglyph.c
               * ftgxval.c
               * ftinit.c
               * ftlcdfil.c
               * ftmm.c
               * ftotval.c
               * ftpatent.c
               * ftpfr.c
               * ftstroke.c
               * ftsynth.c
               * ftsystem.c
               * fttype1.c
               * ftwinfnt.c
               * ftxf86.c
            * bdf/bdf.c
            * cache/ftcache.c
            * cff/cff.c
            * gzip/ftgzip.c
            * lzw/ftlzw.c
            * pcf/pcf.c
            * pfr/pfr.c
            * psaux/psaux.c
            * pshinter/pshinter.c
            * psnames/psmodule.c
            * raster/raster.c
            * sfnt/sfnt.c
            * smooth/smooth.c
            * truetype/truetype.c
            * type1/type1.c
            * type1/type1cid.c
            * type42/type42.c
            * winfonts/winfnt.c
3. The freetype build must set the following flags for the compiler:  
FT_DEBUG_LEVEL_ERROR  
FT_DEBUG_LEVEL_TRACE  
FT2_BUILD_LIBRARY  
4. The refonter library build must include the following files:  
   * refonter/
      * refonter.c
      * refonter_glu_tesselator.c
      * refonter_vertex.c
5. The refonter_export application must build the following files
   * refonter_export/
      * refonter_export.cpp
      * main.cpp
6. refonter_export must link with the refonter and freetype libraries.
7. refonter_export must have refonter/ and libs/freetype/src/include in the include path.

Example of refonter_export usage
------------------

refonter_export supports the following arguments:

    refonter_export [--res <resolution (=72)>] [--size <point_size (=16)>] <charset> <font path>

Here's an example:

    refonter_export.exe --res 72 --size 16 "abcdefghijklmnopqrstuvwxyz0123456789,.'-!() " ..\\..\\..\\cheri.ttf

Remember to include a space character if you want your font to support it.

Adding freetype to the project
------------------
You must manually install freetype in order to build refonter_export.

1. Download freetype 2.4.10 from http://download.savannah.gnu.org/releases/freetype/ 
2. Unpack freetype, and place the 'freetype-2.4.10' folder in 'refonter/libs/freetype'.
3. Rename the 'refonter/libs/freetype/freetype-2.4.10' folder to 'refonter/libs/freetype/src' 

Yes, I am aware that freetype contains build projects. I forget why I decided to do it manually.

