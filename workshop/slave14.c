

/* Conf3000 - slave program (c) The Prophet Hull 1991 */
/* Bugs implemented by Sinjy the Sinj 1992 */

/* my own files */
#include "fmat11.h"
#include "stevelib.h"
#include "packet1.h"
#include "gubbings.h"
#include "user.h"
#include "chan.h"

#include <sys/time.h>
#include <signal.h>
#include <ctype.h>
#include <sys/file.h>
#include <stdio.h>
#include <utmp.h>
#include <sgtty.h>
#include <pwd.h>

int keypress;  /* indicates there is data on the input stream */
int reboot = 0; /*another global var for auto rebooting*/
int wakeup;    /* signal for the suspend(n) routine */
int atimer = 0;    /* indicates timer routine has control */
struct itimerval buzzer; /* buzzer timer */
int nbuzz = 0; /* number of buzzes already occured */
int timeout;   /* time before timeout message */
int stimer;    /* indicates which sort of timer will be used for timeout */
int owner = 0; /* fudge fudge fudge */
char timemesg[80]; /* current timeout message */
FILE *timefile;  /* timout message/time file */
int accept = 0;

int bg = 0;        /* stop nasty people running in the back ground */

int got_sig;       /* received signal from server process */
int spid;          /* process id of the server */
int chanswap = 0;  /* idicate a change of channel taken place */
FILE *userfile;
FILE *aliasfile;
FILE *resfile;      /*A file to keep scum of your own channel(s) */
char chati[80], chato[80];      /* chatio messages */
struct sgttyb newtty, oldtty;   /* terminal states */
int infd;                       /* terminal state fdes */
int msgsfile;                   /* message file (buffer */
int delchar = DEL;              /* default delete character */
char terminalname[10];     
char text[300];                 /* global string */
int lntext;                     /* length of global string */
char teminalname, *ttyname();
int quit;                   
int lnchatname;
int arg;

/* these have to be global because inturrputs will need them */
int slavepipe, serverpipe;
struct slavepk outpk;  /* ouput pcaket */
struct serverpk inpk;  /* input packet */

/* store everything about the user */
struct users user;

extern tailer();
extern starttail();
extern fresh();

main(argc, argv)
int argc;
char *argv[];
{

  char *loginname;
  FILE *serverpid;
  int i,r;
  FILE *chatiofp;
  int home;
  char ch;
  int timegone;     /* psuedo clock timeout counter */
  int recon = 0;    /* indicate reconnect status */
  char zz;
  char cha[3];      /* used to check data on the input stream */
  int id;
  struct passwd *getpwuid();
  int guest;


  /* set up the signal handling routines */
  extern handler(), redotty(), siren(), ioready(), hangup(), bossbut(), onintr(), bailout(), coredump(), boot();
  extern background();
  signal(SIGTTIN,background);
  signal(SIGTTOU,background);
  signal(SIGPIPE,boot);
  arg = 0;


  if ((userfile= fopen(USERLIST, "r")) == NULL) {
    file_error(USERLIST);
  }

 /* Show number of losers on conf*/
  numberusers(); 

  /* get the login name of the user */
  id = getuid();
  loginname = (*getpwuid(id)).pw_name;
  if (argc != 1) {

    if (!(strcmp(argv[1],"-g"))) {
      if((strcmp(loginname,"in6"))) {
	printf("*Sorry guest login's not allowed\n"); 
	exit(1);
      }
      guest = 1;
      printf("GUEST\n");
    } else {
      if ((strcmp(argv[1],"-a")) || (argc != 2)) {
	fprintf(stdout,"*No such argument guv\n");
	exit(1);
      }
      arg = 1;
    }
  }


  /* deal with signal from server */
  signal(SIGUSR1,handler);

  /* assign signal handlers */
  signal(SIGINT,bossbut);
  signal(SIGQUIT,onintr);
  signal(SIGTSTP,onintr);
  signal(SIGTERM,onintr);
  signal(SIGSEGV,coredump);
  signal(SIGHUP,hangup);
  signal(SIGUSR2,bailout);   /* emergency shutdown */
  signal(SIGALRM,siren);     /* deal with the various alarm timers */


  /* open pipe */
  slavepipe = open(SLAVEPIPE,O_WRONLY|O_NDELAY);
  if (slavepipe < 0) {
    system(SERVER);
  /*For when Steve works it out*/
  /*  system(WELCOME); */
    fprintf(stdout,"*Booting Server...\n");
    i = 0;
    /* time it out if it doesn't open fast enough */
    while ((slavepipe = open(SLAVEPIPE,O_WRONLY|O_NDELAY)) < 0) {
      fprintf(stdout,"*Retrying Dammit...\n");
      if ((++i) == 10) {
	fprintf(stdout,"*The Prophet appears and stares at the charred ruins of his program\n From his robes he pulls out a listing, glances down at it, and mutters\n \'Hardware error - try it again\'\n");
	exit(1);
      } /* if */
    } /* while */
  } /* if */

  /* make sure user is using a valid terminal */
  checktty();

  /* get the process ID of the server process */
  serverpid = fopen(SERVERPID,"r");
  if (serverpid == NULL) {
    file_error(SERVERPID);
  } /* if */
  fscanf(serverpid,"%d",&spid);

 
  
  if (strlen(loginname) < 2) {
    /* normally because in a sub shell and it returns nothing not NULL */
    fprintf(stderr,"*The Prophet appears and says \'Can't get your login name - what have you done?\'\n");
    exit(1);
  } /* if */
 
/*more fudge code due to the dweebs in the cc */ 
  if (strlen(loginname) > 5){
    fprintf(stdout, "*Oh deary me, username over 5 characters.  Initiating another fudge patch\n");
    sprintf(loginname , "%d", (*getpwuid(id)).pw_uid);
   }
  strcpy(user.loginname,loginname);
  if (guest == 1) {
    guest = strlen(user.loginname);
    user.loginname[guest] = 'g';
    user.loginname[guest+1] = EOM;
  }

  /* get the terminal info for scanning utmp later */
  strcpy(terminalname,ttyname(fileno(stdout)));

  /* build up CONNECT packet to send */
  strcpy(outpk.text,user.loginname);
  user.pid = getpid();
  outpk.id = user.pid;
  outpk.action = CONNECT;

  /* open pipe to connect to server process*/
  serverpipe = open(SERVERPIPE,O_RDONLY|O_NDELAY);

  transmit();
  if (inpk.status<=MAXUSERS) bg=1;

  /* initialise tty terminal status for program */
  fflush(stdin);
  fflush(stdout);
  fflush(stderr);
  inittty(); 

  /* determine response of server to connection request */
  switch (inpk.status) {
    case (MAXUSERS+1):
      fprintf(stdout,"*You ride up to the gates on your steed. In front of you stands The Prophet\n holding a clipboard. He points at the firmly locked gates and bars yor way\n with his staff \'No more room in the Inn\'\n");
      redotty();
      break;
    case (MAXUSERS+2):
      fprintf(stdout,"*Before the open gates stands The Prophet baring the way. He looks down and\n checks his clip board then looks back up and gives you a cold hard stare\n \'You have sinned and are nolonger welcome within these gates. Begone\'\n");
      redotty();
      break;
    case (MAXUSERS+3):
      fprintf(stdout,"*The Prophet appears checks his clip board and bars your way\n He intones \'Lo mortal you have already passed this way once\'\n            \'Do you wish to reconnect (Y/N)\'\n");
      /* time out response */
      signal(SIGALRM,redotty);
      alarm(7);
      ch = getchar();
      alarm(0);
      signal(SIGALRM,siren);
      if ((ch != 'Y') && (ch != 'y')) {
	redotty();
      }
      /* attempt to reconnect */
      outpk.action = RECONNECT;
      transmit();
      if (inpk.status > MAXCHAN-1) {
	user.id = inpk.status-MAXCHAN;
        outpk.action = WHATCHAN;
	outpk.id = user.id;
	transmit();
	user.channel = inpk.status;
	recon = 1;
      } else {
	user.id = inpk.status;
	user.channel = 0;
      }
      break;


    default:
      /* assign user details */
      user.id = inpk.status;  
      user.channel = 0;
      break;
  } /* switch */

  bg = 1;

  /* open the alias file for reading */
  aliasfile = fopen(ALIASFILE, "r+");
  if (aliasfile == NULL) {
    file_error(ALIASFILE);
  } /* if */


 /* open the reserved channel file for reading */
  resfile = fopen(RESFILE, "r+");
   if (resfile == NULL) {
     file_error(RESFILE);
   } /*if*/

  /* grab info from alias file */
  grabuser();

  /* get the chatio file if one exists */
  home = getenv("HOME");
  sprintf(text,"%s%s",home,"/.chatIO");
  r = 0;
  if (user.chatio == 'F') {
    r = 4;
  } /* if */
  chatiofp = fopen(text,"r");
  if (chatiofp == NULL) {
    r = 1;
  } else {
    fseek(chatiofp,0l,0);
    if (fgets(chati,70,chatiofp) == NULL) r = 2;
    if (fgets(chato,70,chatiofp) == NULL) r = 3;
    if (arg == 1) {
      if (fgets(text,50,chatiofp) == NULL) arg = 0;
    }
    fclose(chatiofp);
  } /* if */
  /* if chatio file don't exist set to default */
  if (r != 0) {
    strcpy(chati,"has entered\n");
    strcpy(chato,"has left\n");
  } /* if */

  user.bell = 'F';

/*  printf("ARG %d %s\n",arg,text); */
  if (arg == 1) {
    int lp;
    char *ptr;
  
    ptr = &text[0];
    lntext = strlen(text);
    ptr = ptr + lntext -1;
    /* pad less than 50 out with spaces */
    for (lp = lntext-1; lp < 50; lp++)
      (*(ptr++)) = ' ';
    (*ptr) = EOM;    
    
    /* notify server */
    outpk.action = CHANGEALIAS;
    outpk.id = user.id;
    strcpy(outpk.text,text);
    transmit();

  } /* arg = 1 */


  /* Open message file for writing at the end of the file*/
  msgsfile = open(MSGSFILE,O_WRONLY|O_APPEND,0);


  /* open timeout message file */
  timefile = fopen(TIMEFILE,"r");
  if (timefile == NULL) {
    file_error(TIMEFILE);
  } /* if */
  

  /* display header info file */
  pg(HEADER,0);

  if (recon != 1) {
    chanmessage(user.channel);
    fprintf(stdout,"\n");

    system(CHECKN);
    fprintf(stdout,"\n");

    /* IF super then SUPER entrance */

   if ( user.super == 'T'){

    sprintf(text,"c%d *The skys darken, thunder rolls....\n",0);
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile, text, lntext);
    }
    /* generate auto logging in message and append to message file */
    sprintf(text,"c%d *%s %s",0,user.chatname,chati);
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile, text, lntext);
    sprintf(text,"r *New arrival on channel 0: %s\n",user.chatname);
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile,text,lntext);
  } else {
    if (user.channel != 0) fprintf(stdout,"*You have been reconnected on channel %d\n",user.channel);
    sprintf(text,"c%d *%s has reconnected\n",user.channel,user.chatname);
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile, text, lntext);
  } /* if */ 

  /* main control loop of program */
  fflush(stdout);
  fflush(stdin);
  starttail(); /* look at last 10 things that happened on chan 0 */
  /* right this bit ignores all key presses before the prog starts looking */
  /* for input. The initial getchar() sets up a buffer which means ungetc */
  /* will work. This has been dumping cores and logging out of seq */
  /* finally nailed it */
  fcntl(0,F_SETFL,FNDELAY);
  getchar();
  while ((read(0,cha,1)) != -1)
    ;
  quit = 0;
  while (!(quit)) {
    /* reset timeout values */
    nbuzz = 0;
    timegone = 0;
    stimer = 0;
    setbuzzer(stimer);  /* set new timeout value */
    /* check for data already on the input stream */
    fcntl(0,F_SETFL,FNDELAY);
    if ((read(0,cha,1)) != -1) {
      keypress = 1;
      /* put it back on */
      ungetc(cha[0],stdin);
    } else {
      keypress = 0;
    }
    /* wait for the user to enter something */
    while (!keypress) {
      /* prog crashes if someone rights directly to terminal unless */
      /* the following two lines are inside this loop */
      signal(SIGIO,ioready); 
      fcntl(0,F_SETFL,FASYNC); /* give signal when keypressed */
      tailer();    /* scan message file while we wait */
      suspend(1);  /* sleep for 1 sec to keep CPU time down */

      /* check for data already on the input stream */
      /* should prevent keyboard locks- I hope */
      if (keypress == 0) {
	fcntl(0,F_SETFL,FNDELAY);
	if ((read(0,cha,1)) != -1) {
	  keypress = 1;
	  /* put it back on */
	  ungetc(cha[0],stdin);
	}
	
	signal(SIGIO,ioready); 
	fcntl(0,F_SETFL,FASYNC); /* give signal when keypressed */
      } /* prevent hang up */

      /* check for a timeout */
      if ((timegone++) == timeout) {
	buzz();
	timegone = 0;
      } /* if */
    } /* while */
    read_command();  /* read in from the keyboard */
    stopalarm();
  } /* while not quit */

} /* main */

