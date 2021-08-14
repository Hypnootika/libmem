/*
 *  ----------------------------------
 * |         libmem - by rdbo         |
 * |      Memory Hacking Library      |
 *  ----------------------------------
 */

#include <libmem.h>
#include <Python.h>
#include <structmember.h>

#define DECL_GLOBAL(mod, name, val) { \
	PyObject *global; \
	global = PyLong_FromLong((long)val); \
	PyObject_SetAttrString(mod, name, global); \
	Py_DECREF(global); \
}

/* Python Types */

typedef struct {
	PyObject_HEAD
	lm_process_t proc;
} py_lm_process_obj;

static PyMemberDef py_lm_process_members[] = {
	{ "pid", T_INT, offsetof(py_lm_process_obj, proc.pid), 0, "" }
#	if LM_OS == LM_OS_WIN
	{ "handle", T_INT, offsetof(py_lm_process_obj, proc.handle), 0, "" }
#	endif
};

static PyTypeObject py_lm_process_t = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "libmem.lm_process_t",
	.tp_doc = "",
	.tp_basicsize = sizeof(py_lm_process_obj),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_members = py_lm_process_members
};

/****************************************/

typedef struct {
	PyObject_HEAD
	lm_module_t mod;
} py_lm_module_obj;

static PyMemberDef py_lm_module_members[] = {
	{ "base", T_ULONG, offsetof(py_lm_module_obj, mod.base), 0, "" },
	{ "end", T_ULONG, offsetof(py_lm_module_obj, mod.end), 0, "" },
	{ "size", T_ULONG, offsetof(py_lm_module_obj, mod.size), 0, "" }
};

static PyTypeObject py_lm_module_t = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "libmem.lm_module_t",
	.tp_doc = "",
	.tp_basicsize = sizeof(py_lm_module_obj),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_members = py_lm_module_members
};

/* Python Functions */
typedef struct {
	PyObject *callback;
	PyObject *arg;
} py_lm_enum_processes_t;

static lm_bool_t
py_LM_EnumProcessesCallback(lm_pid_t   pid,
			    lm_void_t *arg)
{
	py_lm_enum_processes_t *parg = (py_lm_enum_processes_t *)arg;
	PyLongObject           *pypid;
	PyLongObject           *pyret;

	pypid = (PyLongObject *)PyLong_FromPid(pid);
	
	pyret = (PyLongObject *)(
		PyObject_CallFunctionObjArgs(parg->callback,
					     pypid,
					     parg->arg,
					     NULL)
	);

	return PyLong_AsLong((PyObject *)pyret) ? LM_TRUE : LM_FALSE;
}

static PyObject *
py_LM_EnumProcesses(PyObject *self,
		    PyObject *args)
{
	py_lm_enum_processes_t arg;

	if (!PyArg_ParseTuple(args, "O|O", &arg.callback, &arg.arg))
		return NULL;
	
	return PyLong_FromLong(
		LM_EnumProcesses(
			py_LM_EnumProcessesCallback,
			(lm_void_t *)&arg
		)
	);
}

static PyObject *
py_LM_GetProcessId(PyObject *self,
		   PyObject *args)
{
	return (PyObject *)PyLong_FromPid(LM_GetProcessId());
}

static PyObject *
py_LM_GetProcessIdEx(PyObject *self,
		     PyObject *args)
{
	lm_tchar_t    *procstr;

#	if LM_CHARSET == LM_CHARSET_UC
	if (!PyArg_ParseTuple(args, "u", &procstr))
			return NULL;
#	else
	if (!PyArg_ParseTuple(args, "s", &procstr))
		return NULL;
#	endif

	return (PyObject *)PyLong_FromPid(LM_GetProcessIdEx(procstr));
}

static PyObject *
py_LM_GetParentId(PyObject *self,
		  PyObject *args)
{
	return (PyObject *)PyLong_FromPid(LM_GetParentId());
}

static PyObject *
py_LM_GetParentIdEx(PyObject *self,
		    PyObject *args)
{
	long pypid;

	if (!PyArg_ParseTuple(args, "l", &pypid))
		return NULL;

	return (PyObject *)PyLong_FromPid(LM_GetParentIdEx((lm_pid_t)pypid));
}

static PyObject *
py_LM_OpenProcess(PyObject *self,
		  PyObject *args)
{
	py_lm_process_obj *pyproc;

	pyproc = (py_lm_process_obj *)PyObject_CallNoArgs((PyObject *)&py_lm_process_t);

	LM_OpenProcess(&pyproc->proc);
	
	return (PyObject *)pyproc;
}

