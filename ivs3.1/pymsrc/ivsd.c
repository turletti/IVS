/**************************************************************************\
*          Copyright (c) 1992 INRIA Sophia Antipolis, FRANCE.              *
*                                                                          *
* Permission to use, copy, modify, and distribute this material for any    *
* purpose and without fee is hereby granted, provided that the above       *
* copyright notice and this permission notice appear in all copies.        *
* WE MAKE NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY OF THIS     *
* MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", WITHOUT ANY EXPRESS   *
* OR IMPLIED WARRANTIES.                                                   *
\**************************************************************************/
/**************************************************************************\
*                                                                          *
*  File              : ivsd.c                                              *
*  Author            : Thierry Turletti                                    *
*  Last modification : 93/3/19                                             *
*--------------------------------------------------------------------------*
*  Description :  The IVS DAEMON ...                                       *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/



#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>	

#include <X11/Xaw/Command.h>	
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>	
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/AsciiText.h>

#ifdef __sgi
#define vfork fork
#endif
  

#include "BDaemon.bm"
#include "protocol.h"

#define DT_ALARM            10

#define LabelTalk          "IVS talk requested by "
#define LabelTalkRequest   "Received an IVS talk request from "
#define IvsCommand         "ivs"
#define LabelUMChoice      "What type of call ?"


XtAppContext app_con;
XtInputId id_cx, id_command;
Display *dpy;
Widget toplevel, main_form, daemon, popup_request, text_group, text_ttl;
Pixmap BDaemon_on, BDaemon_off;
int sock_listen, sock_cx;
char host_from[100];
char from[150];
BOOLEAN UNICAST=TRUE;
BOOLEAN ALARM;
int fd_command[2];
char addr_group[20];
char ttl[4];


static char defaultTranslations[] = 
  "Ctrl<Key>J:   no-op() \n \
   Ctrl<Key>M:   no-op() \n \
   Ctrl<Key>N:   no-op() \n \
   <Key>Return:  no-op()";


String fallback_resources[] = {
  "IVSD*font: -adobe-courier-bold-r-normal--14-100-100-100-m-90-iso8859-1",
  "IVSD*Form.borderWidth:                 0",
  "IVSD*Form.background:                  LightSteelBlue",
  "IVSD*Command.background:               DarkSlateGrey",
  "IVSD*Command.foreground:               MistyRose",
  "IVSD*Label.background:                 LightSteelBlue",
  "IVSD*Label.borderWidth:                0",
  "IVSD*Daemon.foreground:                Black",
  "IVSD*Daemon.background:                LightSteelBlue",
  "IVSD*MsgWarning.background:            Red",
  "IVSD*Accept.background:                Green",
  "IVSD*Accept.foreground:                Black",
  "IVSD*Refuse.background:                Red",
  "IVSD*Refuse.foreground:                Black",
  "IVSD*MsgRequest.background:            LightSteelBlue",
  "IVSD*MsgRequest.Label.background:      MistyRose",
  "IVSD*Host.background:                  LightSteelBlue",
  "IVSD*Host.label:                       Enter hostname:",
  "IVSD*Group.label:                      Enter multicast address:",
  "IVSD*TTL.label:                        Enter TTL value:",
  "IVSD*Text*editType:                    edit",
  "IVSD*Text.borderWidth:                 0",
  "IVSD*TextHost.width:                   200",
  "IVSD*TextTTL.width:                    30",
  "IVSD*TextGroup.width:                  150",
  NULL,
};



static XtCallbackProc DestroyWarning(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Widget popup;
  Arg args[1];

  XtSetArg(args[0], XtNbitmap, BDaemon_off);
  XtSetValues(daemon, args, 1);

  popup = XtParent((Widget) client_data);
  XtDestroyWidget(popup);
}
  


static void ShowWarning(msg)
     char *msg;
{
  Arg         args[5];
  Widget      popup, dialog;
  Position    x, y;
  Dimension   width, height;
  Cardinal    n;
  Widget      msg_error;

  n = 0;
  XtSetArg(args[0], XtNwidth, &width); n++;
  XtSetArg(args[1], XtNheight, &height); n++;
  XtGetValues(main_form, args, n);
  XtTranslateCoords(main_form, (Position) (width/2), (Position) (height/2),
		    &x, &y);

  n = 0;
  XtSetArg(args[n], XtNx, x); n++;
  XtSetArg(args[n], XtNy, y); n++;

  popup = XtCreatePopupShell("Warning", transientShellWidgetClass, main_form,
			     args, n);

  n = 0;
  XtSetArg(args[n], XtNlabel, msg); n++;
  msg_error = XtCreateManagedWidget("MsgWarning", dialogWidgetClass, popup,
				    args, n);

  XawDialogAddButton(msg_error, "OK", DestroyWarning, (XtPointer) msg_error);

  XtPopup(popup, XtGrabNone);
}



static XtCallbackProc TalkAnswer(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{  
  int id;

  XtRemoveInput(id_command);
  close(fd_command[0]);
  close(fd_command[1]);
  if((id = vfork()) == -1){
    ShowWarning("ivsd: Cannot fork ivs command");
    return;
  }
  if(id == 0){
    execlp(IvsCommand, IvsCommand, host_from, NULL);
  }else{
    if(pipe(fd_command) < 0){
      ShowWarning("ivsd: Cannot create command pipe");
      exit(1);
    }
    id_command = XtAppAddInput(app_con, fd_command[0], XtInputReadMask, 
			       TalkAnswer, NULL);
    return;
  }
}



static XtCallbackProc UnicastTalk(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Widget form, popup;
  int id;
  String hostname;
  unsigned int i1, i2, i3, i4;
  struct hostent *hp;
  char msg[200];
  Arg args[1];

  /* Get the hostname */
  XtSetArg(args[0], XtNstring, &hostname);
  XtGetValues(client_data, args, 1);  

  form = XtParent((Widget) client_data);
  popup = XtParent((Widget) form);
  XtDestroyWidget(popup);
  /* Is it a valid hostname ? */

  if(sscanf(hostname, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
    if((hp=gethostbyname(hostname)) == 0){
      strcpy(msg, "Invalid hostname : ");
      strcat(msg, hostname);
      ShowWarning(msg);
      return;
    }
  }
  strcpy(host_from, hostname);

  if((id = vfork()) == -1){
    ShowWarning("ivsd: Cannot fork ivs command");
    return;
  }
  if(id == 0){
    execlp(IvsCommand, IvsCommand, host_from, NULL);
  }else{
    return;
  }
}