/* read a command in from the keyboard */
read_command()
{
  char ch;
  int lnbuf;
  char buf[300];
  int r;
  int loop =2;

  /* set reading the input back to normal */
  signal(SIGIO,SIG_IGN);
  fflush(stdout);
  fcntl(0,F_SETFL,2);

  /* start up a timeout */
  stimer = 1;
  setbuzzer(stimer);
   
  lnbuf = 0;       /* length of the input buffer */
  ch = 'z';         /* starting value - as long as not return */


  /* wait for a return with atleast 1 character in the input buffer */
  while((ch != RETURN) || (lnbuf == 0)) {
    ch = getchar();
    /* move screen up for Return with empty buffer */
    /* useful to tidy up screen */
    if ((ch == RETURN) && (lnbuf == 0)) {
      putchar(ch);
      fflush(stdout);
      return(0);
    } else {
      /* check type of character */
      if (((!(iscntrl(ch))) && (lnbuf < 230)) || (ch == RETURN)) {
        /* deal with a printable character */
        /* store character in input array and echo to screen */
        buf[lnbuf++] = ch;
        putchar(ch);
	fflush(stdout);
      } else {
        /* deal with a delete character */
        if ((ch == delchar) && (lnbuf != 0)) {
          /* move cursor back and blank out character */
          putchar(ERASE);
          putchar(' ');
          putchar(ERASE);
          fflush(stdout);
          /* alter input array accordingly */
          lnbuf--;
          /* check if no characters are in the input buffer */
          if (lnbuf == 0) {
            /* return to scanning the file */
            return(0);
	  } /* if */
	} else {
	  if (lnbuf == 0) {
	    return(0);
	  } /* if */
	} /* if */
      } /* if */
    } /* if */
  } /* while return */
   
  /* write end of string char to the input string */
  buf[lnbuf] = EOM;    
  fflush(stdout);

  /* deal with the input command */
  switch(buf[0]) {

 
  /*Nice real mud type says*/ 
  case '\"': 
  case '\'':
            
            buf[lnbuf-1]='\"';
            buf[lnbuf]='\n';
            buf[++lnbuf]=EOM;
            sprintf(text, " says \"%s", &buf[1]);
            strcpy(buf, text);
           priatmosphere(buf); 
          break;      

  /*Singing (until Prophet finds out)*/
    case '#':
            if ( lnbuf == 2){
            strcpy(buf, " sings silent nite\n%c", EOM);
            } else {
            buf[lnbuf-1]='#';
            buf[lnbuf]='\n';
            buf[++lnbuf]=EOM;
            sprintf(text, " sings: #%s", &buf[1]);
            strcpy(buf, text);
           }
            priatmosphere(buf);
           break;
  case  '.' :
  case  ',' :
    /* if second char not a space there is no such instruction */
    if (isspace(buf[2])) {
      switch(buf[1]) {

      /* change channel */
      case 'c':
        if (changechan(buf) == 5) who(".w -c\n");
	break;

      /* list of channels - Pete made me do it guv */
      case '&':
        chanlist(buf);
        break;
      case '*':
	slipin(buf);
	break;

      case '<':
	minikill(buf);
	break;

      /* redisplay channel message */
      case '!':
        chanmessage(user.channel);
        break;

      /* count the number of users */
      case '#':
        numberusers();
        break;
 
      case '+':
	system(RDN);
	break;

      /* lock current channel */
      case 'l':
	lockchan();
        break;
	
      /* open current channel */
      case 'o':
	openchan();
        break;

      /* have a look whos on chat */
      case 'w' :
        who(buf);
        break;
      
      /*Remote emote*/
      case 'j':
	rtell(buf);
	break;
     
      /* tell someone something in secret */
      case 't' :
        tell(buf);
        break;

      /* change the second bit of the user name */
      case 'a':
	changealias(buf);
        break;

      /* gag/ungag someone */
      case 'g':
        if (user.killer == 'T') {
	  gag(buf);
        } else {
	  fprintf(stdout,"*No such instruction - stop wasting processor time\n");
        } /* if */
	break;

      /* read the info file */
      case 'i':
	pg(INFOFILE,0);
	break;


      /* kill someone */
      case 'k':
        if (user.killer == 'T') {
	  kick(buf);
        } else {
	  fprintf(stdout,"*No such instruction - stop wasting processor time\n");
        } /* if */
	break;

     /*hitman someone */
     case 'h':
       if (user.killer == 'T') {
          hitman(buf);
         } else {
          fprintf(stdout, "*Your not hard enough to hire a hitman\n");
         } /* if */
         break;


      case 'y':
	yp(buf);
	break;

      /* shell out - temp command prone to crashing */
      case 'x':
        if (user.super == 'T') {
	  ioctl(infd,TIOCSETP,&oldtty);
	  system("/users/cs/dad2/Alphaks/shell");
	  fflush(stdin);
	  inittty();
        } else {
	  fprintf(stdout,"*No such instruction - stop wasting processor time\n");
        } /* if */
	break;

      /* update user */
      case 'm':
        if (user.super == 'T') {
	  if ((update()) == 2) {
	    fprintf(stdout,"\n*Aborted!\n\n");
	  } /* if */
	} else {
	  fprintf(stdout,"*No such instruction - stop wasting processor time\n");
        } /* if */
	break;

      case 'p':
	pint(buf);
	break;

      case '^':
	jelly(buf);
	break;

      /* change the delete character */
      case 'd':
	chdel();
	break;

      /* help, help I need somebody */
      case '?':
	help();
	break;

      /* request chat with unix user */
      case 'r':
        request(buf);
	break;

      /* display etime */
      case 'e':
	fprintf(stdout,"*");
	fflush(stdout);
	system(TIME);
	break;

      /* shout - across channels */
      case 's':
        if (user.shout == 'T') {
	  shout(buf);
        } else {
	  fprintf(stdout,"*No such instruction - stop wasting processor time\n");
        } /* if */
	break;
	
      /* unix userlist */
      case 'u':
	userlist(buf);
	break;

      /* whois (nasty C style script file) */
      case '$':
         whois(buf);
         break; 

      /* refresh channel 0 screen */
      case 'f':
	if (user.channel ==0) {
	  fresh();
	} else {
	  fprintf(stdout,"*Can only refresh channel 0\n");
	}
	break;

      /* view user log */
      case 'v':
	view(buf);
	break;

      /* quit the program, sniff, sniff */
      case 'q' :
	fflush(stdout);
	quit = 1;
	newio(buf);
	fprintf(stdout,"*%s %s",user.chatname,chato);
	fprintf(stdout,"*As you leave The Prophet crosses your name off his clip board. Goodbye.\n");
        logout();
        break;


      /* emergency shut down */
      case 'z':
        if (user.super == 'T') {
	  shutdown(buf);
        } else {
	  fprintf(stdout,"*I'll shut you down, if you try that again\n");
        } /* if */
	break;

      /* toggle noise */
      case 'n':
	nbell();
	break;

      /* ban user */
      case 'b':
        if (user.super == 'T') {
	  ban(buf);
        } else {
	  fprintf(stdout,"*Banning is for supers\n");
        } /* if */
	break;

      default:
        /* can't find instruction */
        fprintf(stdout,"*No such instruction - stop wasting processor time\n");
        break;

      } /* switch */

    } else {
      /* can't find instruction */
      fprintf(stdout,"*No such instruction - stop wasting processor time\n");
    } /* if */
    break;

  /* do an atmosphere, "I love a party with an atmosphere..." */
  case ':' :
  case  ';':
    priatmosphere(buf);
    break;

  default:
    /* if it aint anything else then its gotta be a message */
    primessage(buf);
    break;
    
  } /* switch */

} /* read_command */



/* leave with text diff to ya .chatIO */
newio(buf)
char *buf;
{
  if (user.chatio == 'T') {
    if (strlen(&buf[3]) > 5) {
      strcpy(chato,&buf[3]);
     /* if chato too long */
      if (strlen(chato) > 70 ){
        chato[69]='\n';
        chato[70]=EOM;
       } 
    }
  } /* if */
} /* newio */


/* leave the program - quit with 'chato' message */
logout()
{

  char ag1[70];
  /* notify server */
  outpk.id = user.id;
  outpk.action = LOGOUT;
  transmit();

  if (inpk.status != 0) {
    /* generate auto logging out message and append to message file */
    sprintf(text,"c%d *%s %s",user.channel,user.chatname,chato);
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile, text, lntext);
  } /* if */
  redotty();
} /* logout */



