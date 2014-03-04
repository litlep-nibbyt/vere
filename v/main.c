/* v/main.c
**
** This file is in the public domain.
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <gmp.h>
#include <stdint.h>
#include <uv.h>
#include <sigsegv.h>
#include <curses.h>
#include <termios.h>
#include <term.h>
#include <dirent.h>

#define U2_GLOBAL
#define C3_GLOBAL
#include "all.h"
#include "v/vere.h"

/**  Legacy fixed jet linkage.  Destroy me please.
**/
    /* External drivers.
    */
      extern u2_ho_driver j2_da(k_164);

    /* Built-in battery drivers.   Null `cos` terminates.
    */
      u2_ho_driver *HostDriverBase[] = {
        &j2_k_164_d,
        0
      };

/* _main_readw(): parse a word from a string.
*/
static u2_bean
_main_readw(const c3_c* str_c, c3_w max_w, c3_w* out_w)
{
  c3_w par_w = atol(str_c);

  if ( par_w > 0 && par_w < max_w ) {
    *out_w = par_w;
    return u2_yes;
  }
  else return u2_no;
}

/* _main_getopt(): extract option map from command line.
*/
static u2_bean
_main_getopt(c3_i argc, c3_c** argv)
{
  c3_i ch_i;

  u2_Host.ops_u.abo = u2_no;
  u2_Host.ops_u.bat = u2_no;
  u2_Host.ops_u.gab = u2_no;
  u2_Host.ops_u.loh = u2_no;
  u2_Host.ops_u.dem = u2_no;
  u2_Host.ops_u.pro = u2_no;
  u2_Host.ops_u.veb = u2_yes;
  u2_Host.ops_u.rez = u2_no;
  u2_Host.ops_u.nuu = u2_no;
  u2_Host.ops_u.vno = u2_no;
  u2_Host.ops_u.kno_w = DefaultKernel;

  while ( (ch_i = getopt(argc, argv, "I:f:h:k:l:n:p:r:Labcdgqv")) != -1 ) {
    switch ( ch_i ) {
      case 'I': {
        u2_Host.ops_u.imp_c = strdup(optarg);
        break;
      }
      case 'f': {
        if ( u2_no == _main_readw(optarg, 100, &u2_Host.ops_u.fuz_w) ) {
          return u2_no;
        }
        break;
      }
      case 'h': {
        u2_Host.ops_u.hom_c = strdup(optarg);
        break;
      }
      case 'k': {
        if ( u2_no == _main_readw(optarg, 256, &u2_Host.ops_u.kno_w) ) {
          return u2_no;
        }
        break;
      }
      case 'l': {
        c3_w arg_w;

        if ( u2_yes == _main_readw(optarg, 65536, &arg_w) ) {
          u2_Host.ops_u.rop_u.por_s = arg_w;
        } else return u2_no;
        break;
      }
      case 'n': {
        u2_Host.ops_u.nam_c = strdup(optarg);
        break;
      }
      case 'p': {
        c3_w arg_w;

        if ( u2_yes == _main_readw(optarg, 65536, &arg_w) ) {
          u2_Host.ops_u.por_s = arg_w;
        }
        else return u2_no;
        break;
      }
      case 'r': {
        if ( u2_no == u2_raft_readopt(&u2_Host.ops_u.rop_u, optarg) ) {
          return u2_no;
        }
        break;
      }
      case 'L': { u2_Host.ops_u.loh = u2_yes; break; }
      case 'a': { u2_Host.ops_u.abo = u2_yes; break; }
      case 'b': { u2_Host.ops_u.bat = u2_yes; break; }
      case 'c': { u2_Host.ops_u.nuu = u2_yes; break; }
      case 'd': { u2_Host.ops_u.dem = u2_yes; break; }
      case 'g': { u2_Host.ops_u.gab = u2_yes; break; }
      case 'q': { u2_Host.ops_u.veb = u2_no; break; }
      case 'v': { u2_Host.ops_u.veb = u2_yes; break; }
      case 'V': { u2_Host.ops_u.vno = u2_yes; break; }
      case '?': default: {
        return u2_no;
      }
    }
  }

  if ( (u2_Host.ops_u.rop_u.por_s == 0 && u2_Host.ops_u.rop_u.nam_u != 0) ) {
    fprintf(stderr, "The -r flag requires -l.\n");
    return u2_no;
  }

  if ( u2_yes == u2_Host.ops_u.bat ) {
    u2_Host.ops_u.dem = u2_yes;
    u2_Host.ops_u.nuu = u2_yes;
  }

  if ( u2_Host.ops_u.hom_c == 0 ) {
    u2_Host.ops_u.hom_c = getenv("URBIT_HOME");

    if ( u2_Host.ops_u.hom_c == 0 ) {
      fprintf(stderr, "$URBIT_HOME not set, falling back to $HOME/urbit\n");
      c3_c* hom_c = getenv("HOME");
      c3_w hom_w = strlen(hom_c) + 6;

      if ( !hom_c ) {
        fprintf(stderr, "$URBIT_HOME or $HOME must be set\n");
        exit(1);
      } else {
        u2_Host.ops_u.hom_c = malloc(hom_w + 1);
        snprintf(u2_Host.ops_u.hom_c, hom_w + 1, "%s/urbit", hom_c);
      }
    }
    {
      DIR* rid_u;

      if ( 0 == (rid_u = opendir(u2_Host.ops_u.hom_c)) ) {
        if ( 0 != mkdir(u2_Host.ops_u.hom_c, 0755) ) {
          perror(u2_Host.ops_u.hom_c);
          exit(1);
        }
      }
      else closedir(rid_u);
    }
  }
  printf("vere: urbit home is %s\n", u2_Host.ops_u.hom_c);

  if ( u2_Host.ops_u.nam_c == 0 ) {
    u2_Host.ops_u.nam_c = getenv("HOSTNAME");
    if ( u2_Host.ops_u.nam_c == 0 ) {
      c3_w len_w = sysconf(_SC_HOST_NAME_MAX) + 1;

      u2_Host.ops_u.nam_c = malloc(len_w);
      if ( 0 != gethostname(u2_Host.ops_u.nam_c, len_w) ) {
        perror("gethostname");
        exit(1);
      }
    }
  }
  printf("vere: hostname is %s\n", u2_Host.ops_u.nam_c);

  if ( argc != (optind + 1) ) {
    return u2_no;
  } else {
    {
      c3_c* ash_c;

      if ( (ash_c = strrchr(argv[optind], '/')) && (ash_c[1] == 0) ) {
        *ash_c = 0;
      }
    }

    u2_Host.ops_u.cpu_c = strdup(argv[optind]);
    return u2_yes;
  }
}

