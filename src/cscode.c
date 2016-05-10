#include "cscode.h"
#include "cssymbol.h"
#include "cslist.h"
#include "csutil.h"
#include <string.h>
csC_fraglist fraglist = LIST_HEAD_INIT(fraglist);
static csC_quadlist quadlist;
static csF_frame curframe;

static csC_info c_info_();
static csC_quadlist c_quadlist_();
static csC_frag c_frag_(csF_access access,csC_info inf);
static csC_frag c_procfrag_(csC_quadlist body,csF_frame frame);
static csC_address c_address_();
static csC_address c_addressint_(int intconst);
static csC_address c_addressstr_(csG_string strconst);
static csC_address c_addressbool_(csG_bool boolconst);
static csC_address c_addressenv_(csE_enventry eval);
static csC_address c_addresstemp_(csT_temp tmp);
static csC_address c_addresslable_(csT_label lab);
static csC_quad c_quad_(csC_address arg1,csC_address arg2,csC_address res,c_opkind_ op);
static csC_info c_opdispatch_(csA_op op,csC_info inf,csC_info tmp);
static void c_addrdispatch_(csA_op op,csC_address arg1,csC_address arg2,csC_address res);



static csC_info c_dec_(csS_table val,csS_table type,csA_dec list);
static csC_info c_locdeclist(csS_table vtab,csS_table ttab,csA_locdeclist list);
static csC_info c_simplelist_(csS_table vtab,csS_table ttab,csA_simplelist foo);
static csC_info c_andlist_(csS_table vtab,csS_table ttab,csA_andlist foo);
static csC_info c_andexpr_(csS_table vtab,csS_table ttab,csA_andexpr foo);
static csC_info c_urelexpr_(csS_table vtab,csS_table ttab,csA_urelexpr foo);
static csC_info c_relexpr_(csS_table vtab,csS_table ttab,csA_relexpr foo);
static csC_info c_sumexprlist_(csS_table vtab,csS_table ttab,csA_sumexprlist foo);
static csC_info c_termlist_(csS_table vtab,csS_table ttab,csA_termlist foo);
static csC_info c_uexpr_(csS_table vtab,csS_table ttab,csA_uexpr foo);
static csC_info c_factor_(csS_table vtab,csS_table ttab,csA_factor foo);
static csC_info c_immutable_(csS_table vtab,csS_table ttab,csA_immutable foo);
static csC_info c_mutable_(csS_table vtab,csS_table ttab,csA_mutable foo);

static csC_info c_infoconst_(csT_type ty)
{
	csC_info inf;
	memset(&inf,0,sizeof(csC_info));
	VERIFY(ty);
	switch (ty->kind) {
	case csT_int:
		inf.kind = c_intconst_;
		break;
	case csT_bool:
		inf.kind = c_boolconst_;
		break;
	case csT_string:
		inf.kind = c_strconst_;
		break;	
	default:
		VERIFY(0);
	}
	return inf;
}

static csC_info c_info_()
{
	csC_info inf;
	memset(&inf,0,sizeof(inf));
	return inf;
}

static csC_frag c_frag_(csF_access access,csC_info inf)
{
	csC_frag foo = csU_malloc(sizeof(*foo));
	INIT_LIST_HEAD(&foo->next);
	switch (inf.kind) {
	case c_intconst_:
		foo->kind = csC_intfrag;
		foo->u.intv = inf.u.intconst;
		break;
	case c_strconst_:
		foo->kind = csC_strfrag;
		foo->u.strv = inf.u.strconst;
		break;
	case c_boolconst_:
		foo->kind = csC_boolfrag;
		foo->u.boolv = inf.u.boolconst;
		break;
	default:
		VERIFY(0);
	}
	foo->access = access;
	list_add_tail(&foo->next, &fraglist);
	return foo;
}

static csC_frag c_procfrag_(csC_quadlist body,csF_frame frame)
{
	VERIFY(body);VERIFY(frame);
	csC_frag foo = csU_malloc(sizeof(*foo));
	foo->kind = csC_procfrag;
	foo->u.proc.body = body;
	foo->u.proc.frame = frame;
	list_add_tail(&foo->next, &fraglist);
	return foo;
}