static PyObject *
py_LM_OpenProcessEx(PyObject *self,
		    PyObject *args)
{
	py_lm_process_obj *pyproc;
	long               pypid;

	if (!PyArg_ParseTuple(args, "l", &pypid))
		return NULL;

	pyproc = (py_lm_process_obj *)PyObject_CallNoArgs((PyObject *)&py_lm_process_t);

	LM_OpenProcessEx((lm_pid_t)pypid, &pyproc->proc);
	
	return (PyObject *)pyproc;
}

static PyObject *
py_LM_CloseProcess(PyObject *self,
		   PyObject *args)
{
	py_lm_process_obj *pyproc;
	
	if (!PyArg_ParseTuple(args, "O!", &py_lm_process_t, &pyproc))
		return NULL;

	LM_CloseProcess(&pyproc->proc);

	return PyLong_FromLong(0);
}

static PyObject *
py_LM_GetProcessPath(PyObject *self,
		     PyObject *args)
{
	PyUnicodeObject *pystr = (PyUnicodeObject *)NULL;
	lm_tchar_t      *pathbuf;
	lm_size_t        length;

	pathbuf = LM_CALLOC(LM_PATH_MAX, sizeof(lm_tchar_t));
	if (!pathbuf)
		return NULL;
	
	length = LM_GetProcessPath(pathbuf, LM_PATH_MAX);
	if (!length)
		goto _FREE_RET;
	
#	if LM_CHARSET == LM_CHARSET_UC
	pystr = (PyUnicodeObject *)PyUnicode_FromUnicode(pathbuf, length);
#	else
	pystr = (PyUnicodeObject *)PyUnicode_FromString(pathbuf);
#	endif

_FREE_RET:
	LM_FREE(pathbuf);
	return (PyObject *)pystr;
}

static PyObject *
py_LM_GetProcessPathEx(PyObject *self,
		       PyObject *args)
{
	PyUnicodeObject   *pystr = (PyUnicodeObject *)NULL;
	py_lm_process_obj *pyproc;
	lm_tchar_t        *pathbuf;
	lm_size_t          length;

	if (!PyArg_ParseTuple(args, "O!", &py_lm_process_t, &pyproc))
		return NULL;

	pathbuf = LM_CALLOC(LM_PATH_MAX, sizeof(lm_tchar_t));
	if (!pathbuf)
		return NULL;
	
	length = LM_GetProcessPathEx(pyproc->proc, pathbuf, LM_PATH_MAX);
	if (!length)
		goto _FREE_RET;
	
#	if LM_CHARSET == LM_CHARSET_UC
	pystr = (PyUnicodeObject *)PyUnicode_FromUnicode(pathbuf, length);
#	else
	pystr = (PyUnicodeObject *)PyUnicode_FromString(pathbuf);
#	endif

_FREE_RET:
	LM_FREE(pathbuf);
	return (PyObject *)pystr;
}

static PyObject *
py_LM_GetProcessName(PyObject *self,
		     PyObject *args)
{
	PyUnicodeObject *pystr = (PyUnicodeObject *)NULL;
	lm_tchar_t      *namebuf;
	lm_size_t        length;

	namebuf = LM_CALLOC(LM_PATH_MAX, sizeof(lm_tchar_t));
	if (!namebuf)
		return NULL;
	
	length = LM_GetProcessName(namebuf, LM_PATH_MAX);
	if (!length)
		goto _FREE_RET;
	
#	if LM_CHARSET == LM_CHARSET_UC
	pystr = (PyUnicodeObject *)PyUnicode_FromUnicode(namebuf, length);
#	else
	pystr = (PyUnicodeObject *)PyUnicode_FromString(namebuf);
#	endif

_FREE_RET:
	LM_FREE(namebuf);
	return (PyObject *)pystr;
}

static PyObject *
py_LM_GetProcessNameEx(PyObject *self,
		       PyObject *args)
{
	PyUnicodeObject   *pystr = (PyUnicodeObject *)NULL;
	py_lm_process_obj *pyproc;
	lm_tchar_t        *namebuf;
	lm_size_t          length;

	if (!PyArg_ParseTuple(args, "O!", &py_lm_process_t, &pyproc))
		return NULL;

	namebuf = LM_CALLOC(LM_PATH_MAX, sizeof(lm_tchar_t));
	if (!namebuf)
		return NULL;
	
	length = LM_GetProcessNameEx(pyproc->proc, namebuf, LM_PATH_MAX);
	if (!length)
		goto _FREE_RET;
	
#	if LM_CHARSET == LM_CHARSET_UC
	pystr = (PyUnicodeObject *)PyUnicode_FromUnicode(namebuf, length);
#	else
	pystr = (PyUnicodeObject *)PyUnicode_FromString(namebuf);
#	endif

_FREE_RET:
	LM_FREE(namebuf);
	return (PyObject *)pystr;
}

