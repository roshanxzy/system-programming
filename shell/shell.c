/**
 * Shell Lab
 * CS 241 - Spring 2019
 */
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
 
#include "format.h"
#include "shell.h"
#include "vector.h"

#define terminate_shell() write_log(); shell_cleaner(); return 0;
#define prompt_cleaner(a, b) free(a); free(b); a = NULL; b = NULL;
extern char * optarg;
extern int optind, opterr, optopt;

static char* HISTORY_FILE = NULL;
static char* COMMAND_FILE = NULL;
static char* HISTORY_PATH = NULL;
static char* COMMAND_PATH = NULL;
static FILE* HISTORY_FILE_POINTER = NULL;
static FILE* COMMAND_FILE_POINTER = NULL;

static vector* LOG = NULL;
static bool NO_FLAG = false;
static bool H_FLAG = false;
static bool F_FLAG = false;

typedef struct process {
    char *command;
    pid_t pid;
} process;

void command_dispatcher(char*, int);

bool argc_validator(int argc) {
	if (argc == 1 || argc == 3 || argc == 5) {
		return true;
	} else {
		return false;
	}
}

int cmd_validator(char* cmd) {
	char* and_pos = strstr(cmd, "&&");
	char* or_pos = strstr(cmd, "||");
	char* semi_col_pos = strstr(cmd, ";");
	if (and_pos != NULL) {
		if (or_pos != NULL || semi_col_pos != NULL) {
			return -1;
		} else {
			return 1;
		}
	}
	if (or_pos != NULL) {
		if (and_pos != NULL || semi_col_pos != NULL) {
			return -1;
		} else {
			return 2;
		}
	}
	if (semi_col_pos != NULL) {
		if (and_pos != NULL || or_pos != NULL) {
			return -1;
		} else {
			return 3;
		}
	}
	return 0;
}

void signal_handler(int signal) {
	if (signal == SIGINT) {
		return;
	} else if (signal == SIGCHLD) {
		// TODO
	} else {
		return;
	}
}

void shell_cleaner() {
	if (HISTORY_FILE) {
		free(HISTORY_FILE);
		HISTORY_FILE = NULL;
	}
	if (COMMAND_FILE) {
		free(COMMAND_FILE);
		COMMAND_FILE = NULL;
	}
	if (HISTORY_PATH) {
		free(HISTORY_PATH);
		HISTORY_PATH = NULL;
	}
	if (COMMAND_PATH) {
		free(COMMAND_PATH);
		COMMAND_PATH = NULL;
	}
	if (LOG) {
		free(LOG);
		LOG = NULL;
	}
	return;
}

bool option_setup(int argc, char** argv) {
	int opt;
	while ((opt = getopt(argc, argv, "h:f:")) != -1) {
		if (opt == 'h') {
			H_FLAG = true;
			if (HISTORY_FILE == NULL && optarg != NULL) {
				HISTORY_FILE = strdup(optarg);
			} else {
				return false;
			}
		} else if (opt == 'f') {
			F_FLAG = true;
			if (COMMAND_FILE == NULL && optarg != NULL) {
				COMMAND_FILE = strdup(optarg);
			} else {
				return false;
			}
		} else {
			return false;
		}
	}
	return true;
}

bool log_cmd(char* cmd) {
	if (LOG != NULL) {
		//+1 for NUL Byte and +1 for adding newline char
		size_t cmd_len = strlen(cmd);
		char cmd_to_push[cmd_len + 1];
		strcpy(cmd_to_push, cmd);
		
		//FIX NUL Byte and newline
		cmd_to_push[cmd_len] = '\n';
		cmd_to_push[cmd_len + 1] = '\0';
	
		vector_push_back(LOG, cmd_to_push);
		return true;
	}
	return false;
}

void write_log() {
	if (H_FLAG == false) {
		return;
	}
	char* cmd = NULL;
	if (HISTORY_FILE_POINTER != NULL) {
		void** _it = vector_begin(LOG);
		void** _end = vector_end(LOG);
		for( ; _it != _end; ++_it) {
			cmd = *_it;
			fprintf(HISTORY_FILE_POINTER, "%s", (char*) cmd);
		}
	}
	vector_clear(LOG);
	fseek(HISTORY_FILE_POINTER, 0, SEEK_SET);
	return;
}

bool number_validator(char* cmd) {
	size_t i;
	for (i = 0; i < strlen(cmd); i++) {
		if (!isdigit(cmd[i])) {
			return false;
		}
	}
	return true;
}

void exec_cd(char* cmd) {
	log_cmd(cmd);
	size_t cmd_len = strlen(cmd);
	if (cmd_len <= 3) {
		print_no_directory("");
		return;
	}
	
	char* directory = cmd + 3;
	
	if (chdir(directory) == -1) {
		print_no_directory(directory);
	}
}

