#pragma once

#define	TABLE_SIZE 2048


/*  the structure of a symbol table entry  */

typedef struct bucket bucket;
struct bucket
{
    struct bucket *link;
    struct bucket *next;
    char *name;
    char *tag;
    short value;
    short index;
    short prec;
    char cls;
    char assoc;
};


class ErrorReporter;

class SymbolTable
{
protected:
  int tableSize;
  bucket **symbol_table;
  bucket *first_symbol;
  bucket *last_symbol;
  ErrorReporter* erp;

public:

  bucket* GetFirstSymbol(){return this->first_symbol;}
  bucket* GetLastSymbol(){return this->last_symbol;}

public:
  
  SymbolTable(ErrorReporter* erp,unsigned int tableSize = TABLE_SIZE);

  ~SymbolTable();

protected:

  void create_symbol_table(unsigned int tableSize);

  void free_symbol_table();
  
  void free_symbols();

public:
  
  bucket* append(bucket* bp);

  int hash(char* name);

  bucket * make_bucket(char* name);
  bucket * lookup(char* name);
  
};