static PyObject *
py_LM_GetSystemBits(PyObject *self,
		    PyObject *args)
{
	return PyLong_FromLong(LM_GetSystemBits());
}

static PyObject *
py_LM_GetProcessBits(PyObject *self,
		     PyObject *args)
{
	return PyLong_FromLong(LM_GetProcessBits());
}

static PyObject *
py_LM_GetProcessBitsEx(PyObject *self,
		       PyObject *args)
{
	py_lm_process_obj *pyproc;

	if (!PyArg_ParseTuple(args, "O!", &py_lm_process_t, &pyproc))
		return NULL;
	
	return PyLong_FromLong(LM_GetProcessBitsEx(pyproc->proc));
}

/****************************************/

typedef struct {
	PyObject *callback;
	PyObject *arg;
} py_lm_enum_threads_t;

static lm_bool_t
py_LM_EnumThreadsCallback(lm_tid_t   tid,
			  lm_void_t *arg)
{
	PyLongObject         *pyret;
	PyLongObject         *pytid;
	py_lm_enum_threads_t *parg = (py_lm_enum_threads_t *)arg;

	pytid = (PyLongObject *)PyLong_FromLong((long)tid);

	pyret = (PyLongObject *)(
		PyObject_CallFunctionObjArgs(parg->callback,
					     pytid,
					     parg->arg,
					     NULL)
	);

	return PyLong_AsLong((PyObject *)pyret) ? LM_TRUE : LM_FALSE;
}

static PyObject *
py_LM_EnumThreads(PyObject *self,
		  PyObject *args)
{
	py_lm_enum_threads_t arg;

	if (!PyArg_ParseTuple(args, "O|O", &arg.callback, &arg.arg))
		return NULL;
	
	return PyLong_FromLong(
		LM_EnumThreads(py_LM_EnumThreadsCallback,
			       (lm_void_t *)&arg)
	);
}

static PyObject *
py_LM_EnumThreadsEx(PyObject *self,
		    PyObject *args)
{
	py_lm_process_obj   *pyproc;
	py_lm_enum_threads_t arg;

	if (!PyArg_ParseTuple(args, "O!O|O", &py_lm_process_t, &pyproc,
			      &arg.callback, &arg.arg))
		return NULL;
	
	return PyLong_FromLong(
		LM_EnumThreadsEx(pyproc->proc,
				 py_LM_EnumThreadsCallback,
				 (lm_void_t *)&arg)
	);
}

static PyObject *
py_LM_GetThreadId(PyObject *self,
		  PyObject *args)
{
	return (PyObject *)PyLong_FromLong((long)LM_GetThreadId());
}

static PyObject *
py_LM_GetThreadIdEx(PyObject *self,
		    PyObject *args)
{
	py_lm_process_obj *pyproc;

	if (!PyArg_ParseTuple(args, "O!", &py_lm_process_t, &pyproc))
		return NULL;

	return (PyObject *)(
		PyLong_FromLong((long)LM_GetThreadIdEx(pyproc->proc))
	);
}

/****************************************/

typedef struct {
	PyObject *callback;
	PyObject *arg;
} py_lm_enum_modules_t;

static lm_bool_t
py_LM_EnumModulesCallback(lm_module_t  mod,
			  lm_tstring_t path,
			  lm_void_t   *arg)
{
	PyLongObject         *pyret;
	PyUnicodeObject      *pypath;
	py_lm_module_obj     *pymod;
	py_lm_enum_modules_t *parg = (py_lm_enum_modules_t *)arg;

	pymod = (py_lm_module_obj *)(
		PyObject_CallNoArgs((PyObject *)&py_lm_module_t)
	);
	pymod->mod = mod;

#	if LM_CHARSET == LM_CHARSET_UC
	pypath = (PyUnicodeObject *)PyUnicode_FromUnicode(path,
							  LM_STRLEN(path));
#	else
	pypath = (PyUnicodeObject *)PyUnicode_FromString(path);
#	endif

	pyret = (PyLongObject *)(
		PyObject_CallFunctionObjArgs(parg->callback,
					     pymod,
					     pypath,
					     parg->arg,
					     NULL)
	);

	return PyLong_AsLong((PyObject *)pyret) ? LM_TRUE : LM_FALSE;

	return LM_TRUE;
}

