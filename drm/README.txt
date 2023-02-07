Test the Linux Direct Rendering Manager

drm_fb.c drm_fb.h
- library to present a display as a memory mapped "frame buffer"
test_rec.c
- Draw a bunch of rectangles.

Notes:
  - The Raspberry PI DRM doesn't appear to support 1 bit per pixel (1bpp) monochome mode.
  - We can use 8bpp (RRRGGGBB) mode, and translate the Mgr monochrome bitmaps into this format.
  - see: https://fs.emersion.fr/protected/presentations/present.html?src=kms-foss-north/index.md
    for most concise description I could find on how to do this.
