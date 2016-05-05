#ifndef CS_SYMBOL_H
#define CS_SYMBOL_H
#include "csglobal.h"
#include "cshash.h"

typedef struct s_symbol_ *csS_symbol;
typedef struct s_table_ *csS_table;


extern void *		csS_look(csS_table t, csS_symbol sym);
extern csS_table 	csS_empty(csH_tabfreefp fp,csS_table top);
extern void 		csS_insert(csS_table tab, csS_symbol sym, void *value);
extern csG_string 	csS_name(csS_symbol sym);
extern csS_symbol 	csS_mksymbol(csG_string name);
extern void 		csS_tabfree(csS_table t);
#endif/*!CS_SYMBOL_H*/