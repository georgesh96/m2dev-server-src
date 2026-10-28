#include "stdafx.h"

#ifdef OS_WINDOWS
#include <thread>

// Windows has no SIGTERM; treat console Ctrl+C/Break/close as a graceful shutdown request
// so db flushes its caches and game saves characters instead of being killed mid-cache.
static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
	switch (ctrl_type)
	{
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
			shutdowned = TRUE;
			return TRUE;

		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			shutdowned = TRUE;
			Sleep(4500); // the process is terminated when this returns; give the main loop time to flush
			return TRUE;
	}

	return FALSE;
}

// stop.py signals Local\m2_shutdown_<pid> to request a graceful shutdown of one process.
static void shutdown_event_watcher(HANDLE ev)
{
	WaitForSingleObject(ev, INFINITE);
	shutdowned = TRUE;
}

void signal_setup()
{
	SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

	char name[64];
	snprintf(name, sizeof(name), "Local\\m2_shutdown_%lu", GetCurrentProcessId());

	HANDLE ev = CreateEventA(NULL, TRUE, FALSE, name);
	if (ev)
		std::thread(shutdown_event_watcher, ev).detach();
}

void signal_timer_disable() {}
void signal_timer_enable(int timeout_seconds) {}
#else
#define RETSIGTYPE void

RETSIGTYPE reap(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
    signal(SIGCHLD, reap);
}


RETSIGTYPE checkpointing(int sig)
{
    if (!tics.load())
    {
        sys_err("CHECKPOINT shutdown: tics did not updated.");
        abort();
    }
    else
	    tics.store(0);
}


RETSIGTYPE hupsig(int sig)
{
    shutdowned = TRUE;
    sys_err("SIGHUP, SIGINT, SIGTERM signal has been received. shutting down.");
}

RETSIGTYPE usrsig(int sig)
{
    core_dump();
}

void signal_timer_disable(void)
{
    struct itimerval itime;
    struct timeval interval;

    interval.tv_sec	= 0;
    interval.tv_usec	= 0;

    itime.it_interval = interval;
    itime.it_value = interval;

    setitimer(ITIMER_VIRTUAL, &itime, NULL);
}

void signal_timer_enable(int sec)
{
    struct itimerval itime;
    struct timeval interval;

    interval.tv_sec	= sec;
    interval.tv_usec	= 0;

    itime.it_interval = interval;
    itime.it_value = interval;

    setitimer(ITIMER_VIRTUAL, &itime, NULL);
}

void signal_setup(void)
{
    signal_timer_enable(30);

    signal(SIGVTALRM, checkpointing);

    /* just to be on the safe side: */
    signal(SIGHUP, hupsig);
    signal(SIGCHLD, reap);
    signal(SIGINT, hupsig);
    signal(SIGTERM, hupsig);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGUSR1, usrsig);
}

#endif
