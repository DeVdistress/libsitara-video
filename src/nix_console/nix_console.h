//
// nix_console.h
//
//  Created on: 15 сент. 2020 г.
//      Author: devdistress
//
#ifndef SRC_NIX_CONSOLE_NIX_CONSOLE_H_
#define SRC_NIX_CONSOLE_NIX_CONSOLE_H_

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>

#define CM_PATH_MAX PATH_MAX
#define CMTHR_RET_T void *
#define CMTHR_FUNC_DECL
#define PROMPT  "test> "
#define STR_LEN_MAX 1024
#define CMD_LEN_MAX 128
#define CMD_NUM_MAX 20
#define CM_SYMBOL 0
#define CM_DELIM 1
#define _is_delim(c,delim) ( (long) strchr(delim, c) )

#ifdef __cplusplus
 extern "C" {
#endif
#ifdef USE_TEST
//------------------------------------------
/// commands for console
//------------------------------------------
int cm_start_cap(char *args);
int cm_stop_cap(char *args);
int cm_on_display(char *args);
int cm_of_display(char *args);
int cm_start_disp(char *args);
int cm_stop_disp(char *args);
//------------------------------------------
#endif

extern const char *cm_delim;

typedef struct	{
					int exit;
					pthread_mutex_t csec;
					pthread_t id;
				} cm_thread_t;

struct cm_cmd	{
					char name[CMD_LEN_MAX];
					int (*exec) (char *args);
					char *arguments;
					char *help;
				};

int cm_thr_create(cm_thread_t * thr, CMTHR_RET_T (*start_routine) (void *));
int cm_thr_terminate(cm_thread_t * thr);
void __quit(int status, void *pid);
void cm_set_exit_func(void);
void cm_onexit(void);
int get_token_num(const char *str, const char *delim);
int execute_command(const char *command_name, char *args);
void get_cmd_and_args(char *str, char **cmd, char **args);
int interpret_str(char *str);
int register_command(const char *name, int (*execute_func) (char *args), char *arguments, char *help);
int command_usage(const char *command_name);
int cm_quit(char *args);
int help_command(const char *command_name);
int cm_help(char *args);
void registerAllCommands(void);
void workWithConsole(void);
int cm_log(char *args);

#ifdef __cplusplus
 }
#endif

#endif // SRC_NIX_CONSOLE_NIX_CONSOLE_H_
