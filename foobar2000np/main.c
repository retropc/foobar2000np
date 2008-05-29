#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock.h>
#include <stdlib.h>
#include <stdio.h>

static char PLAYING[1024], NOTPLAYING[1024];

#define DEFAULT_FOOBARVERSION "v0.9.5.2"

#define LISTEN_PORT 4014
#define LISTEN_ADDR inet_addr("127.0.0.1") /*htonl(INADDR_ANY)*/

#define FATAL(x) { MessageBoxA(0, x, "Foobar2000NowPlaying", 16); WSACleanup(); exit(0); }

#define PLAYING_LEN strlen(PLAYING)

void setupscanners(char *version) {
  sprintf_s(PLAYING, sizeof(PLAYING), "[foobar2000 %s]", version);
  sprintf_s(NOTPLAYING, sizeof(NOTPLAYING), "foobar2000 %s", version);
}

void startup(void) {
  WSADATA wsaData;
  if(WSAStartup(MAKEWORD(2, 2), &wsaData))
    FATAL("Unable to startup Winsock.");
}

SOCKET sockbind(int port, long addr) {
  struct sockaddr_in listenaddr;
  
  SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
  if(s == INVALID_SOCKET)
    FATAL("Unable to get listening socket.");
  
  memset(&listenaddr, 0, sizeof(listenaddr));
  
  listenaddr.sin_family = AF_INET;
  listenaddr.sin_port = htons(port);
  listenaddr.sin_addr.s_addr = addr;
  
  if(bind(s, (struct sockaddr *)&listenaddr, sizeof(listenaddr)) < 0) {
    closesocket(s);
    FATAL("Unable to bind socket.");
  }
  
  if(listen(s, 5) < 0) {
    closesocket(s);
    FATAL("Unable to listen on socket.");
  }
  return s;
}

int checknotplaying(char *title) {
  return !strcmp(title, NOTPLAYING);
}

int checkplaying(char *title) {
  size_t len = strlen(title);
  if((len > PLAYING_LEN) && !strcmp(title + len - PLAYING_LEN, PLAYING))
    return 1;
    
  return 0;
}

BOOL __stdcall enumcallback(HANDLE h, LPARAM wb) {
  HANDLE *h2 = (HANDLE *)wb;
  char buf[1024];
  buf[sizeof(buf) - 1] = '\0';
  
  if(GetWindowTextA(h, buf, sizeof(buf) - 1)) {
    if(checknotplaying(buf) || checkplaying(buf)) {
      *h2 = h;
      return 0;
    }
  }
  
  return 1;
}

char *getplaystring(void) {
  HANDLE h = INVALID_HANDLE_VALUE;
  static char buf[1024];
  
  EnumWindows(enumcallback, (LPARAM)&h);
  
  if((h != INVALID_HANDLE_VALUE) && GetWindowTextA(h, buf, sizeof(buf) - 1)) {
    if(checknotplaying(buf))
      return "not playing";
  
    if(checkplaying(buf)) {
      int pos = (int)(strlen(buf) - PLAYING_LEN);
      buf[pos] = '\0';
      
      for(;pos >= 0;pos--) {
        if(buf[pos] == ' ') {
          buf[pos] = '\0';
        } else {
          break;
        }
      }
      return buf;
    }
  }
  
  return "unknown";
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  SOCKET s;
  struct sockaddr_in peer;
  int peerlen;
  char *playstring;
  
  if(!lpCmdLine || !lpCmdLine[0]) {
    setupscanners(DEFAULT_FOOBARVERSION);
  } else {
    setupscanners(lpCmdLine);
  }
  
  startup();
  
  s = sockbind(LISTEN_PORT, LISTEN_ADDR);
  peerlen = sizeof(peer);
  memset(&peer, 0, sizeof(peer));
  
  for(;;) {
    SOCKET s2 = accept(s, (struct sockaddr *)&peer, &peerlen);
    if(s2 == INVALID_SOCKET) {
      closesocket(s2);
      continue;
    }
    
    playstring = getplaystring();

    send(s2, playstring, (int)strlen(playstring), 0);
    send(s2, "\r\n", 2, 0);
    closesocket(s2);
  }
  
  WSACleanup();
  return 0;
}
