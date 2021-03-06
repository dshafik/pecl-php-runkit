/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2006 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sara Golemon <pollita@php.net>                               |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php_runkit.h"

#ifdef PHP_RUNKIT_MANIPULATION

#define RUNKIT_MAGIC_SETS(mname, fname, acc)	if (!strcasecmp(method, mname)) { ce->fname = fe; fe->common.fn_flags |= acc; return; }
#define RUNKIT_MAGIC_SET(mname)			RUNKIT_MAGIC_SETS(#mname, mname, 0)

void php_runkit_add_magic_method(zend_class_entry *ce, const char *method, zend_function *fe) {
#ifdef ZEND_ENGINE_2
	RUNKIT_MAGIC_SETS("__construct", constructor, ZEND_ACC_CTOR)
	RUNKIT_MAGIC_SETS(ce->name,      constructor, ZEND_ACC_CTOR)
	RUNKIT_MAGIC_SETS("__destruct",  destructor,  ZEND_ACC_DTOR)
	RUNKIT_MAGIC_SETS("__clone",     clone,       ZEND_ACC_CLONE)
	RUNKIT_MAGIC_SET(__get)
	RUNKIT_MAGIC_SET(__set)
	RUNKIT_MAGIC_SET(__isset)
	RUNKIT_MAGIC_SET(__unset)
	RUNKIT_MAGIC_SET(__call)
#ifdef ZEND_ENGINE_2_3
	RUNKIT_MAGIC_SET(__callstatic)
#endif
#ifdef ZEND_ENGINE_2_2
	RUNKIT_MAGIC_SET(__tostring)
#endif
	RUNKIT_MAGIC_SETS("__sleep",     serialize_func,   0)
	RUNKIT_MAGIC_SETS("__wakeup",    unserialize_func, 0)
#endif /* ZEND_ENGINE_2 */
}
#undef RUNKIT_MAGIC_SET
#undef RUNKIT_MAGIC_SETS

#define RUNKIT_MAGIC_DEL(fname)                 if (ce->fname == fe) { ce->fname = NULL; }

void php_runkit_del_magic_method(zend_class_entry *ce, zend_function *fe) {
#ifdef ZEND_ENGINE_2
	RUNKIT_MAGIC_DEL(constructor)
	RUNKIT_MAGIC_DEL(destructor)
	RUNKIT_MAGIC_DEL(clone)
	RUNKIT_MAGIC_DEL(__get)
	RUNKIT_MAGIC_DEL(__set)
	RUNKIT_MAGIC_DEL(__isset)
	RUNKIT_MAGIC_DEL(__unset)
	RUNKIT_MAGIC_DEL(__call)
#ifdef ZEND_ENGINE_2_3
	RUNKIT_MAGIC_DEL(__callstatic)
#endif
#ifdef ZEND_ENGINE_2_2
	RUNKIT_MAGIC_DEL(__tostring)
#endif
	RUNKIT_MAGIC_DEL(serialize_func)
	RUNKIT_MAGIC_DEL(unserialize_func)
#endif /* ZEND_ENGINE_2 */
}
#undef RUNKIT_MAGIC_DEL

#ifndef ZEND_ENGINE_2
/* {{{ _php_runkit_locate_scope
    ZendEngine 1 hack to determine a function's scope */
zend_class_entry *_php_runkit_locate_scope(zend_class_entry *ce, zend_function *fe, char *methodname, int methodname_len)
{
    zend_class_entry *current = ce->parent, *top = ce;
    zend_function *func;

    while (current) {
        if (zend_hash_find(&current->function_table, methodname, methodname_len + 1, (void **)&func) == FAILURE) {
            /* Not defined at this point (or higher) */
            return top;
        }
        if (func->op_array.opcodes != fe->op_array.opcodes) {
            /* Different function above this point */
            return top;
        }
        /* Same opcodes */
        top = current;

        current = current->parent;
    }

    return top;
}
/* }}} */
#endif

