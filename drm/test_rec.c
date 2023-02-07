#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "drm_fb.h"
/*
 * Test dwawing a bunch of rectangle
 */
int main(int argc, char **argv) {
  struct display display;
  struct display *d = &display;
  int i, w, h;
  char *name = argc > 1 ? argv[1] : "/dev/dri/card1";
  char mode = argc > 2 ? atoi(argv[2]) : 0;

  srand(time(NULL));
  if (open_display(d, name, mode)) {
    perror(name);
    return 1;
  }
  w = d->width, h = d->height;
  fill(d, solid_color(get_color(255,0,0)));
  for(i = 0; i <1000; i++) {
    fill_rect(d, rand()%w-w/30, rand()%h-h/30, w/15, h/15, rand()&255);
  }
  sleep(10);
  close_display(d);
  return 0;
}