static csT_typelist c_mktypelist_(csS_table ttab,csA_paramlist foo)
{
	csT_typelist list = csT_mktypelist();
	csA_param pos;
	list_for_each_entry(pos, foo, next) {
		VERIFY(pos);
		csS_symbol tyname = csA_paramtype(pos);
		csT_type ty = csS_look(ttab, tyname);
		VERIFY(ty);
		csT_typelistadd(list,ty);
	}
	return list;
}

static csC_address c_infotoaddr(csC_info inf)
{
	csC_address addr;
	switch (inf.kind) {
		case c_addr_:
			addr = inf.u.addr;
			break;
		case c_intconst_:
			addr = c_addressint_(inf.u.intconst);
			break;
		case c_strconst_:
			addr = c_addressstr_(inf.u.strconst);
			break;
		case c_boolconst_:
			addr = c_addressbool_(inf.u.boolconst);
			break;
		default:
			VERIFY(0);
	}
	return addr;
}

csC_info c_declist_(csS_table val,csS_table type,csA_declist list)
{
	csC_info inf = c_info_();
	if (!list) return inf;
	csA_dec pos = NULL;
	list_for_each_entry(pos, list, next) {
		VERIFY(pos);
		c_dec_(val,type,pos);
	}
}

static csC_info c_dec_(csS_table vtab,csS_table ttab,csA_dec foo)
{
	VERIFY(foo);
	csC_info inf = c_info_();
	switch (foo->kind) {
	case csA_vardec: {
		quadlist = NULL;
		csS_symbol tyname = csA_decvartype(foo);
		csS_symbol name = csA_decvarname(foo);
		csT_type ty = csS_look(ttab, tyname);
		VERIFY(ty);
		csE_enventry e = csS_look(vtab, name);
		VERIFY(!e);
		csF_access access = csF_allocglobal(ty);
		e = csE_varentry(ty,access,name);
		csS_insert(vtab, name, e);
		csA_simplelist list = csA_decvarlist(foo);
		if (list) {
			csC_info info = c_simplelist_(vtab,ttab,list);
			c_frag_(access, info);
		} else {
			c_frag_(access, c_infoconst_(ty));
		}
		break;
	}
	case csA_fundec:{
		csS_symbol restype = csA_decfunrestype(foo);
		csS_symbol name = csA_decfunname(foo);
		csT_type ty = csS_look(ttab, restype);
		VERIFY(ty);
		csE_enventry e = csS_look(vtab, name);
		VERIFY(!e);
		csA_paramlist plist = csA_decfunparamlist(foo);
		VERIFY(plist);
		csT_typelist list = c_mktypelist_(ttab,plist);
		csF_frame frame = csF_newframe(name);
		curframe = frame;
		csT_label lable = csT_namedlabel(csS_name(name));
		e = csE_funentry(list,ty,name,frame);
		csS_insert(vtab, name, e);
		{
			csS_beginscope(vtab);
			csA_param pos = NULL;
			list_for_each_entry(pos, plist, next) {
				csF_access access = csF_alloclocal(frame);
				csT_type type = csS_look(ttab, csA_paramtype(pos));
				csS_symbol name = csA_paramname(pos);
				csS_insert(vtab, name, csE_varentry(type,access,name));
			}
			c_quadlist_();
			c_quad_(c_address_(),c_address_(),c_addresslable_(lable),csC_lable);
			csA_locdeclist list = csA_decfunloclist(foo);
			if (list)
				c_locdeclist(vtab,ttab,list);
			csS_endscope(vtab);
		}
		c_procfrag_(quadlist,frame);
		break;
	}
	default:
		VERIFY(0);
	}
	return inf;
}


