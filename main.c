#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <readline/readline.h>
#include "parse.h"

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("Must supply a database filename.\n");
        exit(EXIT_FAILURE);
    }
    char *inputBuffer = NULL;
    Table *table = db_open(argv[1]);
    while(1)
    {
        inputBuffer = readline("> ");
        if(inputBuffer == NULL) continue;
        if('.' == inputBuffer[0]) 
        {
            switch(do_meta_command(inputBuffer, table))
            {
                case META_COMMAND_OK:
                    break;
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    printf("Unknown Command: \"%s\"\n", inputBuffer);
                    break;
            }
            free(inputBuffer);
            continue;
        }

        Statement statement;
        switch(prepare_statement(inputBuffer, &statement))
        {
            case PREPARE_SUCCESS:
                break;
            case PREPARE_UNRECOGNIZED_STATEMENT:
                printf("Unrecognized keyword at start of '%s.'\n", inputBuffer);
                free(inputBuffer);
                continue;
            case PREPARE_SYNTAX_ERROR:
                printf("Syntax error. !\n");
                free(inputBuffer);
                continue;
            case PREPARE_NEGATIVE_ID:
                printf("ID must be positive.\n");
                free(inputBuffer);
                continue;
            case PREPARE_STRING_TOO_LONG:
                printf("String is too long.\n");
                free(inputBuffer);
                continue;
        }

        switch(execute_statement(&statement, table))
        {
            case EXECUTE_SUCCESS:
                break;
            case EXECUTE_TABLE_FULL:
                printf("Error: Table Full.\n");
                break;
        }
        free(inputBuffer);
    }
    return 0;
}
