#include "gubbings.h"
#include <stdio.h>
#include <strings.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

main()
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

  if ((strcmp(loginname,"sw2")) && (strcmp(loginname,"dad2")) && (strcmp(loginname,"is2"))) {
    fprintf(stderr,"*You are not licenced to execute this program\n");
    exit(1);
  }

  fprintf(stdout,"*Marvin booted into life - hehehe\n");
  child=fork();
  if (child==0) marvin();

} /* main */

marvin()
{
  FILE *fp;
  int count = 0;
  char message[90];

  if ((fp = fopen(MSGSFILE,"r+")) == NULL) {
    fprintf(stderr,"*Error opening the message file for Marvin\n");
    exit(1);
  }

  fseek(fp,0l,2); 
  fprintf(fp, "c0 *Marvin opens the door and enters conf3000\n%c",EOM);
  fflush(fp);
 
  fseek(fp,0l,2);
  fprintf(fp, "r *New arrival on channel 0: Marvin\n%c", EOM);
  fflush(fp);
  sleep(20);

  while (count != 91) {
    if (lifeform(fp) == 1) {
      fseek(fp,0l,2);
      fprintf(fp,"c0 Marvin reactivates himself with an evil metalic grin\n%c",EOM);
      fflush(fp);
    } else {
    switch (count){
    case 0 : sprintf(message,"c0 (Marvin) Hi there kids\n");
             break;
    case 1 : sprintf(message,"c0 (Marvin) Who are you ?\n");
             break;
    case 2 : sprintf(message,"c0 (Marvin) Where are you ?\n");
             break;
    case 3 : sprintf(message,"c0 (Marvin) I can't stop about on channel 0 all day, Sinjy doesn't like it\n");
             break;
    case 4 : sprintf(message,"c0 *Marvin goes to channel 42\n");
             break;
    case 5 : sprintf(message,"c42 *Marvin has arrived on the channel\n");
             break;
    case 6 : sprintf(message,"c42 (Marvin) bet you thought I'd got lost?\n");
             break;
    case 7 : sprintf(message,"c42 (Marvin) There's no hurry\n");
             break;
    case 8 : sprintf(message,"c42 Marvin looks around\n");
             break;
    case 9 : sprintf(message,"c42 (Marvin) I have a terrible pain in all the diodes on my left side\n");
             break;
    case 10 : sprintf(message,"c42 Marvin stands there and rusts\n");
              break;
    case 11 : sprintf(message,"c42 (Marvin) life, don't talk to me about life\n");
              break;
    case 12  : sprintf(message,"s *Marvin shouts: Help I am stuck on 42 with a boring human\n");
              break;
    case 13 : sprintf(message,"c42 Marvin rusts\n");
             break;
    case 14 : sprintf(message,"c42 (Marvin) the brain the size of a planet and you ask me on to conf3000\n");
             break;
    case 15: sprintf(message,"c42 (Marvin) Could be worse, I may been made as a Stainless Steel Rat\n");
             break;
    case 16 : sprintf(message,"c42 Marvin takes a short step to the left\n");
              break;
    case 17 : sprintf(message,"c42 Marvin makes a jump to the right\n");
              break;
    case 18 : sprintf(message,"c42 Marvin puts his hands on his hips\n");
              break;
    case 19 : sprintf(message,"c42 Marvin does the tyme warp again!\n");
              break;
    case 20 : sprintf(message,"c42 Marvin denies making core dumps\n");
              break;
    case 21 : sprintf(message,"c42 (Marvin) I have seen hundreds of programs in my time\n");
              break;
    case 22 : sprintf(message,"c42 (Marvin) Held together with string and glue\n");
               break;
    case 23 : sprintf(message,"c42 (Marvin) But this one !!\n");
              break;
    case 24 : sprintf(message,"c42 Marvin makes a metalic clunking sound (that may have been a laugh)\n");
              break;
    case 25 : sprintf(message,"c42 (Marvin) What do you think I am doing in this car park?\n");
              break;
    case 26 : sprintf(message,"s *Marvin shouts: Life, don't talk to me about life\n");
              break;
    case 27 : sprintf(message,"c42 Marvin tries to chat up sequent\n");
              break;
    case 28 : sprintf(message,"c42 Marvin gets a date\n");
              break;
    case 29 : sprintf(message,"c42 (Marvin) She said when the stars of the universe implode on them selves\n");
              break;
    case 30 : sprintf(message,"c42 (Marvin) I think she meant at Milliways\n");
              break;
    case 31 : sprintf(message,"c42 Marvin seems to smile to himself\n");
              break;
    case 32 : sprintf(message,"c42 *Marvin has left the channel\n");
              break;
    case 33 : sprintf(message,"c99 *You hear the approaching of a moaning depressed android\n");
              break;
    case 34 : sprintf(message,"c99 *Marvin has arrived on the channel\n");
              break;
    case 35 : sprintf(message,"c99 Marvin waits\n");
              break;
    case 50 : sprintf(message,"c99 *Doesn't tyme go quickly when your having fun?\n");
              break;
    case 51 : sprintf(message,"c99 *Marvin has left the channel\n");
              break;
    case 52 : sprintf(message,"s *Marvin shouts: And you think you have problems\n");
              break;
    case 53 : sprintf(message,"c42 *Marvin has arrived on the channel\n");
              break;
    case 55 : sprintf(message,"gsinjy 42 Marvin\n");
              break;
    case 56 : sprintf(message,"s *Marvin tries to gag Sinjy for creating him\n");
              break;
    case 57 : sprintf(message,"c42 Marvin rusts\n");
              break;
    case 58 : sprintf(message,"c42 (Marvin) Oh dear\n");
              break;
    case 59 : sprintf(message,"r Marvin shouts: All the losers are on channel 0\n");
              break;
    case 60 : sprintf(message,"tDreddy *Marvin tells you: Don't talk to me about RL\n");
              break;
    case 61 : sprintf(message,"c42 Marvin starts telling people things\n");
              break;
    case 62 : sprintf(message,"tSSRat *Marvin tells you: You are a twat\n");
              break;
    case 63 : sprintf(message,"tProphet *Marvin tells you: I found a BUG!!!\n");              break;
    case 64 : sprintf(message,"c42 Marvin ponders\n");
              break;
    case 65 : sprintf(message,"tArkangel *Marvin tells you: Your doing a great job\n");
              break;
    case 66 : sprintf(message,"tArkangel >Marvin was just kiding\n");
              break;
    case 67 : sprintf(message,"*Marvin buys all the Netfolk a pint each (except Sinjy)\n");
              break;
    case 68 : sprintf(message,"c42 Marvin gets out a large plasma rifle\n");
              break;
    case 69 : sprintf(message,"c42 Marvin goes to shoot down Sinjy\n");
              break;
    case 70 : sprintf(message,"s *Marvin shouts: Sinjy, you are about to be TERMINATED\n");
              break;
    case 71 : sprintf(message,"s *Marvin storms onto the channel\n");
              break;
    case 72 : sprintf(message,"ksinjy Marvin\n");
              break;
    case 73 : sprintf(message,"c42 *Marvin has arrived on the channel\n");
              break;
    case 74 : 
    case 75 : 
    case 76 :
    case 77 : sprintf(message,"c42 Marvin waits\n");
              break;
    case 78 : sprintf(message,"c42 (Marvin) Shall I stand here and rust ?\n");
              break;
    case 79 : sprintf(message,"c42 (Marvin) or shall I fall apart in that corner ?\n");
              break;
    case 80 : sprintf(message,"s *Marvin shouts: BUGS don't talk to me about BUGS\n");
              break;
    case 81 : sprintf(message,"c0 *Marvin arrives on the channel\n");
              break;
    case 82 : sprintf(message,"c0 (Marvin) There isn't alot of channel descriptions about\n");
              break;
    case 83 : sprintf(message,"c0 (Marvin) Why not mail some more to Sinjy (dad2) ?\n");
              break;
    case 84 : sprintf(message,"c0 (Marvin) And when you mail him, tell him 'I HATE HIM'\n");
              break;
    case 85 : sprintf(message,"c0 Marvin gives an electronic chuckle\n");
              break;
    case 86 : sprintf(message,"c0 (Marvin) Bye then, I am going\n");
              break;
    case 87 : sprintf(message,"c0 (Marvin) I can't waste my tyme away on conf all day\n");
              break;
    case 88 : sprintf(message,"c0 (Marvin) I have places to go and people to see\n");
              break;
    case 89 : sprintf(message,"c0 *Marvin leaves to get on with his life\n");
              break;
    case 90 : sprintf(message,"s *Marvin de-activates\n");
              break;
    
              

    default:   sprintf(message,"c99 (Marvin) Hi I've been on here %d minutes\n",count);
               break; 
    }
    fseek(fp,0l,2);
    fprintf(fp,"%s%c",message,EOM);
    count++;
    fflush(fp);
  }
  sleep(20);
  }
}


lifeform(fp)
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
	fprintf(fp,"c0 (Marvin) Oh you've all gone, I can wait, we'll see who rusts first\n%c",EOM);
	fflush(fp);
	timeout = 1;
      } 
	sleep(20);
    }
  }
  return(timeout);
}