/* open current channel */
openchan()
{

  if (user.channel == 0) {
    fprintf(stdout,"*It's impossible to lock channel 0 in the first place\n");
    return(0);
  } /* if */

  /* notify server */
  outpk.action = OPEN;
  outpk.id = user.id;
  transmit();
  
  if (inpk.status == 1) {
    fprintf(stdout,"*Its already open you fool\n");
    return(0);
  } /* if */

  /* open the channel */
  sprintf(text,"c%d *%s has opened the channel\n",user.channel,user.chatname);
  lntext = strlen(text);
  text[++lntext] = EOM;
  write(msgsfile, text, lntext);
  
} /* open chan */


/* lock current channel */
lockchan()
{

  if (user.channel == 0) {
    fprintf(stdout,"*Not even The Prophet can lock channel 0\n");
    return(0);
  } /* if */

  /* notify server */
  outpk.action = LOCK;
  outpk.id = user.id;
  transmit();

  if (inpk.status == 1) {
    fprintf(stdout,"*The channel is already locked you fool\n");
    return(0);
  } 

  /* can't locak a chanel with only 1 person on */
  if (inpk.status == 2) {
    fprintf(stdout,"*Who needs friends when you can sit alone on a locked channel?\n");
    return(0);
  }

  /* lock channel */
  sprintf(text,"c%d *%s has locked the channel\n",user.channel,user.chatname);
  lntext = strlen(text);
  text[++lntext] = EOM;
  write(msgsfile, text, lntext);

} /* lockchan */


/* change channel */
changechan(buf)
char *buf;
{

  char *ptr1, *ptr2;
  int ln;
  int chan;
  int r;
  long int number;
  char ag1[20];
  int ln1;

  ag1[0] = EOM;
  buf[6] = EOM;  /* take first 7 characters */
  
  /* point to argument list and take out first argument */
  ptr1  = &buf[3];
  sscanf(ptr1,"%s",ag1);
  if (strlen(ag1) == 0) {
    fprintf(stdout,"*Well I'm not going to choose a channel for you\n");
    return(0);
  }

  sscanf(ptr1, "%d", &number);

  if ((number < 0) || (number > MAXCHAN-1)) {
    fprintf(stdout,"*There is no such channel - what are you doing making them up?\n");
    return(1);
  } /* if */

  chan = number;

  if (chan == user.channel) {
    fprintf(stdout,"*You are already on channel %d you fool\n", user.channel);
    return(1);
  } /* if */


  if ((r=reserved(chan, user.loginname)) == 1){
     sprintf(text, "c%d *%s is attempting to gatecrash the channel\n", chan, user.chatname);
   lntext = strlen(text);
   text[++lntext]=EOM;
   write(msgsfile, text, lntext);
   return(1);
  }

  if(r==2) {
    fprintf(stdout, "*You show your invite and are allowed in\n");

    outpk.id=user.id;
    outpk.action = FLIPCHAN;
    outpk.number = chan;
    transmit();
  } else {

    /* notify server */
    outpk.id = user.id;
    outpk.action = CHANGECHAN;
    outpk.number = chan;
    transmit();
    
    if (inpk.status == 1) {
      fprintf(stdout,"*Sorry channel %d is locked\n",chan);
      return(1);
    } /* if */
  }

  /* generate parting message and append to file */
  sprintf(text,"c%d *%s has left the channel\n",user.channel, user.chatname);
  lntext = strlen(text);
  text[++lntext] = EOM;
  write(msgsfile,text,lntext);

  /* set new channel */
  user.channel = chan;

  /* generate arrival message on new channel */ 
  fprintf(stdout,"*You are now on channel %d\n\n",user.channel); 
  chanmessage(user.channel);
  sprintf(text,"c%d *%s has arrived on the channel\n",user.channel,user.chatname); 
  lntext = strlen(text); 
  text[++lntext] = EOM;
  write(msgsfile,text,lntext);

  chanswap = 1;
  /* who(".w -c\n"); */

  return(5);
} /* changech */


killchan(buf)
char *buf;
{

  char *ptr1, *ptr2;
  int ln;
  int chan;
  int r;
  long int number;
  char ag1[20];
  int ln1;

  owner=0; /* can't own channel 0 */

  /* notify server */
  outpk.id = user.id;
  outpk.action = CHANGECHAN;
  outpk.number = 0;
  transmit();

  /* generate parting message and append to file */
  sprintf(text,"c%d *%s phones the local constabulary\n",user.channel, buf);
  lntext = strlen(text);
  text[++lntext] = EOM;
  write(msgsfile,text,lntext);

  sprintf(text,"c%d *%s has been forcibly removed from the party\n",user.channel, user.chatname);
  lntext = strlen(text);
  text[++lntext] = EOM;
  write(msgsfile,text,lntext);

  /* set new channel */
  user.channel = 0;

  /* generate arrival message on new channel */ 
  fprintf(stdout,"*You have been thrown out the party by %s on to channel 0\n\n",buf); 
  chanmessage(user.channel);
  sprintf(text,"c%d *%s has been thrown down the stairs onto channel 0 by the police\n",user.channel,user.chatname); 
  lntext = strlen(text); 
  text[++lntext] = EOM;
  write(msgsfile,text,lntext);

  chanswap = 1;
  /* who(".w -c\n"); */

  return(5);
} /* changech */

  /* code wonderfully implemented by sinjy one afternoon */
  /* while doing 14 other tasks as well */
  /* (and the code werks unlike yours steve) */
  /* implemented cos conf is now dead pop. and we don't want scum on */
  /* some channels */

  /*reserved channels*/
  reserved(chan, lname)
  int chan;
  char *lname;
  {
    char line[190];
    char uname[10];
    int number =3;
    int digit;
    int found=0;
    int ok=0;
    int channel;
    
    owner = 0;

    rewind(resfile);
    while ( (!found) && ( fgets(line, 185, resfile) != NULL) ){
      sscanf(line, "%d", &channel);
      owner=0;
      /*if yer trying to get onto a resevered channel*/
      if (channel == chan){
        found=1;
        fprintf(stdout, "*This is an invite only channel\n");
        /*check usernames that channel is reserved for*/
        while((line[number]!=' ') && (!ok)){
	  digit=0;
	  while( line[number] != '*') {
	    uname[digit]= line[number];
	    digit++;
	    number++;
          } /*while*/
	  uname[digit]='\000';
	  number++;
	  owner++;
	  /*got the whole name now compare it with users username*/
	  if(!(strcmp(uname, lname))){
            ok=1;
           }
	} /*if*/
      } /*while*/
    } /*while*/
    if (found && !ok){
      fprintf(stdout,"*%s\n",&line[++number]);
      return(1);
    }
    if (owner==1) return(2);
    return(0);

  } /*res chan*/

/* change channel */
flipchan(ag1, num)
char *ag1;
int num;
{
  char ch;
  int chan;
  int rem;
  struct itimerval value, ovalue;
  long int fudge = 0;
  int rem1;

  value.it_interval.tv_sec = 0;
  value.it_interval.tv_usec = 0;
  value.it_value.tv_sec = 6;
  value.it_value.tv_usec = 0;

  rem1=atimer;
  atimer=3;
  setitimer(ITIMER_REAL,&value,&ovalue);
  accept=0;
  fprintf(stdout,"*%s has requested a chat with you on channel %d - Accept (Y/N)",ag1,num);
  fcntl(0, F_SETFL, FNDELAY);
  while (((ch=getchar()) == EOF) && (!accept)) 
    ;
  fcntl(0,F_SETFL,2);
  atimer=rem1;
  setitimer(ITIMER_REAL,&ovalue,&value);
  fprintf(stdout,"\n");
  if ((ch!='y') || (accept==1)) {
    fprintf(stdout,"*Ok be like that then\n");
    return(0);
  }

  chan = num;

  /* notify server */
  outpk.id = user.id;
  outpk.action = FLIPCHAN;
  outpk.number = chan;
  transmit();

  /* generate parting message and append to file */
  sprintf(text,"c%d *%s has left the channel\n",user.channel, user.chatname);
  lntext = strlen(text);
  text[++lntext] = EOM;
  write(msgsfile,text,lntext);

  /* set new channel */
  user.channel = chan;

  /* generate arrival message on new channel */ 
  fprintf(stdout,"*You are now on channel %d\n",user.channel); 
  chanmessage(user.channel);
  sprintf(text,"c%d *%s slips onto the channel clutching an invite from %s\n",user.channel,user.chatname, ag1); 
  lntext = strlen(text); 
  text[++lntext] = EOM;
  write(msgsfile,text,lntext);

  chanswap = 1;
  /* who(".w -c\n"); */

  return(5);
} /* changech */

/* append a message to the message file buffer */
primessage(message)
char *message;
{
  char ag0[300];
  int ln0;

  /* check that the user is not gagged */
  if (user.gagged == 'F') {
       sprintf(text,"c%d+(%s)+%s\n",user.channel,user.chatname,message);
    lntext = format(text);
    write(msgsfile, text, lntext);
    return(0);
  }

  /* gagged user - display different messages */
  sscanf(message,"%s",ag0);
  ln0 = strlen(ag0);
  switch(ln0) {
  case 1: 
    strcpy(message,"Hmmmph Hmm Hmmm....");
    break;
  case 3:
    strcpy(message,"MMr errmm ar Oook?");
    break;
  case 4:
    strcpy(message,"Uf meg miss ukkin dag uf");
    break;
  case 2: 
    strcpy(message,"Grr Hmmm whmmmmmph");
    break;
  case 5:
    strcpy(message,"MMmmm Arrrrr Ug?");
    break;
  case 6:
    strcpy(message,"MMet Hmm Whmmmph!!!");
    break;
  default:
    strcpy(message,"Mmmmmmmmmph");
  } /* switch */
  sprintf(text,"c%d+(%s)+%s\n",user.channel,user.chatname,message);
  lntext = format(text);
  write(msgsfile, text, lntext);
  
} /* primessage */


/* have a look at whos on chat */
who(buf)
char *buf;
{
  char ag1[10];
  int ln1;
  ag1[0] = EOM;

  buf[10] = EOM;
  sscanf(&buf[3],"%s",ag1);
  ln1 = strlen(ag1);
  if (ln1 == 0) {
    fprintf(stdout,"\n*Users Currently Logged On:\n");
    pg(USERLIST,0);
    fprintf(stdout,"\n");
    return(0);
  }
  if ((ln1 > 2) || (ag1[0] != '-')) {
    fprintf(stdout,"*No such option for '.w'\n");
    return(0);
  }
  switch (ag1[1]) {
  case 'c':
    fprintf(stdout,"\n*Users on channel %d:\n",user.channel);
    pg(USERLIST,1);
    fprintf(stdout,"\n");
    break;
  default:
    fprintf(stdout,"*No such option for '.w'\n");
    return(0);
    break;
  }

} /* who */


