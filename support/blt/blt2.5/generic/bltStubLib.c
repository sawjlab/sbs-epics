#ifndef STATIC_BUILD
#ifndef USE_BLT_STUBS
#define USE_BLT_STUBS
#endif
#endif
#undef USE_BLT_STUB_PROCS

#include "tcl.h"
#include "bltDecls.h"

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT

#ifndef MODULE_SCOPE
#   ifdef __cplusplus
#       define MODULE_SCOPE extern "C"
#   else
#       define MODULE_SCOPE extern
#   endif
#endif


MODULE_SCOPE BltStubs *bltStubsPtr;
BltStubs *bltStubsPtr;

#ifdef Blt_InitStubs
#undef Blt_InitStubs
#endif

MODULE_SCOPE CONST char *
Blt_InitStubs(interp, version, exact)
    Tcl_Interp *interp;
    char *version;
    int exact;
{
    CONST char *actualVersion;

    actualVersion = Tcl_PkgRequireEx(interp, "BLT", version, exact,
        (ClientData *) &bltStubsPtr);
    if (!actualVersion) {
        return NULL;
    }

    if (!bltStubsPtr) {
        Tcl_SetResult(interp,
            "This implementation of Tk does not support stubs",
            TCL_STATIC);
            return NULL;
    }
    
    return actualVersion;
}


