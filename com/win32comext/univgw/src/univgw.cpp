/*
** Universal gateway module
*/

#include "stdafx.h"

static HMODULE g_hModule = NULL;
static PyObject *univgwError;

// ### copied from PyGatewayBase.cpp
static const GUID IID_IInternalUnwrapPythonObject = {
	0x25d29cd0, 0x9b98, 0x11d0, { 0xae, 0x79, 0x4c, 0xf1, 0xcf, 0x0, 0x0, 0x0 }
};

typedef HRESULT (STDMETHODCALLTYPE * pfnGWMethod)(struct gw_object * _this);

typedef struct gw_vtbl
{
	DWORD		magic;
#define GW_VTBL_MAGIC	0x20534A47

	PyObject *	dispatcher;	// dispatcher from the COM2Py thunk definition
	IID			iid;		// the IID of this interface
	UINT		cMethod;	// count of methods

	// the vtable (the actual methods)
#pragma warning ( disable : 4200 )
	pfnGWMethod	methods[];
#pragma warning ( default : 4200 )

} gw_vtbl;

typedef struct gw_object
{
	pfnGWMethod *					vtbl;
	IInternalUnwrapPythonObject *	punk;		// the identity interface
	LONG							cRef;

} gw_object;

#define GET_DEFN(gwo)	((gw_vtbl *)((char *)(gwo)->vtbl - offsetof(gw_vtbl, methods)))

static void set_error(REFIID riid, LPCOLESTR desc)
{
	ICreateErrorInfo *pICEI;
	HRESULT hr = CreateErrorInfo(&pICEI);
	if ( SUCCEEDED(hr) )
	{
		CComBSTR b(desc);

		// ### temporary
		OutputDebugStringW(desc);
		OutputDebugStringW(L"\n");

		pICEI->SetGUID(riid);
		pICEI->SetDescription(b);

		IErrorInfo *pIEI;
		Py_BEGIN_ALLOW_THREADS
		hr = pICEI->QueryInterface(IID_IErrorInfo, (LPVOID*) &pIEI);
		Py_END_ALLOW_THREADS
		if ( SUCCEEDED(hr) )
		{
			SetErrorInfo(0, pIEI);
			pIEI->Release();
		}
		pICEI->Release();			
	}
}

static HRESULT univgw_dispatch(DWORD index, gw_object * _this, va_list argPtr)
{
	PY_GATEWAY_METHOD;
	gw_vtbl * vtbl = GET_DEFN(_this);

	// get a pointer for the va_list and convert it to a (long) integer
	PyObject *obArgPtr = PyLong_FromVoidPtr((void *)VA_LIST_PTR(argPtr));

	// prep the rest of the arguments
	PyObject *obArgs = PyTuple_New(3);
	PyObject *obIndex = PyInt_FromLong(index);

	if ( obArgPtr == NULL || obArgs == NULL || obIndex == NULL )
	{
		// ### what-to-do is pretty close here. maybe refine it some.
		set_error(vtbl->iid, L"could not create argument tuple");
		Py_XDECREF(obArgPtr);
		Py_XDECREF(obArgs);
		Py_XDECREF(obIndex);
		PyErr_Clear();
		return E_OUTOFMEMORY;	// ### select a different value?
	}

	// get the underlying Python object
	PyObject * instance;
	_this->punk->Unwrap(&instance);

	// obArgs takes our references
	PyTuple_SET_ITEM(obArgs, 0, instance);
	PyTuple_SET_ITEM(obArgs, 1, obIndex);
	PyTuple_SET_ITEM(obArgs, 2, obArgPtr);

	// call the provided method
	PyObject *result = PyEval_CallObjectWithKeywords(vtbl->dispatcher, obArgs, NULL);

	// done with the arguments and the contained objects
	Py_DECREF(obArgs);

	if ( result == NULL )
		return PyCom_SetCOMErrorFromPyException(vtbl->iid);

	HRESULT hr;
	if ( result == Py_None )
	{
		hr = S_OK;
	}
	else
	{
		if ( !PyInt_Check(result) )
		{
			Py_DECREF(result);

			// ### pretty close here. maybe refine it some.
			set_error(vtbl->iid, L"expected integer return value");
			return E_UNEXPECTED;	// ### select a different value?
		}

		hr = PyInt_AS_LONG(result);
	}

	Py_DECREF(result);

	// ### what to do for non-HRESULT return values?
	return hr;
}