/* do an atmosphere - ya know emote */
priatmosphere(atmosphere)
char *atmosphere;
{
  char *ptr;
  char ag0[100];
  int ln0 = 0;

  /* check user is not gagged */
  if (user.gagged == 'F') {
    ptr = &atmosphere[1];
    if (((*ptr) == '\'') && ((*(ptr+1)) == 's') && ((*(ptr+2)) == ' ')) {
      ptr = ptr + 2;
      sprintf(text,"c%d+%s\'s+%s",user.channel,user.chatname,++ptr);
    } else {
      sprintf(text,"c%d+%s+%s",user.channel,user.chatname,ptr);
    }
    lntext = format(text);
    write(msgsfile, text, lntext);
    return(0);
  }

  /* gagged - display different messages */
  sscanf(atmosphere,"%s",ag0);
  ln0 = strlen(ag0);
  switch(ln0) {
  case 1: 
    strcpy(atmosphere,"struggles with the gag");
    break;
  case 2:
    strcpy(atmosphere,"falls to his knees and prays for forgiveness");
    break;
  case 4: 
    strcpy(atmosphere,"tries in vain to remove the gag");
    break;
  case 3:
    strcpy(atmosphere,"appears speechless");
    break;
  case 7:
    strcpy(atmosphere,"looks about for help");
    break;
  case 5:
    strcpy(atmosphere,"mutters something");
    break;
  case 6:
    strcpy(atmosphere,"asks for a gottle of gear");
    break;
  case 18:
    strcpy(atmosphere,"does a lot of typing for nothing");
    break;
  default:
    strcpy(atmosphere,"mutters from behind the gag");
  } /* switch */
  sprintf(text,"c%d+%s+%s\n",user.channel,user.chatname,atmosphere);
  lntext = format(text);
  write(msgsfile, text, lntext);
  
} /* priatmosphere */


/* deal with disruptive interupt */
/* ya can check out any time but you can never leave... */
onintr()
{
  fprintf(stdout,"*Oi! \'You can check out any time but you can never leave\' - Use '.q' will you?\n");

} /*onintr*/

/*boss button*/
bossbut()  
{
  system("/usr/ucb/clear");
  system("/bin/cat /etc/motd");
   
} /* bossbut */


/* catch signal from server */
handler()
{
  signal(SIGUSR1,handler);
  got_sig++;
} /* handler */


/* deal with those nasty file errors */
file_error(filename)
char filename[];
{
  fprintf(stderr, "*BLOODY SERIOUS FILE ERROR: %s unable to access\n",filename);
  exit(1);
} /* file_error */


/* sends output packet and receives input packet */
transmit()
{

  int r;     /* general result var */

  /* send packet and notify server */
  write(slavepipe,&outpk,sizeof(outpk));
  kill(spid,SIGUSR1);

  reboot = 0;
  stopalarm();
  atimer = 2;
  alarm(7);
  
  /* wait for server to read info sent */
  while ((!got_sig) && (!reboot) )  {
    pause();
  } /* while */
  if (reboot) boot();
  stopalarm();
     
  got_sig--;

  /* read pipe and notify server that it has been read */
  r = read(serverpipe,&inpk,sizeof(inpk));

  /* notify server that data has been read */
  /* server hang up if this isn't sent */
  kill(spid,SIGUSR2);

} /* transmit */


/* get info on the user */
grabuser()
{
  int i = 0;         /* general purpose counter */
  int found = 0;     /* flag */
 
  /* temp vars for reading in from a file */
  char line[90];
  char loginname[6]; 
  char chatname[11];
  char *ptr;  /* pointer to decompose string */   

  /* REW to start of alias file */
  fseek(aliasfile,0l,0);

  /* scan alias file for rest of info */
  while (((fgets(line,85,aliasfile)) != NULL) && (!found)) {
    sscanf(line,"%s%s",loginname,chatname);

    if (!(strcmp(loginname,user.loginname))) {
      found = 1;
      /* dump the info from file into user structure */
      ptr = &line[17];
      sscanf(ptr,"%50c",user.alias);
      user.alias[50] = EOM;
      strcpy(user.chatname,chatname);
      user.gagged = line[68];
      user.chatio = line[69];
      user.shout = line[70];
      user.super = line[71];
      user.killer = line[72];
      user.netfolk = line[73];
    } /* if */
  } /* while */

  /* if not found then set to default values */
  if (!(found)) {
    strcpy(user.chatname,user.loginname);
    strcpy(user.alias,BLANK);
    user.gagged = 'F';
    user.chatio = 'F';
    user.shout = 'F';
    user.super = 'F';
    user.killer = 'F';
    user.netfolk = 'F';
  } /* if */

} /* grabuser */


/* grab tty details */
checktty()
{
  char ch;

  infd = fileno(stdin),ch;
  if (!isatty(infd)) {
    fprintf(stdout,"*The Prophet appears and says \'Stdin NOT a tty\'\n \'Pipe things to my program would you?\'\n");
    exit(1);
  } /* if */
  
  /* save terminal status */
  ioctl(infd,TIOCGETP,&oldtty);

} /* checktty */



/* initialise tty */
inittty()
{
  newtty = oldtty;
  newtty.sg_flags |= CBREAK;
  newtty.sg_flags &= ~ECHO;
  ioctl(infd, TIOCSETP, &newtty);
} /* inittty */


/* page a file */
/* option idicates a unique feature to be used; 0 = normal */
pg(file,option)
char *file;
int option;
{
  FILE *fp;
  int count = 0;
  int number;
  char ch;
  char line[90];
  int pag = 23;

  char *ctime();
  long time();
  long t;
  char *month, *ptr;

  time(&t);
  month = ctime(&t) + 4;
  *(month+7) = EOM;

  fp = fopen(file,"r");
  if (fp == NULL) return(5);
  fseek(fp,0l,0); 

  if (option == 3) pag = 22;
  
  while (fgets(line,82,fp) != NULL) {
 
    switch (option) {
    case 1:
      sscanf(&line[62],"%d",&number);
      if (number == user.channel) {
	fprintf(stdout,"%s",line);
	count++;
      }
      break;
    case 2:
      ptr = &line[20];
      if (!(strncmp(month,ptr,6))) {
	fprintf(stdout,"%s",line);
	count++;
      }
      break;
    default:
      fprintf(stdout,"%s",line);
      count++;
      break;
    } /* switch */

    if (count == pag) {
      /* wait for a key press - ie. page */
      fprintf(stdout,"[Strike A Key...]");
      if ((ch = getchar()) == 'q'){
         fprintf(stdout, "\n*Abort reading\n");
         fclose(fp);
         return(0);
        } else {
         
      fprintf(stdout,"\n");
      count = 0;
      } /*if*/
    } /* if */
  } /* while */
  fclose(fp);
  return(0);
} /* pg */


/* restore terminal and exit */
redotty()
{
  ioctl(infd,TIOCSETP,&oldtty);
  exit(0);
} /* redotty */


/* received io alarm - something in the buffer */
ioready()
{
  signal(SIGIO,SIG_IGN);
  keypress=1;
} /* ioready */


/* catch the alarm signal */
siren()
{
  signal(SIGALRM,siren);
  /* check which alarm is active and deal with it */
  switch(atimer){
      case 0: 
              wakeup = 1;
              break;
      case 1:
              buzz();
              break;
      case 2:
              reboot = 1;
              break;
      case 3:
              accept=1;
              break;
  } 
} /* siren */


/* set the next timeout alarm to go off */
setbuzzer(timer)
int timer;
{
  struct itimerval *v;
  char line [90];
  char number[10];

  if (nbuzz == 0) {
    rewind(timefile);
  }

   
  /* get the next timeout message from the file */
  if ((fgets(line,82,timefile)) == NULL) {
    /* no more timeout messages */
    sprintf(chato,"has been kicked off for being boring\n");
    logout();
  } else {
    sscanf(line,"%s",number);
    strcpy(timemesg,&line[(strlen(number))+1]);
    sscanf(line,"%d",&timeout);
  } /* if */


  /* set alarm according to which timer method being used */
  if (timer == 1) {
    buzzer.it_value.tv_sec = timeout;
    buzzer.it_interval.tv_sec = 0;
    buzzer.it_interval.tv_usec = 0;
    buzzer.it_value.tv_usec = 0;
    v = &buzzer;
    setitimer(ITIMER_REAL,v,v);
  } 

} /* set buzzer */


stopalarm()
{
  struct itimerval value, *v;

  value.it_value.tv_sec = 0;
  value.it_interval.tv_sec = 0;
  value.it_interval.tv_usec = 0;
  value.it_value.tv_usec = 0;
  v = &value;
  setitimer(ITIMER_REAL,v,v);

} /* stop alarm */



/* catch a buzzer timeout alarm */
/* display message and set next timeout */
buzz()
{

  /* if user no loger on unix system */
  if (!(scanutmp())) {
    strcpy(chato,"has crashed out of chat\n");
    logout();
  }

  /* display timeout message and set timeout */
  nbuzz++;
  fprintf(stdout,"%s",timemesg);
  setbuzzer(stimer);
}


/* suspend activity for a period of time */
suspend(sec)
long sec;
{
  struct itimerval value, ovalue, *v, *ov;
  
  value.it_interval.tv_sec = 0;
  value.it_interval.tv_usec = 0;
  value.it_value.tv_sec = sec;
  value.it_value.tv_usec = 0;
  v = &value;

  setitimer(ITIMER_REAL,v,v);
  atimer = 0;  /* indicate which timer now operating */
  wakeup = 0;
  /* waits for io ready or timed alarm of 1 second */
  while ((!wakeup) && (!keypress)) {
    pause();
  }
  stopalarm();  /* DON'T FORGET!!! */
  atimer = 1;

} /* suspend */


