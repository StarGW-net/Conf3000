
/* Replaces that nasty bug ridden script file :-) */

#include <stdio.h>
#include "gubbings.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#define ALT ".alt"
#define CHATIO ".chatIO"
#define CONF3 "/users/cs/dad2/conf3/conf3"

char store[90*3];
char *line[3];

main(argc, argv)
int argc;
char *argv[];
{
  char path[60];
  FILE *alt;
  FILE *chatio;
  int iofound = 0;
  int tfound = 0;
  int n, fd;
  struct stat finfo, *finfop;
  int perms;

  finfop = &finfo;
  for(n=0;n<3;n++) {
    line[n] = &store[90*n];
  } 

  if (argc != 1) {
    sprintf(path,"%s/%s",getenv("HOME"),ALT);
    if ((alt = fopen(path,"r")) != NULL) {
      for(n=1;n<argc;n++) {
	if (((*argv[n]) == '-') && (tfound == 0)) {
	  if ((tfound = findflag(alt,argv[n],1)) == 0) fprintf(stdout,"*Unable to find alias flag %s\n",argv[n]);
	} else {
	  if (iofound == 0) {
	    if ((iofound = findflag(alt,argv[n],2)) == 0) fprintf(stdout,"*Unable to find IO flag %s\n",argv[n]);
	  } /* if */
	} /* if */
      } /* for */
    } else {
      fprintf(stdout,"*You do not have a .alt file to set flags (consult ~dad2/Alphaks/alt)\n");
      iofound = 0;
      tfound = 0;
    } /* if */
  } /* if */
  if (iofound == 0) {
    sprintf(path,"%s/%s",getenv("HOME"),CHATIO);
    if ((chatio = fopen(path,"r")) != NULL) {
      iofound = 2;
      if (fgets(line[0],75,chatio) == NULL) iofound = 0;
      if (fgets(line[1],75,chatio) == NULL) iofound = 0;
      fclose(chatio);
    } /* if */
  } /* if */

  if (iofound == 2) fprintf(stdout,"*Keeping same .chatIO\n");
  if (iofound == 1) fprintf(stdout,"*Changing .chatIO\n");

  sprintf(path,"%s/%s",getenv("HOME"),CHATIO);


  if (iofound != 0) {
    chatio = fopen(path,"w+");
    fprintf(chatio,"%s%s",line[0],line[1]);
  } else {
    if (tfound != 0) {
      chatio = fopen(path,"w+");
      fprintf(stdout,"*Creating default .chatIO\n");
      fprintf(chatio,"has entered\n");
      fprintf(chatio,"has left\n");
    } /* if */
  } /* if */
  if (tfound != 0) {
    fprintf(stdout,"*New alias is: %s",line[2]);
    fprintf(chatio,"%s",line[2]);
  }
  if ((tfound != 0) || (iofound != 0)) {
    fflush(chatio);
    fclose(chatio);
    chmod(path,0644);
  } /* if */

  fd = fileno(stdin);
  fstat(fd,finfop);
  perms = ((finfop->st_mode) & 0000022);

  if (perms == 18) {
    if ((fchmod(fd,0611)) != -1) fprintf(stdout,"*Messages turned off\n");
  }

  if (tfound != 0) execl(CONF3,"conf3","-a",0);

  execl(CONF3,"conf3",0);


} /* main */

findflag(file,flag,option)
FILE *file;
char *flag;
int option;
{
  int result = 0;
  char text[90];
  char fg[95];
  int count = 0;

  rewind(file);
  while((fgets(text,85,file)) != NULL) {
    if (text[0] == '%') {
      int i = 0;
      while ((text[i+1] != '%') && (i<strlen(text))) {
	fg[i++] = text[i+1];
      } /* while */
      fg[i] = EOM;
      if (!(strcmp(flag,fg))) {
	switch (option) {
	case 2:
	  strcpy(line[count++],&text[i+2]);
	  if (count == 2) return (1);
	  break;
	case 1:
	  strcpy(line[2],&text[i+2]);
	  return(1);
	  break;
	} /* switch */
      } /* if */
    } /* if */
  } /* while */
} /* find flag */

