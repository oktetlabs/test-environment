/* $CHeader: exec.c 1.109 1994/03/24 11:04:17 $ */
/*
 *              Copyright 1992, CONVEX Computer Corporation.
 * This document is copyrighted.  All rights are reserved.  This document
 * may not, in whole or part, be copied, duplicated, reproduced, translated,
 * electronically stored or reduced to machine readable form without prior
 * written consent from CONVEX Computer Corporation.
 */

#include <setjmp.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#include <time.h>
#ifdef __convexc__
#include <sys/sysinfo.h>
#endif
#ifdef hpux
#include <sys/syscall.h>
#define getrusage(a, b)  syscall(SYS_GETRUSAGE, a, b)
#endif
#include "td.h"
#include "statistics.h"
#include "syserr.h"

extern FILE *TETout;

#define MASK(sig)  (01 << ((sig) - 1))
/*-------------------------------------------------------------------------*/
#define MAXKEYS    4096      /* hard-wired maximum number of keys/testcase */

static DB_key KEYS[MAXKEYS];       /* array of keys to be written   */
static int    lastkey = 0;
static char *ind=NULL;
PUBLIC  void wrap_exec_stmtlist(char *tn, TD_stmt *stmt);
static void exec_stmtlist(char *tn, TD_stmt *stmt);
static void update_environ(TD_stmt *stmt);
static int env_builtin( TD_arg *arglist );
static void exec_exception_stmts(char *tn, TD_stmt *stmt);
static void tet_exec_stmtlist (char *tn, TD_stmt *stmt, int indent) ;
static void tet_exec_prog (struct progstmt *psp, int indent);
static void tet_exec_getenv( struct getenvstmt *ssp, int indent );
static void tet_exec_setenv(struct setenvstmt *ssp, int indent);
static void tet_exec_unsetenv(struct unsetenvstmt *usp, int indent);
static void tet_exec_clearenv(struct clearenvstmt *csp, int indent);
char *tet_argref(TD_arg *arg, char *strquot);

PUBLIC void setenvvars( struct symbol *sp );
static void exec_stmt(char *tn, TD_stmt *stmt, 
		      void (*exec_list)(char *, TD_stmt *));
void st_setfailure(ST_symbol *st, char *value);
void eval_intersect (struct intersectstmt *inp);
void checkresources(int what);
void st_setfailure(ST_symbol *st, char *value);
void statProg(struct progstmt *psp, int result, struct timeval *tm, 
	      struct rusage *ru);
void time_except(struct progstmt *psp, char *key, int timelimit);
void statSig(char *sig);
void initStats(char *db);
char *st_name(ST_symbol *st);
char *st_value(ST_symbol *st);
void exec_toprog(struct progstmt *psp, char *key, int timelimit);
uid_t cvt_uid(char *uid, char *p_descrip );
gid_t cvt_gid(char *gid, char *p_descrip );
void tet_eval_strsub (struct strsubstmt *ssp, FILE *fptr, int indent);
void tet_eval_intersect (struct intersectstmt *inp, FILE *fptr, int indent);
void tet_eval_system (struct systemstmt *ssp, int indent);
void tet_eval_strcat (struct strcatstmt *scp, FILE *fptr, int indent);
void tet_eval_assign (struct assignstmt *scp, FILE *fptr, int indent);
void tet_add_quit_code(TD_att att, char *ind);

PUBLIC int DEBUG_CHILD = 0;

extern int prog_run_no;

IMPORT char *traverse_stack(char *testname);
IMPORT void add_prog_wallclock(struct progstmt *psp, struct timeval *wctime);
IMPORT void add_prog_cpuclock(struct progstmt *psp, double cctime);
IMPORT int get_timeout(int deflt, const char *symbol);
IMPORT char *eval_builtin(TD_arg *, int);
IMPORT void ex_result(char *cause, char *act, char *exp, char *diff);
IMPORT void ex_proginfo(TD_progstmt *psp);
IMPORT int get_signal(int, char *);

/*-------------------------------------------------------------------------*/
     
char           *Xtest;          /* name of currently executing test */
int             Sig;            /* either SIGALRM or SIGCHLD */
int             Sig_caught=0;
unsigned        Timeleft;       /* for alarm */
int             Cpulimit;
IMPORT uid_t    Childuid;
IMPORT gid_t    Childgid;
IMPORT int      Noncriterr;
IMPORT jmp_buf  Knownfail;
IMPORT struct symbol *Xst;
IMPORT DB_testcase *Xent;
IMPORT DB       *Xdb;
IMPORT int      errno;
IMPORT char     *old_fail_reason;
extern char     Fail_reason[];

IMPORT int      progsrun;  /* count of prog's actually executed */
IMPORT int      reruntest; /* set to 1 if test should be rerun */

char *makcmdline ();

static void exec_prog(struct progstmt *psp, char *key);
static void noexec_prog(struct progstmt *psp, char *key);
static void exec_setenv(struct setenvstmt *ssp);
static void exec_unsetenv(struct unsetenvstmt *usp);
static void exec_clearenv(struct clearenvstmt *csp);
static void exec_getenv(struct getenvstmt *ssp);
static void eval_system(struct systemstmt *ssp);

/*----------------------------------------------------------------------------*
 *  PUBLIC  void wrap_exec_stmtlist (char *tn, TD_stmt *stmt) 
 * 
 *  Description:   Reset lastkey and call exec_stmtlsi()
 *----------------------------------------------------------------------------*/
PUBLIC  void wrap_exec_stmtlist (char *tn, TD_stmt *stmt) 
{
    lastkey = 0;
    if (Sig_caught == SIGINT || Sig_caught == SIGQUIT) return;
    if (TET) {
	tet_exec_stmtlist(tn,stmt,4);
    } else {
	setenvvars(Xst);
	exec_stmtlist(tn,stmt);
    }
}

int    push_prog_key(TD_progstmt *prog);
/*---------------------------------------------------------------------------*
 *  static void exec_stmtlist (char *tn, TD_stmt *stmt) 
 * 
 *---------------------------------------------------------------------------*/
static void exec_stmtlist (char *tn, TD_stmt *stmt) 
{
    if (stmt == NULL) {
	if (Warn_of_export) {
	    fprintf(Rfile,">> Warning: Rule contains no executable progs.\n");
	}
	return;
    }
    for (; stmt; stmt = stmt->s_next) {
	exec_stmt(tn, stmt, exec_stmtlist);
    }
}

static int except_done;
static ST_symbol *resume_var;
static int except_result;
static int rerunning = 0;
int exec_exception(char *tn, TD_stmt *stmt) 
{
    except_result = 0;
    resume_var = NULL;
    except_done = 0;
    setenvvars(Xst);
    exec_exception_stmts(tn, stmt);
    resume_var = NULL;
    except_done = 0;
    return except_result;
}

static int ex_modifier(TD_stmt *stmt)
{
    char *sdisp;
    ST_symbol *variable;
    int disp;

    disp = stmt->s_u.eq_p->eq_modifier;
    if (disp == A_NOCARE) {
	variable = stmt->s_u.eq_p->eq_modvar;
	sdisp = (variable->st_workval)?
	  (variable->st_workval):
            ((variable->st_initval)?(variable->st_initval):"");
	if (strcmp(sdisp, "") == 0) {
            disp = A_CRIT;
        } else if (strcmp(sdisp,"critical") == 0) {
            disp = A_CRIT;
        } else if (strcmp(sdisp,"noncritical") == 0) {
            disp = A_NONCRIT;
        } else if (strcmp(sdisp,"ignore") == 0) {
            disp = A_IGNORE;          
        } else if (strcmp(sdisp,"exit") == 0) {
            disp = A_EXIT;            
        } else { 
	    disp = A_CRIT;
        }
    } 
    return disp;
}


static void exec_exception_stmts(char *tn, TD_stmt *stmt)
{
    for (; stmt && (!except_done); stmt = stmt->s_next) {
	switch(stmt->s_type) {
	  case EXEXIT:
	    except_result = A_EXIT;
	    except_done = 1;
	    break;
	  case EXNONCRITICAL:
	    except_result = A_NONCRIT;
	    except_done = 1;
	    break;
	  case EXCRITICAL:
	    except_result = A_CRIT;
	    except_done = 1;
	    break;
	  case EXPASS:
	    except_result = A_IGNORE;
	    except_done = 1;
	    break;
          case EXRERUNTEST:
	    if (! reruntest) {
		old_fail_reason = strsave(Fail_reason);
		reruntest = 1;
		longjmp(Knownfail, -1);
	    } else {
		reruntest = 0;
		except_result = A_CRIT;
	    }
	    except_done = 1;
	    break;
	  case EXGOTO:
	    except_done = 1;
	    except_result = A_CRIT;
	    break;
	  case EXRERUN:
	    if (!rerunning) {
		old_fail_reason = strsave(Fail_reason);
		prog_run_no++;
		except_result = EXRERUN;
	    } else {
		except_result = ex_modifier(stmt);
	    }
	    except_done = 1;
	    break;
	  case EXRESUME:
	    resume_var = stmt->s_u.eq_p->eq_resumevar;
	    except_result = ex_modifier(stmt);
	    except_done = 1;
	  default:
	    exec_stmt(tn, stmt, exec_exception_stmts);
	    break;
	}
    }
    rerunning = (except_result == EXRERUN);
    return;
}