static csC_info c_locdeclist(csS_table vtab,csS_table ttab,csA_locdeclist list)
{
	csC_info inf = c_info_();
	VERIFY(list);
	csA_locdec pos = NULL;
	list_for_each_entry(pos, list, next) {
		VERIFY(pos);
		csS_symbol tyname = csA_locdectype(pos);
		csS_symbol name = csA_locdecname(pos);
		csT_type ty = csS_look(ttab, tyname);
		VERIFY(ty);
		csE_enventry e = csS_looktop(vtab, name);
		VERIFY(!e);
		csF_access access = csF_alloclocal(curframe);
		csA_simplelist list = csA_locdecsimlist(pos);
		if (list) {
			csC_info tmp = c_simplelist_(vtab,ttab,list);
			csE_enventry env = csE_varentry(ty,access,name);

			csC_address arg1 = c_infotoaddr(tmp);
			csC_address arg2 = c_address_();
			csC_address res = c_addressenv_(env);
			c_quad_(arg1,arg2,res,csC_assign);
		}
		csS_insert(vtab, name, csE_varentry(ty,access,name));
	}
	return inf;
}

static csC_info c_simplelist_(csS_table vtab,csS_table ttab,csA_simplelist foo)
{
	csC_info inf = c_info_();
	csA_simpleexpr pos;
	if (!foo) VERIFY(0);
	list_for_each_entry(pos, foo, next) {
		csC_info tmp = c_andlist_(vtab,ttab,csA_simpleexprand(pos));
		if (inf.kind != c_empty_) {
			VERIFY(tmp.ty);
			VERIFY(tmp.ty->kind == csT_bool);
			if (tmp.kind == c_boolconst_ && inf.kind == c_boolconst_)
				tmp.u.boolconst = inf.u.boolconst || tmp.u.boolconst;
			else {
				csC_address arg1 = c_infotoaddr(inf);
				csC_address arg2 = c_infotoaddr(tmp);
				csC_address res = c_addresstemp_(csT_newtemp());
				c_quad_(arg1,arg2,res,csC_or);
				tmp.u.addr = res;
				tmp.kind = c_addr_;
			}
		}
		inf = tmp;
	}
	return inf;
}

static csC_info c_andlist_(csS_table vtab,csS_table ttab,csA_andlist foo)
{
	csC_info inf = c_info_();
	csA_andexpr pos;
	if (!foo) VERIFY(0);
	list_for_each_entry(pos, foo, next) {
		csC_info tmp = c_urelexpr_(vtab,ttab,csA_andexprurel(pos));
		if (inf.kind != c_empty_) {
			VERIFY(tmp.ty);
			VERIFY(tmp.ty->kind == csT_bool);
			if (tmp.kind == c_boolconst_ && inf.kind == c_boolconst_)
				tmp.u.boolconst = inf.u.boolconst && tmp.u.boolconst;
			else {
				csC_address arg1 = c_infotoaddr(inf);
				csC_address arg2 = c_infotoaddr(tmp);
				csC_address res = c_addresstemp_(csT_newtemp());
				c_quad_(arg1,arg2,res,csC_and);
				tmp.u.addr = res;
				tmp.kind = c_addr_;
			}
		} 
		inf = tmp;
	}
	return inf;
}

/*
struct a_urelexpr_ {
	csL_list next;
	csG_bool flags; 
	csA_relexpr rel;
	csG_pos pos;
};
*/
static csC_info c_urelexpr_(csS_table vtab,csS_table ttab,csA_urelexpr foo)
{
	csC_info inf;
	VERIFY(foo);
	if (foo->flags)
		VERIFY(0);
	inf = c_relexpr_(vtab,ttab,csA_urelexprrel(foo));
	return inf;
}
/*
struct a_relexpr_ {
	csA_sumexprlist sum1;
	csA_op op;
	csA_sumexprlist sum2;
	csG_pos pos;
};
*/
static csC_info c_relexpr_(csS_table vtab,csS_table ttab,csA_relexpr foo)
{
	csC_info inf;
	VERIFY(foo);
	inf = c_sumexprlist_(vtab,ttab,csA_relexprsum1(foo));
	VERIFY(!csA_relexprop(foo));
	return inf;
}

