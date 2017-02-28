#include "mbed.h"
#include "cli.h"

RawSerial cli(USBTX, USBRX, 115200);

Mail<mail_command_t, 16> cli_mail_box;
Thread thread_cli;

static char cli_buffer[CLI_BUFFER_SIZE];
static uint8_t cli_buffer_idx = 0;
static char cli_params[CLI_MAX_PARAM][CLI_BUFFER_SIZE];
static uint8_t cli_num_params = 0;
char const *CLI_PROMPT = "\r\nARM mbed IoT CLI> ";

// static void cli_help(void *device, int argc, char argv[][CLI_BUFFER_SIZE]);
const cli_command cli_command_table[] = {
    {"help", "Print commands list", 0, cli_function_help},
    {"show", "Show configuration data", 0, cli_function_show}};

static void cli_prompt(void)
{
    mail_command_t *mail = cli_mail_box.alloc();
    mail->name = CLI_PROMPT;
    cli_mail_box.put(mail);
    memset(cli_buffer, 0, sizeof(cli_buffer));
    memset(cli_params, 0, sizeof(cli_params));
    cli_buffer_idx = 0;
}

void cli_command_not_found(void)
{
    cli.printf("\ncommand not found");
}

void print_params(void)
{
    cli.printf("\ncli_num_params(%d) cli_params[%s]\n", cli_num_params, cli_params);
}

static void cli_function_help(int argc, char argv[][CLI_BUFFER_SIZE])
{
    uint8_t blank = 0;

    for (uint8_t i = 0; i < CLI_COMMAND_TABLE_SIZE; i++)
    {
        blank = CLI_MAX_CMD_CHAR_LINE - strlen(cli_command_table[i].name);
        cli.printf(cli_command_table[i].name);

        for (int j = 0; j < blank; ++j)
        {
            cli.printf(" ");
        }

        cli.printf(";");
        cli.printf(cli_command_table[i].description);

        // don't add an extra LF if this is the last element in the list
        if (i < (CLI_COMMAND_TABLE_SIZE - 1))
        {
            cli.printf("\n");
        }
    }
}

static void cli_function_show(int argc, char argv[][CLI_BUFFER_SIZE])
{
    cli.printf("%s", "mbed IoT version 0.1");
}

static void cli_parse_params(void)
{
    uint8_t i = 0;            /* counter for the buffer */
    uint8_t j = 0;            /* counter for each param */
    bool is_str_open = false; /* to know if double quote is for open or close */
    cli_num_params = 0;

    for (i = 0; i < (cli_buffer_idx - 2); ++i)
    {
        if ((cli_buffer[i] != ' ') && (cli_buffer[i] != '\"'))
        {
            cli_params[cli_num_params][j++] = cli_buffer[i];
        }
        else if ((cli_buffer[i] == '\"') && (is_str_open == false))
        {
            is_str_open = true;
        }
        else if ((cli_buffer[i] == '\"') && (is_str_open == true))
        {
            is_str_open = false;
            cli_params[cli_num_params][j] = '\0';
            j = 0;            /* Reset param counter */
            cli_num_params++; /* update number of params */
        }
        else if ((cli_buffer[i] == ' ') && (is_str_open == true))
        {
            cli_params[cli_num_params][j++] = cli_buffer[i];
        }
        else if (((cli_buffer[i] == ' ') && (cli_buffer[i - 1] == ' ')) ||
                 ((cli_buffer[i] == ' ') && (cli_buffer[i - 1] == '\"')))
        {
            continue;
        }
        else
        {
            cli_params[cli_num_params][j] = '\0';
            j = 0;            /* Reset param counter */
            cli_num_params++; /* update number of params */
        }
    }
    cli_num_params++; /* The last param that can not see! */
}

void cli_thread(void)
{
    while (true)
    {
        osEvent evt = cli_mail_box.get();
        if (evt.status == osEventMail)
        {
            mail_command_t *mail = (mail_command_t *)evt.value.p;
            cli.printf("%s", mail->name);
            cli_mail_box.free(mail);
        }
    }
}

static void cli_get_command(char *name, cli_command *cmd_found)
{
    uint8_t i;

    for (i = 0; i < CLI_COMMAND_TABLE_SIZE; i++)
    {
        if (strncmp(name, cli_command_table[i].name, strlen(cli_command_table[i].name)) == 0)
        {
            cmd_found->name = cli_command_table[i].name;
            cmd_found->description = cli_command_table[i].description;
            cmd_found->cmdFunction = cli_command_table[i].cmdFunction;
            cmd_found->device = cli_command_table[i].device;
            return;
        }
    }

    /* If we don't find any command, return null. */
    cmd_found = 0;
}

void cli_command_parser(void)
{
    cli_command cmd = {NULL, NULL, NULL, NULL};
    char c;

    if (cli.readable())
    {
        c = cli.getc();
        if (c != '\r' && c != '\n')
        {
            cli.putc(c);
        }
        cli_buffer[cli_buffer_idx++] = c;
    }

    if ((cli_buffer_idx != 0) &&
        (cli_buffer[cli_buffer_idx - 2] == '\r') && (cli_buffer[cli_buffer_idx - 1] == '\n'))
    {
        /* No message, only enter command! */
        if (cli_buffer_idx == 2)
        {
            cli_buffer_idx = 0;
            cli_prompt();
            return;
        }

        cli_get_command(cli_buffer, &cmd);

        if (cmd.name != NULL)
        {
            cli_parse_params();
            cli.printf("\n");
            cmd.cmdFunction(cli_num_params, cli_params);
        }
        else
        {
            cli_command_not_found();
            print_params();
        }

        cli_buffer_idx = 0;
        cli_prompt();
    }
    else if (cli_buffer_idx > CLI_BUFFER_SIZE - 1)
    {
        cli_buffer_idx = 0;
        cli_prompt();
    }
}

int main(void)
{
    thread_cli.start(callback(cli_thread));
    cli_prompt();

    while (true)
    {
        // Waits until there is data to fetch on the UART
        cli_command_parser();
    }
}