/* {{{ php_runkit_fetch_class
 */
int php_runkit_fetch_class(char *classname, int classname_len, zend_class_entry **pce TSRMLS_DC)
{
	zend_class_entry *ce;
#ifdef ZEND_ENGINE_2
	zend_class_entry **ze;
#endif

	php_runkit_skip_leading_ns_sep(&classname, &classname_len);
	php_strtolower(classname, classname_len);

#ifdef ZEND_ENGINE_2
	if (zend_hash_find(EG(class_table), classname, classname_len + 1, (void**)&ze) == FAILURE ||
		!ze || !*ze) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s not found", classname);
		return FAILURE;
	}
	ce = *ze;
#else
	if (zend_hash_find(EG(class_table), classname, classname_len + 1, (void**)&ce) == FAILURE ||
		!ce) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s not found", classname);
		return FAILURE;
	}
#endif

	if (ce->type != ZEND_USER_CLASS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s is not a user-defined class", classname);
		return FAILURE;
	}

#ifdef ZEND_ENGINE_2
	if (ce->ce_flags & ZEND_ACC_INTERFACE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s is an interface", classname);
		return FAILURE;
	}
#endif

	if (pce) {
		*pce = ce;
	}

    return SUCCESS;
}
/* }}} */

#ifdef ZEND_ENGINE_2
/* {{{ php_runkit_fetch_interface
 */
int php_runkit_fetch_interface(char *classname, int classname_len, zend_class_entry **pce TSRMLS_DC)
{
	php_strtolower(classname, classname_len);

	if (zend_hash_find(EG(class_table), classname, classname_len + 1, (void**)&pce) == FAILURE ||
		!pce || !*pce) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "interface %s not found", classname);
		return FAILURE;
	}

	if (!((*pce)->ce_flags & ZEND_ACC_INTERFACE)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s is not an interface", classname);
		return FAILURE;
	}

    return SUCCESS;
}
/* }}} */
#endif

/* {{{ php_runkit_fetch_class_method
 */
static int php_runkit_fetch_class_method(char *classname, int classname_len, char *fname, int fname_len, zend_class_entry **pce, zend_function **pfe 
TSRMLS_DC)
{
	HashTable *ftable = EG(function_table);
	zend_class_entry *ce;
	zend_function *fe;
#ifdef ZEND_ENGINE_2
	zend_class_entry **ze;
#endif

	/* We never promised the calling scope we'd leave classname untouched :) */
	php_runkit_skip_leading_ns_sep(&classname, &classname_len);
	php_strtolower(classname, classname_len);

#ifdef ZEND_ENGINE_2
	if (zend_hash_find(EG(class_table), classname, classname_len + 1, (void**)&ze) == FAILURE ||
		!ze || !*ze) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s not found", classname);
		return FAILURE;
	}
	ce = *ze;
#else
	if (zend_hash_find(EG(class_table), classname, classname_len + 1, (void**)&ce) == FAILURE ||
		!ce) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s not found", classname);
		return FAILURE;
	}