static csC_info c_sumexprlist_(csS_table vtab,csS_table ttab,csA_sumexprlist foo)
{
	csC_info inf;
	csA_sumexpr pos;
	VERIFY(foo);
	csA_op op  = 0;
	list_for_each_entry(pos, foo, next) {
		csC_info tmp = c_termlist_(vtab,ttab,csA_sumexprterm(pos));
		if (op) {
			VERIFY(tmp.ty);
			VERIFY(tmp.ty->kind == csT_int || tmp.ty->kind == csT_string);
			if (tmp.kind == c_intconst_ && inf.kind == c_intconst_) {
				tmp = c_opdispatch_(op,inf,tmp);
			} else if (tmp.kind == c_strconst_ && inf.kind == c_strconst_) {
				VERIFY(0);
			} else {
				csC_address arg1 = c_infotoaddr(inf);
				csC_address arg2 = c_infotoaddr(tmp);
				csC_address res = c_addresstemp_(csT_newtemp());
				c_addrdispatch_(op,arg1,arg2,res);
				tmp.u.addr = res;
				tmp.kind = c_addr_;
			}
		}
		op = csA_sumexprop(pos);
		inf = tmp;
	}
	return inf;
}
/*
struct a_sumexpr_ {
	csL_list next;
	csA_op op;
	csA_termlist list;
	csG_pos pos;
};
*/

static csC_info c_termlist_(csS_table vtab,csS_table ttab,csA_termlist foo)
{
	csC_info inf;
	csA_term pos;
	VERIFY(foo);
	csA_op op  = 0;
	list_for_each_entry(pos, foo, next) {
		csC_info tmp = c_uexpr_(vtab,ttab,csA_termuexpr(pos));
		if (op) {
			if (tmp.kind == c_intconst_ && inf.kind == c_intconst_) {
				tmp = c_opdispatch_(op,inf,tmp);
			} else if (tmp.kind == c_strconst_ && inf.kind == c_strconst_) {
				VERIFY(0);
			}  else {
				csC_address arg1 = c_infotoaddr(inf);
				csC_address arg2 = c_infotoaddr(tmp);
				csC_address res = c_addresstemp_(csT_newtemp());
				c_addrdispatch_(op,arg1,arg2,res);
				tmp.u.addr = res;
				tmp.kind = c_addr_;
			}
		}
		op = csA_termop(pos);
		inf = tmp;
	}
	return inf;
}
static csC_info c_uexpr_(csS_table vtab,csS_table ttab,csA_uexpr foo)
{
	csC_info inf;
	VERIFY(foo);
	if (foo->flags)
		VERIFY(0);
	inf = c_factor_(vtab,ttab,csA_uexprfac(foo));
	return inf;
}

static csC_info c_factor_(csS_table vtab,csS_table ttab,csA_factor foo)
{
	csC_info inf;
	VERIFY(foo);
	switch (foo->kind) {
	case csA_immut:
		inf = c_immutable_(vtab,ttab,csA_factorimmut(foo));
		break;
	case csA_mut:
		inf = c_mutable_(vtab,ttab,csA_factormut(foo));
		break;
	default:
		VERIFY(0);
	}
	return inf;
}
/*
struct a_immutable_ {
	csG_pos pos;
	enum {
		csA_expr,csA_call,csA_num,csA_char,
		csA_str,csA_bool
	} kind;
	union {
		int val; 	// NUM CHAR true false
		csG_string str;	// STRING
		//CSastExpr expr;
		/* exprList is optional
		struct {csA_arglist args;csS_symbol id;} call;
	} u;
};
*/
//NUMCONST | CHARCONST | STRINGCONST | true | false
static csC_info c_immutable_(csS_table vtab,csS_table ttab,csA_immutable foo)
{
	csC_info inf = c_info_();
	VERIFY(foo);
	switch (foo->kind) {
    case csA_num_:
    	inf.kind = c_intconst_;
    	inf.u.intconst = csA_immutnum(foo);
    	inf.ty = csT_typeint();
    	break;
    case csA_char_:
    	VERIFY(0);
    	break;
    case csA_str_:
    	inf.kind = c_strconst_;
    	inf.u.strconst = csA_immutstr(foo);
    	inf.ty = csT_typestring();
    	break;
    case csA_bool_:
    	inf.kind = c_boolconst_;
    	inf.u.boolconst = csA_immutbool(foo);
    	inf.ty = csT_typebool();
    	break;
    default:
    	VERIFY(0);
	}
	return inf;
}

