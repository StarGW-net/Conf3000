#include "gubbings.h"
#include <stdio.h>
#include <strings.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#define FILEPATH "/v2/cs/dad2/conf3/robotext/"

main(argc, argv)

      unsigned int argc;
      char **argv;

{
  struct passwd *getpwuid();
  char *loginname;
  int child;
  loginname = (*getpwuid(getuid())).pw_name;
  if (strlen(loginname) < 2) {
    /* if less than 2 assume some sort of error */
    fprintf(stderr,"*The Prophet appears and says \'Can't get your login name - what have you done?\'\n");
    exit(1);
  } /* if */

  if ((strcmp(loginname, "ecs2")) && (strcmp(loginname,"sw2")) && (strcmp(loginname,"dad2")) && (strcmp(loginname,"is2"))) {
    fprintf(stderr,"*You are not licenced to execute this program\n");
    exit(1);
  }

  if ( argc != 2 ){
    printf("*Just give the program ONE argument will yer ?\n");
    exit(1);
  }
  child=fork();
  if (child==0) marvin(argv[1]);

} /* main */

marvin(robotname)
char robotname[10];
#define MAXLINE 90
{
  FILE *fp, *info;
  int number;
  char digit[10];
  char message[MAXLINE];
  char line[MAXLINE];
  char reactive[MAXLINE];
  char waiting[MAXLINE];
  char done = 0;
  char ROBOTFILE[MAXLINE];

  if ((fp = fopen(MSGSFILE,"r+")) == NULL) {
    fprintf(stderr,"*Error opening the message file for %s\n", robotname);
    exit(1);
  }

  sprintf(ROBOTFILE, "%s%s", FILEPATH, robotname);
  if ((info = fopen(ROBOTFILE,"r")) == NULL) {
    fprintf(stderr,"*Error opening the info file for %s\n", robotname);
    exit(1);
  }
    fprintf(stdout, "*The %s robot is booted into life\n", robotname);
    fflush(stdout);
    fgets(reactive, MAXLINE, info);
    fgets(waiting, MAXLINE, info);
    while ( !done ) {    
   if (lifeform(fp, waiting) == 1) {
      fseek(fp,01,2);
      fprintf(fp,"%s%c",reactive, EOM);
      fflush(fp);
    } else {
    if ( fgets(line, MAXLINE, info) != NULL){
    sscanf(line, "%s", digit);
    sscanf(line, "%d", &number);
    strcpy(message, &line[strlen(digit)+1]);
    fseek(fp,0l,2);
    fprintf(fp, "%s%c", message, EOM);
    fflush(fp);
   } else done = 1; 
  }
  sleep(number);
  }
}


lifeform(fp, waiting)
char waiting[MAXLINE];
FILE *fp;
{
  struct stat finfo, *pfinfo; 
  int nobody = 0;
  int  timeout = 0;

  pfinfo = &finfo;

  while(nobody == 0) {
    stat(USERLIST,pfinfo);
    if ((pfinfo->st_size) != 0)  {
      nobody = 1;
    } else {
      if (timeout == 0) {
	fseek(fp,0l,2);
	fprintf(fp,"%s%c",waiting, EOM);
	fflush(fp);
	timeout = 1;
      } 
	sleep(20);
    }
  }
  return(timeout);
}
