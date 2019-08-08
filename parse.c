#include "parse.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;


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

Cursor* table_start(Table *table)
{
    Cursor *cursor = malloc(sizeof(Cursor));
    
    cursor->table = table;
    cursor->row_num = 0;
    cursor->end_of_table = (table->num_rows == 0);

    return cursor;
}

Cursor* table_end(Table *table)
{
    Cursor *cursor = malloc(sizeof(Cursor));

    cursor->table = table;
    cursor->row_num = table->num_rows;
    cursor->end_of_table = true;

    return cursor;
}

void* cursor_value(Cursor *cursor)
{
    uint32_t row_num = cursor->row_num;
    uint32_t page_num = row_num / ROWS_PER_PAGE;


    void *page = get_page(cursor->table->pager, page_num);

    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
} 

void cursor_advance(Cursor *cursor)
{
    cursor->row_num++;
    if(cursor->row_num >= cursor->table->num_rows)
        cursor->end_of_table = true;
}




void* get_page(Pager *pager, uint32_t page_num)
{
    if(page_num > TABLE_MAX_PAGES)
    {
        printf("Tried to fetch page number out of bounds. %d > %d\n", page_num, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if(pager->pages[page_num] == NULL)
    {
        void *page = malloc(PAGE_SIZE);

        uint32_t total_pages = pager->file_length / PAGE_SIZE;

        if(pager->file_length % PAGE_SIZE) 
            total_pages++;

        if(page_num <= total_pages)
        {
            lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->fd, page, PAGE_SIZE);
            if(bytes_read == -1)
            {
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        pager->pages[page_num] = page;
    }
    

    return pager->pages[page_num];
}


MetaCommandResult do_meta_command(char *command, Table *table)
{
    if(strcmp(command, ".exit") == 0)
    {
        free(command);
        db_close(table);
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
        return prepare_insert(command, statement);
    }
    else if(strncmp(command, "select", 6) == 0)
    {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

PrepareResult prepare_insert(char *command, Statement *statement)
{
    statement->type = STATEMENT_INSERT;
    char *op = strtok(command, " ");
    char *id_string = strtok(NULL, " ");
    char *username = strtok(NULL, " ");
    char *email = strtok(NULL, " ");

    if(id_string == NULL || username == NULL || email == NULL)
        return PREPARE_SYNTAX_ERROR;
    int32_t id = atoi(id_string);
    if(id < 0)
        return PREPARE_NEGATIVE_ID;
    if(strlen(username) > USERNAME_SIZE || strlen(email) > EMAIL_SIZE)
        return PREPARE_STRING_TOO_LONG;

    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email, email);

    return PREPARE_SUCCESS;
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

ExecuteResult execute_insert(Statement *statement, Table *table)
{
    if(table->num_rows >= TABLE_MAX_ROWS)
        return EXECUTE_TABLE_FULL;

    Row *row_to_insert = &(statement->row_to_insert);
    Cursor *cursor = table_end(table);
    serialize_row(row_to_insert, cursor_value(cursor));
    table->num_rows += 1;

    free(cursor);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement *statement, Table *table)
{
    Row row;
    Cursor *cursor = table_start(table);
    
    while(!(cursor->end_of_table))
    {
        deserialize_row(cursor_value(cursor), &row);
        print_row(&row);
        cursor_advance(cursor);
    }

    printf("rows:%d\n", table->num_rows);
    free(cursor);
    return EXECUTE_SUCCESS;
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






Table* db_open(const char *filename)
{
    Pager *pager = pager_open(filename);
    // maybe >>....
    /*uint32_t num_rows = pager->file_length / ROW_SIZE;*/
    uint32_t num_rows = (pager->file_length / PAGE_SIZE) * ROWS_PER_PAGE + (pager->file_length % PAGE_SIZE) / ROW_SIZE;

    Table *table = malloc(sizeof(Table));
    table->pager = pager;
    table->num_rows = num_rows;

    return table;
}

void db_close(Table *table)
{
    Pager *pager = table->pager;
    uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE, i = 0;
    for(i = 0; i < num_full_pages; i++)
    {
        if(pager->pages[i] == NULL)
            continue;
        pager_flush(pager, i, PAGE_SIZE);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }
    
    uint32_t additional_rows = table->num_rows % ROWS_PER_PAGE;
    if(additional_rows > 0 && pager->pages[num_full_pages] != NULL)
    {
        pager_flush(pager, num_full_pages, additional_rows * ROW_SIZE);
        free(pager->pages[num_full_pages]);
        pager->pages[num_full_pages] = NULL;
    }

    if(close(pager->fd) == -1)
    {
        printf("Error: closing db file.\n");
        exit(EXIT_FAILURE);
    }

    free(pager);
    free(table);
}

/*void free_table(Table *table)*/
/*{*/
    /*for(uint32_t i = 0; table->pages[i]; i++)*/
    /*{*/
        /*free(table->pages[i]);*/
    /*}*/
    /*free(table);*/
/*}*/