static XtCallbackProc MulticastTalk(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Widget form, popup;
  int id, val;
  String group;
  String ttl_value;
  unsigned int i1, i2, i3, i4;
  char msg[200];
  Arg args[1];

  /* Get address group and TTL value */
  XtSetArg(args[0], XtNstring, &group);
  XtGetValues(text_group, args, 1);  
  XtSetArg(args[0], XtNstring, &ttl_value);
  XtGetValues(text_ttl, args, 1);  

  /* Destroy previous popup */
  form = XtParent((Widget) client_data);
  popup = XtParent((Widget) form);
  XtDestroyWidget(popup);

  /* Is it a good TTL value ? */
  val = atoi(ttl_value);
  if((val < 1) || (val > 255)){
    strcpy(msg, "Bad TTL value : ");
    strcat(msg, ttl_value);
    ShowWarning(msg);
    return;
  }
    
  /* Is it a valid multicast address ? */
  if(sscanf(group, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) == 4){
    if((i4>223) && (i4<240) && (i3<256) && (i2<256) && (i1<256)){
      /* It is a valid multicast address */

      if((id = vfork()) == -1){
	ShowWarning("ivsd: Cannot fork ivs command");
	return;
      }
      if(id == 0){
	execlp(IvsCommand, IvsCommand, "-I", group, "-T", ttl_value, NULL);
      }else{
	/* Save the address group and TTL value */
	strcpy(addr_group, group);
	strcpy(ttl, ttl_value);
	return;
      }
    }
  }
  /* It is a bad multicast address */
  strcpy(msg, "Invalid multicast address : ");
  strcat(msg, group);
  ShowWarning(msg);
  return;

}


static XtCallbackProc AcceptTalk(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Widget popup;
  Arg args[1];

  ALARM = FALSE;
  if(!SendTalkAccept(sock_cx))
    perror("ivs_daemon: send talk accepted");
  XtRemoveInput(id_cx);
  close(sock_cx);
  popup = XtParent((Widget) client_data);
  XtDestroyWidget(popup);
  XtSetArg(args[0], XtNbitmap, BDaemon_off);
  XtSetValues(daemon, args, 1);
  if(!SendPIP_FORK(fd_command)){
    ShowWarning("ivsd: Cannot send PIP_FORK command to pipe");
    exit(1);
  }
  return;
}
  


