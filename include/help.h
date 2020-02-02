#ifndef HELP_H
#define HELP_H

#ifndef APPNAME
#pragma message("APPNAME not defined; are you using the Makefile?")
#define APPNAME "fido2-hmac-secret"
#endif

#ifndef APPVERSION
#pragma message("APPVERSION not defined; are you using the Makefile?")
#define APPVERSION "unknown (not built with APPVERSION defined)"
#endif

void print_version(void);
void print_usage(char *program_name);
void print_help(char *program_name);

#endif