static csC_info c_mutable_(csS_table vtab,csS_table ttab,csA_mutable foo)
{
	csC_info inf;
	VERIFY(foo);
	csS_symbol id = csA_mutid(foo);
	csE_enventry e = csS_look(vtab, id);
	VERIFY(e);
	VERIFY(e->kind == csE_var);
	inf.ty = e->u.var.type;
	VERIFY(inf.ty);
	inf.kind = c_addr_;
	inf.u.addr = c_addressenv_(e);
	return inf;
}

static csC_address c_address_()
{
	csC_address addr;
	memset(&addr,0,sizeof(addr));
	addr.kind = csC_empty;
	return addr;
}
static csC_address c_addressint_(int intconst)
{
	csC_address addr;
	addr.kind = csC_intconst;
	addr.u.ival = intconst;
	return addr;
}
static csC_address c_addressstr_(csG_string strconst)
{
	csC_address addr;
	addr.kind = csC_strconst;
	addr.u.str = strconst;
	return addr;
}
static csC_address c_addressbool_(csG_bool boolconst)
{
	csC_address addr;
	addr.kind = csC_boolconst;
	addr.u.bval = boolconst;
	return addr;
}
static csC_address c_addressenv_(csE_enventry eval)
{
	csC_address addr;
	addr.kind = csC_env;
	addr.u.eval = eval;
	VERIFY(eval);
	return addr;
}
static csC_address c_addresstemp_(csT_temp tmp)
{
	csC_address addr;
	addr.kind = csC_temp;
	addr.u.tmp = tmp;
	return addr;
}
static csC_address c_addresslable_(csT_label lab)
{
	VERIFY(lab);
	csC_address addr;
	addr.kind = caC_lable;
	addr.u.lab = lab;
	return addr;
}
static csC_quad c_quad_(csC_address arg1,csC_address arg2,csC_address res,c_opkind_ op)
{
	csC_quad foo = csU_malloc(sizeof(*foo));
	foo->arg1 = arg1;
	foo->arg2 = arg2;
	foo->res = res;
	foo->kind = op;
	INIT_LIST_HEAD(&foo->next);
	if(quadlist)
		list_add_tail(&foo->next, quadlist);
	return foo;
}
static csC_quadlist c_quadlist_()
{
	csC_quadlist foo = csU_malloc(sizeof(*foo));
	INIT_LIST_HEAD(foo);
	quadlist = foo;
	return foo;
}

static csC_info c_opdispatch_(csA_op op,csC_info inf,csC_info tmp)
{
	switch (op) {
	case csA_plus:
		tmp.u.intconst = inf.u.intconst + tmp.u.intconst;
		break;
	case csA_minus:
		tmp.u.intconst = inf.u.intconst - tmp.u.intconst;
		break;
	case csA_times:
		tmp.u.intconst = inf.u.intconst * tmp.u.intconst;
		break;
	case csA_divide:
		tmp.u.intconst = inf.u.intconst / tmp.u.intconst;
		break;
	default:
		VERIFY(0);
	}
	return tmp;
}
static void c_addrdispatch_(csA_op op,csC_address arg1,csC_address arg2,csC_address res)
{
	switch (op) {
	case csA_plus:
		c_quad_(arg1,arg2,res,csC_add);
		break;
	case csA_minus:
		c_quad_(arg1,arg2,res,csC_sub);
		break;
	case csA_times:
		c_quad_(arg1,arg2,res,csC_multiply);
		break;
	case csA_divide:
		c_quad_(arg1,arg2,res,csC_divide);
		break;
	default:
		VERIFY(0);
	}
}