//#define COMPILE_MOCKUP
#ifdef COMPILE_MOCKUP

STDMETHODIMP mockup(gw_object * _this)
{
	va_list args;
	va_start(args, _this);
	return univgw_dispatch(0x11223344, _this, args);
}

#endif // COMPILE_MOCKUP


static pfnGWMethod make_method(DWORD index, UINT argsize)
{
	unsigned char * code;

#ifdef _M_IX86

	static const unsigned char func_template[] = {
// ; 45   : STDMETHODIMP mockup(gw_object * _this)
// ; 46   : {
//  00000	55				push	 ebp
//  00001	8b ec			mov		 ebp, esp
//  00003	51				push	 ecx
		0x55, 0x8b, 0xec, 0x51,

// ; 47   : 	va_list args;
// ; 48   : 	va_start(args, _this);
//  00004	8d 45 0c		lea		 eax, DWORD PTR __this$[ebp+4]
//  00007	89 45 fc		mov		 DWORD PTR _args$[ebp], eax
		0x8d, 0x45, 0x0c, 0x89, 0x45, 0xfc,

// ; 49   : 	return univgw_dispatch(0x11223344, _this, args);
//  0000a	8b 4d fc		mov		 ecx, DWORD PTR _args$[ebp]
//  0000d	51				push	 ecx
//  0000e	8b 55 08		mov		 edx, DWORD PTR __this$[ebp]
//  00011	52				push	 edx
//  00012	68 44 33 22 11	push	 287454020		; 11223344H
//  00017	e8 00 00 00 00	call	 ?univgw_dispatch@@YAJKPAUgw_object@@PAD@Z ; univgw_dispatch
//  0001c	83 c4 0c	 	add		 esp, 12			; 0000000cH
		0x8b, 0x4d, 0xfc, 0x51, 0x8b, 0x55, 0x08, 0x52, 0x68,
		// offset = 19 (0x13)
		0x44, 0x33, 0x22, 0x11,		// replace these with <index>
		0xe8,
		// offset = 24 (0x18)
		0x00, 0x00, 0x00, 0x00,		// replace these with <univgw_dispatch>
		0x83, 0xc4, 0x0c,

//; 50   : }
//  0001f	8b e5			mov		 esp, ebp
//  00021	5d				pop		 ebp
//  00022	c2 04 00		ret		 4
		0x8b, 0xe5, 0x5d, 0xc2,
		// offset = 35 (0x23)
		0x04, 0x00,					// replace this with argsize
	};

	// make a copy of code and plug in the appropriate values.
	// NOTE: the call address is relative
	code = (unsigned char *)malloc(sizeof(func_template));
	memcpy(code, func_template, sizeof(func_template));
	*(long *)&code[19] = index;
	*(long *)&code[24] = (long)&univgw_dispatch - (long)&code[28];
	*(short *)&code[35] = argsize;

#else	// _M_IX86
#  error make_method not defined for this platform
#endif

	return (pfnGWMethod)code;
}

static STDMETHODIMP univgw_QueryInterface(gw_object * _this, REFIID riid, void **ppv)
{
	// NOTE: ->iid can never be IID_IUnknown, so we don't have to worry
	// about COM equivalence rules here.

	if ( IsEqualIID(riid, GET_DEFN(_this)->iid) )
	{
		/*
		** Note that we don't need to cast since _this already points to a
		** properly-formed vtable-based object. Normally, a C++ object would
		** need the cast to select the proper vtable; we've got it already.
		*/
		*ppv = _this;
		++_this->cRef;
		return S_OK;
	}

	// delegate to the original interface
	return _this->punk->QueryInterface(riid, ppv);
}
static STDMETHODIMP_(ULONG) univgw_AddRef(gw_object * _this)
{
	return InterlockedIncrement(&_this->cRef);
}
static STDMETHODIMP_(ULONG) univgw_Release(gw_object * _this)
{
	LONG cRef = InterlockedDecrement(&_this->cRef);
	if ( cRef == 0 )
	{
		_this->punk->Release();
		free(_this);
		return 0;
	}
	return _this->cRef;
}

