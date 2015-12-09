#include <stdio.h>
#include "dvapi.h"

char panel_lib [] = "examples.plb\x00";   /* ASCIIZ library name  */

main () {

uint ver;
ulong panhan,mywin,retwin,retkey;
int status,buflng;
char *buffer;

  ver = api_init();
  api_level(0x200);
  mywin = win_me();


  panhan = pan_new();                /* NEW step (INITIALIZATION) */

           /* OPEN step using pointer to library filename         */
           /* and explicit length of filename    (INITIALIZATION) */
  status = pan_open(panhan,panel_lib,sizeof panel_lib);

           /* APPLY step using panel name,length explicitly       */
  status = pan_apply(panhan,mywin,"POPUP",5,&retwin,&retkey);

  key_read (retkey,&buffer,&buflng); /* Wait for input            */

  win_free (retwin);                 /* Free new win,keybrd       */

           /* Use the open panel object repeatedly using          */
           /* APPLY (and READ, if input required) as necessary    */

  pan_close (panhan);                /* CLOSE step (TERMINATION)  */
  pan_free (panhan);                 /* FREE step  (TERMINATION)  */

  api_exit();
}
