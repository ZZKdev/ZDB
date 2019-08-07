#ifndef __PARSE_H
#define __PARSE_H

#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>

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



ExecuteResult execute_test(Statement *statement, Table *table);
void print_row(Row *row);
void serialize_row(Row *source, void *destination);
void deserialize_row(void *source, Row *destination);
void *row_slot(Table *table, uint32_t row_num);
MetaCommandResult do_meta_command(char *command);
PrepareResult prepare_statement(char *command, Statement *statement);
ExecuteResult execute_insert(Statement *statement, Table *table);
ExecuteResult execute_select(Statement *statement, Table *table);
ExecuteResult execute_statement(Statement *statement, Table *table);
Table* new_table();
void free_table(Table *table);
ExecuteResult execute_test(Statement *statement, Table *table);
#endif
