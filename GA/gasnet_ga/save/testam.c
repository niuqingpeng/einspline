/*   $Source: /var/local/cvs/gasnet/tests/testam.c,v $
 *     $Date: 2013/03/08 09:41:08 $
 * $Revision: 1.38 $
 * Description: GASNet Active Messages performance test
 * Copyright 2002, Dan Bonachea <bonachea@cs.berkeley.edu>
 * Terms of use are as specified in license.txt
 */

#include <gasnet.h>
uintptr_t maxsz = 0;
#ifndef TEST_SEGSZ
  #define TEST_SEGSZ_EXPR ((uintptr_t)maxsz)
#endif
#include <test.h>

int mynode = 0;
int numnode = 0;
void *myseg = NULL;
int sender, recvr;
int peer;
void *peerseg = NULL;

void report(const char *desc, int64_t totaltime, int iters, uintptr_t sz, int rt) {
  if (sender) {
      char nodestr[10];
      if (numnode > 2) snprintf(nodestr,sizeof(nodestr),"%i: ",mynode);
      else nodestr[0] = '\0';
      printf("%c: %s%-46s: %6.3f sec %8.3f us", TEST_SECTION_NAME(),
        nodestr, desc, ((double)totaltime)/1000000, ((double)totaltime)/iters);
      if (sz) printf("  %7.3f MB/s", 
        (((double)sz)*(rt?2:1)*iters/(1024*1024)) / (((double)totaltime)/1000000));
      printf("\n");
      fflush(stdout);
  }
}

gasnet_hsl_t inchsl = GASNET_HSL_INITIALIZER;
#define INC(var) do {           \
    gasnet_hsl_lock(&inchsl);   \
    var++;                      \
    gasnet_hsl_unlock(&inchsl); \
  } while (0)

/* ------------------------------------------------------------------------------------ */
#define hidx_ping_shorthandler   201
#define hidx_pong_shorthandler   202

#define hidx_ping_medhandler     203
#define hidx_pong_medhandler     204

#define hidx_ping_longhandler    205
#define hidx_pong_longhandler    206

#define hidx_ping_shorthandler_flood   207
#define hidx_pong_shorthandler_flood   208

#define hidx_ping_medhandler_flood     209
#define hidx_pong_medhandler_flood     210

#define hidx_ping_longhandler_flood    211
#define hidx_pong_longhandler_flood    212

#define hidx_done_shorthandler   213

volatile int flag = 0;
volatile int reply_node = -1;
void ping_shorthandler(gasnet_token_t token) {
  GASNET_Safe(gasnet_AMReplyShort1(token, hidx_pong_shorthandler, mynode));
}
void pong_shorthandler(gasnet_token_t token, gasnet_handlerarg_t arg0) {
  reply_node = arg0;
  flag++;
}


void ping_medhandler(gasnet_token_t token, void *buf, size_t nbytes) {
  GASNET_Safe(gasnet_AMReplyMedium0(token, hidx_pong_medhandler, buf, nbytes));
}
void pong_medhandler(gasnet_token_t token, void *buf, size_t nbytes) {
  flag++;
}


void ping_longhandler(gasnet_token_t token, void *buf, size_t nbytes) {
  GASNET_Safe(gasnet_AMReplyLong0(token, hidx_pong_longhandler, buf, nbytes, peerseg));
}

void pong_longhandler(gasnet_token_t token, void *buf, size_t nbytes) {
  flag++;
}
/* ------------------------------------------------------------------------------------ */
void ping_shorthandler_flood(gasnet_token_t token) {
  GASNET_Safe(gasnet_AMReplyShort0(token, hidx_pong_shorthandler_flood));
}
void pong_shorthandler_flood(gasnet_token_t token) {
  INC(flag);
}


void ping_medhandler_flood(gasnet_token_t token, void *buf, size_t nbytes) {
  GASNET_Safe(gasnet_AMReplyMedium0(token, hidx_pong_medhandler_flood, buf, nbytes));
}
void pong_medhandler_flood(gasnet_token_t token, void *buf, size_t nbytes) {
  INC(flag);
}


void ping_longhandler_flood(gasnet_token_t token, void *buf, size_t nbytes) {
  GASNET_Safe(gasnet_AMReplyLong0(token, hidx_pong_longhandler_flood, buf, nbytes, peerseg));
}

void pong_longhandler_flood(gasnet_token_t token, void *buf, size_t nbytes) {
  INC(flag);
}


volatile int done = 0;
void done_shorthandler(gasnet_token_t token) {
  done = 1;
}

