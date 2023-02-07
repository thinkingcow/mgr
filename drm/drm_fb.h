/*
 * Linux 8bit frame buffer interface using DRM
 */

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

/* display stucture - don't change the values */
struct display {
  uint8_t *map;		// the mapped memory
  int width;		// screen width (in pixels)
  int height;		// screen height (in pixels)
  int pitch;		// pixel offset from one row to the next
  // private members below here
  int fd;               // File descriptor for dri device
  int handle; 		// frame buffer handle
  drmModeCrtcPtr s_crtc; // where to save exiting mode to restore it
  drmModeConnector *conn; // Connector for display
  uint32_t fb;		// frame buffer identifier
  char *error;		// returned if fd = 0;
};

/*
 * Open the display, filling in the supplied "dsp" stucture.
 */
int open_display(struct display *dsp, char *name, int mode);

/*
 * Clean up the display resources
 */
int close_display(struct display *dsp);

/* 
 * Fill the entire display with the specified color
 */
void fill(struct display *dsp, uint8_t color);

/*
 * Draw a rectangle of the specified color.
 * Uses byte operations
 */
int fill_rect(struct display *dsp, int x, int y, int w, int h, uint8_t color);

/*
 * Get the RRRGGGBB color value from components (0-255)
 */
uint8_t get_color(int red, int green, int blue);

/*
 * Create an 8 byte solid color
 */
uint64_t solid_color(uint8_t byte);
