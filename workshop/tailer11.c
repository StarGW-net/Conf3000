
/* Conf3000 - tailer program (c) The Prophet Hull 1991 */

#include <sys/file.h>
#include <stdio.h>
#include <strings.h>
#include "gubbings.h"
#include "user.h"
#include <signal.h>

extern struct users user;
extern int chanswap;
extern char chato[80];
extern logout();
extern flipchan();
extern killchan();
extern siren();
extern handler();

int pointer = 0;
char buf[520];
int lnbuf;
int seof = 0;
int messagefile;
int *link[2];
char text[10];
int r;

tailer()
{
  char message[512];
  int lnmessage;
  char ch;
  int number;
  char *ptr;
  int ln, r;
  char ag1[20], ag2[20];
  int ln1;

  signal(SIGALRM,siren);
  signal(SIGUSR1,handler);

  /* check that channel hasn't been changed */
  if (chanswap == 1) {
    if (user.channel == 0) {
      /* display last 10 messages on channel 0 */
      int fpos = last10();
      lseek(messagefile,fpos,0);
    } else {
      /* skip to end of file and start new channel */
      lseek(messagefile,0l,2);
    } /* if */
    chanswap = 0;
  } /* if */

  lnmessage = getmessage(message);

  /* while there are still messages in the buffer */
  while (lnmessage != 0) {

    /* decompose and process the message */
    ch = message[0];
    switch (ch) {

    /* normal message */
    case 'c':
      ptr = &message[1];
      sscanf(ptr,"%d",&number);
      if (user.channel == number) {
	if (user.bell == 'T') {
	  fprintf(stdout,"\007");
	  fflush(stdout);
	}
	if (number > 9) {
	  ln = 4;
	} else {
	  ln = 3;
	} /* if */
	ptr = &message[ln];
	write(1,ptr,lnmessage-ln);
      } /* if */
      break;

    /* check a tell message */
    case 't':
      ptr = &message[1];
      sscanf(ptr,"%s",ag1);
      if (!(cstrcmp(ag1,user.chatname))) {
	ln1 = strlen(ag1);
	ptr = &message[2+ln1];
	if (user.bell == 'T') {
	  fprintf(stdout,"\007");
	  fflush(stdout);
	}
	write(1,ptr,lnmessage-ln1-2);
      }
      break;

    /* alert rest of people not on channel 0 that James or Jon may */
    /* have snuck on */
    case 'r':
      if (user.channel != 0) {
        /*Warning bell taken out for Dreddy */
        ptr = &message[2];
        write(1,ptr,lnmessage-2);
      }
      break;

    case 's':
      ptr = &message[2];
      if (user.bell == 'T') {
	fprintf(stdout,"\007");
	fflush(stdout);
      }
      write(1,ptr,lnmessage-2);
      break;

	
    /* check a gag instruction */  
    case 'g':
      ptr = &message[1];
      sscanf(ptr,"%s %d %s",ag1,&number,ag2);
      if (!(cstrcmp(ag1,user.chatname))) {
	user.gagged = 'T';
	if (user.bell == 'T') {
	  fprintf(stdout,"\007");
	  fflush(stdout);
	}
	fprintf(stdout,"*You have been gagged by %s\n",ag2);
      } else {
	if (number == user.channel) {
	  if (user.bell == 'T') {
	    fprintf(stdout,"\007");	
	    fflush(stdout);
	  }
	  fprintf(stdout,"*%s has been gagged by %s\n",ag1,ag2); 
	} /* if */
      } /* if */
      break;
	
    /* check a invite instruction */  
    case 'h':
      ptr = &message[1];
      sscanf(ptr,"%s %d %s",ag1,&number,ag2);
      if ((!(cstrcmp(ag1,user.chatname))) && (number!=user.channel)) {
	if (user.bell == 'T') {
	  fprintf(stdout,"\007");
	  fflush(stdout);
	}
	flipchan(ag2, number);
      } /* if */
      break;
    case 'm':
      ptr = &message[1];
      sscanf(ptr,"%s %s",ag1,ag2);
      if ((!(cstrcmp(ag1,user.chatname)))) {
	if (user.bell == 'T') {
	  fprintf(stdout,"\007");
	  fflush(stdout);
	}
	killchan(ag2);
      } /* if */
      break;
	 
    /* check an ungag instruction */  
    case 'u':
      ptr = &message[1];
      sscanf(ptr,"%s %d %s",ag1,&number,ag2);
      if (!(cstrcmp(ag1,user.chatname))) {
	user.gagged = 'F';
	if (!(cstrcmp(ag2,user.chatname))) {
	  if (user.bell == 'T') {
	    fprintf(stdout,"\007");
	    fflush(stdout);
	  }
	  fprintf(stdout,"*You ungag yourself\n");
	} else {
	  if (user.bell == 'T') {
	    fprintf(stdout,"\007");
	    fflush(stdout);
	  }
	  fprintf(stdout,"*You have been ungagged by %s\n",ag2);
	}
      } else {
	if (number == user.channel) {
	  if (!(cstrcmp(ag1,ag2))) {
	    if (user.bell == 'T') {
	      fprintf(stdout,"\007");
	      fflush(stdout);
	    }
	    fprintf(stdout,"*%s removes the gag\n",ag1);
	  } else {
	    if (user.bell == 'T') {
	      fprintf(stdout,"\007");
	      fflush(stdout);
	    }
	    fprintf(stdout,"*%s has been ungagged by %s\n",ag1,ag2); 
	  } /* if */
	} /* if */
      } /* if */
      break;


     case 'k':
      ptr = &message[1];
      sscanf(ptr,"%s %s",ag1,ag2);
      if (!(cstrcmp(ag1,user.chatname))) {
	if (user.bell == 'T') {
	  fprintf(stdout,"\007");
	  fflush(stdout);
	}
	fprintf(stdout,"*You have been violently killed by %s\n",ag2);
	sprintf(chato,"has been hit by %s doing 2D8 damage.. oh dear %s is dead\n",ag2, ag1);
	logout();
      } /* if */
      break;

    /* check a netfolk message */
    case 'n':
      ptr = &message[1];
      sscanf(ptr,"%s",ag1);
      if (user.netfolk == 'T') {
	if ((cstrcmp(ag1,user.chatname))) {
	  ln1 = strlen(ag1);
	  ptr = &message[2+ln1];
	  if (user.bell == 'T') {
	    fprintf(stdout,"\007");
	    fflush(stdout);
	  }
	  write(1,ptr,lnmessage-ln1-2);
	}
      }
      break;

    } /* switch */

    lnmessage = getmessage(message);
    } /* while */

} /* tailer */


