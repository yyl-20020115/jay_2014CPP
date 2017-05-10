#pragma once

/* global variables */
class ErrorReporter;

struct GlobalInfo
{
public:

  char *myname;
  char *file_prefix;
  char *temp_form ;


public:

  char *input_file_name;
  char *action_file_name;
  char *prolog_file_name;
  char *local_file_name;
  char *verbose_file_name;

public:

  FILE *action_file;
  FILE *input_file;
  FILE *prolog_file;
  FILE *local_file;
  FILE *verbose_file;

public:

  bool tflag;
  bool vflag;
  bool csharp;

public:
  char* line_format;
  char* default_line_format;
  ErrorReporter* erp;

public:

  GlobalInfo();

  ~GlobalInfo();

public:

  void getargs(int argc, char* argv[]);

  void done(int k);

  void open_files();

protected:

  void create_file_names();

  void print_skel_dir();

  void usage();

} ;