/* ------------------------------------------------------------------------------------ */
int crossmachinemode = 0;
int insegment = 1;
int iters=0;
int pollers=0;
int i = 0;
uintptr_t maxmed, maxlongreq, maxlongrep;
void *doAll(void*);

/* ------------------------------------------------------------------------------------ */
void doAMShort(void) {
    GASNET_BEGIN_FUNCTION();
    printf("%d: sender = %d\n", mynode, sender);
    if (sender) { /* warm-up */
      flag = 0;                                                                                  
      for (i=0; i < iters; i++) {
        GASNET_Safe(gasnet_AMRequestShort0(peer, hidx_ping_shorthandler_flood));
      }
      GASNET_BLOCKUNTIL(flag == iters);
      GASNET_Safe(gasnet_AMRequestShort0(peer, hidx_ping_shorthandler));
      GASNET_BLOCKUNTIL(flag == iters+1);
    }
    BARRIER();
    /* ------------------------------------------------------------------------------------ */
    if (TEST_SECTION_BEGIN_ENABLED() && sender) {
      int64_t start = TIME();
      flag = -1;
      for (i=0; i < iters; i++) {
        GASNET_Safe(gasnet_AMRequestShort0(peer, hidx_ping_shorthandler));
        GASNET_BLOCKUNTIL(flag == i);
      }
      report("        AMShort     ping-pong roundtrip ReqRep",TIME() - start, iters, 0, 1);
    }
    BARRIER();
    printf("%d: reply_node = %d", mynode, reply_node);
}
int main(int argc, char **argv) {
  int help=0;
  int arg=1;

  gasnet_handlerentry_t htable[] = { 
    { hidx_ping_shorthandler,  ping_shorthandler  },
    { hidx_pong_shorthandler,  pong_shorthandler  },
    { hidx_ping_medhandler,    ping_medhandler    },
    { hidx_pong_medhandler,    pong_medhandler    },
    { hidx_ping_longhandler,   ping_longhandler   },
    { hidx_pong_longhandler,   pong_longhandler   },

    { hidx_ping_shorthandler_flood,  ping_shorthandler_flood  },
    { hidx_pong_shorthandler_flood,  pong_shorthandler_flood  },
    { hidx_ping_medhandler_flood,    ping_medhandler_flood    },
    { hidx_pong_medhandler_flood,    pong_medhandler_flood    },
    { hidx_ping_longhandler_flood,   ping_longhandler_flood   },
    { hidx_pong_longhandler_flood,   pong_longhandler_flood   },

    { hidx_done_shorthandler,  done_shorthandler  }
  };

  GASNET_Safe(gasnet_init(&argc, &argv));

  mynode = gasnet_mynode();
  numnode = gasnet_nodes();
  arg = 1;
  printf("argc = %d arg = %d\n", argc, arg);
  if (!iters) iters = 1000;
  if (!maxsz) maxsz = 2*1024*1024;

  GASNET_Safe(gasnet_attach(htable, sizeof(htable)/sizeof(gasnet_handlerentry_t),
                            TEST_SEGSZ_REQUEST, TEST_MINHEAPOFFSET));
  TEST_PRINT_CONDUITINFO();

  if (insegment) {
    myseg = TEST_MYSEG();
  } else {
    char *space = test_malloc(alignup(maxsz,PAGESZ) + PAGESZ);
    myseg = alignup_ptr(space, PAGESZ);
  }

  maxmed = MIN(maxsz, gasnet_AMMaxMedium());
  maxlongreq = MIN(maxsz, gasnet_AMMaxLongRequest());
  maxlongrep = MIN(maxsz, gasnet_AMMaxLongReply());
  printf("iters = %d maxsz = %d crossmachinemode = %d\n", 
          iters, maxsz, crossmachinemode);

  peer = mynode ^ 1;
  sender = mynode % 2 == 0;
  if (peer == numnode) {
      peer = mynode;
  }

  recvr = !sender || (peer == mynode);

  peerseg = TEST_SEG(peer);

  BARRIER();

  if (mynode == 0) {
      printf("Running %sAM performance test with %i iterations"
             "...\n",
             (crossmachinemode ? "cross-machine ": ""),
             iters
            );
      printf("   Msg Sz  Description                             Total time   Avg. time   Bandwidth\n"
             "   ------  -----------                             ----------   ---------   ---------\n");
      fflush(stdout);
  }
  doAMShort();
  if (recvr) GASNET_Safe(gasnet_AMRequestShort0(mynode, hidx_done_shorthandler));
  MSG("done.");

  gasnet_exit(0);
  return 0;
}

