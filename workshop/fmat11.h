#include "gubbings.h"

char *ptr;
int lnbuf = 0;


format(buf)
char *buf;
{

  char ch;          /* current character from standard input steam */
  int n = 0;        /* simple character counter compared to MAX */
  int lp;           /* general purpose loop variable */

  int flag = FALSE; /* indicate status of main loop */
  int last = TRUE;  /* indicate nature of space - start/end of word */

  int length = 0;       /* number of charts on a line minus spaces */
  int spaces = 0;       /* length of initial headr - used to indent */
  int ln = 0;           /* length of current word */
  int left = 0;         /* hold the number of characters left on current line */
  int line = 0;     /* count the chars already on that line */

  int ws = 0;       /* count the chars in the current ,word */
  char word[MAX];   /* stores the current word */

  char temp[300];
  strcpy(temp,buf);
  lnbuf = 0;

  /* collect info on header */
  ptr = &temp[0];
  spaces = name(buf);
  length = SCREEN - spaces;


  /* main formatting loop */
  while (!flag) {
    ch = *(ptr++);
    /* test for end of stream */
    if (ch == RETURN) {
      flag = TRUE;
      ch = ' ';  /* set ch to space to tidy up current word then exit */
    }
    if (ch == ' ') {

      /* test if word is bigger than allocated screen line */
      if (ws > length) {
	line = maxline(buf, word, ws, line, spaces, length);
	ws = 0;
	last = FALSE; }
      else {
	/* calculate length of word */
	ln = ws + 1;

       
	if (ln > 1) {
	  /* closing space ie. at end of word - deal with word */
	  left = length - line;

	  /* test to see if word will fit on current line */
	  if (ln > left) {
	    /* create a new line */
	    buf[lnbuf++] = RETURN;
	    for (lp = 1; lp <= spaces; lp++)
	      buf[lnbuf++] = ' ';
	    line = 0;
	  }

	  if (last == FALSE && line != 0) {
	    /* add a space before the word */
	    buf[lnbuf++] = ' ';
	    line++;
	  }

	  /* put word on current line */
	  last = FALSE;
	  for (lp = 1; lp <= ws; lp++)
	    buf[lnbuf++] = word[lp];
	  line = line + ws;
	  ws = 0; }
	else if (last = FALSE) {
	  /* deal with an opening space ie. at the start of a word */
	  line++;
	  buf[lnbuf++] = ' ';
	  last = TRUE; /* set to ignore consecutive opening spaces */
	}
      } 
    }
    else {
      /* add non-space to current word */
      ws++;
      word[ws] = ch;
    }


  } /* while */

  buf[lnbuf++] = RETURN;
  buf[lnbuf++] = EOM;
  return(strlen(buf)+1);

} /* main */

/* calculate the length of the header                    */
/* end of header is indicated by a single space          */
int name(buf)
char buf[];
{
  char ch;
  int n = 0;
  int lp;
  int first = 0;

  /* output chars up to and including the first space */
  do {
    ch = *(ptr++);
    if ((ch == '+') && (first == 0)) {
      ch = ' ';
      first = 1;
    }
    if (first) n++;
    buf[lnbuf++] = ch;
  } while (ch != '+');
  buf[lnbuf-1]  = ' ';
  /* return the header length for indentation purposes */
  return (n-1);

}


/*deal with a word bigger than a single line */
int maxline (buf, word, ws, line, spaces, length)
char buf[];
int ws, line, spaces, length;
char word[];
{

  int lp;
  int old;
  int new;

  /* if not currently on a new line the create one */
  if (line != 0) {
    buf[lnbuf++] = RETURN;
    for (lp = 1; lp <= spaces; lp++)
      buf[lnbuf++] = ' ';
    line = 0;
  }

  old = 1;
  /* decompose line till will fit on a single line */
  while ((ws - old) > length) {
    /* put part of word on a single line */
    new = old + length;
    for (lp = old; lp <= new; lp++)
      buf[lnbuf++] = word[lp];
    old = new + 1;
    /* create new line */
    buf[lnbuf++] = RETURN;
    for (lp =1; lp <= spaces; lp++)
      buf[lnbuf++] = ' ';
    line = 0;
  }

  /* put the remaining chars of the word on a line */
  for (lp = old; lp <= ws; lp++) {

    buf[lnbuf++] = word[lp];
  }   
  line = ws - old;
  /* returns the length of the current line */  
  return line;
  
}