/* tell another user on chat something */
tell(buf)
char *buf;
{
  char ag0[300], ag1[15], ag2[300];
  char *ptr;
  int i;
  int ln, ln0, ln1, ln2;

  int lnbuf = strlen(buf);

  ag0[0] = EOM;
  ag1[0] = EOM;
  ag2[0] = EOM;

  /* point to first argument in list - ie. the name */
  /* ie. get the name of the user to tell */
  ptr = &buf[3];

  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);

  /* too long */
  if (ln2 > 12) {
    fprintf(stdout,"*No such user has ever existed\n");
    return(0);
  }

  /* no text atall */
  if (ln2 == 0) {
    fprintf(stdout,"*Tell who what?\n");
    return(0);
  }

  /* store first argument */
  strcpy(ag1,ag2);
  ln1 = ln2;
  ag2[0]=EOM;

  if(expand(ag1)==1) return(0);

  if (!(cstrcmp(user.chatname,ag1))) {
    fprintf(stdout,"*Tell yourself? Are you mentally unstable?\n");
    return(0);
  }

  /* deal with special group netfolk */
  if (!(cstrcmp("netfolk",ag1))) {
    netfolk(buf,0);
    return(0);
  }

  /* notify server */
  outpk.action = FIND;
  outpk.id = user.id;
  strcpy(outpk.text,ag1);
  transmit();
  
  /* can't find name */
  if (inpk.status == 1) {
    fprintf(stdout,"*The dwarves of falseness throw mud at your back\n");
    return(0);
  }

  /* check if user is gagged */
  if (user.gagged == 'T') {
    fprintf(stdout,"*You fail to attract their attention\n");
    return (0);
  }

  /* capitalise the first letter - just for display */
  if (islower(ag1[0])) {
     ag1[0] = toupper(ag1[0]);
   }
  
  /* point to second argument ie. message part */
  i = 0;
  while( (isspace(*ptr)) && (i <= (lnbuf-3)) ) {
    ptr++;
    i++;
  } /* while */

  /*  adjust pointer to skip over first argument */
  ptr = ptr + ln1 + 1;

  ag2[0] = EOM;
  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);
  
  /* no message component */
  if (ln2 == 0) {
    fprintf(stdout,"*Tell %s what?\n",ag1);
    return(0);
  }

  /* build up sentence for message file */
  sprintf(text,"t%s+*%s tells you:+%s\n",ag1,user.chatname,ptr);
  lntext = format(text);
  write(msgsfile,text,lntext);

  fprintf(stdout,"*%s has been told\n",ag1);

} /* tell*/


/* deal with the special case of tell */
netfolk(buf,sinj)
char *buf;
int sinj;
{
  int ln2, i;
  char ag2[300];
  char *ptr;
  int lnbuf = strlen(buf);
  ptr = &buf[3];

  /* notify server */
  outpk.action = NETFOLK;
  outpk.id = user.id;
  transmit();

  /* member of netfolk is not on */
  if (inpk.status == 0) {
    if (user.netfolk == 'F') {
      fprintf(stdout,"*There are no Netfolk on\n");
    } else {
      fprintf(stdout,"*You are the only Netfolker on\n");
    } /* if */
    return(0);
  } /* if */

  if (user.gagged == 'T') {
    fprintf(stdout,"*You fail to attract their attention\n");
    return (0);
  }
  
  /* point to second argument ie. message component */
  i = 0;
  while( (isspace(*ptr)) && (i <= (lnbuf-3)) ) {
    ptr++;
    i++;
  } /* while */

  /*  adjust pointer to skip over first argument */
  ptr = ptr + 7 + 1;

  ag2[0] = EOM;
  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);
  
  if (ln2 == 0) {
    if (sinj == 0) {
      fprintf(stdout,"*Tell the Netfolk what?\n");
      return(0);
    } else {
      fprintf(stdout,"*Emote what to the Netfolk, eh?\n");
      return(0);
    }
  }

  /* build up sentence for message file */
  if (sinj == 0) {
    sprintf(text,"n%s+*Netfolk message from %s:+%s\n",user.chatname,user.chatname,ptr);
  } else {
    sprintf(text,"n%s+*Netfolk emote: %s+%s\n",user.chatname,user.chatname,ptr);
  }
  lntext=format(text);
  write(msgsfile,text,lntext);

  fprintf(stdout,"*The netfolk have been told\n");

} /* netfolk */

/* change second part of user name */
changealias(buf)
char *buf;
{

  char *ptr, *ptr1;
  int lp;
  int lnbuf = strlen(buf);

  if (user.gagged == 'T'){
    fprintf(stdout, "*You can't tell the alias changer man your new alias because your gagged.\n");
    return(0);
   }

  /* skip blanks at the start */
  lp = 0;
  ptr = &buf[3];
  while (((*(ptr++)) == ' ') && (lp != (lnbuf-3))) {
    lp++;
  }
  ptr--;
  lnbuf=strlen(ptr);
  ptr1 = ptr + lnbuf -1;

  /* pad less than 50 out with spaces */
  for (lp = lnbuf-1; lp < 50; lp++)
    (*(ptr1++)) = ' ';
  (*(ptr+50)) = EOM;    
    
  /* notify server */
  outpk.action = CHANGEALIAS;
  outpk.id = user.id;
  strcpy(outpk.text,ptr);
  transmit();
 
  /* display message to users on channel */  
  sprintf(text,"c%d *%s is now: %s %s\n",user.channel,user.chatname,user.chatname,ptr);
  lntext = strlen(text);
  text[++lntext] = EOM;
  /* lntext=format(text); */
  write(msgsfile,text,lntext);


} /* change alias */


/* gag/ungag a user */
gag(buf)
char *buf;
{
  char ag2[300], ag1[15];
  char *ptr;
  int i;
  int ln, ln0, ln1, ln2;

  int lnbuf = strlen(buf);

  ag1[0] = EOM;
  ag2[0] = EOM;

  /* point to first argument in list - ie. the name */
  ptr = &buf[3];

  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);

  /* too long */
  if (ln2 > 12) {
    fprintf(stdout,"*No such user has ever existed\n");
    return(0);
  }

  /* no text atall */
  if (ln2 == 0) {
    fprintf(stdout,"*Gag who? Come on who's the victim\n");
    return(0);
  }

  /* store first argument */
  strcpy(ag1,ag2);
  ln1 = ln2;


  if (!(cstrcmp("netfolk",ag1))) {
    fprintf(stdout,"*Gag the Netfolk eh? They will never be silenced\n");
    return(0);
  }


  if(expand(ag1)==1) return(0);


  /* attempt to gag self */
  if ((!(cstrcmp(user.chatname,ag1))) && (user.gagged == 'F')) {
    fprintf(stdout,"*You are unable to get your hands out of the straight jacket?\n");
    return(0);
  }

  /* notify server */
  outpk.action = GAG;
  outpk.id = user.id;
  strcpy(outpk.text,ag1);
  transmit();

  /* can't find name */
  if (inpk.status == 2*MAXCHAN) {
    fprintf(stdout,"*No one by that name is currently on\n");
    return(0);
  }

  /* capitalise first letter for display */
  if (islower(ag1[0])) {
     ag1[0] = toupper(ag1[0]);
  }

  /* ungag user */
  if ((inpk.status > MAXCHAN-1) && (inpk.status < 2*MAXCHAN)) {
    /* build up sentence for message file */
    i = inpk.status -MAXCHAN;
    sprintf(text,"u%s %d %s\n",ag1,i,user.chatname);
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile,text,lntext);
    /* if not on same channel as user display message */
    if ((inpk.status-MAXCHAN) != user.channel) {
      fprintf(stdout,"*%s has been ungagged\n",ag1);
    } /* if */
    return(0);
  } /* if */

  
  /* gag user */
  /* build up sentence for message file */
  sprintf(text,"g%s %d %s\n",ag1,inpk.status,user.chatname);
  lntext = strlen(text);
  text[++lntext] = EOM;
  write(msgsfile,text,lntext);
  if (inpk.status != user.channel) {
    fprintf(stdout,"*%s has been gagged\n",ag1);
  }
} /* gag */


/* deal with a crash */
hangup()
{
  strcpy(chato,"has hung up\n");
  logout();
} /* hangup */


/* change the default delete character */
chdel()
{
  char ch;

  printf("*Hit key to be used as delete\n");
  ch = getchar();
  if (!(iscntrl(ch))) {
    printf("*Sorry, I refuse to assign delete as that\n");
  } else {
    if (ch == RETURN) {
      printf("*What, you want RETURN as delete? Who are you Jon?\n");
    } else {
      delchar = ch;
      printf("*Delete reassigned\n");
    } /* if */
  } /* if */
} /* chdel */

/* whois horrible script file*/
whois(buf)
char buf[];
{
  char ag0[40],ag1[20];
  char *p;

  int lnbuf = strlen(buf);
  ag0[0]=EOM;
  ag1[0]=EOM;

  buf[20]=EOM;
  p = &buf[3];

  sscanf(p, "%s", ag1); 
  sprintf(ag0, "%s %s\n",WHOIS, ag1);
  system(ag0);

} /*thumb*/ 

/* shell out and find out whos on the system */
userlist(buf)
char buf[];
{

  char ag0[40], ag1[20];
  char *p;  
  
  int lnbuf = strlen(buf);
  ag1[0] = EOM;
  ag0[0] = EOM;

  /* only take the first 10 chars */
  buf[10] = EOM;

  /* point to the argument part */
  p = &buf[3];

  /* store the first argument in ag1 */
  sscanf(p, "%s",ag1);

  /* store whole command in ag0 */
  sprintf(ag0,"%s %s\n",UNIXWHO, ag1);

  /* execute system command */
  system(ag0);


} /* userlist */


/* display current options */
help()
{
  int ch;

  printf("\nCommands Are:\n\n");

  /* commands */
  printf("  .u [-gu]         Sequent User List\n");
  printf("  .a text          Change Alias On Who List\n");
  printf("  .c nn            Change Channel (0-%d)\n",MAXCHAN-1);
  printf("  .l               Lock Current Channel\n");
  printf("  .o               Open Current Channel\n");
  printf("  .w [-c]          See Who's On [your channel]\n");
  printf("  .q               Quit\n");
  printf("  .r user [tty]    Request Chat With Sequent User\n");
  printf("  .* user          Invite user on to your channel\n");
  printf("  .i               Read Info File\n"); 
  printf("  .f               reFresh Channel 0 screen\n");
  printf("  .y [user]        Read [users] Yellow Pages\n");
  printf("  .d               Reassign Delete Key\n");
  printf("  .t user text     Tell User In Private\n"); 
  printf("  .j user text     J-mote To A User In Private\n");
  printf("\n\n[Press a Button...]");
  ch=getchar();
  printf("\n\nMore commands:\n\n");
  printf("  .e               Display the time\n");
  printf("  .v [-d]          View user log [today]\n");
  printf("  .n               Noise toggle on reading messages\n");
  printf("  :text            Do An Atmosphere\n");
  printf("  .p user [text]   Buys a pint\n");
  printf("  .$ text          Tells you who someone is\n");
  printf("  .!               Redisplays channel description\n");
  printf("  # [text]         Sing\n");
  printf("  \" [text]         A mud type say\n");
  printf("  .#               Shows the number of users\n");
  printf("  .& nn            Channel Index (0-%d)\n", MAXCHAN-1);
  printf("  .+               Read the Bulletin Board\n");
  printf("  .< user          Evict User From Channel\n");    
  printf("  <cntrl-C>        The BOSS key\n");

  if ((user.killer == 'T') || (user.shout == 'T') || (user.super == 'T')) {
    printf("\n\n[Press a Button...]");
    ch = getchar();
    printf("\n\nExtra Commands:\n\n");
  }
  if (user.killer == 'T') {
    printf("  .g user          Gag/Ungag User\n");              
    printf("  .k user          Kill A User\n");
  } /* if */
  if (user.shout == 'T') {
    printf("  .s text          Shout A Message\n");
  }
  if (user.super == 'T') {
    printf("  .z [user]        Zap User/Shutdown Conf\n");
    printf("  .m               Make/Update User\n");
    printf("  .b [user]        Ban User/Check Ban List\n");              
    printf("  .^ text          The silly command\n");
  }


  /* last bit */ 
  printf("\n");
} /* help */