static XtCallbackProc RefuseTalk(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Widget popup;
  Arg args[1];

  ALARM = FALSE;
  if(!SendTalkRefuse(sock_cx))
    perror("ivs_daemon: send talk refused");
  XtRemoveInput(id_cx);
  close(sock_cx);
  popup = XtParent((Widget) client_data);
  XtDestroyWidget(popup);
  XtSetArg(args[0], XtNbitmap, BDaemon_off); 
  XtSetValues(daemon, args, 1);
}
  


static XtCallbackProc Cancel(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Widget popup;
  Arg args[1];

  popup = XtParent((Widget) client_data);
  XtDestroyWidget(popup);
}
  


static void ShowRequest(from)
     char *from;
{
  Arg         args[5];
  Widget      dialog, msg_from;
  Position    x, y;
  Dimension   width, height;
  Cardinal    n;
  char        data[100];


  n = 0;
  XtSetArg(args[0], XtNwidth, &width); n++;
  XtSetArg(args[1], XtNheight, &height); n++;
  XtGetValues(main_form, args, n);
  XtTranslateCoords(main_form, (Position) (width/2), (Position) (height/2),
		    &x, &y);

  n = 0;
  XtSetArg(args[n], XtNx, x); n++;
  XtSetArg(args[n], XtNy, y); n++;

  popup_request = XtCreatePopupShell("Request", transientShellWidgetClass, 
			     main_form, args, n);

  n = 0;
  strcpy(data, LabelTalk);
  strcat(data, from);
  XtSetArg(args[n], XtNlabel, data); n++;
  msg_from = XtCreateManagedWidget("MsgRequest", dialogWidgetClass, 
				   popup_request, args, n);

  XawDialogAddButton(msg_from, "Accept", AcceptTalk, (XtPointer) msg_from);

  XawDialogAddButton(msg_from, "Refuse", RefuseTalk, (XtPointer) msg_from);

  XtPopup(popup_request, XtGrabNone);
  XtRealizeWidget(popup_request);
}



static XtCallbackProc UnicastChoice(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Arg         args[5];
  Widget      popup, form, label_host, text_host, button_ok, button_cancel;
  Position    x, y;
  Dimension   width, height;
  Cardinal    n;
  XtTranslations trans_table;


  popup = XtParent((Widget) client_data);
  XtDestroyWidget(popup);

  trans_table = XtParseTranslationTable(defaultTranslations);

  n = 0;
  XtSetArg(args[0], XtNwidth, &width); n++;
  XtSetArg(args[1], XtNheight, &height); n++;
  XtGetValues(main_form, args, n);
  XtTranslateCoords(main_form, (Position) (width/2), (Position) (height/2),
		    &x, &y);

  n = 0;
  XtSetArg(args[n], XtNx, x); n++;
  XtSetArg(args[n], XtNy, y); n++;

  popup = XtCreatePopupShell("Selection", transientShellWidgetClass, 
			     main_form, args, n);


  form = XtCreateManagedWidget("Form", formWidgetClass, popup,
			       NULL, 0);

  label_host = XtCreateManagedWidget("Host", labelWidgetClass, 
				      form, NULL, 0);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_host); n++;
  XtSetArg(args[n], XtNstring, host_from); n++;
  text_host = XtCreateManagedWidget("TextHost", asciiTextWidgetClass, form, args, n);

  XtOverrideTranslations(text_host, trans_table);

  n = 0;
  XtSetArg(args[n], XtNfromVert, label_host); n++;
  button_ok = XtCreateManagedWidget("OK", commandWidgetClass,
				    form, args, n);
  XtAddCallback(button_ok, XtNcallback, UnicastTalk, (XtPointer) text_host);

  n = 0;
  XtSetArg(args[n], XtNfromVert, label_host); n++;
  XtSetArg(args[n], XtNfromHoriz, button_ok); n++;
  button_cancel = XtCreateManagedWidget("Cancel", commandWidgetClass,
					form, args, n);
  XtAddCallback(button_cancel, XtNcallback, Cancel, (XtPointer) form);
  
  XtPopup(popup, XtGrabNone);
  XtRealizeWidget(popup);
}



