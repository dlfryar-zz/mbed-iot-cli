#ifndef CLI_H
#define CLI_H

#define CLI_BUFFER_SIZE 50
#define CLI_MAX_CHARS_PER_LINE 60
#define CLI_MAX_CMD_CHAR_LINE 20
#define CLI_MAX_STATUS_CHAR_LINE 10
#define CLI_MAX_PARAM 10

#define CLI_COMMAND_TABLE_SIZE (sizeof cli_command_table / sizeof cli_command_table[0])

typedef struct
{
    const char *name;
} mail_command_t;

typedef struct
{
    char const *name;
    char const *description;
    void *device;
    void (*cmdFunction)(int argc, char argv[][CLI_BUFFER_SIZE]);
} cli_command;

static void cli_function_help(int argc, char argv[][CLI_BUFFER_SIZE]);
static void cli_function_show(int argc, char argv[][CLI_BUFFER_SIZE]);

#endif