chanlist(buf)
char *buf[];
{
 char number[30];
 int numb;
 char text[100];
 
 if ( strlen(buf) != 3){
 sscanf(buf, "%s %d\n", number, &numb);
  if ( numb >= 0 && numb <= 99 ){
   sprintf(text, "/bin/grep -w \"^%d\" %s", numb, CHANLIST);
   fprintf(stdout, "*The title of channel ");
   fflush(stdout);
   system(text);
   } else {
    printf("*No such channel\n");
   }
 } else {
 pg(CHANLIST, 0);
 }
}

/* check user still on unix system */
scanutmp()
{
 struct utmp buffer;
 FILE *utmpfile;
 int stillon=0;

 if ((utmpfile=fopen("/etc/utmp","r"))==NULL) printf("Yargh! It's Fooked\n");
 while (fread((char*)&buffer,sizeof(struct utmp),1,utmpfile))
 if (*buffer.ut_name)
 {
  /* Note: Assuming 8 is unwise, but how else to do it?! */
  if (! (strncmp(buffer.ut_name,user.loginname,8) || 
   strcmp(buffer.ut_line,terminalname+5)) ) stillon=1;
   /* the +5 skips the /dev/ part ---^^ */
 }
 fclose(utmpfile);
 return stillon;
} /* scanutmp */


/*Hit Man*/
hitman(buf)
char *buf;
{
  char ag2[30], ag1[80];
  char *ptr;
  int ln1, ln2;
  int lnbuf = strlen(buf);

ag1[0]=EOM;
ag2[0]=EOM;

/*point the first argument - The name */

ptr= &buf[3];

  sscanf(ptr,"%s",ag2); 
  ln2 = strlen(ag2);      

if(ln2 == 0){
 fprintf(stdout, "*Who do you wanna rub out ?\n");
 fflush(stdout);
 return(0);
}

if(ln2>12){
 fprintf(stdout, "*No such user has ever existed\n");
 fflush(stdout);
 return(0);
}

if(!(strcmp(ag2, "netfolk"))){
  fprintf(stdout, "*Tempting thought, but no way\n");
  fflush(stdout);
  return(0);
}

  if(expand(ag2)==1) return(0);

if (!(strcmp(ag2,user.chatname))){
  fprintf(stdout, "*You don\'t wanna hit yourself\n");
  fflush(stdout);
  return(0);
 }


  fprintf(stdout, "*Hit out on %s\n", ag2);
  fflush(stdout);
  sprintf(ag1, ".k %s", ag2);
  kick(ag1);
return(0);

}
 
/* kill a user */
kick(buf)
char *buf;
{
  char ag2[300], ag1[15];
  char *ptr;
  int i;
  int ln, ln0, ln1, ln2;

  int lnbuf = strlen(buf);

  ag1[0] = EOM;
  ag2[0] = EOM;

  /* point to first argument in list - ie. the name */
  ptr = &buf[3];

  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);

  /* too long */
  if (ln2 > 12) {
    fprintf(stdout,"*No such user has ever existed\n");
    return(0);
  }

  /* no text atall */
  if (ln2 == 0) {
    fprintf(stdout,"*Kill who? Come on who we gonna nail?\n");
    return(0);
  }

  /* store first argument */
  strcpy(ag1,ag2);
  ln1 = ln2;

  if (!(cstrcmp("netfolk",ag1))) {
    fprintf(stdout,"*Kill the netfolk? I think not");
    return(0);
  }
  if(expand(ag1)==1) return(0);


  /* attempt to kill self */
  if (!(cstrcmp(user.chatname,ag1))) {
    fprintf(stdout,"*Suicide ain't in the rules of the game\n");
    return(0);
  }


  /* notify server */
  outpk.action = KILL;
  outpk.id = user.id;
  strcpy(outpk.text,ag1);
  transmit();

  /* can't find name */
  if (inpk.status == MAXCHAN+1) {
    fprintf(stdout,"*No one by that name is currently on\n");
    return(0);
  }

  /* capitalise first letter for display */
  if (islower(ag1[0])) {
     ag1[0] = toupper(ag1[0]);
  }
  
  /* kill user */
  /* build up sentence for message file */
  sprintf(text,"k%s %s\n",ag1,user.chatname);
  lntext = strlen(text);
  text[++lntext] = EOM;
  write(msgsfile,text,lntext);
  if (inpk.status != user.channel) {
    fprintf(stdout,"*%s hits %s doing 2D8 damage... Oh dear %s is dead\n",user.chatname, ag1, ag1);
  }
} /* kick */


slipin(buf)
char *buf;
{
  char ag2[300], ag1[15];
  char *ptr;
  int i;
  int ln, ln0, ln1, ln2;

  int lnbuf = strlen(buf);

  ag1[0] = EOM;
  ag2[0] = EOM;

  /* point to first argument in list - ie. the name */
  ptr = &buf[3];

  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);

  /* too long */
  if (ln2 > 12) {
    fprintf(stdout,"*No such user has ever existed\n");
    return(0);
  }

  /* no text atall */
  if (ln2 == 0) {
    fprintf(stdout,"*Request who on your channel? Come on who we gonna chat up?\n");
    return(0);
  }

  /* store first argument */
  strcpy(ag1,ag2);
  ln1 = ln2;

  if (!(cstrcmp("netfolk",ag1))) {
    fprintf(stdout,"*Fook off I'm not coding that\n");
    return(0);
  }

  if(expand(ag1)==1) return(0);

  /* attempt to **** self */
  if (!(cstrcmp(user.chatname,ag1))) {
    fprintf(stdout,"*What yourself? Please logout and don't use conf again\n");
    return(0);
  }


  /* notify server */
  outpk.action = KILL;
  outpk.id = user.id;
  strcpy(outpk.text,ag1);
  transmit();

  /* can't find name */
  if (inpk.status == MAXCHAN+1) {
    fprintf(stdout,"*No one by that name is currently on\n");
    return(0);
  }

  /* capitalise first letter for display */
  if (islower(ag1[0])) {
     ag1[0] = toupper(ag1[0]);
  }
  
  if(inpk.status==user.channel) { 
    fprintf(stdout,"*%s is already on your channel do a .w -c fool\n",ag1);
  } else {
    sprintf(text,"h%s %d %s\n",ag1,user.channel,user.chatname);
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile,text,lntext);
    fprintf(stdout,"*Yeah, yeah I've requested %s onto the channel\n",ag1);

  }
} /* slipin*/


minikill(buf)
char *buf;
{
  char ag2[300], ag1[15];
  char *ptr;
  int i;
  int ln, ln0, ln1, ln2;

  int lnbuf = strlen(buf);

  ag1[0] = EOM;
  ag2[0] = EOM;

  /* point to first argument in list - ie. the name */
  ptr = &buf[3];

  /* not owner */
  if(owner != 1 ) {
    fprintf(stdout,"*You can't evict party goers from a channel you don't own\n");
    return(1);
  }

  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);

  /* too long */
  if (ln2 > 12) {
    fprintf(stdout,"*No such user has ever existed\n");
    return(0);
  }

  /* no text atall */
  if (ln2 == 0) {
    fprintf(stdout,"*Just chill out dude\n");
    return(0);
  }

  /* store first argument */
  strcpy(ag1,ag2);
  ln1 = ln2;

  if (!(cstrcmp("netfolk",ag1))) {
    fprintf(stdout,"*The party will go down hill if you throw the Netfolk out\n");
    return(0);
  }

  if(expand(ag1)==1) return(0);

  /* attempt to **** self */
  if (!(cstrcmp(user.chatname,ag1))) {
    fprintf(stdout,"*What yourself? Maybe you should cut down on the wacky backy\n");
    return(0);
  }


  /* notify server */
  outpk.action = KILL;
  outpk.id = user.id;
  strcpy(outpk.text,ag1);
  transmit();

  /* can't find name */
  if (inpk.status == MAXCHAN+1) {
    fprintf(stdout,"*No one by that name is currently partying on conf\n");
    return(0);
  }

  /* capitalise first letter for display */
  if (islower(ag1[0])) {
     ag1[0] = toupper(ag1[0]);
  }
  
  if(inpk.status != user.channel) { 
    fprintf(stdout,"*%s isn't partying on your channel\n",ag1);
  } else {
    fprintf(stdout,"*You phone the police to complain about %s, they'll be here shortly\n",ag1);
    sprintf(text,"m%s %s\n",ag1,user.chatname);
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile,text,lntext);
  }
} /* minikill */


/* request a chat with another sequent user */
request(buf)
char buf[];
{

  char ag0[70], ag1[25], ag2[25];    /* arguments */
  char *p;                           /* general pointer */
  int l;                             /* length variable */
  char r;

  /* reset arguments */
  ag1[0] = EOM;             /* username or alias */
  ag2[0] = EOM;             /* tty */

  /* truncate search buffer to 18 characters */
  buf[25] = EOM;

  /* pick up the 2 arguments */
  p = &buf[3];
  sscanf(p,"%s %s",ag1,ag2);

  /* check for corrrect format */
  if (strlen(ag1) == 0) {
    printf("*Supply username and/or tty, what could be more simple?\n");
  } else {
    if(expand(ag1)==1) return(0);

    /* shell out and pass the arguments to a shell script */
    /* I'll put this bit in C when I find out how to do it */ 
    sprintf(ag0,"%s %s %s %s %s",PAGE, user.chatname, user.loginname, ag1, ag2);
/*    printf("X:%s:X\n",ag0); */
/*    seteuid(uid);  */

    system(ag0);

/*    seteuid(euid); */

  } /* if */

} /* request */



shout(buf)
char buf[];
{
  char ag1[20], ag2[300], ag0[300];    /* arguments */
  char *p, *p1;                        /* pointers */
  int lp, lp1, lp2;                    /* loop vars */
  int l, l1;                           /* length vars */

  /* reset arguments */
  ag2[0] = EOM;
  ag1[0] = EOM;
  ag0[0] = EOM;

  if (user.gagged == 'T') {
    fprintf(stdout,"*You whimper into your gag\n");
    return(0);
  }    

  /* point to start of arguments */
  p = &buf[3];

  /* check length of shout */
  sscanf(p,"%s",ag2);
  l = strlen(ag2);
  if (l == 0) {
    printf ("*Er, shout what exactly?\n");
  } else {
    strcpy(ag2,p);
    l1 = strlen(ag2);
    ag2[++l1] = RETURN;

    sprintf(text,"s+*%s shouts:+%s\n",user.chatname,ag2);
    lntext = format(text);
    write(msgsfile,text,lntext);
	  
  } /* if */
 
} /* shout */



