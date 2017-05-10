#pragma once

class ErrorReporter
{
private:
  GlobalInfo* info;
  Reader* reader;

public:

  ErrorReporter(GlobalInfo* info,Reader* reader);

public:
  
  void SetReader(Reader* reader){this->reader = reader;}

public:

  void fatal(char* msg);
  void no_space();

  void print_pos(char* st_line,char* st_cptr);
  void undefined_goal(char* s);
  void undefined_symbol_warning(char* s);
  void unterminated_comment(int c_lineno,char* c_line,char* c_cptr);
  void unterminated_text(int c_lineno,char* c_line,char* c_cptr);
  void unterminated_string(int c_lineno,char* c_line,char* c_cptr);
  void syntax_error(int c_lineno,char* c_line,char* c_cptr);
  void illegal_character(char* c_cptr);
  void used_reserved(char* s);
  void unexpected_EOF();
  void illegal_tag(int t_lineno,char* t_line, char* t_cptr);
  void tokenized_start(char* s);
  void retyped_warning(char* s);
  void reprec_warning(char* s);
  void revalued_warning(char* s);
  void terminal_start(char *s);
  void restarted_warning();
  void no_grammar();
  void terminal_lhs(int s_lineno);
  void prec_redeclared();
  void unterminated_action(int a_lineno,char* a_line, char* a_cptr);
  void untyped_rhs(int i, char* s);
  void unknown_rhs(int i);
  void untyped_lhs();
  void dollar_error(int a_lineno,char* a_line,char* a_cptr);
  void dollar_warning(int a_lineno, int i);
  void default_action_warning();
  void open_error(char* filename);

protected:
  void done(int k);


};