static XtCallbackProc MulticastChoice(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Arg         args[5];
  Widget      popup, form, label_group, label_ttl, button_ok, button_cancel;
  Position    x, y;
  Dimension   width, height;
  Cardinal    n;
  XtTranslations trans_table;


  popup = XtParent((Widget) client_data);
  XtDestroyWidget(popup);

  trans_table = XtParseTranslationTable(defaultTranslations);

  n = 0;
  XtSetArg(args[0], XtNwidth, &width); n++;
  XtSetArg(args[1], XtNheight, &height); n++;
  XtGetValues(main_form, args, n);
  XtTranslateCoords(main_form, (Position) (width/2), (Position) (height/2),
		    &x, &y);

  n = 0;
  XtSetArg(args[n], XtNx, x); n++;
  XtSetArg(args[n], XtNy, y); n++;

  popup = XtCreatePopupShell("Selection", transientShellWidgetClass, 
			     main_form, args, n);

  form = XtCreateManagedWidget("Form", formWidgetClass, popup,
			       NULL, 0);

  label_group = XtCreateManagedWidget("Group", labelWidgetClass, 
				      form, NULL, 0);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_group); n++;
  XtSetArg(args[n], XtNstring, addr_group); n++;
  text_group = XtCreateManagedWidget("TextGroup", asciiTextWidgetClass, form, 
				     args, n);

  XtOverrideTranslations(text_group, trans_table);

  n = 0;
  XtSetArg(args[n], XtNfromVert, label_group); n++;
  label_ttl = XtCreateManagedWidget("TTL", labelWidgetClass, 
				    form, args, n);

  n = 0;
  XtSetArg(args[n], XtNfromVert, label_group); n++;
  XtSetArg(args[n], XtNfromHoriz, label_ttl); n++;
  XtSetArg(args[n], XtNstring, ttl); n++;
  text_ttl = XtCreateManagedWidget("TextTTL", asciiTextWidgetClass, form, 
				   args, n);

  XtOverrideTranslations(text_ttl, trans_table);

  n = 0;
  XtSetArg(args[n], XtNfromVert, label_ttl); n++;
  button_ok = XtCreateManagedWidget("OK", commandWidgetClass,
				    form, args, n);
  XtAddCallback(button_ok, XtNcallback, MulticastTalk, (XtPointer) text_group);

  n = 0;
  XtSetArg(args[n], XtNfromVert, label_ttl); n++;
  XtSetArg(args[n], XtNfromHoriz, button_ok); n++;
  button_cancel = XtCreateManagedWidget("Cancel", commandWidgetClass,
					form, args, n);
  XtAddCallback(button_cancel, XtNcallback, Cancel, (XtPointer) form);
  
  XtPopup(popup, XtGrabNone);
  XtRealizeWidget(popup);
}



static XtCallbackProc ShowUMChoice(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Arg         args[5];
  Widget      popup, dialog, msg;
  Position    x, y;
  Dimension   width, height;
  Cardinal    n;
  char        data[100];


  n = 0;
  XtSetArg(args[0], XtNwidth, &width); n++;
  XtSetArg(args[1], XtNheight, &height); n++;
  XtGetValues(main_form, args, n);
  XtTranslateCoords(main_form, (Position) (width/2), (Position) (height/2),
		    &x, &y);


  n = 0;
  XtSetArg(args[n], XtNx, x); n++;
  XtSetArg(args[n], XtNy, y); n++;

  popup = XtCreatePopupShell("Selection", transientShellWidgetClass, 
			     main_form, args, n);

  n = 0;
  XtSetArg(args[n], XtNlabel, LabelUMChoice); n++;
  msg = XtCreateManagedWidget("MsgRequest", dialogWidgetClass, 
			      popup, args, n);

  XawDialogAddButton(msg, "Unicast call", UnicastChoice, (XtPointer) msg);

  XawDialogAddButton(msg, "Multicast call", MulticastChoice, (XtPointer) msg);

  XawDialogAddButton(msg, "Cancel", Cancel, (XtPointer) msg);

  XtPopup(popup, XtGrabNone);
  XtRealizeWidget(popup);
}



static void OneBeep()
{
    XBell(dpy, 100);
}


static void Beep()
{
  if(ALARM){
    OneBeep();
    XSync(dpy, True);
    alarm((u_int) DT_ALARM);
    signal(SIGALRM, Beep);
  }
}




static int CreateListenSocket(port_number)
     int port_number;
{
  struct sockaddr_in addr;
  char hostname[64];
  struct hostent *hp;
  int sock, len_addr;

  /* Create the listen socket */

  len_addr = sizeof(addr);
  gethostname(hostname, 64);
  if((hp=gethostbyname(hostname)) == 0){
    fprintf(stderr, "Unknown host %s\n", hostname);
    exit(0);
  }
  bzero((char *) &addr, len_addr);
  bcopy((char *)hp->h_addr_list[0], (char *) &addr.sin_addr, 4);
  addr.sin_family = AF_INET;
  addr.sin_port = htons((u_short)port_number);
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("ivs_daemon'socket");
    exit(-1);
  }
  if(bind(sock, (struct sockaddr *) &addr, len_addr) < 0){
    perror("ivs_daemon'bind");
    exit(-1);
  }
  if(listen(sock , 1) < 0){
    perror("ivs_daemon'listen");
    exit(-1);
  }
  return(sock);
}  




