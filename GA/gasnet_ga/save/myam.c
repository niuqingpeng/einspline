/*   $Source: /var/local/cvs/gasnet/tests/testam.c,v $
 *     $Date: 2013/03/08 09:41:08 $
 * $Revision: 1.38 $
 * Description: GASNet Active Messages performance test
 * Copyright 2002, Dan Bonachea <bonachea@cs.berkeley.edu>
 * Terms of use are as specified in license.txt
 */

#include <gasnet.h>
#include <test.h>

int mynode = 0;
int numnode = 0;
int sender, recvr;
int peer;

#define hidx_ping_shorthandler   201
#define hidx_pong_shorthandler   202
#define hidx_done_shorthandler   213

volatile int flag = 0;
volatile int reply_node = -1;
void ping_shorthandler(gasnet_token_t token) {
  GASNET_Safe(gasnet_AMReplyShort1(token, hidx_pong_shorthandler, mynode));
}

void pong_shorthandler(gasnet_token_t token, gasnet_handlerarg_t arg0) {
  reply_node = arg0;
  flag=1;
}


volatile int done = 0;
void done_shorthandler(gasnet_token_t token) {
  done = 1;
}


void doAMShort(void) {
    printf("%d: sender = %d\n", mynode, sender);
    BARRIER();
    if (sender) {
      int64_t start = TIME();
      flag = -1;
        GASNET_Safe(gasnet_AMRequestShort0(peer, hidx_ping_shorthandler));
        GASNET_BLOCKUNTIL(flag == 1);
    }
    BARRIER();
    printf("%d: reply_node = %d", mynode, reply_node);
}

int main(int argc, char **argv) {
  gasnet_handlerentry_t htable[] = {
    { hidx_ping_shorthandler,  ping_shorthandler  },
    { hidx_pong_shorthandler,  pong_shorthandler  },
    { hidx_done_shorthandler,  done_shorthandler  }
  };

  GASNET_Safe(gasnet_init(&argc, &argv));

  mynode = gasnet_mynode();
  numnode = gasnet_nodes();

  GASNET_Safe(gasnet_attach(htable, sizeof(htable)/sizeof(gasnet_handlerentry_t),
                            TEST_SEGSZ_REQUEST, TEST_MINHEAPOFFSET));

  peer = mynode ^ 1;
  sender = mynode % 2 == 0;
  if (peer == numnode) {
      peer = mynode;
  }

  recvr = !sender || (peer == mynode);

  BARRIER();

  doAMShort();
  if (recvr) GASNET_Safe(gasnet_AMRequestShort0(mynode, hidx_done_shorthandler));
  MSG("done.");

  gasnet_exit(0);
  return 0;
}