static void exec_stmt(char *tn, TD_stmt *stmt, 
		      void (*exec_list)(char *, TD_stmt *))
{
    if (resume_var != NULL || except_done) { 
	/* don't do anything, we are trying to resume somewhere */
	return;
    } 
    switch(stmt->s_type) {
      case FOREACH:  {
	  char           *p;
	  struct loopvar *lv;
	      
	  lv = loopvarinit(STMT_GET_FORSTMT(stmt)->fs_loopvars );
	  while( p = loopvarget( lv ) ) {
	      STMT_GET_FORSTMT(stmt)->fs_forvar->st_workval = strsave(p);
	      if (*p == '@' && *(p+1) == '\0' ) { 
		  /*skip */;
	      } else {
		  if (*p == ' ' && *(p+1) == '\0' ) {
		      (*exec_list)(tn, STMT_GET_FORSTMT(stmt)->fs_body); 
		  } else { 
		      char *q = strsave(p);
		      char *r = q;
		      /*
		       * Convert occurences of '/' or '.' to '_' to avoid 
		       * problems with the key, which uses '.' as a 
		       * separator...
		       */ 
		      while (q && *q) {
			  if ((*q == '/')|| (*q == '.')) *q = '_';
			  q++;
		      }
		      push(r);    
		      (*exec_list)(tn, STMT_GET_FORSTMT(stmt)->fs_body);
		      pop();
		  }
	      }
	      free(STMT_GET_FORSTMT(stmt)->fs_forvar->st_workval);
	      STMT_GET_FORSTMT(stmt)->fs_forvar->st_workval = NULL;
	      if (resume_var == (STMT_GET_FORSTMT(stmt)->fs_forvar)) {
		  resume_var = NULL; /* resume normal execution */
	      }
	  }
	  STMT_GET_FORSTMT(stmt)->fs_forvar->st_workval = 0;
	  loopvarfree( lv );
	  break;
      }
      case SWITCH: {
	  struct switchcase *cl;
	  cl = evalswitch(STMT_GET_SWITCHSTMT(stmt));
	  if (!cl) break;
	  (*exec_list)(tn, cl->case_body);
	  break;
      }
      case IF: 
	if (EXPRT_eval_expr(STMT_GET_IFSTMT(stmt)->if_exp)) 
	  (*exec_list)(tn, STMT_GET_IFSTMT(stmt)->if_body);
	break;
      case ELSE:
	if (EXPRT_eval_expr(STMT_GET_IF_ELSE_STMT(stmt)->if_e_exp))
	  (*exec_list)(tn, STMT_GET_IF_ELSE_STMT(stmt)->if_e_body);
	else 
	  (*exec_list)(tn, STMT_GET_IF_ELSE_STMT(stmt)->else_body);
	break;
      case PROG:  {
	  int topop = push_prog_key(STMT_GET_PROGSTMT(stmt));
	  prog_run_no = 1;
	      
	  /* Get the stuff currently in the stack, make a
	   * key, and add it to the array of keys.
	   */
	  assert(lastkey != MAXKEYS);
	  KEYS[lastkey].dptr = STMT_GET_PROGSTMT(stmt)->key = 
	    traverse_stack(tn);
	  KEYS[lastkey].dsize = strlen(KEYS[lastkey].dptr)+1;
	  STMT_GET_PROGSTMT(stmt)->ps_pcur++;
	  while (topop--) 
	    free(pop());
	  if( Noexecute )
	    noexec_prog( stmt->s_u.ps_p, KEYS[lastkey].dptr );
	  else {
	      st_setfailure(Xst,"0");
	      exec_prog( stmt->s_u.ps_p, KEYS[lastkey].dptr );
	  }
	  lastkey++;
	  break;
      }
      case STRSUB: {
	  eval_strsub(STMT_GET_STRSUBSTMT(stmt));
	  break;
      }
      case INTERSECT: {
	  eval_intersect(STMT_GET_INTERSECTSTMT(stmt));
	  break;
      }
      case SYSTEM: {
	  eval_system(STMT_GET_SYSTEMSTMT(stmt));
	  break;
      }
      case STRCAT: {
	  eval_strcat(STMT_GET_STRCATSTMT(stmt));
	  break;
      }
      case ASSIGN: {
	  eval_assign(STMT_GET_ASSIGNSTMT(stmt));
	  break;
      }
      case DEFINED:
	break;
      case ECHOT: 
	{
	    char *p = a_str( stmt->s_u.ec_p->ec_arg );
		
	    if( Noexecute ) {
		fprintf( Rfile, "/bin/echo \"%s\"\n", (p)?p:"" );
		break;
	    }
	    if( p ) {
		fprintf( Rfile, "%s\n", p );
		(void)fflush(Rfile);
	    } else {
		fprintf( Rfile, ">> No value for echo variable\n" );
		(void)fflush(Rfile);
	    }
	    break;
	}
      case CHDIR:
	{
	    char *p = a_str( stmt->s_u.cd_p->cd_arg );
	    if( Noexecute ) {
		fprintf( Rfile, "cd %s\n", (p)?p:"<unknown>" );
		break;
	    }
	    if (p) {
		if (chdir(p)) {
		    fprintf(Rfile, "Unable to change directory to %s\n", p);
		    (void)fflush(Rfile);
		} else {
		    fprintf(Rfile, "\t*** Current directory changed to %s ***\n", p);
		    (void)fflush(Rfile);
		}
	    } else {
		fprintf(Rfile, ">> No value for chdir variable\n");
		(void)fflush(Rfile);
	    }
	    break;
	}
	    
      case VERSION: 
	{
	    char *p = a_str( stmt->s_u.vs_p->vs_arg );
		
	    if( Noexecute ) {
		fprintf( Rfile, "vers %s\n", (p)?p:"<unknown>" );
		break;
	    }
	    if( p )
	      if( *p )
		ver_print( Rfile, p );
	      else
		fprintf( Rfile,">> Null value for version variable\n");
	    else
	      fprintf( Rfile, ">> No value for version variable\n" );
	    break;
	}
	    
      case GETENV:
	exec_getenv( stmt->s_u.ge_p );
	break;
	    
      case SETENV:
	exec_setenv( stmt->s_u.se_p );
	break;
	    
      case UNSETENV:
	exec_unsetenv( stmt->s_u.ue_p );
	break;
	    
      case CLEARENV:
	exec_clearenv( stmt->s_u.ce_p );
	break;
	    
      case BUILTIN:
	eval_builtin( stmt->s_u.bt_p->bt_arg, B_EXEC );
	env_builtin( stmt->s_u.bt_p->bt_arg );
	break;
	    
      default: assert(0);
    }
    update_environ(stmt);
}