static PyObject *
py_LM_EnumModules(PyObject *self,
		  PyObject *args)
{
	py_lm_enum_modules_t arg;

	if (!PyArg_ParseTuple(args, "O|O", &arg.callback, &arg.arg))
		return NULL;
	
	return PyLong_FromLong(
		LM_EnumModules(py_LM_EnumModulesCallback,
			       (lm_void_t *)&arg)
	);
}

static PyObject *
py_LM_EnumModulesEx(PyObject *self,
		    PyObject *args)
{
	py_lm_process_obj   *pyproc;
	py_lm_enum_modules_t arg;

	if (!PyArg_ParseTuple(args, "O!O|O", &py_lm_process_t, &pyproc,
			      &arg.callback, &arg.arg))
		return NULL;
	
	return PyLong_FromLong(
		LM_EnumModulesEx(pyproc->proc,
				 py_LM_EnumModulesCallback,
				 (lm_void_t *)&arg)
	);
}

static PyObject *
py_LM_GetModule(PyObject *self,
		PyObject *args)
{
	py_lm_module_obj *pymod;
	PyLongObject     *pyflags;
	PyObject         *pymodarg;
	lm_int_t          flags;

	if (!PyArg_ParseTuple(args, "O!O", &PyLong_Type, &pyflags, &pymodarg))
		return NULL;
	
	pymod = (py_lm_module_obj *)(
		PyObject_CallNoArgs((PyObject *)&py_lm_module_t)
	);
	flags = (lm_int_t)PyLong_AsLong((PyObject *)pyflags);

	if (flags == LM_MOD_BY_STR) {
		PyUnicodeObject *pymodstr = (PyUnicodeObject *)pymodarg;
		lm_tchar_t      *modstr;

#		if LM_CHARSET == LM_CHARSET_UC
		modstr = (lm_tchar_t *)(
			PyUnicode_AsUnicode((PyObject *)pymodstr)
		);
#		else
		modstr = (lm_tchar_t *)(
			PyUnicode_AsUTF8((PyObject *)pymodstr)
		);
#		endif

		LM_GetModule(flags, modstr, &pymod->mod);
	} else if (flags == LM_MOD_BY_ADDR) {
		PyLongObject *modaddr = (PyLongObject *)pymodarg;
		
		LM_GetModule(flags,
			     (lm_void_t *)PyLong_AsLong((PyObject *)modaddr),
			     &pymod->mod);
	} else {
		pymod->mod.base = (lm_address_t)LM_BAD;
		pymod->mod.size = 0;
		pymod->mod.end = pymod->mod.base;
	}

	return (PyObject *)pymod;
}

static PyObject *
py_LM_GetModuleEx(PyObject *self,
		  PyObject *args)
{
	py_lm_module_obj  *pymod;
	py_lm_process_obj *pyproc;
	PyLongObject      *pyflags;
	PyObject          *pymodarg;
	lm_int_t           flags;

	if (!PyArg_ParseTuple(args, "O!O!O", &py_lm_process_t, &pyproc,
			      &PyLong_Type, &pyflags, &pymodarg))
		return NULL;
	
	pymod = (py_lm_module_obj *)(
		PyObject_CallNoArgs((PyObject *)&py_lm_module_t)
	);
	flags = (lm_int_t)PyLong_AsLong((PyObject *)pyflags);

	if (flags == LM_MOD_BY_STR) {
		PyUnicodeObject *pymodstr = (PyUnicodeObject *)pymodarg;
		lm_tchar_t      *modstr;

#		if LM_CHARSET == LM_CHARSET_UC
		modstr = (lm_tchar_t *)(
			PyUnicode_AsUnicode((PyObject *)pymodstr)
		);
#		else
		modstr = (lm_tchar_t *)(
			PyUnicode_AsUTF8((PyObject *)pymodstr)
		);
#		endif

		LM_GetModuleEx(pyproc->proc, flags, modstr, &pymod->mod);
	} else if (flags == LM_MOD_BY_ADDR) {
		PyLongObject *modaddr = (PyLongObject *)pymodarg;
		
		LM_GetModuleEx(pyproc->proc,
			       flags,
			       (lm_void_t *)PyLong_AsLong((PyObject *)modaddr),
			       &pymod->mod);
	} else {
		pymod->mod.base = (lm_address_t)LM_BAD;
		pymod->mod.size = 0;
		pymod->mod.end = pymod->mod.base;
	}

	return (PyObject *)pymod;
}

