
/* Fooking ace program by the Arch C Wiz The Prophet */

#include <stdio.h>  /* sotra like FROM ... IMPORT */
#include <pwd.h>
#include "gubbings.h"

#define GIT1 "ecs2"     /* splitter */
#define GIT2 "dad2"
#define GIT3 "sw2"
#define GIT4 "is2"

/* main program - for those MOD-2 losers */
main(argc, argv)
int argc;
char *argv[];
{
  struct passwd *getpwuid();     /* complex data structure or what? */
  char buf[60];
  char *git;
  char pathname[80];
  int option = 0;
  char *getwd();
  /* check it's the GIT and is using the *secret* option */

  git = ((*getpwuid(getuid())).pw_name);
  if (((strcmp(GIT1,git)) && (strcmp(GIT4,git)) && (strcmp(GIT3,git)) && (strcmp(GIT2,git)))) {
    /* fake system message - chuckle */
    fprintf(stderr,"%s: Permission denied.\n",argv[0]);
    exit(-1);
  }

    if ( argc != 0 ){
      execl(AUTOREBOOT,"autoreboot",git,argv[1],0);
    } else {
      execl(AUTOREBOOT,"autoreboot",git,0);
    }
  exit(0);
 } 

