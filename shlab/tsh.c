/*
 * tsh - A tiny shell program with job control
 *
 * <Put your name and login ID here>
 * shc 200110732
 *
 * shell执行一系列的读/求值步骤，然后终止
 *  1. 读步骤       读取来自用户的一个命令行     fgets
 *  2. 求值步骤     解析命令行，并代表用户运行   eval
 *      2.1     前台 or 后台                                     parseline
 *      2.2     whether shell内置命令 or executable可执行文件     builtin_command ?
 *      2.3     not builtin_command  --- execve / builtin_command --- execute immediately
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * **At most 1 job can be in the FG state.**
 */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */

struct job_t
{                          /* The job struct */
    pid_t pid;             /* job PID */
    int jid;               /* job ID [1, 2, ...] */
    volatile int state;             /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE]; /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */ //  记录tsh下的各个job的状态（只记录tsh亲自fork出的子进程的作业状态）
/* End global variables */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv); //  TODO bg/fg
void do_bgfg(char **argv);    //  TODO
void waitfg(pid_t pid);       //  ok
// void waitfg(sigset_t *);
void do_kill(char **argv);

void sigchld_handler(int sig); //  ok
void sigtstp_handler(int sig); //  ok
void sigint_handler(int sig);  //  ok
// void sigchld_handler2(int sig);                 //  ok

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

static size_t sio_strlen(char s[]);
ssize_t sio_puts(char s[]);

volatile sig_atomic_t foreground_child_over_flag = 0;

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF)
    {
        switch (c)
        {
        case 'h': /* print help message */
            usage();
            break;
        case 'v': /* emit additional diagnostic info */
            verbose = 1;
            break;
        case 'p':            /* don't print a prompt */
            emit_prompt = 0; /* handy for automatic testing */
            break;
        default:
            usage();
        }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT, sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1)
    {
        /* Read command line */
        if (emit_prompt)
        {
            printf("%s", prompt);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        if (feof(stdin))
        { /* End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }

        /* Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
        fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
 */
void eval(char *cmdline)
{
    char *argv[MAXARGS]={0};
    int is_bg = parseline(cmdline, argv);
    int is_builtin = builtin_cmd(argv);

    if (!is_builtin) //  not builtin
    {
        int pid = 0;
        sigset_t mask, backup_mask, mask_all;
        sigfillset(&mask_all);
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, &backup_mask); //  block child 在fork之前 main将SIGCHLD阻塞，防止还没有将其addjoblists，子进程就结束了。

        //  child process runs user job
        if ((pid = fork()) == 0)
        {
            sigprocmask(SIG_SETMASK, &backup_mask, NULL); //  unblock child

            Signal(SIGINT, SIG_DFL);  //  change to default (so child progess would terminated instead of forwarding the signal)
            Signal(SIGTSTP, SIG_DFL); //  change to default (so child progess would stopped instead of forwarding the signal)

            setpgid(0, 0); //  puts the child in a new process group whose group ID is identical to the child’s PID.
            if (execve(argv[0], argv, environ) < 0)
            { //  execve
                printf("%s: Command not found\n", argv[0]);
            }
            exit(0); //  never reach unless child error
        }

        // printf("DEBUG IS_BG %d\n",is_bg);
        //  parents wait for children
        if (!is_bg)
        {
            sigprocmask(SIG_BLOCK, &mask_all, NULL);      //  handler 和 main 为并发关系。且他们均有可能操作jobs structure ，因此要先将所有信号阻塞 防止打断
            addjob(jobs, pid, FG, cmdline);               //  add to list as FG
            sigprocmask(SIG_SETMASK, &backup_mask, NULL); //  parent 恢复 mask
            waitfg(pid);                                  //  wait foreground的all progess
        }
        else
        {
            sigprocmask(SIG_SETMASK, &mask_all, NULL);    //  handler 和 main 为并发关系（在main addjob时，handler可能会被某些信号触发（除了之前屏蔽的SIGCHLD，在handler中有可能改变jobs结构）。且他们均有可能操作jobs structure ，因此要先将所有信号阻塞 防止打断
            addjob(jobs, pid, BG, cmdline);               //  add to list as BG
            sigprocmask(SIG_SETMASK, &backup_mask, NULL); //  parent 恢复 mask  / unlock child
            printf("[%d] (%d) %s",pid2jid(pid),pid,cmdline);      //  print
        }
    }

    return;
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv)
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';   /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'')
    {
        buf++;
        delim = strchr(buf, '\'');
    }
    else
    {
        delim = strchr(buf, ' ');
    }

    while (delim)
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;

        if (*buf == '\'')
        {
            buf++;
            delim = strchr(buf, '\'');
        }
        else
        {
            delim = strchr(buf, ' ');
        }
    }
    argv[argc] = NULL;

    if (argc == 0) /* ignore blank line */
        return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
    {
        argv[--argc] = NULL;
    }
    return bg;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv)
{
    // printf("DEBUG %s %d\n",argv[0],!strncmp(argv[0], "jobs",4));
    // printf("DEBUG %s\n",argv[0]);
    if (!strncmp(argv[0], "quit",4))
    {
        exit(0);
    }
    else if (!strncmp(argv[0], "jobs",4))
    {
        listjobs(jobs);
        return 1;
    }
    else if (!strncmp(argv[0], "bg",2) || !strncmp(argv[0], "fg",2))
    {
        do_bgfg(argv);
        return 1;
    }
    else if (!strncmp(argv[0], "kill",4))
    {
        do_kill(argv);
        return 1;
    }
    else if (!strncmp(argv[0], "&",1))
    {
        // continue
        return 1;
    }
    return 0; /* not a builtin command */
}