/* free the gw_vtbl object. also works on a partially constructed gw_vtbl. */
static void __cdecl free_vtbl(void * cobject)
{
	gw_vtbl * vtbl = (gw_vtbl *)cobject;

	Py_XDECREF(vtbl->dispatcher);

	// free the methods. 0..2 are the constant IUnknown methods
	for ( int i = vtbl->cMethod; --i > 2; )
		if ( vtbl->methods[i] != NULL )
			free(vtbl->methods[i]);
	free(vtbl);
}

static PyObject * univgw_CreateVTable(PyObject *self, PyObject *args)
{
	PyObject *obDef;
	if ( !PyArg_ParseTuple(args, "O:CreateVTable", &obDef) )
		return NULL;

	PyObject *obIID = PyObject_CallMethod(obDef, "iid", NULL);
	if ( obIID == NULL )
		return NULL;
	IID iid;
	if ( !PyWinObject_AsIID(obIID, &iid) )
	{
		Py_DECREF(obIID);
		return NULL;
	}
	Py_DECREF(obIID);

	/*
	** It doesn't make sense to create one of these for IUnknown, since
	** we need to use the original PyGatewayBase object as the base
	** object for all gateways (for COM equivalence testing)
	**
	** NOTE: it isn't worth it to try to optimize the other interfaces
	** on PyGatewayBase
	*/
	if ( iid == IID_IUnknown )
	{
		PyErr_SetString(PyExc_ValueError, "tear-off not allowed for IUnknown");
		return NULL;
	}

	PyObject * methods = PyObject_CallMethod(obDef, "vtbl_argsizes", NULL);
	if ( methods == NULL )
		return NULL;

	int count = PyObject_Length(methods);
	if ( count == -1 )
	{
		Py_DECREF(methods);
		return NULL;
	}
	count += 3;	// the methods list should not specify IUnknown methods

	// compute the size of the structure plus the method pointers
	size_t size = sizeof(gw_vtbl) + count * sizeof(pfnGWMethod);
	gw_vtbl * vtbl = (gw_vtbl *)malloc(size);
	if ( vtbl == NULL )
	{
		Py_DECREF(methods);
		PyErr_NoMemory();
		return NULL;
	}
	memset(vtbl, 0, size);

	vtbl->magic = GW_VTBL_MAGIC;
	Py_INCREF(obDef);
	vtbl->iid = iid;
	vtbl->cMethod = count;

	vtbl->dispatcher = PyObject_GetAttrString(obDef, "dispatch");
	if ( vtbl->dispatcher == NULL )
		goto error;

	vtbl->methods[0] = (pfnGWMethod)univgw_QueryInterface;
	vtbl->methods[1] = (pfnGWMethod)univgw_AddRef;
	vtbl->methods[2] = (pfnGWMethod)univgw_Release;

	// add the methods. NOTE: 0..2 are the constant IUnknown methods
	int i;
	for ( i = vtbl->cMethod - 3; i--; )
	{
		PyObject * obArgSize = PySequence_GetItem(methods, i);
		if ( obArgSize == NULL )
			goto error;

		int argSize = PyInt_AsLong(obArgSize);
		Py_DECREF(obArgSize);
		if ( argSize == -1 && PyErr_Occurred() )
			goto error;

		// dynamically construct a function with the provided argument
		// size; reserve additional space for the _this argument.
		pfnGWMethod meth = make_method(i, argSize + sizeof(gw_object *));
		if ( meth == NULL )
		{
			(void)PyErr_NoMemory();
			goto error;
		}

		vtbl->methods[i + 3] = meth;
	}
	Py_DECREF(methods);

	PyObject *result;
	result = PyCObject_FromVoidPtr(vtbl, free_vtbl);
	if ( result == NULL )
	{
		free_vtbl(vtbl);
		return NULL;
	}

	return result;

  error:
	Py_DECREF(methods);
	free_vtbl(vtbl);
	return NULL;
}