static void exec_prog (struct progstmt *psp, char *key)
{
    struct loopvar *lv;
    int pid;			/* process id from fork */
    int oldmask;		/* saved signal mask */
    IMPORT char **environ;	/* The environment */
    int save_fd[3];		/* A place to save file descriptors */
    struct symbol *tca_v;       /* sym pointer for variable with exec nm */
    char *tca_exec, *to_time;   /* values of above syms */
    int timelimit;
    int cpulimit;
    struct timeval before, after;
    struct timezone zero;
    int eerr;			/* child's errno code on exec failure */
    char childexecstatus[25];	/* if this file exists, child did not exec */
    char *tmp;
    TD_arg *saved_args, *copyargs(TD_arg *);
    void freeargs(TD_arg *);
    struct rusage cbefore, cafter;
    char *fallkey;   /* key used if fallback was used */
    
    assert(psp);
    
    progsrun++;
    /*
     *  Remove blanks leading words from the command line
     */
    
    /*
     * set up child exec status file
     */
    sprintf(childexecstatus, "__child_exec_failed.%d", getpid());
    unlink(childexecstatus);		/* just in case */
    
    
    lv = cmdargsinit (psp->ps_name, psp->ps_args); 
    
    /* need to sigpause without missing sigs */
    oldmask = sigblock (MASK(SIGCHLD) | MASK(SIGALRM));
    
    /* Now create the name of the file to redirect the prog's stdin from.
     * We can not do this in the child because it mallocs memory and that
     * is very bad when we are using vfork.
     */
    translate_stdin( Xtest, psp, key );
    
    eerr = 0;			/* The child may change this on an error */
    
    zero.tz_minuteswest = zero.tz_dsttime = 0; /* init time zone to zero */

    gettimeofday(&before,&zero);
#ifdef hpux
    getrusage(RUSAGE_CHILDREN, &cbefore);
#endif
    /* check for a CPU time limit before forking */
    if (Maxcpu) {
	cpulimit = get_timeout(Maxcpu, "CPUTIME");
    } else {
	cpulimit = get_timeout(0, "CPUTIME");
    }
    
    
    if( (pid = fork ()) == -1) {
	fi_prcmd (lv, psp);
	fi_printf ("\tfork failed\n");
	test_res_printf("%s/%s: FAILED; system error; prog=%s; fork failed: %s\n",
			Subsuite, Xtest, key, SYSERR);
	(void) sigsetmask (oldmask);
	loopvarfree (lv);
        checkresources(PROCESSES); 
        st_setfailure(Xst,"1");
	longjmp (Knownfail, 1);
    }
    
    if( pid ) {			/* parent */
	
	struct rusage   ru;
	int             wpid;	/* of child process */
	int             status;
	double          childtm ();
	struct stat	sbuf;	/* to see if child exec failed */
	time_t          tm0;
	
	/* Now get on with waiting for the child */
	Timeleft = timelimit = get_timeout(Maxwall, "TIMEOUT");
      SP:
	/* could check to see if Timeleft > timelimit */
	(void) alarm (Timeleft ? Timeleft : 1);
	(void) sigpause (0);  /* goes to sleep until a signal */
	/* mask is restored at this point (i.e. all blocked) */
	switch (Sig) {
	    
	  case SIGALRM:
	    /* kill process group that was set up for child
	     * child might have already exited, so ignore any errors.
	     * We first send a SIGTERM to just the parent and then we wait
	     * for 10 seconds.  We then send a SIGKILL to the whole process
	     * group to make sure it died.  This stuff gives the test case
	     * a chance to clean itself up before it is brutally murdered.
	     */
	    (void) killpg( pid, get_signal(SIGTERM, "TOSIGNAL"));
#if SLEEP_ISNT_BROKEN
	    sleep( 10 );	/* Give it 10 seconds to clean up */ 
#else
	    tm0 = time(NULL);
	    while ((time(NULL) - tm0) < 10)
	      fseek(Rfile, 0, SEEK_END); /* use I/O to not spin the CPU */
#endif
	    (void) killpg( pid, SIGKILL );
	    
	    /* child has either exited on its own, or we've killed it. */
	    /* wait for it now, just so it's not hanging around... */
	    (void) wait3 (&status, WNOHANG, (struct rusage *)0 );
	    
	    (void) sigsetmask (oldmask);
	    /* Doug says that any blocked signals will get delivered after */
	    /* a system call.  Do this so that they don't mess up real stuff */
	    (void) getpid ();   /*  i.e. get'em now and ignore 'em  */

	    /* 
	     * just in case the child wrote to the reportfile, seek to the end
	     */
	    fseek(Rfile, 0, SEEK_END);
	    
	    fi_prcmd (lv, psp);
	    if(  Sig_caught ) {
		if(  Sig_caught == SIGTERM ) {
		    Sig_caught = 0;
		}  
		fi_printf("\ttest case terminated by user\n");
		test_res_printf ("%s/%s: FAILED; terminated; prog=%s;\n",
				 Subsuite, Xtest, key);
		st_setfailure(Xst,"1");
		statProg(psp, TERMINATED, (struct timeval *)NULL,
                         (struct rusage *)NULL);
	    } else {
		time_except(psp, key, timelimit);
	    }
	    loopvarfree (lv);
	    longjmp (Knownfail, 1);
	    break;
	    
	  case SIGCHLD:
	    /* wait for it */
	  W:
	    /* need to wait for as many as it takes to find "pid"! */
	    /* (we might not have gotten a killed one in the wait above) */
	    wpid = (int)wait3 (&status, WNOHANG, &ru);
	    if( wpid == 0) {
		/* No children to wait for */
		goto SP;
	    }
	    if( wpid != pid) {
		/* Not the child we wanted, try again */
		goto W;
	    }
	    /* found who we wanted; go on */
	    break;
	    
	  case SIGTSTP:
	    goto SP;		/* Just restart our wait */
	    break;
	    
	  default:
	    /* just in case... */
	    fprintf (Rfile, ">> UNEXPECTED SIGNAL (%d); ABORTING...\n", Sig);
	    (void) fflush (Rfile);
	    abort ();
	}
	
	(void) sigsetmask (oldmask);
	/* Doug says that any blocked signals will get delivered after */
	/* a system call.  Do this so that they don't mess up real stuff. */
	/* N.B.  Must not be a "fast" system call!!! */
	(void) getpid ();
	
	/* If the child encountered some type of error, it will have created
	 * the file pointed to by childexecstatus and returned the errno as
	 * its exit status.  We can simply inspect it for a non-zero value.
	 */
	
	/* 
	 * just in case the child wrote to the report file, seek to the end
	 */
	fseek(Rfile, 0, SEEK_END);

#ifdef hpux
	getrusage(RUSAGE_CHILDREN, &cafter);
	ru.ru_utime.tv_sec = cafter.ru_utime.tv_sec - cbefore.ru_utime.tv_sec;
	ru.ru_utime.tv_usec = cafter.ru_utime.tv_usec 
	  - cbefore.ru_utime.tv_usec;
	
#endif
        gettimeofday(&after,&zero);
        after.tv_sec -= before.tv_sec;
        after.tv_usec -= before.tv_usec;
        if (after.tv_usec < 0) {
	    after.tv_sec--;
            after.tv_usec += 1000000;
        }
        /* after now contains the elapsed time */
        add_prog_wallclock(psp, &after);
	add_prog_cpuclock(psp, childtm(&ru));
	if( stat(childexecstatus, &sbuf) != -1 ) {
	    char sig[256];
	    
	    /* child failed before exec */
	    unlink(childexecstatus);	/* don't want next test to fail */
	    fi_prcmd (lv, psp);
	    errno = eerr = WEXITSTATUS(status);
	    fi_printf ("\texec of '%s' failed[%d]: %s\n",*lv->argv,errno,
		       SYSERR);
	    test_res_printf("%s/%s: FAILED; system error; prog=%s; exec failed;\n",
			    Subsuite, Xtest, key);
	    sprintf(sig, "system error; prog=%s; exec failed;", key);
	    statSig(sig);
	    loopvarfree (lv);
            if (eerr  == ENOMEM)		/* no more free swap space */
	      checkresources(SWAPSPACE);
            st_setfailure(Xst,"1");
	    statProg(psp, EXECFAIL, &after, &ru);
	    longjmp (Knownfail, 1);
	}
	
	/* check everything here */
	
#ifndef hpux
	/* Check to see if the child died because of CPU resource limits */
	if( WTERMSIG(status) == SIGXCPU ) {
	    if (cpu_except(psp, key, cpulimit)) {
		char sig[256];
	    
		/* CPU limit reached */
		fi_printf( "\tCPU time limit (%d) exceeded\n", cpulimit );
		test_res_printf( "%s/%s: FAILED; CPU time limit; prog=%s;\n",
				Subsuite, Xtest, key );
		sprintf(sig,"CPU time limit; prog=%s;",key);
		statSig(sig);
		loopvarfree( lv );
		st_setfailure(Xst,"1");
		statProg(psp, CPUFAIL, &after, &ru);
		longjmp( Knownfail, 1 );
	    }
	}
#endif
	
        /* We no longer use a displacement from ps_actres and ps_actres.
         * Instead we reuse the same location over and over again.
         */
        psp->ps_actres = (struct result *)malloc(sizeof(struct result));
        psp->ps_expres = (struct result *)malloc(sizeof(struct result));
	
	cstat_cvt (status, &psp->ps_actres->re_rc);
	psp->ps_actres->re_time = (float) childtm (&ru);
	
        /* We need to get the expected results from the database */
        {
	    DB_record *e;
	    e = NULL;
	    e = tget_results(Xent,key,psp,&fallkey);
	    /*
	     *  Must check that we don't return an erro string here...
	     */
	    {
		char *ptr = cstat_get(RSLTREC_GET(DBRECORD_GET(e,results),exp_rc),
				      &psp->ps_expres->re_rc);
		if (ptr) {
		    fprintf(Rfile, ">> Warning: %s, assuming 0\n",ptr);
		}
	    }
	    if (RSLTREC_GET(DBRECORD_GET(e,results),exp_rc)) {
		free(RSLTREC_GET(DBRECORD_GET(e,results),exp_rc));
	    }
	    psp->ps_expres->re_time = RSLTREC_GET(DBRECORD_GET(e,results),exp_time);
	    psp->ps_expres->re_stdout = RSLTREC_GET(DBRECORD_GET(e,results),
						    exp_stdout);
	    psp->ps_expres->re_stderr = RSLTREC_GET(DBRECORD_GET(e,results),
						    exp_stderr);
	    if (fallkey && V && !(Newrc && Newstdout && Newstderr && Newtime)){
		fi_printf("couldn't find '%s' used '%s'\n", key, fallkey);
	    }
	    
	    if (DBRECORD_GET(e,results)) free (DBRECORD_GET(e,results));
	    if (e) free(e);
        }
	if( Tca) {
	    if (TCA_var) {
		if ((tca_v = st_find(Xst, Exec_var_nm)) != NULL) {
		    tca_exec = tca_v->st_workval ? 
		      tca_v->st_workval : tca_v->st_initval;
		    td_tca_var(Xtest, psp, tca_exec);
		} else {
		    fprintf(Rfile, ">> Warning: no such variable for tcasetup: %s\n",
			    Exec_var_nm);
		}
	    } else
	      td_tca (Xtest, psp, lv);
	}
	
	switch( checkres( Xtest, psp, lv, key, fallkey ) ) {
	  case PROGPASS:			/* everything OK, continue */
	    statProg(psp, PASS, &after, &ru);
	    break;
	  case PROGNON:		/* non critical error, note it and continue */
	    Noncriterr++;
            st_setfailure(Xst,"1");
	    statProg(psp, RESFAIL, &after, &ru);
	    break;
	  case PROGCRIT:		/* critical error, we can quit now */
	    loopvarfree (lv);
            st_setfailure(Xst,"1");
	    if (psp->ps_expres->re_stdout) free(psp->ps_expres->re_stdout);
	    if (psp->ps_expres->re_stderr) free(psp->ps_expres->re_stderr);
	    if (psp->ps_expres) free (psp->ps_expres);
	    if (psp->ps_actres) free (psp->ps_actres);
	    psp->ps_expres = NULL;
	    psp->ps_actres = NULL;
	    statProg(psp, RESFAIL, &after, &ru);
	    if (fallkey) free(fallkey);
	    fallkey = NULL;
	    longjmp (Knownfail, 1); /* wrote error in checkres */
	  case PROGEXIT:		/* "exited"-type error */
	    loopvarfree (lv);
            st_setfailure(Xst,"1");
	    if (psp->ps_expres->re_stdout) free(psp->ps_expres->re_stdout);
	    if (psp->ps_expres->re_stderr) free(psp->ps_expres->re_stderr);
	    if (psp->ps_expres) free (psp->ps_expres);
	    if (psp->ps_actres) free (psp->ps_actres);
	    psp->ps_expres = NULL;
	    psp->ps_actres = NULL;
	    statProg(psp, EXITED, &after, &ru);
	    if (fallkey) free(fallkey);
	    fallkey = NULL;
	    longjmp (Knownfail, 1); /* wrote error in checkres */
	  case PROGREDO:              /* rerun */
	    if (psp->ps_expres) {
		if (psp->ps_expres->re_stdout) free(psp->ps_expres->re_stdout);
		if (psp->ps_expres->re_stderr) free(psp->ps_expres->re_stderr);
		free (psp->ps_expres);
		psp->ps_expres = NULL;
	    }
	    if (psp->ps_actres) {
		free(psp->ps_actres);
		psp->ps_actres = NULL;
	    }
	    exec_prog (psp, key);	    
	    break;
	}
	if (fallkey) free(fallkey);
	fallkey = NULL;
	
	(void) fflush( Rfile );
	if (psp->ps_expres) {
	    if (psp->ps_expres->re_stdout) free(psp->ps_expres->re_stdout);
	    if (psp->ps_expres->re_stderr) free(psp->ps_expres->re_stderr);
	    free (psp->ps_expres);
	    psp->ps_expres = NULL;
	}
	if (psp->ps_actres) {
	    free(psp->ps_actres);
	    psp->ps_actres = NULL;
	}
	loopvarfree (lv);	
	psp->ps_pcur++;		
	
    } else {			/* child */
	
	struct rlimit cpu_rlim;
	struct rlimit stack_rlim;
	struct rlimit core_rlim;
	struct rlimit concur_rlim;
	
#ifdef __convexc__
	struct pattributes pattr;
#endif
	int pid = getpid();
	int i, max;
	
	(void) alarm (0);   /* in case it's inherited from parent */
	(void) sigsetmask (oldmask);	/* restore to parent original */
	
	while (DEBUG_CHILD) 
	  DEBUG_CHILD = debug_child();
	
	/* should we use the pid for the process group, or a fixed value? */
	/* if a fixed value, what should it be?  What happens, (in either */
	/* case), if we pick a value that is already used?  Do we blow away */
	/* other random processes? */
	if( setpgid (0, getpid ()) == -1) {
	    /* it's incredibly hard to imagine that this call could fail */
	    eerr = errno;
	    close(creat(childexecstatus, (mode_t)0666));
	    exit (eerr);	/* tell parent why we failed */
	}
	
	/* set up 0, 1, 2; everything else better be close on exec */
	if( setupfiles (Xtest, psp, key)) {
	    eerr = errno;	/* Just flag the error */
	    close(creat(childexecstatus, (mode_t)0666));
	    exit(eerr);
	}
	
	setupugids();		/* Setup the UID and GID for execution */
	
	environ = env_var_setup(); /* instate new environment */
	
#ifndef hpux
	/* Setup CPU resource limit, if any */
	if (cpulimit != 0) {
	    cpu_rlim.rlim_cur = cpulimit;
	    cpu_rlim.rlim_max = cpulimit;
	    setrlimit( RLIMIT_CPU, &cpu_rlim );
	}
	
	/* setup stacksize resource limit, if any */
	getrlimit(RLIMIT_STACK, &stack_rlim);  /* get current values */
	if (psp->ps_att[A_STACKSIZE].at_disp == A_FILE) {
	    char *val = a_str(psp->ps_att[A_STACKSIZE].at_file);
	    if (*val) {
		if (strcmp(val, "unlimited") == 0)  {
		    stack_rlim.rlim_cur = RLIM_INFINITY;
		} else {
		    if ((stack_rlim.rlim_cur = atobytes(val)) == -1) {
			stack_rlim.rlim_cur = atoi(val);
			fprintf(Rfile, "Error! stacksize value: %s for prog %s invalid, using %d\n", val, psp->progid, 
				stack_rlim.rlim_cur);
		    }
		}
	    } else {
		stack_rlim.rlim_cur = 512 * 1024;
	    }
	    if (stack_rlim.rlim_cur < 10240) {
		fprintf(Rfile, "Error! stacksize value: %d for prog %s too small, using %d\n", stack_rlim.rlim_cur, psp->progid, stack_rlim.rlim_max);
		stack_rlim.rlim_cur = stack_rlim.rlim_max;
	    }
	    if (stack_rlim.rlim_max < stack_rlim.rlim_cur) {
		stack_rlim.rlim_max = stack_rlim.rlim_cur;
	    }
	    (void)setrlimit(RLIMIT_STACK, &stack_rlim);
	}
	
	/* setup coresize resource limit, if any */
	getrlimit(RLIMIT_CORE, &core_rlim);  /* get current values */
	if (psp->ps_att[A_CORESIZE].at_disp == A_FILE) {
	    char *val = a_str(psp->ps_att[A_CORESIZE].at_file);
	    if (*val) {
		if (strcmp(val, "unlimited") == 0)  {
		    core_rlim.rlim_cur = RLIM_INFINITY;
		} else {
		    if ((core_rlim.rlim_cur = atobytes(val)) == -1) {
			core_rlim.rlim_cur = atoi(val);
			fprintf(Rfile, "Error! coresize value: %s for prog %s invalid, using %d\n", val, psp->progid, 
				core_rlim.rlim_cur);
		    }
		}
	    }
	    if (core_rlim.rlim_max < core_rlim.rlim_cur) {
		core_rlim.rlim_max = core_rlim.rlim_cur;
	    }
	    (void)setrlimit(RLIMIT_CORE, &core_rlim);
	}
#endif
	
#ifdef __convexc__
	/* setup concurrency resource limit, if any */
	getrlimit(RLIMIT_CONCUR, &concur_rlim);  /* get current values */
	if (psp->ps_att[A_MAXCONCUR].at_disp == A_FILE) {
	    char *val = a_str(psp->ps_att[A_MAXCONCUR].at_file);
	    if (*val) {
		if (strcmp(val, "unlimited") == 0) {
		    concur_rlim.rlim_cur = RLIM_INFINITY;
		} else if (strcmp(val, "headsavail") == 0) {
		    struct system_information si;
		    getsysinfo(SYSINFO_SIZE, &si);
		    concur_rlim.rlim_cur = (int) si.cpu_count;
		} else {
		    if ((concur_rlim.rlim_cur = atobytes(val)) == -1) {
			concur_rlim.rlim_cur = atoi(val);
			fprintf(Rfile, 
				"Error! maxconcur value: %s for prog %s "
				"invalid, using %d\n", val, psp->progid, 
				concur_rlim.rlim_cur);
		    }
		}
	    }
	    if (concur_rlim.rlim_max < concur_rlim.rlim_cur) {
		concur_rlim.rlim_max = concur_rlim.rlim_cur;
	    }
	    (void)setrlimit(RLIMIT_CONCUR, &concur_rlim);
	}
	
	/* setup process attributes, if any */
	getpattr(pid, &pattr);
	if (psp->ps_att[A_FIXED].at_disp == A_DISTINCT) {
	    pattr.pattr_pfixed = 1; /* set fixed */
	} else if (psp->ps_att[A_FIXED].at_disp == A_FILE) {
	    char *val = a_str(psp->ps_att[A_FIXED].at_file);
	    if (*val) {
		if (strcmp(val,"TRUE") == 0 ||
		    strcmp(val,"true") == 0 ||
		    strcmp(val,"yes") == 0 ||
		    strcmp(val, "t") == 0 ||
		    strcmp(val, "y") == 0 ||
		    strcmp(val, "1") == 0) 
		  pattr.pattr_pfixed = 1;
		else
		  pattr.pattr_pfixed = 0;
	    } else {
		pattr.pattr_pfixed = 1;
	    }
	}
	setpattr(pid, &pattr);
#endif
	execvp( *lv->argv, lv->argv );
	
	eerr = errno;		/* Indicate cause of error */
	close(creat(childexecstatus, (mode_t)0666));
	exit (eerr);
    }
    
    return;
}