void do_kill(char **argv)
{
    sigset_t mask_all, backup_mask;
    sigfillset(&mask_all);
    sigprocmask(SIG_SETMASK, &mask_all, &backup_mask);

    int to_jid = atoi(argv[1]);
    // for(int i)
    struct job_t *to_job = getjobjid(jobs, to_jid);
    deletejob(jobs, to_job->pid);

    sigprocmask(SIG_SETMASK, &backup_mask, NULL);
}

/*
 * do_bgfg - Execute the builtin bg and fg commands
 *  The bg <job> command restarts <job> by sending it a SIGCONT signal,
 *  and then runs it inthe background.
 *  The <job> argument can be either a PID or a JID
 *  bg %1 在后台运行已经被停止的JID=1的prog
 *  bg 1  在后台运行已经被停止的PID=1的prog
 *
 *  The fg <job> command restarts <job> by sending it a SIGCONT signal,
 *  and then runs it in the foreground.
 *  The <job> argument can be either a PID or a JID.
 *  fg %1 在前台运行已经被停止的JID=1的prog
 *  fg  1 在前台运行已经被停止的PID=1的prog
 */
void do_bgfg(char **argv)
{
    struct job_t *job;
    int jid;
    int pid;
    const char *tmp = argv[1];
    if (tmp == NULL)
    {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }
    //  jid
    if (tmp[0] == '%')
    {
        if ((tmp + 1) == NULL)
        {
            sio_puts("No such job");
            return;
        }

        //  get jid
        jid = atoi(&tmp[1]);

        //  get job
        job = getjobjid(jobs, jid);

        //  get pid
        if (job == NULL)
        {
            printf("%s: No such job\n", tmp);
            return;
        }
        pid = job->pid;
        
    }
    //  pid
    else if (isdigit(tmp[0]))
    {
        //  get pid
        pid = atoi(&tmp[0]);
        //  get job
        job = getjobpid(jobs, pid);
        if (job == NULL)
        {
            printf("(%d): No such process\n", pid);
            return;
        }
        //  get jid
        jid = job->jid;
    }
    else
    {
        printf("%s argument must be a PID or %%jobid\n", argv[0]);
        return;
    }

    //  无论是bg还是fg，都会将停止的进程运行起来
    //  所以先运行起来 先发个SIGCONT信号
    //  被停止了 则继续运行。没被停止则continue
    kill(-(job->pid), SIGCONT);

    if (!strncmp(argv[0], "fg",2))
    {
        job->state = FG;
        waitfg(pid); //  占据中断。后台进程切换到前台
    }
    else
    {
        job->state = BG;
        printf("[%d] (%d) %s", jid, pid, job->cmdline);
    }

    return;
}