static PyObject *
py_LM_GetModulePath(PyObject *self,
		    PyObject *args)
{
	PyUnicodeObject  *pymodpath = (PyUnicodeObject *)NULL;
	py_lm_module_obj *pymod;
	lm_tchar_t       *modpath;

	if (!PyArg_ParseTuple(args, "O!", &py_lm_module_t, &pymod))
		return NULL;
	
	modpath = LM_CALLOC(LM_PATH_MAX, sizeof(lm_tchar_t));
	if (!modpath)
		return NULL;
	
	if (LM_GetModulePath(pymod->mod, modpath, LM_PATH_MAX)) {
#		if LM_CHARSET == LM_CHARSET_UC
		pymodpath = (PyUnicodeObject *)PyUnicode_FromUnicode(modpath);
#		else
		pymodpath = (PyUnicodeObject *)PyUnicode_FromString(modpath);
#		endif
	}

	LM_FREE(modpath);

	return (PyObject *)pymodpath;
}

static PyObject *
py_LM_GetModulePathEx(PyObject *self,
		      PyObject *args)
{
	PyUnicodeObject   *pymodpath = (PyUnicodeObject *)NULL;
	py_lm_process_obj *pyproc;
	py_lm_module_obj  *pymod;
	lm_tchar_t        *modpath;

	if (!PyArg_ParseTuple(args, "O!O!", &py_lm_process_t, &pyproc,
			      &py_lm_module_t, &pymod))
		return NULL;
	
	modpath = LM_CALLOC(LM_PATH_MAX, sizeof(lm_tchar_t));
	if (!modpath)
		return NULL;
	
	if (LM_GetModulePathEx(pyproc->proc, pymod->mod,
			       modpath, LM_PATH_MAX)) {
#		if LM_CHARSET == LM_CHARSET_UC
		pymodpath = (PyUnicodeObject *)PyUnicode_FromUnicode(modpath);
#		else
		pymodpath = (PyUnicodeObject *)PyUnicode_FromString(modpath);
#		endif
	}

	LM_FREE(modpath);

	return (PyObject *)pymodpath;
}

static PyObject *
py_LM_GetModuleName(PyObject *self,
		    PyObject *args)
{
	PyUnicodeObject  *pymodname = (PyUnicodeObject *)NULL;
	py_lm_module_obj *pymod;
	lm_tchar_t       *modname;

	if (!PyArg_ParseTuple(args, "O!", &py_lm_module_t, &pymod))
		return NULL;
	
	modname = LM_CALLOC(LM_PATH_MAX, sizeof(lm_tchar_t));
	if (!modname)
		return NULL;
	
	if (LM_GetModuleName(pymod->mod, modname, LM_PATH_MAX)) {
#		if LM_CHARSET == LM_CHARSET_UC
		pymodname = (PyUnicodeObject *)PyUnicode_FromUnicode(modname);
#		else
		pymodname = (PyUnicodeObject *)PyUnicode_FromString(modname);
#		endif
	}

	LM_FREE(modname);

	return (PyObject *)pymodname;
}

static PyObject *
py_LM_GetModuleNameEx(PyObject *self,
		      PyObject *args)
{
	PyUnicodeObject   *pymodname = (PyUnicodeObject *)NULL;
	py_lm_process_obj *pyproc;
	py_lm_module_obj  *pymod;
	lm_tchar_t        *modname;

	if (!PyArg_ParseTuple(args, "O!O!", &py_lm_process_t, &pyproc,
			      &py_lm_module_t, &pymod))
		return NULL;
	
	modname = LM_CALLOC(LM_PATH_MAX, sizeof(lm_tchar_t));
	if (!modname)
		return NULL;
	
	if (LM_GetModuleNameEx(pyproc->proc, pymod->mod,
			       modname, LM_PATH_MAX)) {
#		if LM_CHARSET == LM_CHARSET_UC
		pymodname = (PyUnicodeObject *)PyUnicode_FromUnicode(modname);
#		else
		pymodname = (PyUnicodeObject *)PyUnicode_FromString(modname);
#		endif
	}

	LM_FREE(modname);

	return (PyObject *)pymodname;
}

static PyObject *
py_LM_LoadModule(PyObject *self,
		 PyObject *args)
{
	PyObject         *ret;
	lm_tchar_t       *modpath;
	py_lm_module_obj *pymodbuf;
	lm_module_t       modbuf;

#	if LM_CHARSET == LM_CHARSET_UC
	if (!PyArg_ParseTuple(args, "u|O!", &modpath,
			      &py_lm_module_t, &pymodbuf))
			return NULL;
#	else
	if (!PyArg_ParseTuple(args, "s|O!", &modpath,
			      &py_lm_module_t, &pymodbuf))
		return NULL;
#	endif

	ret = PyLong_FromLong(LM_LoadModule(modpath, &modbuf));

	if (pymodbuf)
		pymodbuf->mod = modbuf;

	return ret;
}

