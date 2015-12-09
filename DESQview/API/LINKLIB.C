#include <stdio.h>
#include "dvapi.h"

extern char panel_lib [];            /* link to panel data        */
extern ulong lib_size;               /* link to length var        */

main () {

uint ver;
ulong panhan,mywin,retwin,retkey;
int status,buflng;
char *buffer;

  ver = api_init();
  api_level(0x200);
  mywin = win_me();


  panhan = pan_new();                /* NEW step (INITIALIZATION) */

           /* OPEN step using pointer to panel data and panel     */
           /* data length variable               (INITIALIZATION) */
  status = pan_open(panhan,panel_lib,(int)lib_size);

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