#endif

	if (ce->type != ZEND_USER_CLASS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s is not a user-defined class", classname);
		return FAILURE;
	}

	if (pce) {
		*pce = ce;
	}

	ftable = &ce->function_table;

	php_strtolower(fname, fname_len);
	if (zend_hash_find(ftable, fname, fname_len + 1, (void**)&fe) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s() not found", classname, fname);
		return FAILURE;
	}

	if (fe->type != ZEND_USER_FUNCTION) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s() is not a user function", classname, fname);
		return FAILURE;
	}

	if (pfe) {
		*pfe = fe;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_update_children_methods
	Scan the class_table for children of the class just updated */
int php_runkit_update_children_methods(zend_class_entry *ce ZEND_HASH_APPLY_ARGS_TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *ancestor_class =  va_arg(args, zend_class_entry*);
	zend_class_entry *parent_class =  va_arg(args, zend_class_entry*);
	zend_class_entry *scope;
	zend_function *fe =  va_arg(args, zend_function*), fecopy;
	char *fname = va_arg(args, char*);
	int fname_len = va_arg(args, int);
	zend_function *cfe = NULL;
#ifndef ZEND_ENGINE_2_3
	TSRMLS_FETCH();
#endif

#ifdef ZEND_ENGINE_2
	ce = *((zend_class_entry**)ce);
#endif

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	if (zend_hash_find(&ce->function_table, fname, fname_len + 1, (void**)&cfe) == SUCCESS) {
		scope = php_runkit_locate_scope(ce, cfe, fname, fname_len);
		if (scope != ancestor_class) {
			/* This method was defined below our current level, leave it be */
			return ZEND_HASH_APPLY_KEEP;
		}
		php_runkit_function_reflection_remove(cfe TSRMLS_CC);
	}

	/* Process children of this child */
	zend_hash_apply_with_arguments(EG(class_table) ZEND_HASH_APPLY_ARGS_TSRMLS_CC, (apply_func_args_t)php_runkit_update_children_methods, 5, ancestor_class, ce, fe, fname, fname_len);

	fecopy = *fe;
	fe = &fecopy;
	php_runkit_function_copy_ctor(fe, NULL);

	php_runkit_del_magic_method(ce, cfe);
	if (cfe && ce->constructor &&
	    !strcasecmp(fname, ce->constructor->common.function_name)) {
		/* Function being replaced is the constructor */
		ce->constructor = NULL;
	}
	if (zend_hash_add_or_update(&ce->function_table, fname, fname_len + 1, fe, sizeof(zend_function), (void**)&fe, cfe ? HASH_UPDATE : HASH_ADD) ==  FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error updating child class");
		return ZEND_HASH_APPLY_KEEP;
	}
	fe->common.prototype = fe;

	php_runkit_add_magic_method(ce, fname, fe);
	if (!ce->constructor && parent_class->constructor) {
		/* We have no constructor, but our parent does.
		 * Bind it from our inherited copy
		 */
		zend_function *pctor = parent_class->constructor, *ctor;
		if (zend_hash_find(&ce->function_table,
		                   pctor->common.function_name, strlen(pctor->common.function_name) + 1,
		                   (void**)&ctor) == SUCCESS) {
			ce->constructor = ctor;
		}
	}

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_clean_children
	Scan the class_table for children of the class just updated */
int php_runkit_clean_children_methods(zend_class_entry *ce ZEND_HASH_APPLY_ARGS_TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *ancestor_class =  va_arg(args, zend_class_entry*);
	zend_class_entry *parent_class =  va_arg(args, zend_class_entry*);
	zend_class_entry *scope;
	char *fname = va_arg(args, char*);
	int fname_len = va_arg(args, int);
	ulong fname_hash = zend_get_hash_value(fname, fname_len + 1); /* TODO: Make this part of args */
	zend_function *cfe = NULL;
#ifndef ZEND_ENGINE_2_3
	TSRMLS_FETCH();
#endif

#ifdef ZEND_ENGINE_2
	ce = *((zend_class_entry**)ce);
#endif

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	if (zend_hash_quick_find(&ce->function_table, fname, fname_len + 1, fname_hash, (void**)&cfe) == SUCCESS) {
		scope = php_runkit_locate_scope(ce, cfe, fname, fname_len);
		if (scope != ancestor_class) {
			/* This method was defined below our current level, leave it be */
			return ZEND_HASH_APPLY_KEEP;
		}
	}

	if (!cfe) {
		/* Odd.... nothing to destroy.... */
		return ZEND_HASH_APPLY_KEEP;
	}

	/* Process children of this child */
	zend_hash_apply_with_arguments(EG(class_table) ZEND_HASH_APPLY_ARGS_TSRMLS_CC, (apply_func_args_t)php_runkit_clean_children_methods, 4, ancestor_class, ce, fname, fname_len);

	php_runkit_del_magic_method(ce, cfe);
	php_runkit_function_delete(&ce->function_table, fname, fname_len + 1, fname_hash TSRMLS_CC);

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_method_add_or_update
 */
static void php_runkit_method_add_or_update(INTERNAL_FUNCTION_PARAMETERS, int add_or_update)
{
	char *classname, *methodname, *arguments, *phpcode;
	int classname_len, methodname_len, arguments_len, phpcode_len;
	zend_class_entry *ce, *ancestor_class = NULL;
	zend_function func, *fe, *oldfunc = NULL;
	long argc = ZEND_NUM_ARGS();
	ulong methodname_hash;
#ifdef ZEND_ENGINE_2
	long flags = ZEND_ACC_PUBLIC;
#else
	long flags; /* dummy variable */
#endif

	if (zend_parse_parameters(argc TSRMLS_CC, "s/s/ss|l",	&classname, &classname_len,
															&methodname, &methodname_len,
															&arguments, &arguments_len,
															&phpcode, &phpcode_len,
															&flags) == FAILURE) {
		RETURN_FALSE;
	}
	php_strtolower(classname, classname_len);
	php_strtolower(methodname, methodname_len);
	methodname_hash = zend_get_hash_value(methodname, methodname_len + 1);

#ifndef ZEND_ENGINE_2
	if ((argc > 4) && flags) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Flags parameter ignored in PHP versions prior to 5.0.0");
	}
#endif

	if (!classname_len || !methodname_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty parameter given");
		RETURN_FALSE;
	}

	if (add_or_update == HASH_UPDATE) {
		if (php_runkit_fetch_class_method(classname, classname_len, methodname, methodname_len, &ce, &oldfunc TSRMLS_CC) == FAILURE) {
			RETURN_FALSE;
		}
		ancestor_class = php_runkit_locate_scope(ce, oldfunc, methodname, methodname_len);

		if (php_runkit_check_call_stack(&oldfunc->op_array TSRMLS_CC) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot redefine a method while that method is active.");
			RETURN_FALSE;
		}
	} else {
		if (php_runkit_fetch_class(classname, classname_len, &ce TSRMLS_CC) == FAILURE) {
			RETURN_FALSE;
		}
		ancestor_class = ce;
	}

	if (php_runkit_generate_lambda_method(arguments, arguments_len, phpcode, phpcode_len, &fe TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	func = *fe;
	php_runkit_function_copy_ctor(&func, estrndup(methodname, methodname_len));
#ifdef ZEND_ENGINE_2
	func.common.scope = ce;

	if (flags & ZEND_ACC_PRIVATE) {
		func.common.fn_flags &= ~ZEND_ACC_PPP_MASK;
		func.common.fn_flags |= ZEND_ACC_PRIVATE;
	} else if (flags & ZEND_ACC_PROTECTED) {
		func.common.fn_flags &= ~ZEND_ACC_PPP_MASK;
		func.common.fn_flags |= ZEND_ACC_PROTECTED;
	} else {
		func.common.fn_flags &= ~ZEND_ACC_PPP_MASK;
		func.common.fn_flags |= ZEND_ACC_PUBLIC;
	}

	func.common.fn_flags |= flags & (ZEND_ACC_STATIC|ZEND_ACC_ALLOW_STATIC);
#endif

	zend_hash_apply_with_arguments(EG(class_table) ZEND_HASH_APPLY_ARGS_TSRMLS_CC, (apply_func_args_t)php_runkit_update_children_methods, 5, ancestor_class, ce, &func, methodname, 
methodname_len);

	if (zend_hash_quick_add_or_update(&ce->function_table, methodname, methodname_len + 1, methodname_hash, &func, sizeof(zend_function), (void**)&fe, add_or_update) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add method to class");
		RETURN_FALSE;
	}
	if (oldfunc) {
		php_runkit_function_reflection_remove(oldfunc TSRMLS_CC);
	}

	if (php_runkit_function_delete(EG(function_table), RUNKIT_TEMP_FUNCNAME, sizeof(RUNKIT_TEMP_FUNCNAME), RUNKIT_TEMP_FUNCNAME_HASH TSRMLS_CC) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove temporary function entry");
		RETURN_FALSE;
	}

	fe->common.prototype = fe;

	php_runkit_add_magic_method(ce, methodname, fe);
	RETURN_TRUE;
}
/* }}} */

/* {{{ php_runkit_method_copy
 */
static int php_runkit_method_copy(char *dclass, int dclass_len, char *dfunc, int dfunc_len,
                                  char *sclass, int sclass_len, char *sfunc, int sfunc_len,
                                  char *orig_dfunc TSRMLS_DC)
{
	zend_class_entry *dce, *sce;
	zend_function dfe, *sfe;

	if (php_runkit_fetch_class_method(sclass, sclass_len, sfunc, sfunc_len, &sce, &sfe TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	if (php_runkit_fetch_class(dclass, dclass_len, &dce TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	if (zend_hash_exists(&dce->function_table, dfunc, dfunc_len + 1)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Destination method %s::%s() already exists", dclass, dfunc);
		return FAILURE;
	}

	dfe = *sfe;
	php_runkit_function_copy_ctor(&dfe, orig_dfunc);

#ifdef ZEND_ENGINE_2
	dfe.common.scope = dce;
#endif

	if (zend_hash_add(&dce->function_table, dfunc, dfunc_len + 1, &dfe, sizeof(zend_function), (void**)&sfe) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error adding method to class %s::%s()", dclass, dfunc);
		return FAILURE;
	}
	sfe->common.prototype = sfe;
	php_runkit_add_magic_method(dce, dfunc, sfe);

	zend_hash_apply_with_arguments(EG(class_table) ZEND_HASH_APPLY_ARGS_TSRMLS_CC, (apply_func_args_t)php_runkit_update_children_methods, 5, dce, dce, &dfe, dfunc, dfunc_len);

	return SUCCESS;
}
/* }}} */

/* **************
   * Method API *
   ************** */

/* {{{ proto bool runkit_method_add(string classname, string methodname, string args, string code[, long flags])
	Add a method to an already defined class */
PHP_FUNCTION(runkit_method_add)
{
	php_runkit_method_add_or_update(INTERNAL_FUNCTION_PARAM_PASSTHRU, HASH_ADD);
}
/* }}} */

/* {{{ proto bool runkit_method_redefine(string classname, string methodname, string args, string code[, long flags])
	Redefine an already defined class method */
PHP_FUNCTION(runkit_method_redefine)
{
	php_runkit_method_add_or_update(INTERNAL_FUNCTION_PARAM_PASSTHRU, HASH_UPDATE);
}
/* }}} */

/* {{{ proto bool runkit_method_remove(string classname, string methodname)
	Remove a method from a class definition */
PHP_FUNCTION(runkit_method_remove)
{
	char *classname, *methodname;
	int classname_len, methodname_len;
	zend_class_entry *ce, *ancestor_class = NULL;
	zend_function *fe;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/",	&classname, &classname_len,
																	&methodname, &methodname_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (!classname_len || !methodname_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty parameter given");
		RETURN_FALSE;
	}

	if (php_runkit_fetch_class_method(classname, classname_len, methodname, methodname_len, &ce, &fe TSRMLS_CC) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown method %s::%s()", classname, methodname);
		RETURN_FALSE;
	}

	ancestor_class = php_runkit_locate_scope(ce, fe, methodname, methodname_len);

	zend_hash_apply_with_arguments(EG(class_table) ZEND_HASH_APPLY_ARGS_TSRMLS_CC, (apply_func_args_t)php_runkit_clean_children_methods, 4, ancestor_class, ce, methodname, methodname_len);

	if (php_runkit_function_delete(&ce->function_table, methodname, methodname_len + 1, 0 TSRMLS_CC) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove method from class");
		RETURN_FALSE;
	}

	php_runkit_del_magic_method(ce, fe);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool runkit_method_rename(string classname, string methodname, string newname)
	Rename a method within a class */
PHP_FUNCTION(runkit_method_rename)
{
	char *classname, *methodname, *newname;
	int classname_len, methodname_len, newname_len;
	zend_class_entry *ce, *ancestor_class = NULL;
	zend_function *fe, func;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/s/",	&classname, &classname_len,
																	&methodname, &methodname_len,
																	&newname, &newname_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (!classname_len || !methodname_len || !newname_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty parameter given");
		RETURN_FALSE;
	}

	if (php_runkit_fetch_class_method(classname, classname_len, methodname, methodname_len, &ce, &fe TSRMLS_CC) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown method %s::%s()", classname, methodname);
		RETURN_FALSE;
	}

	php_strtolower(newname, newname_len);

	if (zend_hash_exists(&ce->function_table, newname, newname_len + 1)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s() already exists", classname, newname);
		RETURN_FALSE;
	}

	ancestor_class = php_runkit_locate_scope(ce, fe, methodname, methodname_len);
	zend_hash_apply_with_arguments(EG(class_table) ZEND_HASH_APPLY_ARGS_TSRMLS_CC, (apply_func_args_t)php_runkit_clean_children_methods, 4, ancestor_class, ce, methodname, methodname_len);

	func = *fe;
	php_runkit_function_copy_ctor(&func, estrndup(newname, newname_len));

	php_runkit_del_magic_method(ce, fe);

	if (php_runkit_function_delete(&ce->function_table, methodname, methodname_len + 1, 0 TSRMLS_CC) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove old method reference from class");
		RETURN_FALSE;
	}

	if (zend_hash_add(&ce->function_table, newname, newname_len + 1, &func, sizeof(zend_function), (void**)&fe) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add new reference to class method");
		zend_function_dtor(&func);
		RETURN_FALSE;
	}

	fe->common.prototype = fe;
	php_runkit_add_magic_method(ce, newname, fe);
	zend_hash_apply_with_arguments(EG(class_table) ZEND_HASH_APPLY_ARGS_TSRMLS_CC, (apply_func_args_t)php_runkit_update_children_methods, 5, ce, ce, fe, newname, newname_len);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool runkit_method_copy(string destclass, string destmethod, string srcclass[, string srcmethod])
	Copy a method from one name to another or from one class to another */
PHP_FUNCTION(runkit_method_copy)
{
	char *dclass, *dfunc, *sclass, *sfunc = NULL, *orig_dfunc;
	int dclass_len, dfunc_len, sclass_len, sfunc_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/s/|s/",   &dclass,    &dclass_len,
																		&dfunc,     &dfunc_len,
																		&sclass,    &sclass_len,
																		&sfunc,     &sfunc_len) == FAILURE) {
		RETURN_FALSE;
	}
	orig_dfunc = estrndup(dfunc, dfunc_len);

	php_strtolower(sclass, sclass_len);
	php_strtolower(dclass, dclass_len);
	php_strtolower(dfunc, dfunc_len);

	if (!sfunc) {
		sfunc = dfunc;
		sfunc_len = dfunc_len;
	} else {
		php_strtolower(sfunc, sfunc_len);
	}

	if (FAILURE == php_runkit_method_copy(dclass, dclass_len, dfunc, dfunc_len,
	                                      sclass, sclass_len, sfunc, sfunc_len,
                                              orig_dfunc TSRMLS_CC)) {
		efree(orig_dfunc);
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */
#endif /* PHP_RUNKIT_MANIPULATION */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