static PyObject *
py_LM_LoadModuleEx(PyObject *self,
		   PyObject *args)
{
	PyObject          *ret;
	lm_tchar_t        *modpath;
	py_lm_module_obj  *pymodbuf;
	py_lm_process_obj *pyproc;
	lm_module_t        modbuf;

#	if LM_CHARSET == LM_CHARSET_UC
	if (!PyArg_ParseTuple(args, "O!u|O!", &modpath,
			      &py_lm_process_t, &pyproc,
			      &py_lm_module_t, &pymodbuf))
			return NULL;
#	else
	if (!PyArg_ParseTuple(args, "O!s|O!", &modpath,
			      &py_lm_process_t, &pyproc,
			      &py_lm_module_t, &pymodbuf))
		return NULL;
#	endif

	ret = PyLong_FromLong(LM_LoadModuleEx(pyproc->proc, modpath, &modbuf));

	if (pymodbuf)
		pymodbuf->mod = modbuf;

	return ret;
}

static PyObject *
py_LM_UnloadModule(PyObject *self,
		   PyObject *args)
{
	py_lm_module_obj *pymod;

	if (!PyArg_ParseTuple(args, "O!", &py_lm_module_t, &pymod))
		return NULL;
	
	return PyLong_FromLong(LM_UnloadModule(pymod->mod));
}

static PyObject *
py_LM_UnloadModuleEx(PyObject *self,
		     PyObject *args)
{
	py_lm_process_obj *pyproc;
	py_lm_module_obj  *pymod;

	if (!PyArg_ParseTuple(args, "O!O!", &py_lm_process_t, &pyproc,
			      &py_lm_module_t, &pymod))
		return NULL;
	
	return PyLong_FromLong(LM_UnloadModuleEx(pyproc->proc, pymod->mod));
}

/****************************************/

typedef struct {
	PyObject *callback;
	PyObject *arg;
} py_lm_enum_symbols_t;

static lm_bool_t py_LM_EnumSymbolsCallback(lm_cstring_t symbol,
					   lm_address_t addr,
					   lm_void_t   *arg)
{
	py_lm_enum_symbols_t *parg = (py_lm_enum_symbols_t *)arg;
	PyUnicodeObject      *pysymbol;
	PyLongObject         *pyaddr;
	PyObject             *pyret;

	pysymbol = (PyUnicodeObject *)PyUnicode_FromString(symbol);
	pyaddr = (PyLongObject *)PyLong_FromVoidPtr(addr);

	pyret = PyObject_CallFunctionObjArgs(parg->callback,
					     pysymbol,
					     pyaddr,
					     parg->arg,
					     NULL);

	return PyLong_AsLong(pyret) ? LM_TRUE : LM_FALSE;
}


static PyObject *
py_LM_EnumSymbols(PyObject *self,
		  PyObject *args)
{
	py_lm_enum_symbols_t arg;
	py_lm_module_obj    *pymod;

	if (!PyArg_ParseTuple(args, "O!O|O", &py_lm_module_t, &pymod,
			      &arg.callback,
			      &arg.arg))
		return NULL;
	
	return PyLong_FromLong(
		LM_EnumSymbols(pymod->mod,
			       py_LM_EnumSymbolsCallback,
			       (lm_void_t *)&arg)
	);
}

static PyObject *
py_LM_EnumSymbolsEx(PyObject *self,
		    PyObject *args)
{
	py_lm_enum_symbols_t arg;
	py_lm_process_obj   *pyproc;
	py_lm_module_obj    *pymod;

	if (!PyArg_ParseTuple(args, "O!O!O|O", &py_lm_process_t, &pyproc,
			      &py_lm_module_t, &pymod,
			      &arg.callback,
			      &arg.arg))
		return NULL;
	
	return PyLong_FromLong(
		LM_EnumSymbolsEx(pyproc->proc,
				 pymod->mod,
				 py_LM_EnumSymbolsCallback,
				 (lm_void_t *)&arg)
	);
}

static PyObject *
py_LM_GetSymbol(PyObject *self,
		PyObject *args)
{
	py_lm_module_obj *pymod;
	lm_cstring_t      symstr;

	if (!PyArg_ParseTuple(args, "O!s", &py_lm_module_t, &pymod,
			      &symstr))
		return NULL;
	
	return PyLong_FromVoidPtr(LM_GetSymbol(pymod->mod, symstr));
}