static XtCallbackProc ManageTalk(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  int lr;
  long timer;
  char data[10];
  char msg[150];
  Arg args[1];

  XtSetArg(args[0], XtNbitmap, BDaemon_on);
  XtSetValues(daemon, args, 1);
  OneBeep();
  XSync(dpy, True);
  alarm((u_int) DT_ALARM);
  signal(SIGALRM, Beep);
  ALARM = TRUE;
  
  bzero(data, 10);
  if((lr = read(sock_cx, data, 10)) <= 0){
    ShowWarning("ivs_daemon: read failed");
    close(sock_cx);
    return;
  }

  switch(data[0]){
  case CALL_REQUEST:
    strcpy(from, &data[1]);
    strcat(from, "@");
    strcat(from, host_from);
    ShowRequest(from);
    break;
  case CALL_ABORT:
    XtRemoveInput(id_cx);
    if(ALARM){
      time(&timer);
      ALARM = FALSE;
      close(sock_cx);
      XtDestroyWidget(popup_request);
      strcpy(msg, LabelTalkRequest);
      strcat(msg, from);
      strcat(msg, "\nAt : ");
      strcat(msg, ctime(&timer));
      ShowWarning(msg);
    }
    break;
  default:
    XtRemoveInput(id_cx);
    ShowWarning("ivs_daemon: Bad talk request format");
    close(sock_cx);
  }
}




static XtCallbackProc CreateSockCx(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  struct sockaddr_in addr_cx;
  int len_addr;
  struct hostent *host;
  int statusp;

  waitpid(0, &statusp, WNOHANG);
  len_addr = sizeof(addr_cx);
  bzero(&addr_cx, len_addr);
  if((sock_cx = accept(sock_listen, (struct sockaddr *)&addr_cx, 
		       &len_addr)) < 0){
    perror("ivs_daemon'accept");
    return;
  }
  if((host=gethostbyaddr((char *)(&(addr_cx.sin_addr)), 
			 sizeof(struct in_addr), addr_cx.sin_family)) ==NULL){
    strcpy(host_from, inet_ntoa(addr_cx.sin_addr));
  }else{
    strcpy(host_from, host->h_name);
  }
  id_cx = XtAppAddInput(app_con, sock_cx, XtInputReadMask, ManageTalk, NULL);
}  



static void DestroyZombies()
{
  int statusp;

  waitpid(0, &statusp, WNOHANG);
}



static void Quit()
{
  XtDestroyApplicationContext(app_con);
  close(sock_listen);
  exit(0);
}




int main(argc, argv)
     int argc;
     char **argv;
{
  int n;
  Arg args[2];

  sock_listen = CreateListenSocket(IVSD_PORT);

  dpy = XOpenDisplay(0);

  toplevel = XtAppInitialize(&app_con, "IVSD", NULL, 0, &argc, argv, 
			     fallback_resources, NULL, 0);

  main_form = XtCreateManagedWidget("Form", formWidgetClass, toplevel,
				    NULL, 0);

  BDaemon_on = XCreateBitmapFromData(XtDisplay(toplevel),
				     RootWindowOfScreen(XtScreen(toplevel)),
				     BDaemon_on_bits, BDaemon_on_width, 
				     BDaemon_on_height);

  BDaemon_off = XCreateBitmapFromData(XtDisplay(toplevel),
				      RootWindowOfScreen(XtScreen(toplevel)),
				      BDaemon_off_bits, BDaemon_off_width, 
				      BDaemon_off_height);

  n = 0;
  XtSetArg(args[n], XtNbitmap, BDaemon_off); n++;
  daemon = XtCreateManagedWidget("Daemon", commandWidgetClass, main_form, 
				 args, n);
  XtAddCallback(daemon, XtNcallback, ShowUMChoice, NULL);
  if(pipe(fd_command) < 0){
    perror("ivsd'pipe");
    exit(1);
  }

  strcpy(addr_group, MULTI_IP_GROUP);
  gethostname(host_from, 100);
  sprintf(ttl, "%d", DEFAULT_TTL);

  signal(SIGCHLD, DestroyZombies);
  signal(SIGINT, Quit);
  signal(SIGQUIT, Quit);

  XtAppAddInput(app_con, sock_listen, XtInputReadMask, CreateSockCx, NULL);
  id_command = XtAppAddInput(app_con, fd_command[0], XtInputReadMask, 
			     TalkAnswer, NULL);
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app_con);

}