/*
 * Parent(Shell) waits for foreground job to terminate or stop
                                        terminate  ctrl + c(SIGINT) / exit(0)
                                        stop       ctrl + z(SIGTSTP)
 * waitfg - Block until process pid is no longer the foreground process
 * 等待前台进程组结束
 *
 * One of the tricky parts of the assignment is deciding on the allocation of work between the waitfg
    and sigchld handler functions. We recommend the following approach:
    – In waitfg, use a busy loop around the sleep function.
    – In sigchld handler, use exactly one call to waitpid.
 */
void waitfg(pid_t pid)
{
    //  如果pid对应的prog不存在
    if (getjobpid(jobs, pid) == NULL)
    {
        return;
    }
    //  spin 自旋
    //  当foreground进程 还是 pid 时 那么一直自旋等待 (占据终端等待)
    //  消耗性能！
    //  应当有更好的办法? sigsuspend ?
    while (pid == fgpid(jobs))
    {
        //  如果在这里也waitpid的话，那么会和sigchld handler产生data race
        //  child产生了SIGCHLD之后，谁来handle？handler还是waitfg ? 一个信号只能接收一次。会产生竞争，。
        // printf("job->state = %d\n",getjobpid(jobs, pid)->state);
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/*
 * BG的CHILD terminate / stop 之后 ，触发sigchld_handler
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job **terminates** (becomes a zombie), or **stops** because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.
 */
void sigchld_handler(int sig)
{
    int backup_errno = errno;
    int status;
    int pid;
    //   WNOHANG 如果等待集合中，没有可回收的，就跳出循环
    //   == 0 剩余的子进程都没terminate/stop
    //   == -1 error
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) //  不阻塞 + 终止 / 停止
    {                                                             //  不加WUNTRACED呢？停止的就wait不到？
        if (WIFEXITED(status))                                    //  exist
        {
            sigset_t mask_all, backup_mask;
            sigfillset(&mask_all);

            deletejob(jobs, pid); //  delete from list

            sigprocmask(SIG_SETMASK, &backup_mask, NULL);
            // char s[100]={0};
            // sprintf(s,"child %d terminated normally with exit status %u\n", pid, WEXITSTATUS(status));
            // sio_puts(s);
        }
        else if (WIFSIGNALED(status)) //  killed by signal
        {
            struct job_t *job = getjobpid(jobs, pid);
            sigset_t mask_all, backup_mask;
            sigfillset(&mask_all);

            deletejob(jobs, pid); //  delete from list

            sigprocmask(SIG_SETMASK, &backup_mask, NULL);

            char s[100] = {0};
            sprintf(s, "Job [%d] (%d) killed by signal %d\n", job->jid, job->pid, WTERMSIG(status));
            sio_puts(s);
        }
        else if (WIFSTOPPED(status)) //  stopped by signal
        {
            struct job_t *job = getjobpid(jobs, pid);
            job->state = ST; //  改变记录的运行状态 改为stopped

            char s[100] = {0};
            sprintf(s, "Job [%d] (%d) stopped by signal %d\n", job->jid, job->pid, WSTOPSIG(status));
            sio_puts(s);
        }
    }
    // wait 阻塞时才用
    // if(errno!=ECHILD){
    //     printf("wait error ? %s\n",strerror(errno));
    // }
    errno = backup_errno;
    return;
}

/*
 * forward signal to foreground group
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  **Catch it and send it along
 *    to the foreground job.**
 */
void sigint_handler(int sig)
{
    pid_t fore_pid = fgpid(jobs);
    // printf("sigint_handler %d\n",fore_pid);
    if (fore_pid == 0)
        return;

    kill(-fore_pid, sig);           //  forward to the foreground
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig)
{
    pid_t fore_pid = fgpid(jobs);
    // printf("sigtstp_handler %d\n",fore_pid);
    if (fore_pid == 0)
        return;
    kill(-fore_pid, sig);       //  forward to the foreground
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job)
{
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs)
{
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid > max)
            max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == 0)
        {
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;
            strcpy(jobs[i].cmdline, cmdline);
            if (verbose)
            {
                printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
        }
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == pid)
        {
            clearjob(&jobs[i]);
            nextjid = maxjid(jobs) + 1;
            return 1;
        }
    }
    return 0;
}