void print_current_shell_session_log() {
	char* cmd = NULL;
	void** _it = vector_begin(LOG);
	void** _end = vector_end(LOG);
	size_t cmd_line_number = 0;
	for( ; _it != _end; ++_it) {
		cmd = *_it;
		printf("%zu\t%s", cmd_line_number, (char*) cmd);
		cmd_line_number++;
	}
	return;
}	

void exec_print_history() {
	//!history is not stored in history
	if (H_FLAG) {
		write_log();
		char cmd_line[1024];
		size_t cmd_line_number = 0;
		while (fgets(cmd_line, sizeof(cmd_line), HISTORY_FILE_POINTER)) {
			printf("%zu\t%s", cmd_line_number, cmd_line);
			cmd_line_number++;
		}
	} else {
		print_current_shell_session_log();
	}
	return;	
}

size_t history_command_counter() {
	if (H_FLAG) {
		write_log();
		char cmd_line[1024];
		size_t cmd_line_count = 0;
		while (fgets(cmd_line, sizeof(cmd_line), HISTORY_FILE_POINTER)) {
			cmd_line_count++;
		}
		return cmd_line_count;
	} else {
		return vector_size(LOG);
	}

bool exec_nth_command(size_t cmd_line_number) {
	//#<n> is not stored in the history
	//But the executed command is 
	if (H_FLAG) {
		write_log();
		char cmd_line[1024];
		size_t curr_cmd_line = 0;
		bool INBOUND = false;
		while (fgets(cmd_line, sizeof(cmd_line), HISTORY_FILE_POINTER)) {
			if (curr_cmd_line == cmd_line_number) {
				cmd_line[strlen(cmd_line) - 1] = '\0';
				INBOUND = true;
				break;
			}
			curr_cmd_line++;
		}
		if (INBOUND == false) {
			return false;
		} else {
			int logic_operator = cmd_validator(cmd_line);
			puts(cmd_line);
			command_dispatcher(cmd_line, logic_operator);
			return true;
		}
	} else {
		char cmd_line[1024];
		void** _it = vector_begin(LOG);
		void** _end = vector_end(LOG);
		size_t curr_cmd_line = 0;
		bool INBOUND = false;
		for(; _it != _end; ++_it) {
			if (curr_cmd_line == cmd_line_number) {
				strcpy(cmd_line, *_it);
				cmd_line[strlen(cmd_line) - 1] = '\0';
				INBOUND = true;
				break;
			}
			curr_cmd_line++;
		}
		if (INBOUND == false) {
			return false;
		} else {
			int logic_operator = cmd_validator(cmd_line);
			puts(cmd_line);
			command_dispatcher(cmd_line, logic_operator);
			return true;
		}
	}
}

bool exec_prefix_command(char* cmd) {
	size_t cmd_len = strlen(cmd);
	bool POUND_ONLY = false;
	if (cmd_len == 1) {
		POUND_ONLY = true;
	}
	char* command_prefix = cmd + 1;
	size_t prefix_len = strlen(command_prefix);
	size_t history_command_count = history_command_counter();
	bool FOUND = false;
	if (H_FLAG) {
		write_log();
		vector* compiled_history = string_vector_create();
		char cmd_line[1024];
		while (fgets(cmd_line, sizeof(cmd_line), HISTORY_FILE_POINTER)) {
			vector_push_back(compiled_history, cmd_line);
		}

		void** _it = vector_end(compiled_history) - 1;
		if (POUND_ONLY) {
			strcpy(cmd_line, *_it);
			cmd_line[strlen(cmd_line) - 1] = '\0';
			puts(cmd_line);
			int logic_operator = cmd_validator(cmd_line);
			command_dispatcher(cmd_line, logic_operator);
			return true;	
		}
		while (true) {
			strcpy(cmd_line, *it);
			if (_it == vector_begin(compiled_history)) {
				if (strncmp(command_prefix, cmd_line, prefix_len) == 0) {
					FOUND = true;
					break;
				}
			} else {
				//strncmp returns 0 if strs are same
				if (strncmp(command_prefix, cmd_line, prefix_len) == 0) {
					FOUND = true;
					break;
				}
			}
			_it--; // UPDATE WHILE LOOP
		}
		vector_destroy(compiled_history);
		if (FOUND == false) {
			return false;	
		} else {
			cmd_line[strlen(cmd_len) - 1] = '\0';
			purts(cmd_line);
			int logic_operator = cmd_validator(cmd_line);
			command_dispatcher(cmd_line, logic_operator);
			return true;
		}
	} else { // H_FLAG == false
		char cmd_line[1024];
		void** _it = vector_end(LOG) - 1;
		if (POUND_ONLY) {
			strcpy(cmd_line, *_it);
			cmd_line[strlen(cmd_line) - 1] = '\0';
			puts(cmd_line);
			int logic_operator = cmd_validator(cmd_line);
			command_dispatcher(cmd_line, logic_operator);
			return true;
		}	
		while (true) {
			strcpy(cmd_line, *_it);
			if (_it == vector_begin(LOG)) {
				if (strncmp(command_prefix, cmd_line, prefix_len) == 0) {
					FOUND = true;
					break;
				}
			} else {
				//strncmp returns 0 if strs are same
				if (strncmp(command_prefix, cmd_line, prefix_len == 0)) {
					FOUND = true;
					break;
				}
			}
			_it--;
		}
		if (FOUND == false) {
			return false;
		} else {
			cmd_line[strlen(cmd_len) - 1] = '\0';
			puts(cmd_line);
			int logic_operator = cmd_validator(cmd_line);
			command_dispatcher(cmd_line, logic_operator);
			return true;
		}
	}
}	

void command_dispatcher(char* cmd, int logic_operator) {
	size_t cmd_len = strlen(cmd);
	if (cmd_len == 0) {
		return;
	}
	if (logic_operator == 0) { // NO LOGIC OPERATOR
		if (cmd_len > 1) {
			if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == 32) {
				exec_cd(cmd);
			}
			if (cmd[0] == '!') {
				if (strstr(cmd, "history")) {//!history
					exec_print_history();
				} else {//!<prefix>
					if (exec_prefix_command(cmd) == false) {
						print_no_history_match();
					}
				}	
			}
			if (cmd[0] == '#') {//#<n>
				if (number_validator(cmd + 1) == false) {
					print_invalid_index();
				} else {
					int cmd_line_number = atoi(cmd + 1);
					if (!exec_nth_command((size_t)cmd_line_number)) {
						print_invalid_index();
					}
				}
			}
		}			
	} else if (logic_operator == 1) { // AND
		//char* and_pos = strstr(cmd, "&&");


	} else if (logic_operator == 2) { // OR
		//char* or_pos = strstr(cmd, "&&");


	} else if (logic_operator == 3) { // SEMI COLON
		//char* semi_col_pos = strstr(cmd, ";");


	} else { //TODO DO nothing
	}
}
bool file_setup() {
	if (HISTORY_FILE != NULL) {
		if (access(HISTORY_FILE, R_OK | W_OK) != -1) {
			HISTORY_PATH = (*get_full_path)(HISTORY_FILE);
			HISTORY_FILE_POINTER = fopen(HISTORY_PATH, "a+");
		} else {
			print_history_file_error();
			HISTORY_FILE_POINTER = fopen(HISTORY_FILE, "a+");
			HISTORY_PATH = (*get_full_path)(HISTORY_FILE);
		}
		if (HISTORY_FILE_POINTER == NULL) { // fopen failed
			print_history_file_error();
			return false;
		}	 
	}
	if (COMMAND_FILE != NULL) {
		if (access(COMMAND_FILE, R_OK) != -1) {
			COMMAND_PATH = (*get_full_path)(COMMAND_FILE);
			COMMAND_FILE_POINTER = fopen(COMMAND_PATH, "r");
		} else {
			print_script_file_error();
			return false;
		}
		if (COMMAND_FILE_POINTER == NULL) { // fopen failed
			print_script_file_error();
			return false;
		}
	}
	return true;
}