static void
  noexec_prog (psp, key)
struct progstmt *psp;
char *key;
{
    struct loopvar *lv;
    struct symbol *tca_v;       /* sym pointer for variable with exec nm */
    char *tca_exec;             /* values of above syms */
    char *fallkey; 
    
    progsrun++;
    lv = cmdargsinit (psp->ps_name, psp->ps_args);
    
    if( Tca) {
	if (TCA_var) {
	    if ((tca_v = st_find(Xst, Exec_var_nm)) != NULL) {
		tca_exec = tca_v->st_workval ? 
		  tca_v->st_workval : tca_v->st_initval;
		td_tca_var(Xtest, psp, tca_exec);
	    } else {
		fprintf(Rfile, ">> Error: no such variable for tcasetup: %s\n",
			Exec_var_nm);
	    }
	} else
	  td_tca (Xtest, psp, lv);
    }
    
    /* Now create the name of the file to redirect the prog's stdin from. */
    translate_stdin( Xtest, psp, key );
    
    /* We no longer use a displacement from ps_actres and ps_actres.
     * Instead we reuse the same location over and over again.
     */
    psp->ps_actres = (struct result *)malloc(sizeof(struct result));
    psp->ps_expres = (struct result *)malloc(sizeof(struct result));
    
    /* We need to get the expected results from the database */
    {
	DB_record *e;
	e = tget_results(Xent,key,psp,&fallkey);
	free(fallkey);
	fallkey = NULL;
	cstat_get(RSLTREC_GET(DBRECORD_GET(e,results),exp_rc),
		  &psp->ps_expres->re_rc);
	if (RSLTREC_GET(DBRECORD_GET(e,results),exp_rc)) {
	    free(RSLTREC_GET(DBRECORD_GET(e,results),exp_rc));
	}
	psp->ps_expres->re_time = RSLTREC_GET(DBRECORD_GET(e,results),exp_time);
	psp->ps_expres->re_stdout = RSLTREC_GET(DBRECORD_GET(e,results),
						exp_stdout);
	psp->ps_expres->re_stderr = RSLTREC_GET(DBRECORD_GET(e,results),
						exp_stderr);
    }
    
    (void) noex_checkres (Xtest, psp, lv, key);
    loopvarfree (lv);	/* could add in front of longjmp's */
    psp->ps_pcur++;	/* won't get done if we longjmp, but then */
    return;
}


/* exec_getenv - process a getenv statement */
static void
  exec_getenv( ssp )
struct getenvstmt *ssp;
{
    char *np;		/* Variable name */
    char *vp;		/* Variable value */
    ST_symbol *val;     /* temporary to hold value of environ */
    char *find_env_value(char *);
    char *is_environ(char *);
    
    if (ssp->ge_args[0]->a_type != VAR) {
	fprintf( Rfile, ">>  first argument to getenv must be a variable \n");
	return;
    }
    if (ssp->ge_args[0]->a_u.var_p->st_class == GLOBAL) {
	fprintf(Rfile, ">> first argument to getenv cannot be global \n");
	return;
    }
    
    np = a_str( ssp->ge_args[ 0 ] );
    vp = a_str( ssp->ge_args[ 1 ] );
    
    if ((val=st_find(Xst,vp)) && 
	(val->st_type == ENVTYPE || val->st_type == RESTYPE)) {
	ssp->ge_args[0]->a_u.var_p->st_workval = 
	  val->st_workval?val->st_workval:val->st_initval;
    } else  {
        ssp->ge_args[0]->a_u.var_p->st_workval = find_env_value(vp);
    } 
    if( Noexecute )
      fprintf( Rfile, "# extracting value '%s' of '%s'; \n", 
	      ssp->ge_args[0]->a_u.var_p->st_workval, (vp ? vp : "" ) );
    
    
    return;
}




/* exec_setenv - process a setenv statement */
static void
  exec_setenv(struct setenvstmt *ssp)
{
    char *np;		/* Variable name */
    char *vp;		/* Variable value */
    
    
    np = a_str( ssp->se_args[ 0 ] );
    vp = a_str( ssp->se_args[ 1 ] );
    
    if( !np || !*np ) {
	fprintf( Rfile, ">> NULL name in exec_setenv\n" );
	return;
    }
    
    if( Noexecute )
      fprintf( Rfile, "setenv %s '%s';\n", np, (vp ? vp : "" ) );
    
    env_var_set( np, vp );	/* Add/update with value */
    
    return;
}


/* exec_unsetenv - process an unsetenv statement */
static void
  exec_unsetenv(struct unsetenvstmt *usp)

{
    char *np;		/* Variable name */
    
    np = a_str( usp->ue_arg );
    
    if( !np || !*np ) {
	fprintf(Rfile, ">> NULL name in exec_unsetenv\n" );
	return;
    }
    
    if( Noexecute )
      fprintf( Rfile, "unsetenv %s\n", np );
    
    env_var_unset( np );
    
    return;
}

/* exec_clearenv - process a clearenv statement */
static void exec_clearenv(struct clearenvstmt *csp)
{
    
    if( Noexecute )
      fprintf( Rfile, "clearenv\n" );
    
    env_var_clean( FALSE );	/* Get rid of variables */
    
    return;
}

void eval_system (struct systemstmt *ssp)
{
    char *t1, *t2, t3[20];
    
    if (strcmp(ssp->sy_args[1]->a_u.var_p->st_name, 
	       ssp->sy_args[2]->a_u.var_p->st_name)) {
	exec_system(  a_str (ssp->sy_args [0]), &t1,  &t2, t3); 
	ssp->sy_args[1]->a_u.var_p->st_workval = t1;
	ssp->sy_args[2]->a_u.var_p->st_workval = t2;
    } else {
	exec_system(  a_str (ssp->sy_args [0]), &t1,  &t1, t3); 
	ssp->sy_args[1]->a_u.var_p->st_workval = t1;
	ssp->sy_args[2]->a_u.var_p->st_workval = t1;
    }
    if (ssp->sy_args[3] != NULL) {
	ssp->sy_args[3]->a_u.var_p->st_workval = strsave(t3);
    }
}

