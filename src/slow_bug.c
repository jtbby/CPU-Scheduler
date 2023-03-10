
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#define SLEEP_USEC 500000 // 1000 = 1ms, 1000000 = 1sec
#define ITERATIONS 10 // Number of times to print

int main(int argc, char *argv[]){
  char *pic[] = {
"Fight Bugs                      |     |        ",
"                                \\\\_V_//        ",
"                                \\/=|=\\/        ",
"                                 [=v=]         ",
"                               __\\___/_____    ",
"                              /..[  _____  ]   ",
"                             /_  [ [  M /] ]   ",
"                            /../.[ [ M /@] ]   ",
"                           <-->[_[ [M /@/] ]   ",
"                          /../ [.[ [ /@/ ] ]   ",
"     _________________]\\ /__/  [_[ [/@/ C] ]   ",
"    <_________________>>0---]  [=\\ \\@/ C / /   ",
"       ___      ___   ]/000o   /__\\ \\ C / /    ",
"          \\    /              /....\\ \\_/ /     ",
"       ....\\||/....           [___/=\\___/      ",
"      .    .  .    .          [...] [...]      ",
"     .      ..      .         [___/ \\___]      ",
"     .    0 .. 0    .         <---> <--->      ",
"  /\\/\\.    .  .    ./\\/\\      [..]   [..]      ",
" / / / .../|  |\\... \\ \\ \\    _[__]   [__]_     ",
"/ / /       \\/       \\ \\ \\  [____>   <____]    ",
"https://www.asciiart.eu/computers/bug Designed by Unknown Artist"
}; //https://asciiartist.com/respect-ascii-artists-campaign/

  int i = 0;
  for(i = 0; i < 22; i++) {
    printf("[PID: %d] %s\n", getpid(), pic[i]);
    usleep(SLEEP_USEC);
  }
  
  return 0;
}