// fgpid - Return PID of current foreground job (and it is the leader of the foreground), 0 if no such job
//  jobs中记录的前台进程 同一时刻只有一个 就是我们运行在tsh前台的进程
//  且 这个FG 进程 ，是它所在进程组的组长 (因为这个进程自己独立出一个进程组)
//  而，这个foreground进程，自己fork出来的进程（也就是foreground group的其他进程，是不会记录在jobs中的，只有tsh的main中fork出的作业才会被加入到jobs中记录）
pid_t fgpid(struct job_t *jobs)
{
    // printf("DEBUG INTO FGPID");
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].state == FG)
            return jobs[i].pid;
    
    // printf("DEBUG OUTOF FGPID");

    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
            return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid)
{
    int i;

    if (jid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid == jid)
            return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
        {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid != 0)
        {
            printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
            switch (jobs[i].state)
            {
            case BG:
                printf("Running ");
                break;
            case FG:
                printf("Foreground ");
                break;
            case ST:
                printf("Stopped ");
                break;
            default:
                printf("listjobs: Internal error: job[%d].state=%d ",
                       i, jobs[i].state);
            }
            printf("%s", jobs[i].cmdline);
        }
    }
}
/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void)
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}

//  sio_puts async-signal-safety
static size_t sio_strlen(char s[])
{
    int i = 0;
    while (s[i] != '\0')
        ++i;
    return i;
}

ssize_t sio_puts(char s[])
{
    return write(STDOUT_FILENO, s, sio_strlen(s));
}

//  send SIGTSTP to forground progess
//  kill()
// sigset_t mask_all,backup_mask;
// for(int i=0;i<MAXJOBS;++i)
// {
//     sigfillset(&mask_all);
//     sigprocmask(SIG_BLOCK,&mask_all,&backup_mask);
//     if(jobs[i].state == FG){
//         kill(jobs[i].pid,sig);
//     }
//     sigprocmask(SIG_SETMASK,&backup_mask,NULL);
// }

//  send SIGINT to forground progess
//  kill()
// sigset_t mask_all,backup_mask;
// sigfillset(&mask_all);

// for(int i=0;i<MAXJOBS;++i)
// {
// sigprocmask(SIG_BLOCK,&mask_all,&backup_mask);          //  只可能会被另一个信号引起的handler打断，所以要屏蔽掉所有信号。就相当于并发编程里的lock
// if(jobs[i].state == FG){
// kill(jobs[i].pid,sig);
// }
// sigprocmask(SIG_SETMASK,&backup_mask,NULL);
// }

// void sigchld_handler2(int sig)
// {
//     int status;
//     while ((foreground_child_over_flag = waitpid(-1, &status, WUNTRACED)) != -1 )
//     {
//         sio_puts("IN WHILE\n");
//         if (WIFEXITED(status))          //  exist
//         {
//             sigset_t mask_all,backup_mask;
//             sigfillset(&mask_all);

//             deletejob(jobs,foreground_child_over_flag);        //  delete from list

//             sigprocmask(SIG_SETMASK,&backup_mask,NULL);

//             char s[100]={0};
//             sprintf(s,"child %d terminated normally with exit status %u\n", foreground_child_over_flag, WEXITSTATUS(status));
//             sio_puts(s);
//         }
//         else if (WIFSIGNALED(status))   //  killed by signal
//         {
//             sigset_t mask_all,backup_mask;
//             sigfillset(&mask_all);

//             deletejob(jobs,foreground_child_over_flag);        //  delete from list

//             sigprocmask(SIG_SETMASK,&backup_mask,NULL);

//             char s[100]={0};
//             sprintf(s,"child %d terminated abnormally because of the signal %u\n", foreground_child_over_flag,WTERMSIG(status));
//             sio_puts(s);
//         }
//         else if (WIFSTOPPED(status))    //  stopped by signal
//         {
//             char s[100]={0};
//             sprintf(s,"child %d stopped because of the signal %d\n", foreground_child_over_flag,WSTOPSIG(status));
//             sio_puts(s);
//         }
//     }
//     return;
// }

// void waitfg(sigset_t* mask)
// {
// foreground_child_over_flag = 0;
// while(!foreground_child_over_flagW){
// sigsuspend(mask);                   //  unblock child ; pause ; block child
// }
// }
//             char s[100]={0};
//             sprintf(s,"child %d stopped because of the signal %d\n", foreground_child_over_flag,WSTOPSIG(status));
//             sio_puts(s);
//         }
//     }
//     return;

