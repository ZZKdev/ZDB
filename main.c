#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <readline/readline.h>

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
#define TABLE_MAX_PAGES 100

typedef enum { META_COMMAND_OK, META_COMMAND_UNRECOGNIZED_COMMAND } MetaCommandResult; 
typedef enum { 
    PREPARE_SUCCESS, 
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR,
    PREPARE_NEGATIVE_ID,
    PREPARE_STRING_TOO_LONG
} PrepareResult;
typedef enum { STATEMENT_INSERT, STATEMENT_SELECT } StatementType;
typedef enum { EXECUTE_SUCCESS, EXECUTE_TABLE_FULL } ExecuteResult;

typedef struct {
    uint32_t id;
    char username[32];
    char email[256];
} Row;

typedef struct {
    uint32_t num_rows;
    void *pages[TABLE_MAX_PAGES];
} Table;

typedef struct {
    StatementType type;
    Row row_to_insert;
} Statement;

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t PAGE_SIZE = 4096;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;


ExecuteResult execute_test(Statement *statement, Table *table);

void print_row(Row *row)
{
    printf("%d, %s, %s\n", row->id, row->username, row->email);
}


void serialize_row(Row *source, void *destination)
{
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, source->username, USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, source->email, EMAIL_SIZE);
}

void deserialize_row(void *source, Row *destination)
{
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(destination->username, source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(destination->email, source + EMAIL_OFFSET, EMAIL_SIZE); 
}

void *row_slot(Table *table, uint32_t row_num)
{
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void *page = table->pages[page_num];
    if(page == NULL)
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
} 



MetaCommandResult do_meta_command(char *command)
{
    if(strcmp(command, ".exit") == 0)
    {
        free(command);
        exit(0);
    }
    else if(strcmp(command, ".help") == 0)
    {
        printf("Can use .exit\n");
        return META_COMMAND_OK;
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

PrepareResult prepare_statement(char *command, Statement *statement)
{
    if(strncmp(command, "insert", 6) == 0)
    {
        statement->type = STATEMENT_INSERT;
        if(3 != sscanf(command, "insert %d %s %s", &(statement->row_to_insert.id), statement->row_to_insert.username, statement->row_to_insert.email))
        {
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
    }
    else if(strncmp(command, "select", 6) == 0)
    {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_insert(Statement *statement, Table *table)
{
    if(table->num_rows >= TABLE_MAX_ROWS)
        return EXECUTE_TABLE_FULL;

    Row *row_to_insert = &(statement->row_to_insert);
    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement *statement, Table *table)
{
    Row row;
    for(uint32_t i = 0; i < table->num_rows; i++)
    {
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }
    printf("rows:%d\n", table->num_rows);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table)
{
    switch(statement->type)
    {
        case STATEMENT_INSERT:
            return execute_insert(statement, table);
        case STATEMENT_SELECT:
            return execute_select(statement, table);
    }
}

Table* new_table()
{
    Table *table = malloc(sizeof(Table));
    table->num_rows = 0;
    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        table->pages[i] = NULL;
    }
    return table;
}

void free_table(Table *table)
{
    for(uint32_t i = 0; table->pages[i]; i++)
    {
        free(table->pages[i]);
    }
    free(table);
}

ExecuteResult execute_test(Statement *statement, Table *table)
{
    printf("start testing...\n");
    printf("insert...\n");
    uint32_t i = 0;
    for(i = 0; i < TABLE_MAX_ROWS; i++)
    {
        if(execute_insert(statement, table) == EXECUTE_TABLE_FULL)
        {
            return EXECUTE_TABLE_FULL;
        }
    }
    return EXECUTE_SUCCESS;
}

int main(void)
{
    char *inputBuffer = NULL;
    Table *table = new_table();
    printf("sizeof: %d\n", sizeof(Row));
    while(1)
    {
        inputBuffer = readline("> ");
        if(inputBuffer == NULL) continue;
        if('.' == inputBuffer[0]) 
        {
            switch(do_meta_command(inputBuffer))
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