char *my_getenv(char ** environ, char *name)
{	/* search environment list for named entry */
    char **s;
    int n = strlen(name);
    
    assert(environ);
    for (s = environ; *s; s++)
      {	/* look for name match */
	  if (!strncmp(*s, name, n) && (*s)[n] == '=')
	    return (char *) &((*s)[n+1]);
      }
    return (NULL);
}


/*----------------------------------------------------------------------------*
  FUNCTION:  char *exec_printf(TD_arg *args)
  DESCRIPTION:
  Executes a printf() statement, and return NULL.  This function is intended
  to be called indirectly from eval_builtin("printf",B_EXEC).
  *----------------------------------------------------------------------------*/
char *exec_printf(TD_arg *args)
{
    extern char *eval_printf(TD_arg *args);
    char *str, *tmp, *format, c;
    TD_arg *ap;
    char *arg1;
    char *arg2;

    if (TET) {
	tmp = str = malloc(1024);
	ap = args;
	assert(ap->a_type == STR);
	format = ap->a_u.str_p;
	ap = args->b_args;
	while (c = *format++) {
	    if (c == '%') {
		format++;
		arg1 = tet_argref(ap, "");
#if sun
		sprintf(tmp, "%s", arg1);
		tmp += strlen(tmp);
#else
		tmp += sprintf(tmp, "%s", arg1);
#endif
		free(arg1);
		ap = ap->b_args;
	    } else if (c == '\\') {
		c = *format++;
		switch(c) {
		  case 'a':
		    *tmp++ = '\a';
		    break;
		  case 'b':
		    *tmp++ = '\b';
		    break;
		  case 'f':
		    *tmp++ = '\f';
		    break;
		  case 'n':
		    break;
		  case 'r':
		    *tmp++ = '\r';
		    break;
		  case 't':
		    *tmp++ = '\t';
		    break;
		  case 'v':
		    *tmp++ = '\v';
		    break;
		  case '\\':
		    *tmp++ = '\\';
		    break;
		} 
	    } else {
		*tmp++ = c;
	    }
	}
	tmp = malloc(strlen(str) + 3);
	fprintf(TETout,"%stet_infoline \"%s\"\n", ind, str);
	sprintf(tmp, "\"%s\"", str);
	free (str);
	return tmp;
    }
    str = eval_printf(args);
    if (! str || ! *str) return NULL;

    if( Noexecute ) {
	fprintf( Rfile, "echo %s\n",str);
    } else {
	fprintf( Rfile, "%s", str );
    } 
    free(str);
    (void)fflush(Rfile);
    return NULL;
}

char *exec_statbase(TD_arg *args)
{
    char *statdb;
    
    if (Update && !TET) {
	statdb = a_str(args);
	if (statdb == NULL || (*statdb == '\0')) {
	    fprintf(Rfile, "Error! bad argument to statbase()\n");
	} else {
	    initStats(statdb);
	}
	return statdb;
    }
    return "";
}

char *re_comp(char *);
int re_exec(char *);
char *exec_cancel(TD_arg *args)
{
    DB_tests_rec *tl;
    DB_testcase *op;
    TD_arg *ap;
    char *err;
    int i;

    tl = DB_fetch_test_list(Xdb);
    if (!args) {
	for (i = 0; i < tl->num_tests; i++) {
	    op = DB_get(Xdb, tl->tests[i]);
	    op->should_run = 0;
	}
    } else {
	for (ap = args; ap; ap=ap->b_args) {
	    if ((err = re_comp(a_str(ap))) == NULL) {
		for (i = 0; i < tl -> num_tests; i++) {
		    if (re_exec(tl->tests[i])) {
			op = DB_get(Xdb, tl->tests[i]);
			op->should_run = 0;
		    }
		}
	    } else {
		fprintf(Rfile, ">>Warning, in cancel, bad regexp '%s': %s\n",
			a_str(ap), err);
	    }
	}
    }
    return NULL;
}




int debug_child(void)
{
    int i;
    
    i = 1;
    return i;
}


/*----------------------------------------------------------------------------*
 *  static void update_environ(TD_stmt *stmt)
 * 
 *  Description:   Update working copy of td's internal environment after 
 *     executing a statement.  The only statements we consider here are those
 *     that change the value of a variable
 *----------------------------------------------------------------------------*/
static void update_environ(TD_stmt *stmt) 
{
    if (!stmt) return;  /* nothing to do */
    switch(stmt->s_type) {
      case STRCAT:  
	if (ARG_GET_TYPE(STMT_GET_STRCATSTMT(stmt)->sc_args)==VAR) {
	    ST_symbol *lhs = 
	      ARG_GET_VAR(STMT_GET_STRCATSTMT(stmt)->sc_args);
	    if (lhs->st_type == ENVTYPE || lhs->st_type == RESTYPE) {
		env_var_set(st_name(lhs),st_value(lhs));
	    }
	}
	break;
      case STRSUB:  /* lhs, if ENVTYPE */
	if (ARG_GET_TYPE(STMT_GET_STRSUBSTMT(stmt)->ss_args[0])==VAR) {
	    ST_symbol *lhs = 
	      ARG_GET_VAR(STMT_GET_STRSUBSTMT(stmt)->ss_args[0]);
	    if (lhs->st_type == ENVTYPE || lhs->st_type == RESTYPE) {
		env_var_set(st_name(lhs),st_value(lhs));
	    }
	}
	break;
      case INTERSECT:  
	if (ARG_GET_TYPE(STMT_GET_INTERSECTSTMT(stmt)->in_args[0])==VAR) {
	    ST_symbol *lhs = 
	      ARG_GET_VAR(STMT_GET_INTERSECTSTMT(stmt)->in_args[0]);
	    if (lhs->st_type == ENVTYPE || lhs->st_type == RESTYPE) {
		env_var_set(st_name(lhs),st_value(lhs));
	    }
	}
	break;
      case ASSIGN:  {
	  ST_symbol *lhs = STMT_GET_ASSIGNSTMT(stmt)->lhs;
	  if (lhs->st_type == ENVTYPE || lhs->st_type == RESTYPE) {
	      env_var_set(st_name(lhs),st_value(lhs));
	  }
	  break;
      }
      case SYSTEM:  {
	  struct systemstmt *ssp = STMT_GET_SYSTEMSTMT(stmt);
	  int i;
	  for (i=1; i <=3 ; i++ ) {
	      if ((ssp->sy_args[i] != NULL) &&
		  (ARG_GET_TYPE(ssp->sy_args[i]) == VAR)) {
		  ST_symbol *lhs = ARG_GET_VAR(ssp->sy_args[i]);
		  if (lhs->st_type == ENVTYPE || lhs->st_type == RESTYPE) {
		      env_var_set(st_name(lhs),st_value(lhs));
		  } /* if */
	      } /* if */
	  } /* for */
      }
      case BUILTIN:
	env_builtin( stmt->s_u.bt_p->bt_arg );
	break;
        /* These statement types do not have any side-effects on
         * variables, so we don't need to update the environment
         */
      case FOREACH:   
      case SWITCH:  
      case IF: 
      case ELSE:
      case PROG:   
      case DEFINED:
      case ECHOT: 
      case CHDIR:
      case VERSION: 
      case GETENV:
      case SETENV:
      case UNSETENV:
      case CLEARENV:
	break;
      default:
	assert(0);
    }
}

static int env_builtin( TD_arg *arglist )
{
    TD_arg *a;
    for (a = arglist; a; a = a->b_args) {
	if (ARG_GET_TYPE(a) == VAR && 
	    (ST_GET_TYPE(ARG_GET_VAR(a)) == ENVTYPE || 
	     ST_GET_TYPE(ARG_GET_VAR(a)) == RESTYPE)) {
	    env_var_set(st_name(ARG_GET_VAR(a)),st_value(ARG_GET_VAR(a)));
	}
    }
}

extern int run_no;   /* from the parser */
void time_except(struct progstmt *psp, char *key, int timelimit)
{
    struct symbol *timeexcept;
    char *exceptname;
    int modifier = A_CRIT;
    char buf[128];
    char sig[256];
 
    sprintf(buf, "%d", timelimit);
    ex_result("walltime", buf, buf, "");
    ex_proginfo(psp);
    if ((timeexcept = st_find(Xst, "TIMEEXCEPT")) != NULL) {
	exceptname = (timeexcept->st_workval) ? timeexcept->st_workval :
	  timeexcept->st_initval;
	if (exceptname && *exceptname) {
	    if ((timeexcept = st_find(Xst, exceptname)) != NULL) {
		if (timeexcept->st_body) {
		    modifier = exec_exception(Xtest, timeexcept->st_body);
		} 
	    } else {
		fi_printf(">> Could not find exception named '%s'\n");
	    }
	}
    }
    fi_printf ("\ttime limit (%d) exceeded\n", timelimit);
    test_res_printf ("%s/%s: FAILED; time limit; prog=%s;\n",
		     Subsuite, Xtest, key);
    sprintf(sig, "time limit; prog=%s;\n",key);
    statSig(sig);
    st_setfailure(Xst,"1");
    statProg(psp, TIMEOUT, (struct timeval *)NULL, (struct rusage *)NULL);
    if (modifier != A_IGNORE) {
	exec_toprog(psp, key, timelimit);
    }
}

int cpu_except(struct progstmt *psp, char *key, int cpulimit)
{
    struct symbol *cpuexcept;
    int modifier = A_CRIT;
    char buf[128];
    char *exceptname;
 
    sprintf(buf, "%d", cpulimit);
    ex_result("cputime", buf, buf, "");
    ex_proginfo(psp);
    if ((cpuexcept = st_find(Xst, "CPUEXCEPT")) != NULL) {
	exceptname = (cpuexcept->st_workval) ? cpuexcept->st_workval :
	  cpuexcept->st_initval;
	if (exceptname && *exceptname) {
	    if ((cpuexcept = st_find(Xst, exceptname)) != NULL) {
		if (cpuexcept->st_body) {
		    modifier = exec_exception(Xtest, cpuexcept->st_body);
		} 
	    } else {
		fi_printf(">> Could not find exception named '%s'\n");
	    }
	}
    }
    return (modifier == A_IGNORE) ? 0 : 1;
}
    