/* get the next message from the buffer */
getmessage(message)
char *message;
{
  int lnmessage = 0;
  int num;

  if (pointer == seof) {
    num = read(messagefile,buf,SIZE);
    if (num <= 0) {
      return(0);
    } /* if */
    seof = num;
    pointer = 0;
  } /* if */

  while ((buf[pointer] != EOM)) {
    message[lnmessage++] = buf[pointer++];
    if (pointer == 512) {
      num = read(messagefile,buf,SIZE);
      seof = num;
      pointer = 0;
    } /* if */
  } /* while */
  pointer++;

  return(lnmessage);

} /* getmessage */



last10()
{
  FILE *fp;
  int fd;
  int nbuf;
  char buf[300];
  int wrap, cmin, cmax;
  int pos1[10];
  int rem, ln;
  int lpos1[10];
  int fpos, count, lp, number;
  char ag1[15], ag2[15];
  char *czero = "c0";
  char *ptr;

  fp = fopen(MSGSFILE, "r");
  
  count = 0;
  wrap = 0;
  while ((fgrabs(buf,fp)) != NULL) {
    nbuf = strlen(buf);
    rem = 0;
  /* if (!(strncmp(czero,buf,2))) { */
/*    printf("X:%s",buf); */
    switch (buf[0]) {
    case 'c':
      ptr = &buf[1];
      sscanf(ptr,"%d",&number);
      if (number == user.channel) rem = 1;
      break;
    case 's':
      rem = 1;
      break;
    case 'g': case 'u':
      sscanf(buf,"%s %d %s",ag1,&number,ag2);
      if (number == user.channel) rem = 1;
      break;
    default:
      rem = 0;
      break;
    } /* switch */
    if (rem == 1) {
    /*  printf("REM = 1\n"); */
      if (count == 10) {
        count = 0;
        wrap = 1;
      }
      pos1[count] = ftell(fp) - nbuf - 1;
      lpos1[count] = nbuf;
      count++;
    } /* if */
  } /* while */

  fpos = ftell(fp);
  fclose(fp);

  if ((wrap == 0) || (count == 10)) {
    cmax = count;
    count = 0;
    wrap = 0;
  } else {
    cmax = 10;
    cmin = count;
  } 

  fd = open(MSGSFILE,O_RDONLY,0);

  while (count != cmax) {
    lseek(fd,pos1[count],0);
    read (fd,buf,lpos1[count]);
    nbuf = lpos1[count];
   /*  printf("B:%s",buf);  */
    switch(buf[0]) {
    case 'c':
      ptr = &buf[1];
      sscanf(ptr,"%d",&number);
      if (number > 9) {
	ln = 4;
      } else {
	ln = 3;
      } /* if */
      ptr = &buf[ln];
      write(1,ptr,nbuf-ln);
      break;
    case 's':
      ptr = &buf[2];
      write(1,ptr,nbuf-2);
      break;
    /* check a gag instruction */  
    case 'g':
      ptr = &buf[1];
      sscanf(ptr,"%s %d %s",ag1,&number,ag2);
      if (!(cstrcmp(ag1,user.chatname))) {
	fprintf(stdout,"*You have been gagged by %s\n",ag2);
      } else {
	  fprintf(stdout,"*%s has been gagged by %s\n",ag1,ag2); 
      } /* if */
      break;
    case 'u':
      ptr = &buf[1];
      sscanf(ptr,"%s %d %s",ag1,&number,ag2);
      if (!(cstrcmp(ag1,user.chatname))) {
	if (!(cstrcmp(ag2,user.chatname))) {
	  fprintf(stdout,"*You ungag yourself\n");
	} else {
	  fprintf(stdout,"*You have been ungagged by %s\n",ag2);
	}
      } else {
	if (number == user.channel) {
	  if (!(cstrcmp(ag1,ag2))) {
	    fprintf(stdout,"*%s removes the gag\n",ag1);
	  } else {
	    fprintf(stdout,"*%s has been ungagged by %s\n",ag1,ag2); 
	  } /* if */
	} /* if */
      } /* if */
      break;


    default:
      fprintf(stdout,"*ERROR: Unrecognised data on message buffer (fook)\n");
      break;
    } /* switch */
/*    write (1,buf,lpos1[count]);  */
    count++;
  }

  if (wrap == 1) {
    count = 0;
    while (count != cmin) {
      lseek(fd,pos1[count],0);
      read (fd,buf,lpos1[count]);
    nbuf = lpos1[count];
     /*  printf("C:%S",buf); */
    switch(buf[0]) {
    case 'c':
      ptr = &buf[1];
      sscanf(ptr,"%d",&number);
      if (number > 9) {
	ln = 4;
      } else {
	ln = 3;
      } /* if */
      ptr = &buf[ln];
      write(1,ptr,nbuf-ln);
      break;
    case 's':
      ptr = &buf[2];
      write(1,ptr,nbuf-2);
      break;
    /* check a gag instruction */  
    case 'g':
      ptr = &buf[1];
      sscanf(ptr,"%s %d %s",ag1,&number,ag2);
      if (!(cstrcmp(ag1,user.chatname))) {
	fprintf(stdout,"*You have been gagged by %s\n",ag2);
      } else {
	  fprintf(stdout,"*%s has been gagged by %s\n",ag1,ag2); 
      } /* if */
      break;
    case 'u':
      ptr = &buf[1];
      sscanf(ptr,"%s %d %s",ag1,&number,ag2);
      if (!(cstrcmp(ag1,user.chatname))) {
	if (!(cstrcmp(ag2,user.chatname))) {
	  fprintf(stdout,"*You ungag yourself\n");
	} else {
	  fprintf(stdout,"*You have been ungagged by %s\n",ag2);
	}
      } else {
	if (number == user.channel) {
	  if (!(cstrcmp(ag1,ag2))) {
	    fprintf(stdout,"*%s removes the gag\n",ag1);
	  } else {
	    fprintf(stdout,"*%s has been ungagged by %s\n",ag1,ag2); 
	  } /* if */
	} /* if */
      } /* if */
      break;
    default:
      fprintf(stdout,"*ERROR: Unrecognised data on message buffer (fook)\n");
      break;
    } /* switch */

/*      write(1,buf,lpos1[count]); */
      count++;
    }

  close(fd);

  }
  return (fpos);

}

/* special stream instruction uses EOM instead of RETURN */
fgrabs(s, iop)
char *s;
FILE *iop;
{
  register int c;
  register char *cs;
  cs = s;
  while ((c = getc(iop)) != EOF)
    if ((*cs++ = c) == EOM)
      break;
  if (c == EOF) {
    return NULL;
  } else {
    return 10;
 } 

} /* fgrabs */


/* initailise the tailer */
starttail()
{

  int fpos;

  /* display the last 10 messages on channel 0 */
  if (user.channel == 0) {
    fpos = last10();
    /* open message file and skip to where last10 finished */
    messagefile = open(MSGSFILE,O_RDONLY,0);
    lseek(messagefile,fpos,0);
  } else {
    /* open message file and skip to where end */
    messagefile = open(MSGSFILE,O_RDONLY,0);
    lseek(messagefile,0l,2);
  }

} /* starttailer */

fresh()
{
  int fpos = last10();
  lseek(messagefile,fpos,0);
}
