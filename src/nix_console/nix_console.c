// nix_console.c
//
//  Created on: 15 сент. 2020 г.
//      Author: devdistress
#include"nix_console.h"
#include"limits.h"

char logfile[CM_PATH_MAX];
cm_thread_t th;
struct cm_cmd *commands[CMD_NUM_MAX] = { NULL };
const char *cm_delim = " \f\n\r\t\v";
FILE * flog = NULL;

#define cmprint(stream,...) \
    do { cm_crit_enter(&th); fprintf(stream,__VA_ARGS__); fflush(stream); cm_crit_leave(&th); } while(0)

#define cmlog(...) \
    do { cm_crit_enter(&th); \
    if (flog) { fprintf(flog,__VA_ARGS__); fflush(flog); };\
    cm_crit_leave(&th); } while(0)

#define cmprint_and_log(stream,...) \
    do { cm_crit_enter(&th); \
    fprintf(stream,__VA_ARGS__); fflush(stream); \
    if (flog) { fprintf(flog,__VA_ARGS__); fflush(flog); };\
    cm_crit_leave(&th); } while(0)

int cm_thr_create(cm_thread_t * thr, CMTHR_RET_T (*start_routine) (void *))
{
    int ret = 0;
    thr->exit = 0;
    pthread_mutex_init(&thr->csec, NULL);
    ret = pthread_create(&thr->id, NULL, start_routine, (void *) thr);
    return ret;
}

int get_token_num(const char *str, const char *delim)
{
    int tn = 0;
    int curch_flag;
    int prevch_flag = 1;             // is delimiter
    const char *s = str;

    if (!str)
        return 0;

    while (*s) {
        curch_flag = _is_delim(*s, delim);
        if (!curch_flag && prevch_flag)
            tn++;
        prevch_flag = curch_flag;
        s++;
    }

    //fprintf(stdout, "get_token_num: tn = %d\n", tn);
    return tn;

}

// returns: NULL - end of str, pointer to first not to miss character
static inline char* _miss_delims_or_symbols(char *str, const char *delim, int type)
{
    char *s = str;
    int cond;

    while (*s) {
        cond = _is_delim(*s, delim);
        if ((type == CM_DELIM) && (!cond))
            return s;           // found symbol while missing delims
        if ((type == CM_SYMBOL) && cond)
            return s;           // found delim while missing symbols
        s++;
    }
    return NULL;
}

static inline void _str_trim(char *str, const char *delim)
{
    char *s = str;
    int i, len = 0;

    while (*s++)
        len++;
    for (i = len; i > 0; i--) {
        if (_is_delim(*s, delim))
            *s = '\0';
        else
            break;
        s--;
    }
}

int cm_thr_terminate(cm_thread_t * thr)
{
	int ret = 0;
    thr->exit = 1;
    ret = pthread_join(thr->id, NULL);
    pthread_mutex_destroy(&thr->csec);
    return ret;
}

#define cm_crit_enter(pthr) pthread_mutex_lock( &((pthr)->csec) )
#define cm_crit_leave(pthr) pthread_mutex_unlock( &((pthr)->csec) )

void __quit(int status, void *pid)
{
    cm_onexit();
}

void cm_set_exit_func(void)
{
    on_exit(__quit, NULL);
}

void cm_onexit(void)
{
    cm_thr_terminate(&th);
    fprintf(stdout, "good bye\n");
    fflush(stdout);
    if (flog) {
        fflush(flog);
        fclose(flog);
    }
}

// rl_gets() Read a string, and return a pointer to it.
// Returns NULL on EOF.
#if (1)
	static char *line_read = (char *) NULL; // for readline
	char *rl_gets(void)
	{
		// If the buffer has already been allocated, return the
		// memory to the free pool.
		if (line_read)
		{
			free(line_read);
			line_read = (char *) NULL;
		}
		// Get a line from the user.
		line_read = readline(PROMPT);
		// If the line has any text in it, save it on the history.
		if (line_read && *line_read)
			add_history(line_read);
		return (line_read);
	}
#else
	static char line_read[STR_LEN_MAX];
	char *rl_gets(void)
	{
		char *ret;

		fprintf(stderr, "%s", PROMPT);
		ret = fgets(line_read, STR_LEN_MAX, stdin);
		if (!ret)
			return NULL;

		return line_read;
	}
#endif

int execute_command(const char *command_name, char *args)
{
    int slot;

    for (slot = 0; slot < CMD_NUM_MAX; slot++) {
        if (commands[slot]) {
            if (strcmp(commands[slot]->name, command_name) != 0)
                continue;
            if(commands[slot]->exec(args) == INT_MAX)
            	return INT_MAX;
            else
            	return 0;
        }
    }
    return -1;
}

// separate and trim cmd and args from str
void get_cmd_and_args(char *str, char **cmd, char **args)
{
    char *start;
    char *finish;
    char *cmd_delim = NULL;

    *cmd = *args = NULL;
    start = _miss_delims_or_symbols(str, cm_delim, CM_DELIM);
    if (start) {
        *cmd = start;
        finish = _miss_delims_or_symbols(start, cm_delim, CM_SYMBOL);
        if (finish) {           // have delim after cmd
            cmd_delim = finish;
            start = _miss_delims_or_symbols(finish, cm_delim, CM_DELIM);
            if (start) {
                _str_trim(start, cm_delim);
                *args = start;
            }
        }
    }
    if (cmd_delim)
        *cmd_delim = '\0';
}