/* response to an emergency */
bailout()
{
  signal(SIGUSR2,bailout);
  fprintf(stdout,"*The Prophet appears and yanks the plug from the wall and walks off muttering\n");
  fflush(stdout);
  redotty();
  exit(1);
}

/* edit a string - give sring and max length*/
edit(string,maxlength)
char *string;
int maxlength;
{
  char ch = 'z';
  int lnstring = strlen(string);
  maxlength--;
  fprintf(stdout,"%s",string);
  while (ch != RETURN) {
    ch = getchar();
    if (ch == ESC) {
      return(0);
    }
    if (((!(iscntrl(ch))) && (lnstring <= maxlength)) ) {
      string[lnstring++] = ch;
      putchar(ch);
    } else {
      if ((ch == delchar) && (lnstring != 0)) {
	putchar(ERASE);
	putchar(' ');
	putchar(ERASE);
	lnstring--;
      } /* if */

    } /*if */
  } /*while */
  string[lnstring] = EOM;
  fprintf(stdout,"\n");
  return(lnstring);
} /* edit */

/* update users */
update()
{
  char ch;
  int lnsearch;
  struct users new;
  char search[15];
  int pos, found;
  char line[86];
  char *ptr;

  search[0] = EOM;

  fprintf(stdout,"*(C)hatname / (L)oginname:");
  ch = getchar();
  if ((ch == 'C') || ch == 'c') {
    lnsearch = 10;
    fprintf(stdout,"\n*Enter Chatname: ");
  } else {
    if ((ch == 'L') || (ch == 'l')) {
      lnsearch = 5;
      fprintf(stdout,"\n*Enter Loginname: ");
    } else {
      return(2);
    } /* if */
  } /* if */
  
  if ((edit(search,lnsearch)) == 0) {
    return(2);
  }

  fseek(aliasfile,0l,0);
  pos = 0;
  found = 0;
  while (((fgets(line,85,aliasfile)) != NULL) && (!found)) {
    sscanf(line,"%s%s",new.loginname,new.chatname);
    if (lnsearch == 5) {
      ptr = &new.loginname[0];
    } else {
      ptr = &new.chatname[0];
    } /* if */
    if (!(cstrcmp(ptr,search))) {
      found = 1;
      new.gagged = line[68];
      new.chatio = line[69];
      new.shout = line[70];
      new.super = line[71];
      new.killer = line[72];
      new.netfolk = line[73];
      fprintf(stdout,"*User exits update details:\n");
    } else {
      pos = ftell(aliasfile);
    } /* if */
  } /* while */
  if (!found) {
    new.gagged = 'F';
    new.chatio = 'T';
    new.shout = 'F';
    new.super = 'F';
    new.killer = 'F';
    new.netfolk = 'F';
    fprintf(stdout,"*Creating new user:\n");
    fseek(aliasfile,0l,2);
    pos = ftell(aliasfile);
    new.chatname[0] = EOM;
    new.loginname[0] = EOM;
    if (lnsearch == 5) {
      strcpy(new.loginname,search);
    } else {
      strcpy(new.chatname,search);
    } /* if */
  } /* if */
  
  fprintf(stdout,"*Loginname? ");
  if ((edit(new.loginname,5)) == 0) {
    return(2);
  }
  fprintf(stdout,"*Chatname? ");
  if ((edit(new.chatname,10)) == 0) {
    return(2);
  }
  fprintf(stdout,"*Gagged? (if currently on will not be altered): %c",new.gagged);
  if ((new.gagged = (answer(new.gagged))) == 0) return(2);

  fprintf(stdout,"*Chat I/O rights? %c",new.chatio);
  if ((new.chatio = (answer(new.chatio))) == 0) return(2);

  fprintf(stdout,"*Able to Shout? %c",new.shout);
  if ((new.shout = (answer(new.shout))) == 0) return(2);

  fprintf(stdout,"*Supervisor? %c",new.super);
  if ((new.super = (answer(new.super))) == 0) return(2);

  fprintf(stdout,"*Able to Kill and Gag users? %c",new.killer);
  if ((new.killer = (answer(new.killer))) == 0) return(2);

  fprintf(stdout,"*Member of the exclusive Nefolkers? %c",new.netfolk);
  if ((new.netfolk = (answer(new.netfolk))) == 0) return(2);

  fprintf(stdout,"*Save changes (Y/N)");
  ch = getchar();
  if ((ch != 'y') && (ch != 'Y')) {
    return(2);
  }
  fseek(aliasfile,pos,0);
  fprintf(aliasfile,"%-5s %-10s",new.loginname,new.chatname);
  if (found) {
    pos = pos + 68l;
    fseek(aliasfile,pos,0);
  } else {
    fprintf(aliasfile,"%s  ",BLANK);
  } /* if */
  fprintf(aliasfile,"%c%c%c%c%c%c",new.gagged,new.chatio,new.shout,new.super,new.killer,new.netfolk);
  if (!found) fprintf(aliasfile,"\n");
  rewind(aliasfile);
  fflush(aliasfile);
  fprintf(stdout,"\n*Changes saved.\n\n");
  return(0);
} /* update */


answer(ch)
char ch;
{
  char ch1;
  int r = 0;
  while (r == 0) {

    ch1 = getchar();
    if (ch1 == RETURN) {
      fprintf(stdout,"\n");
      return(ch);
    }
    if ((ch1 == 't') || (ch1 == 'T')) {
      ch = 'T';
      r = 1;
    }
    if ((ch1 == 'F') || (ch1 == 'f')) {
      ch = 'F';
      r = 1;
    }
    if (ch1 == ESC) {
      return(0);
    }
  } /* while */
  putchar(ERASE);
  putchar(ch);
  fprintf(stdout,"\n");
  return(ch);

} /* answer */


/* ban a user */
ban(buf)
char *buf;
{
  char ag2[300], ag1[15];
  char *ptr;
  int i;
  int ln, ln0, ln1, ln2;
  FILE *banfile;

  int lnbuf = strlen(buf);

  ag1[0] = EOM;
  ag2[0] = EOM;

  /* point to first argument in list - ie. the name */
  ptr = &buf[3];

  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);

  /* too long */
  if (ln2 > 12) {
    fprintf(stdout,"*No such user has ever existed\n");
    return(0);
  }

  /* no text atall */
  if (ln2 == 0) {
    banfile = fopen(BANFILE, "r");
    if (banfile == NULL) {
      file_error(BANFILE);
    } /* if */
    i = 0;
    while((fgets(ag1,12,banfile) != NULL)) {
      if (i ==0) fprintf(stdout,"*Users currently banned:\n");
      fprintf(stdout,"  %s",ag1);
      i++;
    }
    if (i == 0) fprintf(stdout,"*No one is banned\n");
    fprintf(stdout,"\n");
    return(0);
  }

  /* store first argument */
  strcpy(ag1,ag2);
  ln1 = ln2;

  if (!(cstrcmp("netfolk",ag1))) {
    fprintf(stdout,"*Come now thats rather unfriendly\n");
    return(0);
  }
  if(expand(ag1)==1) return(0);

  /* attempt to ban self */
  if (!(cstrcmp(user.chatname,ag1))) {
    fprintf(stdout,"*Ban yourself? Take a seat on the couch...\n");
    return(0);
  }


  /* notify server */
  outpk.action = BAN;
  outpk.id = user.id;
  strcpy(outpk.text,ag1);
  transmit();

  /* capitalise first letter for display */
  if (islower(ag1[0])) {
     ag1[0] = toupper(ag1[0]);
  }

  /* can't find name */
  if (inpk.status == 1) {
    fprintf(stdout,"*%s has been unbanned\n",ag1);
    return(0);
  }
  fprintf(stdout,"*%s has been banned\n",ag1);

} /* ban */


/* set bell to sound on arrival of messages */
nbell()
{
  if (user.bell == 'F') {
    fprintf(stdout,"*\007Noise toggled on\n");
    user.bell = 'T';
  } else {
    fprintf(stdout,"*Noise toggled off\n");
    user.bell = 'F';
  }
} /* nbell */

/* have a look at the user log */
view(buf)
char *buf;
{
  char ag1[10];
  int ln1;
  ag1[0] = EOM;

  buf[10] = EOM;
  sscanf(&buf[3],"%s",ag1);
  ln1 = strlen(ag1);
  if (ln1 == 0) {
    fprintf(stdout,"\n*Users Log:\n");
    pg(USERLOG,3);
    fprintf(stdout,"\n");
    return(0);
  }
  if ((ln1 > 2) || (ag1[0] != '-')) {
    fprintf(stdout,"*No such option for '.v'\n");
    return(0);
  }
  switch (ag1[1]) {
  case 'd':
    fprintf(stdout,"\n*Users who have logged on today:\n");
    pg(USERLOG,2);
    fprintf(stdout,"\n");
    break;
  default:
    fprintf(stdout,"*No such option for '.v'\n");
    return(0);
    break;
  }
} /* view */


pint(buf)
char *buf;
{
  char ag2[300], ag1[15];
  char *ptr;
  int i;
  int ln, ln0, ln1, ln2;

  int lnbuf = strlen(buf);

  ag1[0] = EOM;
  ag2[0] = EOM;

  if ( user.gagged == 'T'){
  fprintf(stdout, "*The bar man can't hear your order because your gagged\n");
  return(0);
  }

  /* point to first argument in list - ie. the name */
  ptr = &buf[3];

  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);

  /* too long */
  if (ln2 > 12) {
    fprintf(stdout,"*No such user has ever existed\n");
    return(0);
  }

  /* no text at all */
  if (ln2 == 0) {
    
    sprintf(text,"c%d *%s buys a pint\n",user.channel,user.chatname);
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile,text,lntext);
    return(0);
  }

  /* store first argument */
  strcpy(ag1,ag2);
  ln1 = ln2;
  ag2[0]=EOM;

  if (!(cstrcmp("netfolk",ag1))) {

    /* notify server */
    outpk.action = NETFOLK;
    outpk.id = user.id;
    strcpy(outpk.text,ag1);
    transmit();


    /* member of netfolk is not on */
    if (inpk.status == 0) {
      if (user.netfolk == 'F') {
	fprintf(stdout,"*There are no netfolk around to buy a pint\n");
      } else {
	fprintf(stdout,"*You are the only netfolker on\n");
      } /* if */
      return(0);
    } /* if */
    sprintf(text,"s *%s buys all the Netfolk a pint each\n",user.chatname);
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile,text,lntext);
    return(0);
  }

  i=ln1+4;
  ptr= &buf[i];
  strcpy(ag2, ptr);
  ln2=strlen(ag2);

  if(expand(ag1)==1) return(0);

  /* attempt to buy self a drink */
  if (!(cstrcmp(user.chatname,ag1))) {
    if (ln2==0){
    sprintf(text,"c%d *%s buys a pint\n",user.channel,user.chatname);
    } else {
    sprintf(text,"c%d *%s buys a pint of %s", user.channel, user.chatname, ag2);
    }
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile,text,lntext);
    return(0);
  }


  /* notify server */
  outpk.action = KILL;  /* but not really */
  outpk.id = user.id;
  strcpy(outpk.text,ag1);
  transmit();

  /* can't find name */
  if (inpk.status == MAXCHAN+1) {
    fprintf(stdout,"*No one by that name is currently on\n");
    return(0);
  }

  /* capitalise first letter for display */
  if (islower(ag1[0])) {
     ag1[0] = toupper(ag1[0]);
  }
  
  
  /* build up sentence for message file */
  if (ln2==0){
  sprintf(text,"c%d *%s buys %s a pint\n",inpk.status,user.chatname,ag1);
  } else {
  sprintf(text, "c%d *%s buys %s a pint of %s", inpk.status, user.chatname, ag1, ag2);
  }
  lntext = strlen(text);
  text[++lntext] = EOM;
  write(msgsfile,text,lntext);
  if (inpk.status != user.channel) {
    fprintf(stdout,"*You have bought %s a pint\n",ag1);
  }
} /* pint */

