
/* Steve Watts personal library for PD use */
#include <ctype.h>

/* compare strings regarless of case */
cstrcmp(temp, search)
char *temp;
char search[];
{
  int a, b, found, lp, done;

  int nsearch = strlen(search);

  done = 0;
  found = 1;
  lp = 0;

  if (strlen(temp) != nsearch) {
    return(1);
  }


  while (!(done)) {
    if (isupper(temp[lp])) {
      a = tolower(temp[lp]);
    } else {
      a = temp[lp];
    } /* if */

    if (isupper(search[lp])) {
      b = tolower(search[lp]);
    } else {
      b = search[lp];
    } /* if */

    if (a != b) {
      done = 1;
      found = 0;
    } else {
      lp++;
      if (lp == nsearch) done = 1;
    } /* if */
 } /* while */

  /* replace searching for string with found string - for case alterations */
  /* oh no you don't */
  /* if (found) strncpy(search, temp, nsearch); */

  return (!(found));

} /* cstrncmp */



/* compare strings regarless of case */
cstrncmp(temp, search,nsearch)
char *temp;
char search[];
int nsearch;
{
  int a, b, found, lp, done;

  done = 0;
  found = 1;
  lp = 0;


  if (strlen(temp) < nsearch) {
    return(1);
  }


  while (!(done)) {
    if (isupper(temp[lp])) {
      a = tolower(temp[lp]);
    } else {
      a = temp[lp];
    } /* if */

    if (isupper(search[lp])) {
      b = tolower(search[lp]);
    } else {
      b = search[lp];
    } /* if */

    if (a != b) {
      done = 1;
      found = 0;
    } else {
      lp++;
      if (lp == nsearch) done = 1;
    } /* if */
 } /* while */

  /* replace searching for string with found string - for case alterations */
  /* oh no you don't */
  /* if (found) strncpy(search, temp, nsearch); */

  return (!(found));

} /* cstrncmp */
