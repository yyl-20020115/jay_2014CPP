#pragma once

#define LINESIZE 512

struct GlobalInfo;

class Reader
{
protected:

  char *cache;
  int cinc;
  int cache_size;
  int ntags;
  int tagmax;
  char **tag_table;
  char saw_eof;
  int linesize;
  bucket *goal;
  int prec;
  int gensym ;
  char last_was_action ;
  int maxitems ;
  bucket **pitem;
  int maxrules ;
  bucket **plhs;
  int maxmethods;
  int name_pool_size;
  char *name_pool ;

protected:

  GlobalInfo* info;
  ErrorReporter* erp;
  SymbolTable* symbolTable;

public:

  char *cptr;
  short *ritem;
  short *rlhs;
  short *rrhs;
  short *rprec;
  char  *rassoc;

  int nitems;
  int nrules;
  int nsyms;
  int ntokens;
  int nvars;
  int nmethods;
  char* line;
  int lineno;
  
  int   start_symbol;
  char  **symbol_name;
  short *symbol_value;
  short *symbol_prec;
  char  *symbol_assoc;
  char  **methods;

public:

  Reader(GlobalInfo* info,ErrorReporter* erp, SymbolTable* symbolTable);
  ~Reader();

public:

  void Read();

protected:

  void start_rule(register bucket *bp, int s_lineno);
  void create_symbol_table();
  void read_declarations();
  void read_grammar();
  void free_symbol_table();
  void free_tags();
  void pack_names();
  void check_symbols();
  void pack_symbols();
  void pack_grammar();
  void free_symbols();
  void print_grammar();

  void cachec(int c);
  void get_line();
  std::string dup_line();
  void skip_comment();
  int nextc();
  int keyword();

  void copy_text(FILE* f);
  int hexval(int c);
  bucket* get_literal();

  int is_reserved(char* name);
  bucket* get_name();

  int get_number();
  char* get_tag(int emptyOk);

  void declare_tokens(int assoc);
  void declare_types();
  void declare_start();
  void initialize_grammar();
  void expand_items();
  void expand_rules();
  void advance_to_start();

  void end_rule();

  void insert_empty_rule();
  void add_symbol();

  void copy_action();
  int mark_symbol();

};