/* this is my code and its better than that low-level array dependant one*/
/* that prophet had - nerrr nerrr nerrr nerrr nerrr */

#include <stdio.h>
#include "gubbings.h"

chanmessage(chan)
 int chan;
{
  FILE *infile; 
  int found =0;
  char chars[85];
  int numb;
  char number[4];
 
 if(( infile=fopen(CHANNELS,"r+")) == NULL) {
    printf("Someone has stolen the channel descriptions\n");
    printf("Please find the owner of the program and kill them\n");
   return(1);
  }

 while ((found != 1) &&  (fgets(chars, 80, infile) != NULL)) {
   sscanf(chars, "%s", number);
   sscanf(number, "%d", &numb);
   if (chan == numb){ 

      while ( (fgets(chars, 80, infile) != NULL ) && (chars[0] != '@') ){
            fprintf(stdout, chars);
     }
     found = 1;
}
}
 fclose(infile);
}