bool log_setup() {
	LOG = string_vector_create();
	if (LOG != NULL) {
		return true;
	} else {
		return false;
	}	
}
			
	
int shell(int argc, char *argv[]) {
	if (!argc_validator(argc)) {
		print_usage();
		terminate_shell();
	}
	
	//expecting signal
	signal(SIGINT, signal_handler);
	signal(SIGCHLD, signal_handler);

	//get options
	if (argc == 1) {
		NO_FLAG = true;
	} else {	
		if (option_setup(argc, argv) == false) {
			print_usage();
			terminate_shell();
		}
	
		//setup files (ONLY IF there is -h or -f)
		if (file_setup() == false) {
		terminate_shell();
		}
	}

	//Get ready to prompt
	log_setup();	
	pid_t pid = getpid();

	char* cmd = NULL;
	size_t cmd_size = 0;
	char* cwd = NULL;
	size_t cwd_size = 0;
	while (true) {
		if ((cwd = getcwd(cwd, cwd_size))) {
			print_prompt(cwd, pid);
		}
		if (getline(&cmd, &cmd_size, stdin) == -1) {
			print_usage();
			prompt_cleaner(cmd, cwd);
			terminate_shell();	
		}
		
		//Change the newline char to NUL char	
		cmd[strlen(cmd) - 1] = '\0';	
	
		//Validate cmd
		//0: NO Logic Operator
		//1: AND
		//2: OR
		//3: SEMICOLON
		//-1: INVALID
		int logic_operator = cmd_validator(cmd);

		//Dispatch Commands (w/ valid cmd)
		if (logic_operator == -1) {
			print_invalid_command(cmd);			
		} else {
			command_dispatcher(cmd, logic_operator);
		}

		//CLEAN UP - ready for next prompt
		prompt_cleaner(cmd, cwd);
	}
	exit(0);
}
