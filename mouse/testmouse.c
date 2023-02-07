/* Test mouse input using /dev/input/mice */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char** argv) {
    int fd, bytes;
    signed char data[3];
    int left, middle, right;

    fd = open("/dev/input/mice", O_RDWR);
    if(fd == -1) {
        perror("open mice");
        return -1;
    }

    printf("Play with the mouse!\n");
    while((bytes = read(fd, data, sizeof(data))) > 0) {
      left = data[0]&0x1 ? 1 : 0;
      right = data[0]&0x2 ? 1 : 0;
      middle = data[0]&0x4 ? 1 : 0;
      printf("  delta=(%d,%d)\tbuttons=%d%d%d\n", data[1], data[2], left, middle, right);
      }   
    return 0; 
}
