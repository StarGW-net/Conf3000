
/* request program to replace the old shell script - (c) Prophet 1991, Hull */

#include <utmp.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <ctype.h>
#include "stevelib.h"

#define AFILE "/etc/utmp"
#define ALIAS "/users/cs/dad2/conf3/user.alias"
#define ERM "*User error: Usage us [-ug]"

int found; /* YARGH a global variable */

main(argc,argv)
int argc;
char *argv[];
{
  struct utmp record, *ptr;
  int fd;
  FILE *ty;
  int got = 0;
  char buf[20]; 
  char *ptr0;
  long *pt;
  char tty[10];
  char store[3000];  /* store the bits we need from the ALIAS file */
  char *start;       /* point to the start of the store */
  int n;
  char *getalias();   /* get the alias */

  setuid(getuid());

  /* read the file into store */
  start = store;
  n = readfile(ALIAS,start);

  ptr0 = getalias(argv[3],start,n);

  ptr = &record;

  /* open the utmp file for scanning */
  if ((fd = open(AFILE,O_RDONLY)) == -1) {
    fprintf(stderr,"*Can't open %s\n",AFILE);
    exit(-1);
  }

  found = 0;
  while((read(fd,ptr,sizeof(record))) > 0) {
    if ((isalnum(ptr->ut_name[0]))) {
      if (!(strcmp(ptr0,ptr->ut_name))) {
	got = 1;
	if (argc == 5) {
	  if(!(strcmp(argv[4],ptr->ut_line))) {
	    found = 1;
	    strcpy(tty,ptr->ut_line);
	  }
	} else {
	  found = 1;
	  strcpy(tty,ptr->ut_line);
	}
      }
    }
  }
  if (!got) {
    fprintf(stdout,"*No such person currently on sequent\n");
    exit(1);
  }

  if (!found) {
    fprintf(stdout,"*%s is not on %s - I know i checked\n",ptr0,tty);
    exit(1);
  }

  sprintf(buf,"/dev/%s",tty);
  if ((ty = fopen(buf,"a")) == NULL) {
    fprintf(stdout,"*Unable to open %s on %s\n",ptr0,tty);
    exit(1);
  }
  fprintf(ty,"\007*****************************************************\n");
  fprintf(ty," Request for a conference about academic issues\n");
  fprintf(ty," From %s (%s)  -  Respond with ~dad2/conf\n",argv[1],argv[2]);
  fprintf(ty,"*****************************************************\n");

  fprintf(stdout,"*request sent to %s on %s\n",ptr0,tty);

  exit(0);
} /* main */

/* read in the file and store in memory */
readfile(filename, start)
char *filename, *start;
{
  FILE *fp;
  char *ptr1, *ptr2, *store;
  int n = 0;
  char buf[90];
  int i;

  store = start;

  ptr1 = start;

  /* initialise memory */
  for(i=0;i<3000;i++) (*(ptr1++)) = '\000';

  if ((fp = fopen(filename,"r")) == NULL) {
    fprintf(stderr,"*Can't open %s for reading\n",filename);
    exit(-1);
  }

  rewind(fp);

  ptr1 = store;
  ptr2 = store+12;

  /* collect the first two fields and store */
  while((fgets(buf,85,fp)) != NULL) {
    sscanf(buf,"%s %s",ptr1,ptr2);
    ptr1 = ptr1 + 25;
    ptr2 = ptr2 + 25;
    n++;
  }
  return(n);
}

/* replace the user name */
char *getalias(string,start,n)
char *string, *start;
int n;
{
  char *name, *cname, *lname;
  int count = 0;
  struct passwd *getpwnam();

  cname = start+12;

  while (n != count++) {
    if (!(cstrcmp(string,cname))) {
      found = 1;
      return(cname-12);
    }
    cname = cname + 25;
  }

  return(string);
}

/* rip those nasty control codes off */
strip(string)
char *string;
{
  char *ptr1, *ptr2;

  ptr1 = string;
  ptr2 = string;

  while(ptr1 != '\000') {
    if (!iscntrl((*(ptr1)))) {
      (*(ptr2++)) = (*(ptr1++));
/*      fprintf(stderr,"%c",(*(ptr2-1))); */
    } else {
      (*(ptr2)) = '\000';
      return(0);
    }
  }
  (*(ptr2)) = '\000';

}

wipe(string)
char *string;
{
  int i;
  for(i=0;i<16;i++) (*(string++)) = '\000';
}