static PyObject * univgw_CreateTearOff(PyObject *self, PyObject *args)
{
	PyObject *obInstance;
	PyObject *obVTable;
	PyObject *obIID = NULL;
	if ( !PyArg_ParseTuple(args, "OO|O:CreateTearOff", &obInstance, &obVTable, &obIID) )
		return NULL;

	IID iidInterface = IID_IUnknown;	// what PyI* to wrap with
	if ( obIID && obIID != Py_None )
		if ( !PyWinObject_AsIID(obIID, &iidInterface) )
			return NULL;

	// obVTable must be a CObject containing our vtbl ptr
	if ( !PyCObject_Check(obVTable) )
	{
		PyErr_SetString(PyExc_ValueError, "argument is not a CObject/vtable");
		return NULL;
	}
	gw_vtbl * vtbl = (gw_vtbl *)PyCObject_AsVoidPtr(obVTable);
	if ( vtbl->magic != GW_VTBL_MAGIC )
	{
		PyErr_SetString(PyExc_ValueError, "argument does not contain a vtable");
		return NULL;
	}

	// obInstance must be a PyIUnknown (or derivative) pointing to a
	// PyGatewayBase object.
	IInternalUnwrapPythonObject * pUnwrap;
	if ( !PyCom_InterfaceFromPyInstanceOrObject(obInstance, IID_IInternalUnwrapPythonObject, (void **)&pUnwrap, /* bNoneOK = */ FALSE) )
		return NULL;

	// construct a C++ object (a block of mem with a vtbl)
	gw_object * punk = (gw_object *)malloc(sizeof(gw_object));
	if ( punk == NULL )
	{
		PyErr_NoMemory();
		pUnwrap->Release();
		return NULL;
	}
	punk->vtbl = vtbl->methods;
	punk->punk = pUnwrap;
	punk->cRef = 1;		// we start with one reference (the object we return)

	return PyCom_PyObjectFromIUnknown((IUnknown *)punk, iidInterface, FALSE);
}

static PyObject * univgw_ReadMemory(PyObject *self, PyObject *args)
{
	PyObject *obPtr;
	int size;
	if ( !PyArg_ParseTuple(args, "Oi:ReadMemory", &obPtr, &size) )
		return NULL;

	void *p = PyLong_AsVoidPtr(obPtr);
	if ( p == NULL && PyErr_Occurred() )
	{
		// reset the error to something meaningful
		PyErr_SetString(PyExc_ValueError, "argument is not an integer");
		return NULL;
	}

	return PyString_FromStringAndSize((char *)p, size);
}

static PyObject * univgw_WriteMemory(PyObject *self, PyObject *args)
{
	PyObject *obPtr;
	void *pSrc;
	int size;
	if ( !PyArg_ParseTuple(args, "Os#:WriteMemory", &obPtr, &pSrc, &size) )
		return NULL;

	void *pDst = PyLong_AsVoidPtr(obPtr);
	if ( pDst == NULL && PyErr_Occurred() )
	{
		// reset the error to something meaningful
		PyErr_SetString(PyExc_ValueError, "argument is not an integer");
		return NULL;
	}

	memcpy(pDst, pSrc, size);

	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMethodDef univgw_functions[] =
{
	{ "CreateVTable", univgw_CreateVTable, 1 },
	{ "CreateTearOff", univgw_CreateTearOff, 1 },
	{ "ReadMemory", univgw_ReadMemory, 1 },
	{ "WriteMemory", univgw_WriteMemory, 1 },

	{ "L64", dataconv_L64, 1 },
	{ "UL64", dataconv_UL64, 1 },
	{ "strL64", dataconv_strL64, 1 },
	{ "strUL64", dataconv_strUL64, 1 },
	{ "interface", dataconv_interface, 1},

	{ NULL } /* sentinel */
};

static int AddObject(PyObject *dict, const char *key, PyObject *ob)
{
	if (!ob) return 1;
	int rc = PyDict_SetItemString(dict, (char*)key, ob);
	Py_DECREF(ob);
	return rc;
}

extern "C" void __declspec(dllexport)
initunivgw(void)
{
//	HRESULT hr;

	PyObject *module = Py_InitModule("univgw", univgw_functions);

	PyObject *dict = PyModule_GetDict(module);

	univgwError = PyString_FromString("univgw.error");
	PyDict_SetItemString(dict, "error", univgwError);

#define ADD_CONSTANT(tok) AddObject(dict, #tok, PyInt_FromLong(tok))
#define ADD_IID(tok) AddObject(dict, #tok, PyWinObject_FromIID(tok))
}


/*
** We want a DllMain simply to capture our modules hInstance value
*/
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if ( dwReason == DLL_PROCESS_ATTACH )
	{
		g_hModule = hInstance;

		/*
		** we don't need to be notified about threads
		*/
		DisableThreadLibraryCalls(hInstance);
	}
	else if ( dwReason == DLL_PROCESS_DETACH )
	{
		g_hModule = NULL;
	}

	return TRUE;    // ok
}


// Include the ATL code here
#include <atlconv.cpp>
