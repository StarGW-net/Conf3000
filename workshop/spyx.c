#include "gubbings.h"
#include <sys/file.h>
#include <stdio.h>
#include <signal.h>
#include <utmp.h>
#include <pwd.h>

char terminalname[10], *ttynam();
char *loginname;
int beep = 0;

main(argc, argv)
int argc;
char *argv[];
{
  int child;
  struct passwd *getpwuid();

  if (argc != 1) {
    if ((strcmp(argv[1],"-n"))) {
      fprintf(stdout,"*Usage: spyx [-n]");
      exit(1);
    } else {
      beep = 1;
    }
  }

  loginname = (*getpwuid(getuid())).pw_name;
  strcpy(terminalname,ttyname(fileno(stdout)));

  child=fork();
  if (child==0) spy();
}


spy()
{

  char chatname[15];
  int flag;
  char lname[15], cname[15];
  FILE *aliasfile;
  FILE *userlist;
  int found;
  char line[85];
  char store1[(MAXUSERS+5)*15];
  char store2[(MAXUSERS+5)*15];
  char *ptr1[MAXUSERS+5];
  char *ptr2[MAXUSERS+5];
  int n1, n2;
  int done1[MAXUSERS+5];
  int done2[MAXUSERS+5];
  int i, i1, i2;
  int silent = 0;

  /* open the alias file for reading */
  aliasfile = fopen(ALIASFILE, "r");
  if (aliasfile == NULL) {
    fprintf(stdout,"*SPYX FILE ERROR: Abandon! Abandon!\n");
    exit(1);
  } /* if */

  /* open the userlist for reading */
  userlist = fopen(USERLIST, "r");
  if (userlist == NULL) {
    fprintf(stdout,"*SPYX FILE ERROR: Abandon! Abandon!\n");
    exit(1);
  } /* if */

  /* REW to start of alias file */
  fseek(aliasfile,0l,0);
  found = 0;
  /* scan alias file for rest of info */
  while (((fgets(line,85,aliasfile)) != NULL) && (!found)) {
    sscanf(line,"%s%s",lname,cname);
    if (!(strcmp(lname,loginname))) {
      found = 1;
      strcpy(chatname,cname);
    } /* if */
  } /* while */

  /* if not found then set to default values */
  if (!(found)) {
    strcpy(chatname,lname);
  } /* if */

  fprintf(stdout,"Started Logging Prophets Conf3000 %s",chatname); 
  if (beep == 1) fprintf(stdout," with noise on");
  fprintf(stdout,"\n");
  for(i=0; i<MAXUSERS+5; i++) {
    ptr1[i] = &store1[i*15];
    ptr2[i] = &store2[i*15];
    done1[i] = 0;
    done2[0] = 0;
  }

  n1 = 0;
  rewind(userlist);
  while((fgets(line,85,userlist)) != NULL) {
    sscanf(line,"%s",ptr1[n1]);
    if (!(strcmp(ptr1[n1],chatname))) silent = 1;
    done1[n1++] = 1;
  }
  if (n1 != 0) n1--;


  flag = 1;
  sleep(5);
  while (scanutmp()) {


    n2 = 0;
    fflush(userlist);
    rewind(userlist);
    while((fgets(line,85,userlist)) != NULL) {
      sscanf(line,"%s",ptr2[n2]);
      done2[n2++] = 1;
    }
 

    i2 = 0;
    while (i2 != n2) {
      if (done2[i2] == 1) {
	i1 = 0;
	found = 0;
	while((i1 != MAXUSERS+3) && (!found)) {
	  if (done1[i1] == 1) {
	    if (!(strcmp(ptr1[i1],ptr2[i2]))) {
	      done1[i1] = 2;
	      done2[i2] = 0;
	      found = 1;
	    } /*if */
	  } /* if */
	  i1++;
	} /* while */
	if (!found) {
	  if (!(strcmp(ptr2[i2],chatname))) {
	    silent = 1;
	  }
	  if (silent == 0) {
	    if (beep) fprintf(stdout,"\007");
	    fprintf(stdout,"*SPYX: %s has logged on to Conf3000\n",ptr2[i2]);
	  }
	    i = 0;
	  while (done1[i] != 0) {
	    i++;
	  } /* while */
	    done1[i] = 2;
	    strcpy(ptr1[i],ptr2[i2]);
	} /* if */
      } /* if */
      i2++;
    } /* while */
    i1 = 0;
    while (i1 != MAXUSERS+3) {
      if (done1[i1] == 1) {
	if (silent == 0) {
	  if (beep == 1) fprintf(stdout,"\007");
	  fprintf(stdout,"*SPYX: %s has logged out of Conf3000\n",ptr1[i1]);
	}
	if (!(strcmp(ptr1[i1],chatname))) {
	  silent = 0;
	}
	
	done1[i1] = 0;
      } else {
	if (done1[i1] == 2) {
	  done1[i1] = 1;
	} /* if */
      } /* if */
      i1++;
    } /* while */
    sleep(5);
  } /* while */
} /* spy */



 
scanutmp()
{
 struct utmp buffer;
 FILE *utmpfile;
 int stillon=0;
 

 utmpfile=fopen("/etc/utmp","r");
 if (utmpfile == NULL) {
   printf("Yargh! It's Fooked\n");
   exit(1);
 }
 while (fread((char*)&buffer,sizeof(struct utmp),1,utmpfile))
   if (*buffer.ut_name)
     {
       /* Note: Assuming 8 is unwise, but how else to do it?! */
       if (! (strncmp(buffer.ut_name,loginname,8) || 
	      strcmp(buffer.ut_line,terminalname+5)) ) stillon=1;
       /* the +5 skips the /dev/ part ---^^ */
     }
 fclose(utmpfile);
 return stillon;
}
