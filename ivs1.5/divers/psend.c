#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "in.h"

main()
{
  int sock, len_addr;
  unsigned char ttl=0x02;
  struct sockaddr_in addr;

  sock=CreateSendSocket(&addr, &len_addr, "2222", "224.2.2.2", &ttl);
  SendDeclarePacket(sock, &addr, len_addr, 0, 0, "tito");
  SendDeclarePacket(sock, &addr, len_addr, 1, 0, "tata");
/*   SendAbortPacket(sock, &addr, len_addr);*/
  SendAskInfoPacket(sock, &addr, len_addr);
}