static PyObject *
py_LM_GetSymbolEx(PyObject *self,
		  PyObject *args)
{
	py_lm_process_obj *pyproc;
	py_lm_module_obj  *pymod;
	lm_cstring_t       symstr;

	if (!PyArg_ParseTuple(args, "O!O!s", &py_lm_process_t, &pyproc,
			      &py_lm_module_t, &pymod,
			      &symstr))
		return NULL;
	
	return PyLong_FromVoidPtr(
		LM_GetSymbolEx(pyproc->proc, pymod->mod, symstr)
	);
}

/****************************************/

/* Python Module */
static PyMethodDef libmem_methods[] = {
	{ "LM_EnumProcesses", py_LM_EnumProcesses, METH_VARARGS, "" },
	{ "LM_GetProcessId", py_LM_GetProcessId, METH_NOARGS, "" },
	{ "LM_GetProcessIdEx", py_LM_GetProcessIdEx, METH_VARARGS, "" },
	{ "LM_GetParentId", py_LM_GetParentId, METH_NOARGS, "" },
	{ "LM_GetParentIdEx", py_LM_GetParentIdEx, METH_VARARGS, "" },
	{ "LM_OpenProcess", py_LM_OpenProcess, METH_NOARGS, "" },
	{ "LM_OpenProcessEx", py_LM_OpenProcessEx, METH_VARARGS, "" },
	{ "LM_CloseProcess", py_LM_CloseProcess, METH_VARARGS, "" },
	{ "LM_GetProcessPath", py_LM_GetProcessPath, METH_NOARGS, "" },
	{ "LM_GetProcessPathEx", py_LM_GetProcessPathEx, METH_VARARGS, "" },
	{ "LM_GetProcessName", py_LM_GetProcessName, METH_NOARGS, "" },
	{ "LM_GetProcessNameEx", py_LM_GetProcessNameEx, METH_VARARGS, "" },
	{ "LM_GetSystemBits", py_LM_GetSystemBits, METH_NOARGS, "" },
	{ "LM_GetProcessBits", py_LM_GetProcessBits, METH_NOARGS, "" },
	{ "LM_GetProcessBitsEx", py_LM_GetProcessBitsEx, METH_VARARGS, "" },
	/****************************************/
	{ "LM_EnumThreads", py_LM_EnumThreads, METH_VARARGS, "" },
	{ "LM_EnumThreadsEx", py_LM_EnumThreadsEx, METH_VARARGS, "" },
	{ "LM_GetThreadId", py_LM_GetThreadId, METH_NOARGS, "" },
	{ "LM_GetThreadIdEx", py_LM_GetThreadIdEx, METH_VARARGS, "" },
	/****************************************/
	{ "LM_EnumModules", py_LM_EnumModules, METH_VARARGS, "" },
	{ "LM_EnumModulesEx", py_LM_EnumModulesEx, METH_VARARGS, "" },
	{ "LM_GetModule", py_LM_GetModule, METH_VARARGS, "" },
	{ "LM_GetModuleEx", py_LM_GetModuleEx, METH_VARARGS, "" },
	{ "LM_GetModulePath", py_LM_GetModulePath, METH_VARARGS, "" },
	{ "LM_GetModulePathEx", py_LM_GetModulePathEx, METH_VARARGS, "" },
	{ "LM_GetModuleName", py_LM_GetModuleName, METH_VARARGS, "" },
	{ "LM_GetModuleNameEx", py_LM_GetModuleNameEx, METH_VARARGS, "" },
	{ "LM_LoadModule", py_LM_LoadModule, METH_VARARGS, "" },
	{ "LM_LoadModuleEx", py_LM_LoadModuleEx, METH_VARARGS, "" },
	{ "LM_UnloadModule", py_LM_UnloadModule, METH_VARARGS, "" },
	{ "LM_UnloadModuleEx", py_LM_UnloadModuleEx, METH_VARARGS, "" },
	/****************************************/
	{ "LM_EnumSymbols", py_LM_EnumSymbols, METH_VARARGS, "" },
	{ "LM_EnumSymbolsEx", py_LM_EnumSymbolsEx, METH_VARARGS, "" },
	{ "LM_GetSymbol", py_LM_GetSymbol, METH_VARARGS, "" },
	{ "LM_GetSymbolEx", py_LM_GetSymbolEx, METH_VARARGS, "" },
	/****************************************/
	{ NULL, NULL, 0, NULL } /* Sentinel */
};