void exec_toprog(struct progstmt *psp, char *key, int timelimit)
{
    char *toprog = NULL;
    char timebuf[128];   /* hold name of timeout stdout and err */
    struct symbol *to_v;
    int retval;
    char cmdbuf[MAXPATHLEN];
    uid_t uid; 
    gid_t gid;	/* UID and GID in integer form */
    
    
    toprog = TOprog;
    if ((to_v = st_find(Xst, "TOPROGRAM")) != NULL) {
	toprog = to_v->st_workval ? to_v->st_workval 
	  : to_v->st_initval;
    }
    if (toprog && *toprog) {
	if (run_no >1) {
	    (void)sprintf(timebuf,"TIMEOUT.%d",run_no);
	} else {
	    (void)sprintf(timebuf,"TIMEOUT");
	} 
	sprintf(cmdbuf, 
		"sh -c '%s 1> %s.%s.SO 2> %s.%s.SE'",
		toprog, Xtest, timebuf, Xtest, timebuf);
	uid = cvt_uid( "-1", "timeout" );
	gid = cvt_gid( "-1", "timeout" );
	
	fprintf(Rfile,"\tExecuting timeout program: %s\n", toprog);
	fflush(Rfile);
	
	if ((retval=do_timeout(cmdbuf, "timeout", uid , gid,
			       FALSE, -1)) != 0) {
	    /* 
	     * just in case the child wrote to the reportfile, 
	     * seek to the end
	     */
	    fseek(Rfile, 0, SEEK_END);
	    if (retval == 1) {
		fputs("\tTimeout program timed out.\n", Rfile);
	    } else {
		fputs("\tNon-zero return from timeout program\n", Rfile);
	    }
	    fflush(Rfile);
	} 
	fseek(Rfile, 0, SEEK_END);
	fprintf(Rfile, "\t    Stdout in: %s.%s.SO\n", Xtest, timebuf);
	fprintf(Rfile, "\t    Stderr in: %s.%s.SE\n", Xtest, timebuf);
    }
}

/*---------------------------------------------------------------------------*
 *  static void exec_stmtlist (char *tn, TD_stmt *stmt) 
 * 
 *---------------------------------------------------------------------------*/
static void tet_exec_stmtlist (char *tn, TD_stmt *stmt, int indent) 
{
    int i;
    char *tmp;
    tmp = ind;
    ind = (char *) malloc(indent+1);
    for (i = 0; i < indent; i++) {
	ind[i] = ' ';
    }
    ind[i] = '\0';
    
    if (stmt == NULL) {
	if (Warn_of_export) {
	    fprintf(Rfile,">> Warning: Rule contains no executable progs.\n");
	}
	return;
    }
    for (; stmt; stmt = stmt->s_next) {
	switch(stmt->s_type) {
	  case FOREACH:  {
	      struct arg *lvs;
	      char *forvar;
	      char *args[50];
	      int numargs=0, i;
	      
	      lvs = STMT_GET_FORSTMT(stmt)->fs_loopvars;
	      while (lvs) {
		  args[numargs++] = tet_argref(lvs, "");
		  lvs = lvs->a_next;
	      }
	      forvar = STMT_GET_FORSTMT(stmt)->fs_forvar->st_name;
	      fprintf(TETout, "%sfor %s in \"", ind, forvar);
	      for (i = 0; i < numargs; i++) {
		  fprintf(TETout, "%s ", args[i]);
		  free(args[i]);
	      }
	      fprintf(TETout,"\"\n%sdo\n",ind);
	      fprintf(TETout,"%s    oldprogkey_TET=$progkey_TET\n", ind);
	      fprintf(TETout,"%s    progkey_TET=\"$%s\"\n", ind, forvar);
	      fprintf(TETout,"%s    if [ ! -z \"$oldprogkey_TET\" ]\n",ind);
	      fprintf(TETout,"%s    then\n", ind);
	      fprintf(TETout,
		      "%s        progkey_TET=\"$progkey_TET.$oldprogkey_TET\"",
		      ind);
	      fprintf(TETout,"\n%s    fi\n", ind);
	      tet_exec_stmtlist(tn, STMT_GET_FORSTMT(stmt)->fs_body,indent+4); 
	      fprintf(TETout,"%s    progkey_TET=$oldprogkey_TET\n", ind);
	      fprintf(TETout,"%sdone\n", ind);
	      break;
	  }
	  case SWITCH: {
	      struct switchcase *cl;
	      char *tmp;
	      fprintf(TETout, "%scase $%s in\n", ind, 
		      STMT_GET_SWITCHSTMT(stmt)->switchvar->st_name);
	      cl = STMT_GET_SWITCHSTMT(stmt)->caselist;
	      while (cl) {
		  if (cl->switchvar->a_type == STR) {
		      tmp = cl->switchvar->a_u.str_p;
		  } else if (cl->switchvar->a_type == VAR) {
		      tmp = cl->switchvar->a_u.var_p->st_name;
		  } else {
		      fprintf(stderr, 
			      "Error: switch case cannot be a function\n");
		      tmp = "*";
		  }
		  fprintf(TETout, "%s%s)\n", ind, tmp);
		  tet_exec_stmtlist(tn, cl->case_body,indent+4);
		  fprintf(TETout, "%s    ;;\n", ind);
		  cl = cl->nextcase;
	      }
	      fprintf(TETout, "%sesac\n", ind);
	      break;
	  }
	  case IF: 
	    fprintf(TETout, "%sif [ ", ind);
	    tet_EXPRT_xlate_expr(STMT_GET_IFSTMT(stmt)->if_exp, TETout);
	    fprintf(TETout, " ]\n%sthen\n", ind);
	    tet_exec_stmtlist(tn, STMT_GET_IFSTMT(stmt)->if_body,indent+4);
	    fprintf(TETout, "%sfi\n", ind);
	    break;
	  case ELSE:
	    fprintf(TETout, "%sif [ ", ind);
	    tet_EXPRT_xlate_expr(STMT_GET_IF_ELSE_STMT(stmt)->if_e_exp,TETout);
	    fprintf(TETout, " ]\n%sthen\n", ind);
	    tet_exec_stmtlist(tn, STMT_GET_IF_ELSE_STMT(stmt)->if_e_body,
			      indent+4);
	    fprintf(TETout, "%selse\n", ind);
	    tet_exec_stmtlist(tn, STMT_GET_IF_ELSE_STMT(stmt)->else_body,
			      indent+4);
	    fprintf(TETout, "%sfi\n", ind);
	    break;
	  case PROG:  {
	      tet_exec_prog(stmt->s_u.ps_p, indent);
	      break;
	  }
	  case STRSUB: {
	      tet_eval_strsub(STMT_GET_STRSUBSTMT(stmt), TETout, indent);
	      break;
	  }
	  case INTERSECT: {
	      tet_eval_intersect(STMT_GET_INTERSECTSTMT(stmt), TETout, indent);
	      break;
	  }
	  case SYSTEM: {
	      tet_eval_system(STMT_GET_SYSTEMSTMT(stmt), indent);
	      break;
	  }
	  case STRCAT: {
	      tet_eval_strcat(STMT_GET_STRCATSTMT(stmt), TETout, indent);
	      break;
	  }
	  case ASSIGN: {
	      tet_eval_assign(STMT_GET_ASSIGNSTMT(stmt), TETout, indent);
	      break;
	  }
	  case DEFINED:
	    break;
	  case ECHOT: 
	    {
		struct arg *arg;
		char *p;

		arg = stmt->s_u.ec_p->ec_arg;
		p = tet_argref(arg, "");
		fprintf(TETout,"%stet_infoline \"%s\"\n", ind, p);
		free(p);
		break;
	    }
	  case CHDIR:
	    {
		struct arg *arg;
		char *p;
		arg = stmt->s_u.cd_p->cd_arg;
		p = tet_argref(arg, "");
		fprintf(TETout,"%scd %s\n", ind, p);
		free(p);
		break;
	    }
	    
	  case VERSION: 
	    {
		struct arg *arg;
		char *p;
		arg = stmt->s_u.vs_p->vs_arg;
		p = tet_argref(arg, "");
		fprintf(TETout, "%stmp_TET=`which vers`\n", ind);
		fprintf(TETout, "%sif [ ! -z \"$tmp_TET\" ]\n%sthen\n", 
			ind, ind);
		fprintf(TETout,"%s    tet_infoline `vers %s`\n", ind, p);
		fprintf(TETout,"%selse\n", ind);
		fprintf(TETout,"%s    tet_infoline `file %s`\n", ind, p);
		fprintf(TETout, "%sfi\n", ind);
		free(p);
                break;
            }
	    
	  case GETENV:
            tet_exec_getenv( stmt->s_u.ge_p, indent );
            break;
	    
	  case SETENV:
            tet_exec_setenv( stmt->s_u.se_p, indent );
            break;
	    
	  case UNSETENV:
            tet_exec_unsetenv( stmt->s_u.ue_p, indent );
            break;
	    
	  case CLEARENV:
            tet_exec_clearenv( stmt->s_u.ce_p, indent );
            break;
	    
	  case BUILTIN:
            free(eval_builtin( stmt->s_u.bt_p->bt_arg, B_EXEC ));
            break;
	    
	  default: assert(0);
	}
	fflush(TETout);
    }
    free(ind);
    ind = tmp;
}