/* u2_ve_usage(): print usage and exit.
*/
static void
u2_ve_usage(c3_i argc, c3_c** argv)
{
  fprintf(stderr, "%s: usage: [-v] [-k stage] [-p ames_port] computer\n", argv[0]);
  exit(1);
}

/* u2_ve_panic(): panic and exit.
*/
static void
u2_ve_panic(c3_i argc, c3_c** argv)
{
  fprintf(stderr, "%s: gross system failure\n", argv[0]);
  exit(1);
}

/* u2_ve_sysopt(): apply option map to system state.
*/
static void
u2_ve_sysopt()
{
  {
    u2_Local = strdup(u2_Host.ops_u.cpu_c);
  }
  u2_System = U2_LIB;

  u2_Flag_Abort = u2_Host.ops_u.abo;
  u2_Flag_Garbage = u2_Host.ops_u.gab;
  u2_Flag_Profile = u2_Host.ops_u.pro;
  u2_Flag_Verbose = u2_Host.ops_u.veb;
}

static jmp_buf Signal_buf;
#ifndef SIGSTKSZ
# define SIGSTKSZ 16384
#endif
static uint8_t Sigstk[SIGSTKSZ];

volatile enum { sig_none, sig_overflow, sig_interrupt } Sigcause;

static void
overflow_handler(int emergency, stackoverflow_context_t scp)
{
  if ( 1 == emergency ) {
    write(2, "stack emergency\n", strlen("stack emergency" + 2));
    exit(1);
  } else {
    Sigcause = sig_overflow;
    longjmp(Signal_buf, 1);
  }
}

static void
interrupt_handler(int x)
{
  Sigcause = sig_interrupt;
  longjmp(Signal_buf, 1);
}