static PyModuleDef libmem_mod = {
	PyModuleDef_HEAD_INIT,
	"libmem",
	NULL,
	-1,
	libmem_methods
};

PyMODINIT_FUNC
PyInit_libmem(void)
{
	PyObject *pymod;

	if (PyType_Ready(&py_lm_process_t) < 0)
		goto _ERR_PYMOD;
	
	if (PyType_Ready(&py_lm_module_t) < 0)
		goto _ERR_PYMOD;

	pymod = PyModule_Create(&libmem_mod);
	if (!pymod)
		goto _ERR_PYMOD;
	
	/* Global Variables */
	DECL_GLOBAL(pymod, "LM_OS_WIN", LM_OS_WIN);
	DECL_GLOBAL(pymod, "LM_OS_LINUX", LM_OS_LINUX);
	DECL_GLOBAL(pymod, "LM_OS_BSD", LM_OS_BSD);
	DECL_GLOBAL(pymod, "LM_OS", LM_OS);

	DECL_GLOBAL(pymod, "LM_ARCH_X86", LM_ARCH_X86);
	DECL_GLOBAL(pymod, "LM_ARCH_ARM", LM_ARCH_ARM);
	DECL_GLOBAL(pymod, "LM_ARCH", LM_ARCH);

	DECL_GLOBAL(pymod, "LM_BITS", LM_BITS);

	DECL_GLOBAL(pymod, "LM_COMPILER_MSVC", LM_COMPILER_MSVC);
	DECL_GLOBAL(pymod, "LM_COMPILER_CC", LM_COMPILER_CC);

	DECL_GLOBAL(pymod, "LM_CHARSET_UC", LM_CHARSET_UC);
	DECL_GLOBAL(pymod, "LM_CHARSET_MB", LM_CHARSET_MB);
	DECL_GLOBAL(pymod, "LM_CHARSET", LM_CHARSET);

	DECL_GLOBAL(pymod, "LM_LANG_C", LM_LANG_C);
	DECL_GLOBAL(pymod, "LM_LANG_CPP", LM_LANG_CPP);
	DECL_GLOBAL(pymod, "LM_LANG", LM_LANG);

	DECL_GLOBAL(pymod, "LM_PROT_R", LM_PROT_R);
	DECL_GLOBAL(pymod, "LM_PROT_W", LM_PROT_W);
	DECL_GLOBAL(pymod, "LM_PROT_X", LM_PROT_X);
	DECL_GLOBAL(pymod, "LM_PROT_RW", LM_PROT_RW);
	DECL_GLOBAL(pymod, "LM_PROT_XR", LM_PROT_XR);
	DECL_GLOBAL(pymod, "LM_PROT_XRW", LM_PROT_XRW);

	DECL_GLOBAL(pymod, "LM_NULL", LM_NULL);
	DECL_GLOBAL(pymod, "LM_NULLPTR", LM_NULLPTR);
	DECL_GLOBAL(pymod, "LM_FALSE", LM_FALSE);
	DECL_GLOBAL(pymod, "LM_TRUE", LM_TRUE);
	DECL_GLOBAL(pymod, "LM_BAD", LM_BAD);
	DECL_GLOBAL(pymod, "LM_OK", LM_OK);
	DECL_GLOBAL(pymod, "LM_MAX", LM_MAX);
	DECL_GLOBAL(pymod, "LM_PATH_MAX", LM_PATH_MAX);
	DECL_GLOBAL(pymod, "LM_MOD_BY_STR", LM_MOD_BY_STR);
	DECL_GLOBAL(pymod, "LM_MOD_BY_ADDR", LM_MOD_BY_ADDR);
	
	/* Types */
	Py_INCREF(&py_lm_process_t);
	if (PyModule_AddObject(pymod, "lm_process_t",
			       (PyObject *)&py_lm_process_t) < 0)
		goto _ERR_PROCESS;
	
	Py_INCREF(&py_lm_module_t);
	if (PyModule_AddObject(pymod, "lm_module_t",
			       (PyObject *)&py_lm_module_t) < 0)
		goto _ERR_MODULE;

	goto _RET; /* No Type Errors */
_ERR_MODULE:
	Py_DECREF(&py_lm_module_t);
	Py_DECREF(pymod);
_ERR_PROCESS:
	Py_DECREF(&py_lm_process_t);
	Py_DECREF(pymod);
_ERR_PYMOD:
	pymod = (PyObject *)NULL;
_RET:
	return pymod;
}