/* Simple DRM framebuffer interface */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include "drm_fb.h"

/* Open the display, filling out the display structure, return 0 in success
 * - name: the display to use (e.g. /dev/dri/card1)
 * - mode: The display mode (0 is highest resolution)
 * - this: pointer to a DISPLAY_SIZE array of bytes (usually statically created in main)
 */
int open_display(struct display *d, char *name, int mode) {
  int bpp=8; // 8 bits per pixel
  int i;
  int format=DRM_FORMAT_RGB332; // each pixel is RRRGGGBB
  drmVersionPtr verp; // hardware version info
  uint64_t has_dumb; // filled out by fb check
  drmModeRes *res; // drm resources
  drmModeEncoder *enc = (drmModeEncoder *)0; // encoder
  int crtc = 0;
  drmModeModeInfo modeInfo;
  uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
  struct drm_mode_map_dumb mreq;
  struct drm_mode_create_dumb creq; // dumb framebuffer request properties

  d->fd = open(name, O_RDWR | O_CLOEXEC);
  if (d->fd < 0) {
    perror("open card");
    return 1;
  }
  verp = drmGetVersion(d->fd);
  fprintf(stderr, "graphics hardware: %s (%s) %s\n", verp->name, verp->desc, verp->date);

  if (drmGetCap(d->fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
    fprintf(stderr, "drm device '%s' does not support dumb buffers\n", name);
    close(d->fd);
    return 2;
  }
  if (!(res = drmModeGetResources(d->fd))) {
    fprintf(stderr, "drm device '%s' error: cant get resources\n", name);
    close(d->fd);
    return 3;
  }
  for (i = 0; i < res->count_connectors; i++) {
    if (!(d->conn = drmModeGetConnector(d->fd, res->connectors[i]))) {
      fprintf(stderr, "cannot retrieve DRM connector %d\n", i);
      continue;
    }
    if (d->conn->connection != DRM_MODE_CONNECTED) {
      fprintf(stderr, "Not connected (0x%x)\n", d->conn->connection);
      continue;
    }
    if (d->conn->count_modes > 0) {
      break;
    }
  }
  if (!d->conn) {
    fprintf(stderr, "No available connectors\n");
    return 4;
  }
  for (i = 0; i < d->conn->count_encoders; ++i) {
    if (!(enc = drmModeGetEncoder(d->fd, d->conn->encoders[i]))) {
      fprintf(stderr, "cannot retrieve encoder %d\n", i);
      continue;
    }
    fprintf(stderr, "encoder %d: id=%d type=0x%x crtc=%d, possible=0x%x\n",
	    i, enc->encoder_id, enc->encoder_type, enc->crtc_id,
	    enc->possible_crtcs);
    crtc = enc->crtc_id;
    drmModeFreeEncoder(enc);
    if (crtc > 0) {
      fprintf(stderr, "Using CRTC %d\n", crtc);
      break;
    }
  }
  memset(&creq, 0, sizeof(creq));
  memcpy(&modeInfo, &d->conn->modes[0], sizeof(modeInfo));
  d->width = creq.width = d->conn->modes[0].hdisplay;
  d->height = creq.height = d->conn->modes[0].vdisplay;
  creq.bpp = bpp;
  fprintf(stderr,"Display: %d x %d\n", creq.width, creq.height);
  if (drmIoctl(d->fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0) {
    fprintf(stderr, "Can't create frame buffer mode %dx%dx%d\n",
	    d->width, d->height, creq.bpp);
    close(d->fb);
    return 5;
  }
  fprintf(stderr,"adding framebuffer\n");
  d->handle = handles[0] = creq.handle;
  d->pitch = pitches[0] = creq.pitch;
  if (drmModeAddFB2(d->fd, creq.width, creq.height, format, handles, pitches, offsets, &d->fb, 0)) {
    fprintf(stderr, "Can't create frame buffer\n");
    perror("AddFb2");
    close(d->fd);
    return 6;
  }
  memset(&mreq, 0, sizeof(mreq));
  mreq.handle = creq.handle;
  fprintf(stderr,"mapping framebuffer\n");
  if (drmIoctl(d->fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq)) {
    fprintf(stderr, "Can't created memory map for frame buffer\n");
    perror("map_dumb");
    close(d->fd);
    return 7;
  }
  fprintf(stderr,"mapping memory\n");
  d->map = mmap(0, creq.size, PROT_READ | PROT_WRITE, MAP_SHARED, d->fd, mreq.offset);
  if (d->map == MAP_FAILED) {
    fprintf(stderr, "cannot mmap dumb buffer\n");
    perror("mmap");
    close(d->fd);
    return 8;
  }
  memset(d->map, 0, creq.size);
  fprintf(stderr,"saving mode\n");
  d->s_crtc = drmModeGetCrtc(d->fd, enc->crtc_id);
  fprintf(stderr,"setting mode\n");
  if (drmModeSetCrtc(d->fd, enc->crtc_id, d->fb, 0, 0, (uint32_t *) d->conn, 1, &modeInfo)) {
    fprintf(stderr, "cannot set crtc mode\n");
    perror("mmap");
    return 9;
  }
  return 0;
}

// XXX leaks stuff
int close_display(struct display *d) {
  struct drm_mode_destroy_dumb dreq;
  drmModeCrtcPtr c = d->s_crtc;
  if (drmModeSetCrtc (d->fd, c->crtc_id, c->buffer_id, c->x, c->y, (uint32_t *) d->conn, 1, &c->mode)) {
    fprintf(stderr, "Can't reset display connector\n");
    perror("SetCrtc");
  }
  drmModeFreeCrtc(d->s_crtc);
  if (drmModeRmFB(d->fd, d->fb)) {
    fprintf(stderr, "Can't remove framebuffer\n");
    perror("RmFB");
  }
  memset(&dreq, 0, sizeof(dreq));
  dreq.handle = d->handle;
  if (drmIoctl(d->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq)) {
    fprintf(stderr, "Can't destroy framebuffer\n");
    perror("DestroyFB");
  }
  close(d->fd);
  d->fd = -1;
  return 0;
}

void fill(struct display *d, uint8_t color) {
  memset(d->map, color, d->pitch * d->height);
}

// uses bytes, need 64 bit aligned version
int fill_rect(struct display *d, int x, int y, int w, int h, uint8_t color) {
  int i,j;
  x = x < 0 ? w+=x, 0 : x;
  y = y < 0 ? h+=y, 0 : y;
  w = x + w >= d->width ? d->width - x : w;
  h = y + h >= d->height ? d->height - y : h;
  if (w <=0 || h <= 0) return 1;
  // fprintf(stderr, "%d,%d %dx%d (%dx%d)\n", x,y,w,h, d->width, d->height);

  for (i = x; i < x+w; i++) {
    for (j=y; j<y+h; j++) {
      (d->map)[j * d->pitch + i] = color;
    }
  }
  return 0;
}

// get 8bit color: 0-255
uint8_t get_color(int red, int green, int blue) {
  return ((red>>5 & 7) << 5) | (green>>5 &7) << 2 | (blue>>6 &3);
}
// get 64 bit color
uint64_t solid_color(uint8_t byte) {
  uint64_t solid =  byte | byte<<8 | byte <<16 | byte<<24;
  return solid | solid<<32;
}