c3_i
main(c3_i   argc,
     c3_c** argv)
{
  c3_w kno_w;

  //  Parse options.
  //
  if ( u2_no == _main_getopt(argc, argv) ) {
    u2_ve_usage(argc, argv);
    return 1;
  }
  u2_ve_sysopt();

  if ( u2_yes == u2_Host.ops_u.dem && u2_no == u2_Host.ops_u.bat ) {
    printf("Starting daemon\n");
  }

  //  Seed prng. Don't panic -- just for fuzz testing and election timeouts.
  //
  srand(getpid());

  //  Instantiate process globals.
  {
    u2_wr_check_init(u2_Host.ops_u.cpu_c);

    if ( (u2_no == u2_Host.ops_u.nuu) &&
          (u2_no == u2_Host.ops_u.rez) &&
          (u2_yes == u2_loom_load()) )
    {
      u2_Host.wir_r = u2_ray_of(0, 0);
      u2_Wire = u2_Host.wir_r;

      u2_Host.cpu_c = u2_Host.ops_u.cpu_c;
      u2_Host.arv_u = u2_Arv;

      u2_Arv->ova.egg_u = u2_Arv->ova.geg_u = 0;

      u2_lo_grab("init", u2_none);

      //  Horrible ancient stuff.
      //
      kno_w = u2_Host.arv_u->kno_w;
      u2_Host.kno_w = kno_w;

      u2_ho_push();
    }
    else {
      u2_loom_boot();
      u2_Host.wir_r = u2_wr_init(c3__rock, u2_ray_of(0, 0), u2_ray_of(1, 0));
      u2_Wire = u2_Host.wir_r;

      u2_Host.cpu_c = u2_Host.ops_u.cpu_c;
      u2_Host.arv_u = u2_Arv;
    }
  }

  //  If we have not loaded from checkpoint, build kernel.
  //
  if ( 0 != u2_Host.arv_u->ent_w ) {
    u2_reck_time(u2_Host.arv_u);
    u2_reck_numb(u2_Host.arv_u);
    {
      c3_c* dyt_c = u2_cr_string(u2_Host.arv_u->wen);

      printf("time: %s\n", dyt_c);
      free(dyt_c);
    }
  }
  else {
    //  Set outside bail trap.  Should not be used, but you never know...
    //
    if ( 0 != u2_cm_trap() ) {
      u2_ve_panic(argc, argv);
    }
    else {
      //  Set boot and goal stages.
      {
        if ( (0 == u2_Host.ops_u.kno_w) || (u2_Host.ops_u.kno_w > 255) ) {
          kno_w = DefaultKernel;
        } else {
          kno_w = u2_Host.ops_u.kno_w;
        }
      }

      //  Load the system.
      //
      {
        u2_Host.kno_w = u2_Host.ops_u.kno_w;

        u2_reck_boot(u2_Host.arv_u);
      }
      u2_cm_done();
    }
  }

  //  Install signal handlers and set buffers.
  //
  //  Note that we use the sigmask-restoring variant.  Essentially, when
  //  we get a signal, we force the system back into the just-booted state.
  //  If anything goes wrong during boot (above), it's curtains.
  {
    if ( 0 != setjmp(Signal_buf) ) {
      switch ( Sigcause ) {
        case sig_overflow: printf("[stack overflow]\r\n"); break;
        case sig_interrupt: printf("[interrupt]\r\n"); break;
        default: printf("[signal error!]\r\n"); break;
      }
      Sigcause = sig_none;

      signal(SIGINT, SIG_DFL);
      stackoverflow_deinstall_handler();

      //  Print the trace, do a GC, etc.
      //
      //  This is half-assed at present, so we exit.
      //
      u2_lo_sway(0, u2k(u2_wire_tax(u2_Wire)));

      u2_lo_bail(u2_Host.arv_u);

      exit(1);
    }
#if 1
    if ( -1 == stackoverflow_install_handler
        (overflow_handler, Sigstk, SIGSTKSZ) )
    {
      fprintf(stderr, "overflow_handler: install failed\n");
      exit(1);
    }
    signal(SIGINT, interrupt_handler);
    signal(SIGIO, SIG_IGN);
#endif
  }

  u2_lo_grab("main", u2_none);

  u2_lo_boot();

  if ( u2_no == u2_Host.ops_u.bat ) {
    u2_lo_loop(u2_Host.arv_u);
  }
  return 0;
}