static void tet_exec_prog (struct progstmt *psp, int indent)
{
    struct loopvar *lv;
    struct symbol *tca_v;       /* sym pointer for variable with exec nm */
    char *tca_exec;             /* values of above syms */
    char *fallkey; 
    char *out, *err, *rc, *in, *diff, *gran;
    struct arg *tmp;
    char *ind, *progargs[100];
    int i, numargs=0;
    TD_arg *arg;

    ind = (char *) malloc(indent+1);
    for (i = 0; i < indent; i++) {
	ind[i] = ' ';
    }
    ind[i] = '\0';
    /* first build up necessary stuff based on the prog attributes */
    fprintf(TETout,"%smach_TET=\"\"\n", ind);
    fprintf(TETout,"%shead_TET=\"\"\n", ind);
    fprintf(TETout,"%smode_TET=\"\"\n", ind);
    fprintf(TETout,"%smemory_TET=\"\"\n", ind);
    if (psp->ps_att[A_GRANULARITY].at_disp == A_FILE) {
	gran = tet_argref(psp->ps_att[A_GRANULARITY].at_file, "'");
	fprintf(TETout,"%sgran_TET=%s\n", ind, gran);
	free(gran);
	fprintf(TETout,
		"%sTET_gran=`echo $gran_TET | sed -e \"s/\\(.\\)/\\1 /g\"`",
		ind);
	fprintf(TETout,"%sfor tmp_TET in $gran_TET\n%sdo\n", ind, ind);
	fprintf(TETout,"%s    case $tmp_TET in\n", ind);
	fprintf(TETout,"%s    m)\n", ind);
	fprintf(TETout,"%s        mach_TET=`uname -m`\n", ind);
	fprintf(TETout,"%s        mach_TET=\"$mach_TET.\"\n", ind);
	fprintf(TETout,"%s        ;;\n", ind);
	fprintf(TETout,"%s    h)\n", ind);
	fprintf(TETout,"%s        head_TET=`getsysinfo -fcpu_count`\n", ind);
	fprintf(TETout,"%s        head_TET=\"$head_TET-head.\"\n", ind);
	fprintf(TETout,"%s        ;;\n", ind);
	fprintf(TETout,"%s    f)\n", ind);
	fprintf(TETout,"%s        if `getsysinfo -fnative_default`\n", ind);
	fprintf(TETout,"%s        then\n", ind);
	fprintf(TETout,"%s            mode_TET=\"native.\"\n", ind);
	fprintf(TETout,"%s        else\n", ind);
	fprintf(TETout,"%s            mode_TET=\"ieee.\"\n", ind);
	fprintf(TETout,"%s        fi\n", ind);
	fprintf(TETout,"%s        ;;\n", ind);
	fprintf(TETout,"%s    i)\n", ind);
	fprintf(TETout,
		"%s        memory_TET=`getsysinfo -fmem_interleave_factor`\n",
		ind);
	fprintf(TETout,"%s        memory_TET=`echo $memory_TET | sed -e \"s/mem_interleave_factor //\"`\n", ind);
	fprintf(TETout,"%s        memory_TET=\"$memory_TET-way.\"\n", ind);
	fprintf(TETout,"%s        ;;\n", ind);
	fprintf(TETout,"%s    esac\n", ind);
	fprintf(TETout,"%sdone\n", ind);
    } else {
	if (psp->ps_att[A_MACHINE].at_disp == A_DISTINCT) {
	    fprintf(TETout,"%smach_TET=`uname -m`\n", ind);
	    fprintf(TETout,"%smach_TET=\"$mach_TET.\"\n", ind);
	}
	if (psp->ps_att[A_HEADS].at_disp == A_DISTINCT) {
	    fprintf(TETout,"%shead_TET=`getsysinfo -fcpu_count`\n", ind);
	    fprintf(TETout,"%shead_TET=\"$head_TET-head.\"\n", ind);
	}
	if (psp->ps_att[A_FPMODE].at_disp == A_DISTINCT) {
	    fprintf(TETout,"%sif `getsysinfo -fnative_default`\n", ind);
	    fprintf(TETout,"%sthen\n", ind);
	    fprintf(TETout,"%s    mode_TET=\"native.\"\n", ind);
	    fprintf(TETout,"%selse\n", ind);
	    fprintf(TETout,"%s    mode_TET=\"ieee.\"\n", ind);
	    fprintf(TETout,"%sfi\n", ind);
	}
	if (psp->ps_att[A_MEMORY].at_disp == A_DISTINCT) {
	    fprintf(TETout,
		    "%smemory_TET=`getsysinfo -fmem_interleave_factor`\n",
		    ind);
	    fprintf(TETout,
		    "%smemory_TET=`echo $memory_TET | sed -e \"s/mem_interleave_factor //\"`\n", ind);
	    fprintf(TETout,"%smemory_TET=\"$memory_TET-way.\"\n", ind);
	}
    }
    fprintf(TETout,
	    "%sthisprogkey_TET=\"$mach_TET$head_TET$mode_TET$memory_TET$progkey_TET\"\n",
	    ind);
    fprintf(TETout, "%sthisprogkey_TET=\"%s.p.%s.$thisprogkey_TET\"\n", ind,
	    Xtest, psp->progid);
    fprintf(TETout,
	    "%sthisprogkey_TET=`echo $thisprogkey_TET | sed -e \"s/\\.$//\"`\n",
	    ind);
    
    if (psp->ps_att[A_STDOUT].at_disp & A_CHECK) {
	out = "1>$thisprogkey_TET.SO";
    } else {
	out = "1>/dev/null";
    }
    if (psp->ps_att[A_STDERR].at_disp & A_CHECK) {
	err = "2>$thisprogkey_TET.SE";
    } else {
	err = "2>/dev/null";
    }
    in = strsave("");
    if (psp->ps_att[A_STDIN].at_disp == A_FILE) {
	if (psp->ps_att[A_STDIN].at_file->a_type == STR) {
	    in = malloc(strlen(psp->ps_att[A_STDIN].at_file->a_u.str_p) + 3);
	    sprintf(in, "< %s", psp->ps_att[A_STDIN].at_file->a_u.str_p);
	} else if (psp->ps_att[A_STDIN].at_file->a_type == VAR) {
	    struct symbol *var = psp->ps_att[A_STDIN].at_file->a_u.var_p;
	    if (*(var->st_name) == REDIRECT_XLAT) {
		char *val, *tmp;
		val = (var->st_workval) ? var->st_workval : var->st_initval;
		if (*val == REDIRECT_XLAT) {
		    val++;
		}
		val = strsave(val);
		tmp=strtok(val, ".");
		in = malloc(strlen(tmp)+15);
		sprintf(in,"< $%sprog_TET.%s", tmp, strtok(NULL, "."));
		free(val);
	    } else {
		in = malloc(strlen(var->st_name) + 4);
		sprintf(in, "< $%s",var->st_name);
	    }
	} else {
	    in = strsave("< /dev/null");
	}
    }
    if (psp->ps_att[A_DIFFCMD].at_disp == A_DIFF) {
	diff = tet_argref(psp->ps_att[A_DIFFCMD].at_file, "");
    } else {
	diff = strsave("diff");
    }

    progargs[numargs++] = tet_argref(psp->ps_name, "'");
    arg = psp->ps_args;
    while (arg) {
	progargs[numargs++] = tet_argref(arg, "\\\"");
	arg = arg->a_next;
    }
    fprintf(TETout, "%scmd_TET=\"%s ", ind, progargs[0]);
    free(progargs[0]);
    for (i = 1; i < numargs; i++) {
	fprintf(TETout, "%s ", progargs[i]);
	free(progargs[i]);
    }
    fprintf(TETout,"%s %s %s\"\n", in, out, err);
    fprintf(TETout,"%seval $cmd_TET\n", ind);
    fprintf(TETout,"%src=$?\n", ind);
    if (psp->ps_att[A_RC].at_disp & A_CHECK) {
	fprintf(TETout, 
		"%sexp_rc=`grep $thisprogkey_TET Results/RC | sed -e \"s/${thisprogkey_TET}: //\"`\n", ind);
	fprintf(TETout,
		"%scheck=check_rc $rc ${exp_rc:-0} $thisprogkey_TET \"$cmd_TET\"\n", ind);
	tet_add_quit_code(psp->ps_att[A_RC], ind);
    }
    if ((psp->ps_att[A_STDOUT].at_disp & A_CHECK) && 
	psp->ps_att[A_STDOUT].at_disp != A_IGNORE) {
	fprintf(TETout,"%scheck=check_result $thisprogkey_TET.SO Results/Stdout/$thisprogkey_TET.so $thisprogkey_TET \"$cmd_TET\" \"%s\"\n", ind, diff);
	tet_add_quit_code(psp->ps_att[A_STDOUT], ind);
    }
    if ((psp->ps_att[A_STDERR].at_disp & A_CHECK) && 
	psp->ps_att[A_STDERR].at_disp != A_IGNORE) {
	fprintf(TETout,"%scheck=check_result $thisprogkey_TET.SE Results/Stderr/$thisprogkey_TET.se $thisprogkey_TET \"$cmd_TET\" \"%s\"\n", ind, diff);
	tet_add_quit_code(psp->ps_att[A_STDERR], ind);
    }
    fprintf(TETout, "%sprevprog_TET=$thisprogkey_TET\n", ind);
    fprintf(TETout, "%s%sprog_TET=$thisprogkey_TET\n", ind, psp->progid);
    if (in) {
	free (in);
    }
    if (diff) {
	free (diff);
    }
    free(ind);
    
    return;
}

void tet_add_quit_code(TD_att att, char *ind)
{
    if (att.at_disp == A_CRIT) {
	fprintf(TETout, "%sif $check\n", ind);
	fprintf(TETout, "%sthen\n", ind);
	fprintf(TETout, "%s    return\n", ind);
	fprintf(TETout, "%sfi\n", ind);
    } else if (att.at_disp == A_EXIT) {
	fprintf(TETout, "%sif $check\n", ind);
	fprintf(TETout, "%sthen\n", ind);
	fprintf(TETout, "%s    FAIL=\"U\"\n", ind);
	fprintf(TETout, "%s    return\n", ind);
	fprintf(TETout, "%sfi\n", ind);
    }
}
	
/* exec_getenv - process a getenv statement */
static void tet_exec_getenv( struct getenvstmt *ssp, int indent )
{
    struct arg *np;		/* Variable name */
    struct arg *vp;		/* Variable value */
    char *val, *ind;
    int i;
    
    np = ssp->ge_args[0];
    vp = ssp->ge_args[1];
    
    ind = (char *)malloc(indent+1);
    for (i = 0; i < indent; i++) {
	ind[i] = ' ';
    }
    ind[i] = '\0';

    if (vp->a_type == STR || vp->a_type == FUN) {
	val = tet_argref(vp, "");
	fprintf(TETout, "%s%s=$%s\n", ind, np->a_u.var_p->st_name, val);
	free(val);
    } else {
	fprintf(TETout, "%s%s=`printenv $%s`\n", ind, np->a_u.var_p->st_name,
		vp->a_u.var_p->st_name);
    }
    free(ind);
    
    return;
}


/* exec_setenv - process a setenv statement */
static void tet_exec_setenv(struct setenvstmt *ssp, int indent)
{
    struct arg *np;		/* Variable name */
    struct arg *vp;		/* Variable value */
    char *val, *ind, *name;
    int i;
    
    np = ssp->se_args[0];
    vp = ssp->se_args[1];
    
    ind = (char *)malloc(indent+1);
    for (i = 0; i < indent; i++) {
	ind[i] = ' ';
    }
    ind[i] = '\0';

    name=tet_argref(np, "");
    val = tet_argref(vp, "\"");
    fprintf(TETout, "%seval \"%s='%s'\"\n", ind, name ,val);
    fprintf(TETout, "%seval \"export %s\"\n", ind, name);
    free(name);
    free(val);
    free(ind);
    return;
}


/* exec_unsetenv - process an unsetenv statement */
static void tet_exec_unsetenv(struct unsetenvstmt *usp, int indent)

{
    struct arg *np;		/* Variable name */
    struct arg *vp;		/* Variable value */
    char *val, *ind;
    int i;
    
    np = usp->ue_arg;
    
    ind = (char *)malloc(indent+1);
    for (i = 0; i < indent; i++) {
	ind[i] = ' ';
    }
    ind[i] = '\0';

    val = tet_argref(np, "");
    fprintf(TETout, "%seval \"unset %s\"\n", ind, val);
    free(val);
    free(ind);
    return;
}