int interpret_str(char *str)
{
    char *cmd = NULL;
    char *args = NULL;

    get_cmd_and_args(str, &cmd, &args);
    if (!cmd)
        return -1;
    //fprintf(stdout, "%s: arg is:%s, len = %d\n", cmd, args, strlen(cmd));

    int res = execute_command(cmd, args);
    if 		(res < 0)
        cmprint(stdout, "unknown command\n");
    else if (res == INT_MAX)
    	return INT_MAX;
    return 0;
}

int register_command(const char *name,
    int (*execute_func) (char *args),
    char *arguments, char *help)
{
    int slot;

    for (slot = 0; slot < CMD_NUM_MAX; slot++) {
        if (!commands[slot])
            break;
    }
    if (slot == CMD_NUM_MAX)
        return -1;             // no free command slots
    commands[slot] = (struct cm_cmd *) malloc(sizeof(struct cm_cmd));
    if (!commands[slot])
        return -1;             // memory fault
    strncpy(commands[slot]->name, name, CMD_LEN_MAX);
    commands[slot]->exec = execute_func;
    commands[slot]->help = help;
    commands[slot]->arguments = arguments;
    return 0;
}

int command_usage(const char *command_name)
{
    int slot;

    for (slot = 0; slot < CMD_NUM_MAX; slot++) {
        if (commands[slot]) {
            if (strcmp(commands[slot]->name, command_name) == 0) {
                cmprint(stdout, "usage: %s", command_name);
                if (commands[slot]->arguments != NULL)
                    cmprint(stdout, " %s", commands[slot]->arguments);
                cmprint(stdout, "\n");
                return 0;
            }
        }
    }
    return -1;
}

// quit
int cm_quit(char *args)
{
    if (args) {
        command_usage("quit");
        return -1;
    }

    //exit(0);
    return INT_MAX;
}
// help
int help_command(const char *command_name)
{
    int slot;

    for (slot = 0; slot < CMD_NUM_MAX; slot++) {
        if (commands[slot]) {
            if (strcmp(commands[slot]->name, command_name) == 0) {
                cmprint(stdout, "%s - ", commands[slot]->name);
                if (commands[slot]->help != NULL)
                    cmprint(stdout, "%s", commands[slot]->help);
                cmprint(stdout, "\n\n");
                cmprint(stdout, "usage: %s", commands[slot]->name);
                if (commands[slot]->arguments != NULL)
                    cmprint(stdout, " %s", commands[slot]->arguments);
                cmprint(stdout, "\n\n");
                return 0;
            }
        }
    }
    return -1;
}

int cm_help(char *args)
{
    int i;
    int args_num;

    args_num = get_token_num(args, cm_delim);

    if (args_num == 0) {
        cmprint(stdout, "\nAvailable commands:\n");
        for (i = 0; i < CMD_NUM_MAX; i++) {
            if (!commands[i])
                continue;
            cmprint(stdout, "   %-15s - ", commands[i]->name);
            if (commands[i]->help != NULL)
                cmprint(stdout, "%s", commands[i]->help);
            cmprint(stdout, "\n");
        }
        cmprint(stdout, "\n");
    } else if (args_num == 1) {
        cmprint(stdout, "\n");
        if (help_command(args) < 0) {
            cmprint(stdout, "help error: unknown command %s\n", args);
            return -1;
        }
        return 0;
    } else {
        command_usage("help");
        return -1;
    }
    return 0;
}

void registerAllCommands(void)
{
    register_command("help", cm_help, "[command name]",
        "display help message on command");

    register_command("quit", cm_quit, NULL, "exit from programm");

#ifdef USE_TEST
    register_command("start", cm_start_cap, NULL,
            "start of capture video command");

    register_command("stop", cm_stop_cap, NULL,
                "stop of capture video command");

    register_command("on", cm_on_display, NULL,
            "displayed video cap");

    register_command("off", cm_of_display, NULL,
                "not displayed video cap");

    register_command("startd", cm_start_disp, NULL,
            "start of display command");

    register_command("stopd", cm_stop_disp, NULL,
                "stop of display command");
#endif
}

void workWithConsole(void)
{
    char *ln;

	fprintf(stdout,
	        "type 'help' for commands info and 'quit' for exit.\n\n");

    while ((ln = rl_gets()))
    {
        int res = interpret_str(ln);

        if (res == INT_MAX)
        	break;
    }

    return;
}

// log
int cm_log(char *args)
{
    int args_num;
    char *tok;

    args_num = get_token_num(args, cm_delim);
    if (args_num != 1 && args_num != 2) {
        goto err;
    }
    tok = strtok(args, cm_delim);

    if (args_num == 1) {       //possible off
        if (strcmp(tok, "off") != 0)
            goto err;
        if (flog == NULL) {
            cmprint(stdout, "logging is switched off already\n");
            return -1;
        }
        cm_crit_enter(&th);
        fclose(flog);
        flog = NULL;
        cm_crit_leave(&th);
        cmprint(stdout, "logging to %s file is switched off\n", logfile);
    } else {                    // on
        if (strcmp(tok, "on") != 0)
            goto err;
        if (flog) {
            cmprint(stdout,
                "logging is switched off already, logfile is \"%s\"\n",
                logfile);
            return -1;
        }
        tok = strtok(NULL, cm_delim);
        strcpy(logfile, tok);
        cm_crit_enter(&th);
        flog = fopen(logfile, "a");
        cm_crit_leave(&th);

        if (flog == NULL) {
            cmprint(stdout, "can't open logfile %s\n", logfile);
            return -1;
        }
        cmprint(stdout, "logging to \"%s\" is switched on\n", logfile);
    }

    return 0;
err:
    command_usage("log");
    return -1;
}

