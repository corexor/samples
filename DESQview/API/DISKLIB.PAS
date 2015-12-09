program Panels;

uses DVAPI;

const
  panel_lib : string = 'examples.plb'#$00;   (* ASCIIZ filename *)

var
  ver : integer;
  panhan,mywin,retwin,retkey : ULONG;
  status,buflng : integer;
  buffer : pointer;

begin

  ver := api_init;
  api_level($200);
  mywin := win_me;

  panhan := pan_new;               (* NEW step (INITIALIZATION) *)

                    (* OPEN step using library filename         *)
                    (*                         (INITIALIZATION) *)
  status := pan_sopen (panhan,panel_lib);

                    (* APPLY step using panel name explicitly   *)
  status := pan_sapply (panhan,mywin,'POPUP',retwin,retkey);

  key_read (retkey,buffer,buflng); (* Wait for input            *)

  win_free (retwin);               (* Free new win,keybrd       *)

            (* Use the open panel object repeatedly using       *)
            (* APPLY (and READ, if input required) as necessary *)

  pan_close (panhan);              (* CLOSE step (TERMINATION)  *)
  pan_free (panhan);               (* FREE step  (TERMINATION)  *)

  api_exit;

end.
