

/* Conf3000 - server program (c) The Prophet Hull 1991 */

#include "stevelib.h"
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/file.h>
#include "packet1.h"
#include "gubbings.h"
#include <errno.h>

/* server program */

/* signal flags */
int read_sig;     /* signal sigusr2 */
int got_sig = 0;  /* signal sigusr1 */
int wakeup = 0;

/* file descriptors */
FILE *userlist;   
FILE *aliasfile;
FILE *serverpid;
FILE *userlog;
FILE *banfile;

int n_users = 0;
int channels[MAXCHAN];

int atimer;
int t_msgs;
struct timeval t_sub, t_main;

int fpos[MAXUSERS]; /* points to the id */

/* store info on a user */
struct user {
  int unused;
  int fpos;
  char loginname[6];
  char chatname[11];
  int channel; 
  int pid;
  int cpid;
  char alias[51];
  char gagged;
  char netfolk;
};

/* this is global at the mo for diagnosic purposes */
struct slavepk inpk;    /* incomming packet */

/* hold up to maxusers users */
struct user users[MAXUSERS+2];  /* +2 playing it safe..... */


main() {

  int r;        /* general purpose result var */
  int i;        /* general purpose counter */
  int inpipe;   /* input pipe */
  int outpipe;  /* output pipe */
  struct serverpk outpk;  /* outgoing packet */
  int slavepid;

  /* signal handlers */
  extern handler();
  extern secondsig();
  extern diagnostic();

  /* put the process number of the server in a file */
  /* so slaves can access it */
  if ((serverpid = fopen(SERVERPID,"w+")) == NULL) {
    file_error(SERVERPID);
  }
  fprintf(serverpid,"%d\n",getpid());
  fclose(serverpid);

  setmain();

  /* set all the user arrays to unused */
  for (i=0;i<MAXUSERS;i++) {
    users[i].unused = 0;
  } /* for */

  /* set all channels to unlocked */
  /* and no one on */
  /* most sig bit used for lock */
  for (i=0;i<MAXCHAN;i++) {
     channels[i] = 0;
   } /*for */

  /* create a new userlist file */
  userlist = fopen(USERLIST, "w+");
  if (userlist == NULL) {
    file_error(USERLIST);
  } /* if */


  /* open the alias file for reading */
  aliasfile = fopen(ALIASFILE, "r+");
  if (aliasfile == NULL) {
    file_error(ALIASFILE);
  } /* if */
  /* open the alias file for reading */
  banfile = fopen(BANFILE, "r+");
  if (banfile == NULL) {
    file_error(BANFILE);
  } /* if */

  /* open the log file */
  userlog = fopen(USERLOG, "r+");
  if (userlog == NULL) {
    file_error(USERLOG);
  } /* if */

  /* open the input pipe */
  inpipe = open(SLAVEPIPE, O_RDONLY|O_NDELAY);
  if (inpipe < 0) {
    file_error(SLAVEPIPE);
  } /* if */

  signal(SIGINT,diagnostic);

  /* deal with sigusr2 */
  signal(SIGUSR2,secondsig);

  /* hang around for a SIGUSR1 - sent by slave to notify of */
  /* incomming packet */ 
  signal(SIGUSR1,handler);
  got_sig = 0;

  /* loop forever (really until last user logged out */
  for (;;) {

    /* do nothing until a sigusr1 is received */
    while (!got_sig) {
      pause();
    } /* while */

    /* read in packet from pipe */
    read(inpipe,&inpk,sizeof(inpk));

    /* act on input packet */
    switch(inpk.action) {
    case CONNECT:
      outpk.status = connect(inpk);
      slavepid = inpk.id;
      break;
    case CHANGECHAN:
      outpk.status = changechan(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case LOCK:
      outpk.status = lockchan(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case OPEN:
      outpk.status = openchan(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case LOGOUT:
      outpk.status = logout(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case FIND:
      outpk.status = find(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case HIT:
      outpk.status = hit(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case KILL:
      outpk.status = kick(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case NETFOLK:
      outpk.status = netfolk(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case CHANGEALIAS:
      outpk.status = changealias(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case SHUTDOWN:
      shutdown(inpk);
      break;
    case FLIPCHAN:
      outpk.status = flipchan(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case GAG:
      outpk.status = gag(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case RECONNECT:
      outpk.status = reconnect(inpk);
      slavepid = inpk.id;
      break;
    case BAN:
      outpk.status = ban(inpk);
      slavepid = users[inpk.id].pid;
      break;
    case WHATCHAN:
      outpk.status = users[inpk.id].channel;
      slavepid = users[inpk.id].pid;
      break;

    default:
      fprintf(stderr,"YARGH: Unknown Packet code\n");
      break;
    } /* switch */

    /* modify signal counter to indicate signal dealt with */
    got_sig--;

    /* open output pipe for writing */
    outpipe = open(SERVERPIPE,O_WRONLY|O_NDELAY); 
    if (outpipe < 0) { 
      file_error(SERVERPIPE); 
    } /* if */

    /* send output package */
    r = write(outpipe, &outpk, sizeof(outpk));
    read_sig = 0;
    /* notify slave that its info is on the server pipe */
    if (kill(slavepid,SIGUSR1) == -1) {
      remove(inpk);
    } else {

      count(2);
      /* wait for slave to read info sent */
      while((!read_sig) && (!wakeup)) {
	pause();
      } /* while */
      if (wakeup == 1) {
	remove(inpk);
      }
      if (wakeup == 0) siren();
    } /* if */

    /* close the ouput pipe until require again */
    close(outpipe);

  } /* for */

} /* main */


/* handle sigusr1 - count number of times recieved */
handler()
{
  got_sig++;
} /* handler */


/* set a flag if sigusr2 is received */
secondsig()
{
  read_sig = 1;
} /* secondsig */


/* deal with those nasty file errors */
file_error(filename)
char filename[];
{
  fprintf(stderr, "*FILE ERROR: %s unable to access\n",filename);
  exit(1);
} /* file_error */



/* deal with a request to connect to server */
connect(inpk)
struct slavepk inpk;
{
  int r;
  int i = 0;         /* general purpose counter */
  int found = 0;     /* flag */

  long pos, t;
  char *ctime();
  long time();

  char logintime[18];
 
  /* temp vars for reading in from a file */
  char line[80];
  char ag1[80];
  char loginname[6]; 
  char chatname[11];

  char *ptr;  /* pointer to decompose string */   
  
  /* check to see of already on chat */
  if (onchat(inpk.text) != (MAXUSERS+3)) {
    return(MAXUSERS+3);
  }  /* if */
  

  /* find first unused record */
  while ((i < MAXUSERS) && (users[i].unused == 1)) {
    i++;
  } /* while */
  /* system is full */
  if (i == MAXUSERS) {
    return(MAXUSERS+1);
  } /* if */

  /* store available info */
  users[i].unused = 1;
  users[i].pid = inpk.id;

  /* set to default channel */
  users[i].channel = 0;

  /* set login name */
  strcpy(users[i].loginname,inpk.text);
  /* fprintf(stderr,"*Received login name: %s\n",users[i].loginname); */

  /* REW to start of alias file */
  fseek(aliasfile,0l,0);

  strcpy(users[i].alias,BLANK);
  /* scan alias file for rest of info */
  while (((fgets(line,80,aliasfile)) != NULL) && (!found)) {
 
    sscanf(line,"%s%s",loginname,chatname);

    if (!(strcmp(loginname,users[i].loginname))) {
      found = 1;

      /* point to alias */
      ptr = &line[17];
      sscanf(ptr,"%50c",users[i].alias);
      users[i].alias[50] = EOM;
      strcpy(users[i].chatname,chatname);
      users[i].gagged = line[68];
      users[i].netfolk = line[73];
    } /* if */
  } /* while */

  /* if not found then set to default values */
  if (!(found)) {
    strcpy(users[i].chatname,users[i].loginname);
    strcpy(users[i].alias,BLANK);
    users[i].gagged = 'F';
    users[i].netfolk = 'F';
  } /* if */

  rewind(banfile);
  found = 0;
  while (((fgets(line,80,banfile)) != NULL) && (!found)) {
    sscanf(line,"%s",chatname);
    if (!(cstrcmp(chatname,users[i].chatname))) {
      found = 1;
      users[i].unused = 0;
      return(MAXUSERS+2);
    } /* if */
  } /* found */

  n_users++;
  users[i].fpos = n_users;
  fpos[n_users] = i;
  channels[0]++;
  logtime(logintime);
  
  /* format the output to the userlist file so channels + dates columns */
  /* are tabulated */
  fseek(userlist,0l,2);
  r = 11 - strlen(users[i].chatname);
  fprintf(userlist,"%s %s%*s%2d   %s\n",users[i].chatname,users[i].alias,r," ",users[i].channel,logintime);
  fflush(userlist);

  found = 0;
  
  /* REW to start of alias file */
  fseek(userlog,0l,0);

  strcpy(ag1,BLANK);
  pos = ftell(userlog);
  /* scan log file for user */
  while (((fgets(line,80,userlog)) != NULL) && (!found)) {
 
    sscanf(line,"%s",chatname);

    if (!(strcmp(chatname,users[i].chatname))) {
      found = 1;
      fseek(userlog,pos,0);
      time(&t);
      fprintf(userlog,"   %-12s %-20s",users[i].chatname,ctime(&t));
      rewind(userlog);
      fflush(userlog);
    } /* if */
  pos = ftell(userlog);
  } /* while */

  if (!found) {
      fseek(userlog,0l,2);
      time(&t);
      fprintf(userlog,"   %-12s %-20s",users[i].chatname,ctime(&t));
      rewind(userlog);
      fflush(userlog);
  } /* if */

  return(i);

} /* connect */


/* find out if that loginname is on chat */
onchat(loginname)
char *loginname;
{
  int i = 0;
  int found = MAXUSERS + 3;

  while ((i < MAXUSERS) && (found == MAXUSERS + 3)) {
    if (users[i].unused == 1) {
      if (!(strcmp(loginname,users[i].loginname))) {
	found = i;
      } /* if */
    } /* if */
    i++;
  } /* while */

  /* return id of user */
  return(found);

} /* onchat */



/* find out if that chatname is on chat */
conchat(chatname)
char *chatname;
{
  int i = 0;
  int found = MAXUSERS + 3;

  while ((i < MAXUSERS) && (found == MAXUSERS+3)) {
    if (users[i].unused == 1) {
      if (!(cstrcmp(chatname,users[i].chatname))) {
	found = i;
      } /* if */
    } /* if */
    i++;
  } /* while */

  /* return id of user */
  return(found);

} /* conchat */


/* see if a user exists */
find(inpk)
struct slavepk inpk; 
{
  if (conchat(inpk.text) == (MAXUSERS + 3)) {
    return(1);
  }
  return(0);
} /* find */


kick(inpk)
struct slavepk inpk; 
{
  int id;
  
  if ((id = conchat(inpk.text)) == (MAXUSERS + 3)) {
    return(1+MAXCHAN);
  }
  return(users[id].channel);
} /* kick */


/* attempt to gag/ungag a user */
gag(inpk)
struct slavepk inpk; 
{
  int id;

  /* user not found */
  if ((id = conchat(inpk.text)) == (MAXUSERS + 3)) {
    return(2*MAXCHAN);
  }

  /* user ungagged */
  if (users[id].gagged == 'T') {
    users[id].gagged = 'F';
    return(users[id].channel+MAXCHAN);
  }

  /* user gagged */
  users[id].gagged = 'T';
  return(users[id].channel);
} /* gag */


/* find members of group netfolk */
netfolk(inpk)
struct slavepk inpk;
{
  int lp;
  int count = 0;

  /* count the netfolk on */
  for(lp=0;lp<MAXUSERS;lp++) {
    if ((users[lp].unused == 1) && (users[lp].netfolk == 'T')) {
      count++;
    } /* if */
  } /* for */

  /* exclude self from counf */
  if (users[inpk.id].netfolk == 'T') {
    count--;
  } /* if */
  return(count);

} /* netfolk */


/* grab the time and date */
logtime(logintime)
char *logintime;
{
  char day[5], date1[5], date2[5], tim[10];
  char *ctime();
  long time();
  long t;

  time(&t);
  sscanf(ctime(&t),"%s %s %s %s",day,date1,date2,tim);

  sprintf(logintime,"%s %s",tim,day);

} /* logtime */




changechan(inpk)
struct slavepk inpk;
{
  int check;
  long int ll;
  int oldchan;

  /* channel is locked - no access */
  if (channels[inpk.number] & 128) {
    return(1);
  } /* if */

  oldchan = users[inpk.id].channel;
  channels[oldchan]--;

  /* unlock a channel auto if less than 2 people on it */
  check = (channels[oldchan] & 127);
  if (check < 2) {
    channels[oldchan] = check;
  } /* if */

  channels[inpk.number]++;

  users[inpk.id].channel = inpk.number;

  /* update the userlist */
  ll = (users[inpk.id].fpos)*80-18;
  fseek(userlist,ll,0); 
  /* the 2 would need changin if chan > 99 to align columns */
  fprintf(userlist,"%2d",inpk.number);
  fflush(userlist);

  return(0);

} /* changechan */

flipchan(inpk)
struct slavepk inpk;
{
  int check;
  long int ll;
  int oldchan;

  oldchan = users[inpk.id].channel;
  channels[oldchan]--;

  /* unlock a channel auto if less than 2 people on it */
  check = (channels[oldchan] & 127);
  if (check < 2) {
    channels[oldchan] = check;
  } /* if */

  channels[inpk.number]++;

  users[inpk.id].channel = inpk.number;

  /* update the userlist */
  ll = (users[inpk.id].fpos)*80-18;
  fseek(userlist,ll,0); 
  /* the 2 would need changin if chan > 99 to align columns */
  fprintf(userlist,"%2d",inpk.number);
  fflush(userlist);

  return(0);

} /* changechan */


/* lock the channel */
lockchan(inpk)
struct slavepk inpk;
{
  int check;

  /* channel is already locked */
  if (channels[users[inpk.id].channel] & 128) {
    return(1);
  } /* if */

  if (channels[users[inpk.id].channel] == 1) {
    return(2);
  } /* if */

  channels[users[inpk.id].channel] = channels[users[inpk.id].channel] + 128;
  return(0);
  
} /* lock chan */


/* open the channel */
openchan(inpk)
struct slavepk inpk;
{

  /* channel is already open */
  if (!(channels[users[inpk.id].channel] & 128)) {
    return(1);
  } /* if */

  channels[users[inpk.id].channel] = channels[users[inpk.id].channel] & 127;
  return(0);

} /* openchan */


/* deal with a logout request */
logout(inpk)
struct slavepk inpk;
{
  char store[MAXUSERS*80];
  char line[85];
  char *ptr[MAXUSERS];
  int i;
  int n = 0;
  int oldchan, check;
  char *ptr1;
  long int pos;
  int found = 0;
  char loginname[20];

  /* intialise pointers to store - multi dim */ 
  for(i=0; i<MAXUSERS; i++) { 
     ptr[i] = &store[i*85]; 
  }  /* for */

  /* read in the userlist file */
  fseek(userlist,0l,0);
  for(i=1; i<=n_users; i++) {
    if (fpos[i] != inpk.id) {
      fgets(ptr[n],82,userlist);
       n++;
      users[fpos[i]].fpos = n;
      fpos[n] = fpos[i];
    } else {
      fgets(line,82,userlist);
    } /* if */
  } /* for */

  fclose (userlist);

  /* re-create userlist + put back remaining users */
  userlist = fopen (USERLIST,"w+");
  for (i=0; i<n; i++) {
    fputs(ptr[i],userlist);
  } /* for */
  fflush(userlist);

  /* remove count from channel */
  oldchan = users[inpk.id].channel;
  channels[oldchan]--;

  /* unlock a channel auto if less than 2 people on it */
  check = (channels[oldchan] & 127);
  if (check < 2) {
    channels[oldchan] = check;
  } /* if */


  /* erase message file (buffer) */
  if (n_users == 1) {
    i = creat(MSGSFILE,0700);
    close(i);
  } /* if */

  /* write info back to the alias file */
  /* REW to start of alias file */
  fseek(aliasfile,0l,0);

  /* scan alias file for users position */
  pos = ftell(aliasfile);
  while (((fgets(line,80,aliasfile)) != NULL) && (!found)) {
 
    sscanf(line,"%s",loginname);
    if (!(strcmp(loginname,users[inpk.id].loginname))) {
      found = 1;
      pos = pos + 17l;
      fseek(aliasfile,pos,0);
      fprintf(aliasfile,"%s %c",users[inpk.id].alias,users[inpk.id].gagged); 
      rewind(aliasfile);
      fflush(aliasfile);
    } /* if */
    pos = ftell(aliasfile);
  } /* while */

  /* indicate space is free */
  users[inpk.id].unused = 0;
  n_users--;

  return(n_users);

} /* logout */



changealias(inpk)
struct slavepk inpk;
{

  int ll;
  strcpy(users[inpk.id].alias,inpk.text);

  /* change the alias on the userlist */
  ll = ((users[inpk.id].fpos)-1)*80+strlen(users[inpk.id].chatname)+1;
  fseek(userlist,ll,0); 
  fprintf(userlist,"%s",inpk.text);
  fflush(userlist);

  return(0);

} /* changealias */


remove(inpk)
struct slavepk inpk;
{
  int r;
  struct serverpk pk;
  char text[70];
  int lntext, msgsfile;

  int outpipe = open(SERVERPIPE,O_RDONLY|O_NDELAY);

  r = read(outpipe,&pk,sizeof(pk));
  close(outpipe);

  kill(users[inpk.id].pid,SIGUSR2);
  msgsfile = open(MSGSFILE,O_WRONLY,0);
  sprintf(text,"c%d *%s has not responded: TERMINATED\n",users[inpk.id].channel,users[inpk.id].chatname);
  lntext = strlen(text);
  text[++lntext] = EOM;
  lseek(msgsfile, 0l, 2);
  write(msgsfile, text, lntext);
  close(msgsfile);

  logout(inpk);

  return(0);

} /* remove */

/* received internal alarm */
siren()
{
  struct itimerval value, m_alarm;
  struct timeval result, result1;
  int msgsfile, lntext;
  char text[90];

  /* deal with alarm depending on which timer in use (atimer) */
  if (atimer == 0) {
    wakeup = 1;

    /* work out time used by sub timer and reload main timer */
    /* less this value */
    if (t_msgs != 0) {
      /* get the time used by the subtimer and subtract it form main timer */
      getitimer(ITIMER_REAL,&value);
      subtract(t_sub,value.it_value,&result);
   /*   printf("%d %d of sub timer gone (sub from main)\n",result.tv_sec,result.tv_usec);  */
      subtract(t_main,result,&result1);
   /*   printf("%d of main timer left\n",result1.tv_sec); */
      /* reload the main alarm and activate */
      m_alarm.it_value.tv_sec = result1.tv_sec;
      m_alarm.it_value.tv_usec = result1.tv_usec;
      m_alarm.it_interval.tv_sec = 0;
      m_alarm.it_interval.tv_usec = 0;
      setitimer(ITIMER_REAL,&m_alarm,&m_alarm);
      atimer = 1;
    } else {
      /* cold possible call setmain from here and this would cater for */
      /* round the clock timeouts - more CPU time though */
      /* reset and disable alarm - no more timeouts */
      m_alarm.it_value.tv_sec = 0;
      m_alarm.it_value.tv_usec = 0;
      m_alarm.it_interval.tv_sec = 0;
      m_alarm.it_interval.tv_usec = 0;
    } /* if */      
  } else {

    /* write message to the message file if there are any users on */
    if (n_users != 0) {
      msgsfile = open(MSGSFILE,O_WRONLY,0);

      switch(t_msgs) {
      case 1:
	sprintf(text,"s *A large clock slowly strikes noon\n");
	break;
      case 2:
	sprintf(text,"s *A patronising old biddy come up to you and tells you to 'sign off'\n");
	break;
      case 3:
	sprintf(text,"s *What are you doing on at this time? Don't you people have homes to go to?\n");
	break;
      case 4:
	sprintf(text,"s *A postgrad arrives and hassles you to leave the department\n");
	break;
      }
            
      lntext = strlen(text);
      text[++lntext] = EOM;
      lseek(msgsfile, 0l, 2);
      write(msgsfile, text, lntext);
      close(msgsfile);
    } /* if */
    /* grab the next alarm time */
    setmain();
  }

}

count(sec)
long sec;
{
  struct itimerval s_alarm, m_alarm;

  t_sub.tv_sec = sec;
  t_sub.tv_usec = 0;
  
  signal(SIGALRM,siren);

  /* load and activate sub alarm */
  s_alarm.it_interval.tv_sec = 0;
  s_alarm.it_interval.tv_usec = 0;
  s_alarm.it_value.tv_sec = sec;
  s_alarm.it_value.tv_usec = 0;

  wakeup = 0;
  setitimer(ITIMER_REAL,&s_alarm,&m_alarm);
  atimer = 0;

  /* remember state of main alarm */
  t_main.tv_sec = m_alarm.it_value.tv_sec;
  t_main.tv_usec = m_alarm.it_value.tv_usec;
/*  printf("Timer pushed back with %d to go\n",t_main.tv_sec); */

} /* suspend */


shutdown(inpk)
struct slavepk inpk;
{
  int i = 0;;
  while (i < MAXUSERS) {
    if (users[i].unused == 1) {
      kill(users[i].pid,SIGUSR2);
    } /* if */
    i++;
  } /* while */

  /* wipe message file */
  i = creat(MSGSFILE,0700);
  close(i);

  /* wipe user list */
  userlist = fopen (USERLIST,"w+");
  fclose(userlist);

  exit(2);
} /* shutdown */


reconnect(inpk)
struct slavepk inpk;
{
  int id;
  id = onchat(inpk.text);
  if (id == MAXUSERS+3) {
    id = connect(inpk);
    return (id);
  }
  kill(users[id].pid,SIGUSR2);
  users[id].pid = inpk.id;
  return(id+MAXCHAN);

} /* reconnect */

ban(inpk)
struct slavepk inpk;
{
  int found;
  char line[80];
  char chatname[20];
  char store[20*15];
  int i, n, r;
  char *ptr[20];

  rewind(banfile);
  found = 0;
  n = 0;
  while (((fgets(line,80,banfile)) != NULL) && (!found)) {
    sscanf(line,"%s",chatname);
    if (!(cstrcmp(chatname,inpk.text))) {
      found = 1;
    } else {
      n++;
    } /* if */
  } /* found */
  if (!found) {
    fseek(banfile,0l,2);
    fprintf(banfile,"%s\n",inpk.text);
    fflush(banfile);
    return(0);
  }

  /* intialise pointers to store - multi dim */ 
  for(i=0; i<20; i++) { 
     ptr[i] = &store[i*15]; 
  }  /* for */

  /* read in the banfile */
  fseek(banfile,0l,0);
  i = 0;
  r = 0;
  while((fgets(line,12,banfile)) != NULL) {
    if (i != n) {
      sprintf(ptr[r++],"%s",line);
    }
    i++;
  } /* while */

  fclose(banfile);

  /* re-create banfile + put back remaining users */
  banfile = fopen (BANFILE,"w+");
  for (i=0; i<r; i++) {
    fputs(ptr[i],banfile);
  } /* for */
  fflush(banfile);

  return(1);

} /* ban */

setmain()
{

  struct itimerval m_alarm;
  struct tm *localtime(), *ltime;
  long time();
  long t;
  long sec;

  /* get local time in seconds */
  time(&t);
  ltime = localtime(&t);
  t_msgs = 0;
  sec = (*ltime).tm_hour*3600 + (*ltime).tm_min*60 + (*ltime).tm_sec;

  /* select target time nearest to */
  if (sec < (12*3600)) {
    m_alarm.it_value.tv_sec = (12*3600) - sec;   
    t_msgs = 1;
  } else {
    if (sec < (17*3600+15*60)) {
      m_alarm.it_value.tv_sec = (17*3600+15*60) - sec;
      t_msgs  = 4;
    } else {
      if (sec < (21*3600+15*60)) {   
	m_alarm.it_value.tv_sec = (21*3600+15*60) - sec;   
	t_msgs = 2;
      } else {   
	if (sec < (24*3600)) {
	  m_alarm.it_value.tv_sec = (24*3600) - sec;
	  t_msgs = 3;
	} else {
	  m_alarm.it_value.tv_sec = 0; 
	} 
      }
    }
  }
  m_alarm.it_value.tv_usec = 0;
  m_alarm.it_interval.tv_sec = 0;
  m_alarm.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL,&m_alarm,&m_alarm);
  atimer = 1;

}


subtract(t1,t2,result)
struct timeval t1,t2,*result;
{
  /* t1 - t2 */
  (*result).tv_sec = t1.tv_sec - t2.tv_sec;
  if (t2.tv_usec > t1.tv_usec) {
    (*result).tv_sec = (*result).tv_sec - 1l;
    t1.tv_usec = t1.tv_usec + 1000000;
  }
  (*result).tv_usec = t1.tv_usec - t2.tv_usec;


} /* subtract */

hit(inpk)
struct slavepk inpk; 
{
  struct slavepk tinpk;
  int id, i;
  
  if ((id = conchat(inpk.text)) == (MAXUSERS + 3)) {
    return(1+MAXCHAN);
  }
  kill(users[id].pid,SIGUSR2);
  i = inpk.id;
  tinpk.id = id;
  inpk.id = i;
  logout(tinpk);

  return(users[id].channel);

} /* hit */

diagnostic()
{
  FILE *diag;
  fprintf(stderr,"*Diagnostic collecting...\n");
  diag = fopen("/users/cs/is2/conf3/diagnostic","w+");

  fprintf(diag,"ID %d %s\n",inpk.id,users[inpk.id].loginname);
  fprintf(diag,"ACTION %d\n",inpk.action);
  fprintf(diag,"TEXT %s\n",inpk.text);
  fclose(diag);
  fprintf(stderr,"*Finshed collecting\n");
}
