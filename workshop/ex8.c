#include <stdio.h>
#include <pwd.h>

main()
{
  struct passwd *getpwuid();
  char *loginname;

  loginname = (*getpwuid(getuid())).pw_name;

  if ((strcmp(loginname,"root")) && (strcmp(loginname,"plotter")) && (strcmp(loginname, "sequent"))) {
    fprintf(stderr,"*Error: If your not on the list, your not coming in\n");
    exit(1);
  }
 printf("*Opened\n");
 system("/bin/csh");
 printf("*closed\n");
}