jelly(buf)
char *buf;
{
   char text[100];
   char text2[100];
   int lntext;


  if ( user.super == 'T') {
    text2[0]=EOM;
    sscanf(&buf[3], "%s", text2);
    sprintf(text, "s *%s shouts: Who wants to eat Jelly off my Belly ?\n", text2);
    lntext = strlen(text);
    text[++lntext] = EOM;
    write(msgsfile,text,lntext);
    return(0);
   } else {

   fprintf(stdout, "*Your not silly enough to use the jelly comand\n");

  }
}
yp(buf)
char *buf;
{
  char line[90];
  FILE *fp, *aliasfile;
  int found;
  char loginname[10], chatname[15];
  char scan[10], target[20];
  int n;
  int own = 0;
  char path[80];
  struct passwd *getpwuid();


  /* attempt to find users loginname from the alias file */  
  buf[20] = EOM;
  target[0] = EOM;

  sscanf(&buf[3],"%s",target);
  if (strlen(target) == 0) {
    strcpy(target,user.chatname);
    own = 1;
  }
  if(expand(target)==1) return(0);  
  aliasfile = fopen(ALIASFILE,"r");
  rewind(aliasfile);
  found = 0;
  while((!found) && ((fgets(line,80,aliasfile)) != NULL)) {
    sscanf(line,"%s %s",loginname,chatname);
    if (!(cstrcmp(chatname,target))) found = 1;
  }
  fclose(aliasfile);
  if (!found) {
    fprintf(stdout,"*No such user registered on chat guv\n");
    return(1);
  }

  /* I hate UNIX cos I wrote a snazzy routine to do this then the next day */
  /* I found out there is a library func that does it for ya - fool */
  sprintf(path,"%s/.ypIO",(*getpwnam(loginname)).pw_dir);

  /* capitalise first letter for display */
  if (islower(target[0])) {
     target[0] = toupper(target[0]);
  }

  if ((fp = fopen(path,"r")) == NULL) {
    if (!own) {
      fprintf(stdout,"*%s has no yellow pages entry\n",target);
    } else {
      fprintf(stdout,"*You do not have a yellow pages entry. Put a .ypIO in your top level directory\n");
    }
    return(1);
  }
  fclose(fp);
  if (!own) {
    fprintf(stdout,"*Yellow pages entry for %s:\n",target);
  } else {
    fprintf(stdout,"*Your yellow pages entry:\n");
  }
  pg(path);
  fprintf(stdout,"\n");
} /* yp */


/* emote to another user on chat in private (oo-er) */
rtell(buf)
char *buf;
{
  char ag0[300], ag1[15], ag2[300];
  char *ptr;
  int i;
  int ln, ln0, ln1, ln2;

  int lnbuf = strlen(buf);

  ag0[0] = EOM;
  ag1[0] = EOM;
  ag2[0] = EOM;

  /* point to first argument in list - ie. the name */
  /* ie. get the name of the user to tell */
  ptr = &buf[3];

  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);

  /* too long */
  if (ln2 > 12) {
    fprintf(stdout,"*No such user has ever existed\n");
    return(0);
  }

  /* no text atall */
  if (ln2 == 0) {
    fprintf(stdout,"*Emote what to who?\n");
    return(0);
  }

  /* store first argument */
  strcpy(ag1,ag2);
  ln1 = ln2;

  if(expand(ag1)==1) return(0);

  if (!(cstrcmp(user.chatname,ag1))) {
    fprintf(stdout,"*Emote to yourself? Take a seat on the couch...\n");
    return(0);
  }

  /* deal with special group netfolk */
  if (!(cstrcmp("netfolk",ag1))) {
    netfolk(buf,1);
/*    fprintf(stdout,"*You can only emote to a single person"); */
    return(0);
  }

  /* notify server */
  outpk.action = FIND;
  outpk.id = user.id;
  strcpy(outpk.text,ag1);
  transmit();
  
  /* can't find name */
  if (inpk.status == 1) {
    fprintf(stdout,"*The dwarves of falseness throw mud at your back\n");
    return(0);
  }

  /* check if user is gagged */
  if (user.gagged == 'T') {
    fprintf(stdout,"*You fail to attract their attention\n");
    return (0);
  }

  /* capitalise the first letter - just for display */
  if (islower(ag1[0])) {
     ag1[0] = toupper(ag1[0]);
   }
  
  /* point to second argument ie. message part */
  i = 0;
  while( (isspace(*ptr)) && (i <= (lnbuf-3)) ) {
    ptr++;
    i++;
  } /* while */

  /*  adjust pointer to skip over first argument */
  ptr = ptr + ln1 + 1;

  ag2[0] = EOM;
  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);
  
  /* no message component */
  if (ln2 == 0) {
    fprintf(stdout,"*Emote what to %s\n",ag1);
    return(0);
  }

  /* build up sentence for message file */
  sprintf(text,"t%s+>%s+%s\n",ag1,user.chatname,ptr);
  lntext = format(text);
  write(msgsfile,text,lntext);

  fprintf(stdout,"*Done it to %s\n",ag1);

} /* rtell*/


coredump()
{
  char cha[4];
  fprintf(stderr,"*YARGH: Moon in the wrong phase - seg. fault - huge core dumped\n");
  fflush(stderr);
  fcntl(0,F_SETFL,FNDELAY);
  while((read(0,cha,1)) != -1)
    ;
  fcntl(0,F_SETFL,FASYNC);
  fflush(stdin);
  redotty();
}

/* shut down */
shutdown(buf)
char *buf;
{
  char ag2[300], ag1[15];
  char *ptr;
  int i;
  int ln, ln0, ln1, ln2;

  int lnbuf = strlen(buf);

  ag1[0] = EOM;
  ag2[0] = EOM;

  /* point to first argument in list - ie. the name */
  ptr = &buf[3];

  sscanf(ptr,"%s",ag2);
  ln2 = strlen(ag2);

  /* too long */
  if (ln2 > 12) {
    fprintf(stdout,"*No such user has ever existed\n");
    return(0);
  }

  /* no text atall */
  if (ln2 == 0) {
    /* kill all processes directly */
    fprintf(stdout,"*Initiating Emergency Shutdown (I hope you know what you are doing?)...\n");
    
    /* notify server */
    outpk.action = SHUTDOWN;
    outpk.id = user.id;
    transmit();
    exit(1);
    return(0);
  }

  /* store first argument */
  strcpy(ag1,ag2);
  ln1 = ln2;

  if (!(cstrcmp("netfolk",ag1))) {
    fprintf(stdout,"*Hit the Netfolk eh? Ya splitter\n");
    return(0);
  }
  if(expand(ag1)==1) return(0);

  /* attempt to gag self */
  if ((!(cstrcmp(user.chatname,ag1))) && (user.gagged == 'F')) {
    fprintf(stdout,"*You are unable to get your hands out of the straight jacket?\n");
    return(0);
  }

  /* notify server */
  outpk.action = HIT;
  outpk.id = user.id;
  strcpy(outpk.text,ag1);
  transmit();

  /* can't find name */
  if (inpk.status == MAXCHAN+1) {
    fprintf(stdout,"*No one by that name is currently on\n");
    return(0);
  }

  /* capitalise first letter for display */
  if (islower(ag1[0])) {
     ag1[0] = toupper(ag1[0]);
  }


  /* build up sentence for message file */
  i = inpk.status;
  sprintf(text,"c%d *%s has been violently removed by %s\n",i,ag1, user.chatname);
  lntext = strlen(text);
  text[++lntext] = EOM;
  write(msgsfile,text,lntext);
  /* if not on same channel as user display message */
  if ((inpk.status) != user.channel) {
    fprintf(stdout,"*%s has been terminated. Hasta la vista baby\n",ag1);
  } /* if */
  return(0);
  
} /* hit */


background()
{
  printf("%d\n",bg);
  if (bg == 1) {
    strcpy(chato,"has been kicked out for running conf in the background\n");
    logout();
  }
  exit(1);
}

numberusers()
{

  unsigned int number = 0;
  char ag0[100];

  clearerr(userfile);
  rewind(userfile);
  while (fgets(ag0, 90, userfile)!= NULL){
     number++;
   }
  if (number==1){
  fprintf(stdout, "*There is only 1 user on Conf 3000\n");
  }else{
  fprintf(stdout, "*There are %d users on Conf 3000\n", number);
  }
  return(0);
}

boot()
{
  execl(AUTOREBOOT, "autoreboot",user.chatname,0);
}  

/* expand a username and look for one that matches */
/* just to out do that Pete Clampitt fellow who put this on chatter */
/* Atleast I don't drink cider */
expand(rabbit)
char *rabbit;
{
  int found = 0;
  char *ptr, *ptr1;
  char buf[90];
  char cname[20],lname[10], posname[20];
  int ln;

  ln = strlen(rabbit);

  clearerr(aliasfile);
  rewind(aliasfile);

  while(((fgets(buf,85,aliasfile)) != NULL) && (found != 2)) {
    sscanf(buf,"%s %s",lname,cname);
    if(!(cstrncmp(rabbit,cname,ln))) {
      if (found==1) {
	fprintf(stdout,"*Eeek '%s' ambiguous\n",rabbit);
	return(1);
      }
      found=1;
      strcpy(posname,cname);
      
      if(cname[ln] == '\000') found = 2;
    }
  }
  if (found) strcpy(rabbit,posname);

  return(0);
}
