#ifndef __PARSE_H
#define __PARSE_H

#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include "pager.h"

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
#define TABLE_MAX_PAGES 100
#define PAGE_SIZE 4096

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
    Pager *pager;
} Table;

typedef struct {
    StatementType type;
    Row row_to_insert;
} Statement;



void print_row(Row *row);
void serialize_row(Row *source, void *destination);
void deserialize_row(void *source, Row *destination);


void *row_slot(Table *table, uint32_t row_num);
void *get_page(Pager* pager, uint32_t page_num);

MetaCommandResult do_meta_command(char *command, Table *table);

PrepareResult prepare_statement(char *command, Statement *statement);
PrepareResult prepare_insert(char *command, Statement *statement);

ExecuteResult execute_statement(Statement *statement, Table *table);
ExecuteResult execute_insert(Statement *statement, Table *table);
ExecuteResult execute_select(Statement *statement, Table *table);
ExecuteResult execute_test(Statement *statement, Table *table);

Table* db_open(const char *filename);
//void free_table(Table *table);
void db_close(Table *table);
#endif
