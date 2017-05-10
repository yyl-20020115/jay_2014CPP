#include "defs.h"

#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__CYGWIN__)
#if defined ( __cplusplus)
extern "C"
{
#endif

  char* mktemp(char* );

#if defined( __cplusplus )
}
#endif

#define mkstemp mktemp

#endif

#if !defined(_WIN32)
#include <unistd.h>
#endif

GlobalInfo::GlobalInfo()
  : myname("yacc")
  , file_prefix("y")
  , temp_form("yacc.XXXXXXX")
  , action_file_name(0)
  , input_file_name(0)
  , prolog_file_name(0)
  , local_file_name(0)
  , verbose_file_name(0)
  , action_file(0)
  , input_file(0)
  , prolog_file(0)
  , local_file(0)
  , verbose_file(0)
  , tflag(0)
  , vflag(0)
  , csharp(0)
  , line_format("\t\t\t\t\t// line %d \"%s\"\n")
  , default_line_format("\t\t\t\t\t// line %d\n")
  , erp(0)
{

}

GlobalInfo::~GlobalInfo()
{
  if (action_file) { fclose(action_file); action_file = 0; unlink(action_file_name); FREE(action_file_name); action_file_name = 0;}
  if (prolog_file) { fclose(prolog_file); prolog_file = 0; unlink(prolog_file_name); FREE(prolog_file_name); prolog_file_name = 0;}
  if (local_file) { fclose(local_file); local_file = 0; unlink(local_file_name); FREE(local_file_name); local_file_name = 0;}
  if (verbose_file) { fclose(verbose_file); verbose_file = 0; unlink(verbose_file_name); FREE(verbose_file_name); verbose_file_name = 0;}

  
}
void GlobalInfo::done(int k)
{
  throw k;
}
void GlobalInfo::usage()
{
  fprintf(stderr, "usage: %s [-tvcp] [-b file_prefix] filename\n", myname);
  fprintf(stderr, "  example: %s -ctv <skeleton.cs cs-parser.jay > cs-parser.cs\n",myname);
  done(1);
}

void GlobalInfo::print_skel_dir(void)
{
  printf ("%s\n", SKEL_DIRECTORY);
  done(0);
}

void GlobalInfo::getargs(int argc, char* argv[])
{
  register int i;
  register char *s;

  if (argc > 0) myname = argv[0];
  for (i = 1; i < argc; ++i)
  {
    s = argv[i];
    if (*s != '-') break;
    switch (*++s)
    {
    case '\0':
      input_file = stdin;
      if (i + 1 < argc) usage();
      return;

    case '-':
      ++i;
      goto no_more_options;

    case 'b':
      if (*++s)
        file_prefix = s;
      else if (++i < argc)
        file_prefix = argv[i];
      else
        usage();
      continue;

    case 't':
      tflag = 1;
      break;

    case 'p':
      print_skel_dir ();
      break;

    case 'c':
      csharp = 1;
      line_format = "#line %d \"%s\"\n";
      default_line_format = "#line default\n";
      break;

    case 'v':
      vflag = 1;
      break;

    default:
      usage();
    }

    for (;;)
    {
      switch (*++s)
      {
      case '\0':
        goto end_of_option;

      case 't':
        tflag = 1;
        break;

      case 'v':
        vflag = 1;
        break;

      case 'p':
        print_skel_dir ();
        break;

      case 'c':
        csharp = 1;
        line_format = "#line %d \"%s\"\n";
        default_line_format = "#line default\n";

        break;

      default:
        usage();
      }
    }
end_of_option:;
  }

no_more_options:;
  if (i + 1 != argc) usage();
  input_file_name = argv[i];
}

void GlobalInfo::create_file_names()
{
  int i = 0, len = 0;
  char *tmpdir = 0;

#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__CYGWIN__)
  tmpdir = ".";
#else
  tmpdir = getenv("TMPDIR");
  if (tmpdir == 0) tmpdir = getenv ("TMP");
  if (tmpdir == 0) tmpdir = getenv ("TEMP");
  if (tmpdir == 0) tmpdir = "/tmp";
#endif

  len = strlen(tmpdir);
  i = len + 13;
  if (len && tmpdir[len-1] != '/')
    ++i;

  action_file_name = (char*)MALLOC(i);
  if (action_file_name == 0) erp->no_space();
  prolog_file_name = (char*)MALLOC(i);
  if (prolog_file_name == 0) erp->no_space();
  local_file_name = (char*)MALLOC(i);
  if (local_file_name == 0) erp->no_space();

  strcpy(action_file_name, tmpdir);
  strcpy(prolog_file_name, tmpdir);
  strcpy(local_file_name, tmpdir);

  if (len && tmpdir[len - 1] != '/')
  {
    action_file_name[len] = '/';
    prolog_file_name[len] = '/';
    local_file_name[len] = '/';
    ++len;
  }

  strcpy(action_file_name + len, temp_form);
  strcpy(prolog_file_name + len, temp_form);
  strcpy(local_file_name + len, temp_form);

  action_file_name[len + 5] = 'a';
  prolog_file_name[len + 5] = 'p';
  local_file_name[len + 5] = 'l';

  mkstemp(action_file_name);
  mkstemp(prolog_file_name);
  mkstemp(local_file_name);

  len = strlen(file_prefix);

  if (vflag)
  {
    verbose_file_name = (char*)MALLOC(len + 8);
    if (verbose_file_name == 0)
      erp->no_space();
    strcpy(verbose_file_name, file_prefix);
    strcpy(verbose_file_name + len, VERBOSE_SUFFIX);
  }
}
void GlobalInfo::open_files()
{
  create_file_names();

  if (input_file == 0)
  {
    input_file = fopen(input_file_name, "r");
    if (input_file == 0)
      erp->open_error(input_file_name);
  }

  action_file = fopen(action_file_name, "w");
  if (action_file == 0)
    erp->open_error(action_file_name);

  prolog_file = fopen(prolog_file_name, "w");
  if (prolog_file == 0)
    erp->open_error(prolog_file_name);

  local_file = fopen(local_file_name, "w");
  if (local_file == 0)
    erp->open_error(local_file_name);

  if (vflag)
  {
    verbose_file = fopen(verbose_file_name, "w");
    if (verbose_file == 0)
      erp->open_error(verbose_file_name);
  }
}