/* exec_clearenv - process a clearenv statement */
static void tet_exec_clearenv(struct clearenvstmt *csp, int indent)
{
    char *val, *ind;
    int i;
    
    ind = (char *)malloc(indent+1);
    for (i = 0; i < indent; i++) {
	ind[i] = ' ';
    }
    ind[i] = '\0';
    
    fprintf(TETout, "%sfor tmp_TET in `printenv`\n%sdo\n", ind, ind);
    fprintf(TETout, "%s    tmp2_TET=`echo $tmp_TET | sed -e \"s/=.*//\"`\n",
	    ind);
    fprintf(TETout, "%s    if [ $tmp2_TET != \"PATH\" -a $tmp2_TET != \"PS1\" -a $tmp2_TET != \"PS2\" -a $tmp2_TET != \"MAILCHECK\" -a $tmp2_TET != \"IFS\" ] ;\n",
	    ind);
    fprintf(TETout, "%s    then\n", ind);
    fprintf(TETout, "%s        unset $tmp2_TET\n", ind);
    fprintf(TETout, "%s    fi\n", ind);
    fprintf(TETout, "%sdone\n", ind);
    free(ind);
    return;
}

void tet_eval_system (struct systemstmt *ssp, int indent)
{
    char *t1, *t2, t3[20];
    char *redir;
    char *ind, *tmp;
    int i;
    
    ind = (char *)malloc(indent+1);
    for (i = 0; i < indent; i++) {
	ind[i] = ' ';
    }
    ind[i] = '\0';

    if (strcmp(ssp->sy_args[1]->a_u.var_p->st_name, 
	       ssp->sy_args[2]->a_u.var_p->st_name)) {
	redir = "1>/tmp/TD_sysout$$ 2>/tmp/TD_syserr$$";
    } else {
	redir = "1>/tmp/TD_sysout$$ 2>&1";
    }
    tmp = tet_argref(ssp->sy_args[0], "");
    fprintf(TETout,"%s%s %s\n", ind, tmp, redir);
    free(tmp);
    if (ssp->sy_args[3] != NULL) {
	fprintf(TETout, "%s%s=$?\n", ind, ssp->sy_args[3]->a_u.var_p->st_name);
    }
    if (strcmp(ssp->sy_args[1]->a_u.var_p->st_name, 
	       ssp->sy_args[2]->a_u.var_p->st_name)) {
	fprintf(TETout,"%s%s=`cat /tmp/TD_sysout$$`\n", ind, 
		ssp->sy_args[1]->a_u.var_p->st_name);
	fprintf(TETout,"%s%s=`cat /tmp/TD_syserr$$`\n", ind, 
		ssp->sy_args[2]->a_u.var_p->st_name);
    } else {
	fprintf(TETout,"%s%s=`cat /tmp/TD_sysout$$`\n", ind, 
		ssp->sy_args[1]->a_u.var_p->st_name);
    }
    free(ind);

}

int numgen = 0; /* used for generating new temporary variables for use in
		   the script */
char sym[11]; /* large enough to hold sym9999_TET */

char *tet_gensym(void)
{
    sprintf(sym, "sym%d_TET", numgen++);
    return sym;
}

extern char *eval_shift(TD_arg *);
char *exec_shift(TD_arg *args)
{
    char *topvar, *shiftvar;
    char *arg, *ret;
    int numargs = 0;


    if (TET) {
	arg = tet_argref(args, "\"");
	if (*arg == '$') {
	    shiftvar = strsave(args->a_u.var_p->st_name);
	} else {
	    shiftvar = strsave(tet_gensym());
	}
	topvar = strsave(tet_gensym());
	fprintf(TETout, "%smyshift \"%s\" \"%s\" %s\n", ind, topvar, shiftvar, 
		arg);
	free(shiftvar);
	free(arg);
	ret = malloc(strlen(topvar)+2);
	sprintf(ret, "$%s", topvar);
	free(topvar);
	return ret;
    } else {
	return eval_shift(args);
    }
}

extern char *eval_numcmp(TD_arg *);

char *exec_numcmp(TD_arg *args)
{
    char *str, *tmp, *format, c;
    TD_arg *ap;
    char *arg1;
    char *arg2;
    char *arg3;

    if (TET) {
	arg1 = tet_argref(args, "\"");
	arg2 = tet_argref(args->b_args, "\"");
	arg3 = tet_argref(args->b_args->b_args, "\"");
	tmp = strsave(tet_gensym());
    } else {
	return eval_numcmp(args);
    }
}

char *exec_save(TD_arg *args)
{
    struct arg *argument;
    ST_symbol *st_ptr;

    if(!TET) {

        for(argument = args; argument; argument = argument -> b_args){
            if(argument -> a_type == VAR) {
                argument -> a_u.var_p -> save = 1;
	    }else{
                st_ptr = st_find(Xst,argument -> a_u.str_p);
                st_ptr -> save = 1;
            }
        }
    }else{
        return NULL;
    }
}

char *exec_quit(TD_arg *args)
{
    if (!V) {
	fi_reset();
    }
    longjmp(Knownfail, -1);
    return NULL;
}
	

void tet_eval_assign (struct assignstmt *scp, FILE *fptr, int indent)
{
    char	*ind;
    int i;
    struct	arg	*ap;
    char *asgargs[100];
    int numargs=0;
    
    ind = malloc(indent+1);
    for (i = 0; i < indent; i++)
      ind[i] = ' ';
    ind[i] = '\0';
    
    for (ap = scp->rhs; ap; ap = ap->a_next)  {
	asgargs[numargs++] = tet_argref(ap, "");
    }
    if (scp->lhs->st_class == EXPORT) {
	fprintf(fptr, "%s%s_tet=\"", ind, scp->lhs->st_name);
    } else {
	fprintf(fptr, "%s%s=\"", ind, scp->lhs->st_name);
    }
    for (i = 0; i< numargs; i++) {
	fprintf(fptr, "%s", asgargs[i]);
	free(asgargs[i]);
    }
    fprintf(fptr, "\"\n");
    free(ind);
}

void tet_eval_strcat (struct strcatstmt *scp, FILE *fptr, int indent)
{
    char	*ind;
    int i;
    struct	arg	*ap;
    char *catargs[100];
    int numargs = 0;


    ind = malloc(indent+1);
    for (i = 0; i < indent; i++)
      ind[i] = ' ';
    ind[i] = '\0';

    for (ap = scp->sc_args->a_next; ap; ap = ap->a_next) {
	catargs[numargs++] = tet_argref(ap, "");
    }
    if (scp->sc_args->a_u.var_p->st_class == EXPORT) {
	fprintf(fptr, "%s%s_tet=\"", ind, 
		scp->sc_args->a_u.var_p->st_name);
    } else {
	fprintf(fptr, "%s%s=\"", ind, scp->sc_args->a_u.var_p->st_name);
    }
    
    for (i = 0; i < numargs; i++)  {
	fprintf(fptr,"%s", catargs[i]);
	free(catargs[i]);
    }
    fprintf(fptr, "\"\n");
    free(ind);
}

void tet_eval_strsub (struct strsubstmt *ssp, FILE *fptr, int indent)
{
    char	*ind;
    int i;
    char *arg1, *arg2, *arg3;

    ind = malloc(indent+1);
    for (i = 0; i < indent; i++)
      ind[i] = ' ';
    ind[i] = '\0';

    arg1 = tet_argref(ssp->ss_args[1], "");
    arg2 = tet_argref(ssp->ss_args[2], "");
    arg3 = tet_argref(ssp->ss_args[3], "");
    if (ssp->ss_args[0]->a_u.var_p->st_class == EXPORT) {
	fprintf(fptr, "%s%s_tet=`", ind, ssp->ss_args[0]->a_u.var_p->st_name);
    } else {
	fprintf(fptr, "%s%s=`", ind, ssp->ss_args[0]->a_u.var_p->st_name);
    }
    fprintf(fptr, "echo \"%s\" | ", arg2);
    fprintf(fptr, "sed -e \"s/%s/", arg3);
    fprintf(fptr, "%s/\"`\n", arg1);
    free(arg1);
    free(arg2);
    free(arg3);
    free(ind);
    return;
}

void tet_eval_intersect (struct intersectstmt *inp, FILE *fptr, int indent)
{
    char	*ind;
    int i;
    char *arg1, *arg2;

    ind = malloc(indent+1);
    for (i = 0; i < indent; i++)
      ind[i] = ' ';
    ind[i] = '\0';
    
    arg1 = tet_argref(inp->in_args[1], "\"");
    arg2 = tet_argref(inp->in_args[2], "\"");
    fprintf(fptr, "%stmp_TET=\"\"\n", ind);
    fprintf(fptr, "%sfor tmp1_TET in ", ind);
    fprintf(fptr, "%s\n", arg1);
    fprintf(fptr, "%sdo\n", ind);
    fprintf(fptr, "%s    for tmp2_TET in ", ind);
    fprintf(fptr, "%s\n", arg2);
    fprintf(fptr,"%s    do\n", ind);
    fprintf(fptr,"%s        if [ $tmp1_TET = $tmp2_TET ]\n", ind);
    fprintf(fptr,"%s        then\n", ind);
    fprintf(fptr,"%s            tmp_TET=\"$tmp_TET $tmp1_TET\"\n", ind);
    fprintf(fptr,"%s            break\n", ind);
    fprintf(fptr,"%s        fi\n", ind);
    fprintf(fptr,"%s    done\n", ind);
    fprintf(fptr,"%sdone\n", ind);
    if (inp->in_args[0]->a_u.var_p->st_class == EXPORT) {
	fprintf(fptr, "%s%s_tet=$tmp_TET\n", ind, 
		inp->in_args[0]->a_u.var_p->st_name);
    } else {
	fprintf(fptr, "%s%s=$tmp_TET\n", ind, 
		inp->in_args[0]->a_u.var_p->st_name);
    }
    free(arg1);
    free(arg2);
    free(ind);
    return;
}

char *tet_argref(TD_arg *arg, char *strquot)
{
    char *tmp;
    char *postfix = "";
    if (arg == NULL) {
	tmp = malloc(strlen(strquot)*2+1);
	sprintf(tmp,"%s%s", strquot, strquot);
    } else {
	switch (arg->a_type) {
	  case STR:
	    tmp = malloc(strlen(arg->a_u.str_p) + strlen(strquot)*2 +1);
	    sprintf(tmp,"%s%s%s", strquot, arg->a_u.str_p, strquot);
	    break;
	  case VAR:
	    tmp = malloc(strlen(arg->a_u.var_p->st_name)+6);
	    if (arg->a_u.var_p->st_class == EXPORT) {
		postfix="_tet";
	    }
	    sprintf(tmp, "$%s%s", arg->a_u.var_p->st_name, postfix);
	    break;
	  case FUN:
	    tmp = eval_builtin(arg, B_EXEC);
	    break;
	}
    }
    return tmp;
}
