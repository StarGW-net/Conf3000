/* slave writes this packet */
struct slavepk {
  int id;
  int action;
  int number;
  char text[81];
};

/* server writes this packet */
struct serverpk {
  int status;
};

#define CONNECT 1
#define CHILDPID 2
#define LOGOUT 3
#define CHANGECHAN 4
#define LOCK 5
#define OPEN 6
#define FIND 7
#define NETFOLK 8
#define CHANGEALIAS 9
#define GAG 10
#define KILL 11
#define SHUTDOWN 12
#define RECONNECT 14
#define WHATCHAN 15
#define BAN 16
#define HIT 17
#define FLIPCHAN 18

