/*
 * bltWinop.c --
 *
 *	This module implements simple window commands for the BLT toolkit.
 *
 * Copyright 1991-1998 Lucent Technologies, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and warranty
 * disclaimer appear in supporting documentation, and that the names
 * of Lucent Technologies any of their entities not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 *
 * Lucent Technologies disclaims all warranties with regard to this
 * software, including all implied warranties of merchantability and
 * fitness.  In no event shall Lucent Technologies be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether in
 * an action of contract, negligence or other tortuous action, arising
 * out of or in connection with the use or performance of this
 * software.
 */

#include "bltInt.h"

#ifndef NO_WINOP

#include "bltImage.h"
#include <X11/Xutil.h>
#ifndef WIN32
#include <X11/Xproto.h>
#endif

static Tcl_CmdProc WinopCmd;

static int
GetRealizedWindow(interp, string, tkwinPtr)
    Tcl_Interp *interp;
    char *string;
    Tk_Window *tkwinPtr;
{
    Tk_Window tkwin;

    tkwin = Tk_NameToWindow(interp, string, Tk_MainWindow(interp));
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    if (Tk_WindowId(tkwin) == None) {
	Tk_MakeWindowExist(tkwin);
    }
    *tkwinPtr = tkwin;
    return TCL_OK;
}

static Window
StringToWindow(interp, string)
    Tcl_Interp *interp;
    char *string;
{
    int xid;

    if (string[0] == '.') {
	Tk_Window tkwin;

	if (GetRealizedWindow(interp, string, &tkwin) != TCL_OK) {
	    return None;
	}
	if (Tk_IsTopLevel(tkwin)) {
	    return Blt_GetRealWindowId(tkwin);
	} else {
	    return Tk_WindowId(tkwin);
	}
    } else if (Tcl_GetInt(interp, string, &xid) == TCL_OK) {
#ifdef WIN32
	static TkWinWindow tkWinWindow;

	tkWinWindow.handle = (HWND)xid;
	tkWinWindow.winPtr = NULL;
	tkWinWindow.type = TWD_WINDOW;
	return (Window)&tkWinWindow;
#else
	return (Window)xid;
#endif
    }
    return None;
}

#ifdef WIN32

static int
GetWindowSize(
    Tcl_Interp *interp,
    Window window,
    int *widthPtr, 
    int *heightPtr)
{
    int result;
    RECT region;
    TkWinWindow *winPtr = (TkWinWindow *)window;

    result = GetWindowRect(winPtr->handle, &region);
    if (result) {
	*widthPtr = region.right - region.left;
	*heightPtr = region.bottom - region.top;
	return TCL_OK;
    }
    return TCL_ERROR;
}

#else

/*
 *----------------------------------------------------------------------
 *
 * XGeometryErrorProc --
 *
 *	Flags errors generated from XGetGeometry calls to the X server.
 *
 * Results:
 *	Always returns 0.
 *
 * Side Effects:
 *	Sets a flag, indicating an error occurred.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
static int
XGeometryErrorProc(clientData, errEventPtr)
    ClientData clientData;
    XErrorEvent *errEventPtr;
{
    int *errorPtr = clientData;

    *errorPtr = TCL_ERROR;
    return 0;
}

static int
GetWindowSize(interp, window, widthPtr, heightPtr)
    Tcl_Interp *interp;
    Window window;
    int *widthPtr, *heightPtr;
{
    int result;
    int any = -1;
    int x, y, borderWidth, depth;
    Window root;
    Tk_ErrorHandler handler;
    Tk_Window tkwin;
    
    tkwin = Tk_MainWindow(interp);
    handler = Tk_CreateErrorHandler(Tk_Display(tkwin), any, X_GetGeometry, 
	    any, XGeometryErrorProc, &result);
    result = XGetGeometry(Tk_Display(tkwin), window, &root, &x, &y, 
	  (unsigned int *)widthPtr, (unsigned int *)heightPtr,
	  (unsigned int *)&borderWidth, (unsigned int *)&depth);
    Tk_DeleteErrorHandler(handler);
    XSync(Tk_Display(tkwin), False);
    if (result) {
	return TCL_OK;
    }
    return TCL_ERROR;
}
#endif /*WIN32*/


#ifndef WIN32
/*ARGSUSED*/
static int
ColormapOp(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    register int i;
    Tk_Window tkwin;
#define MAXCOLORS 256
    register XColor *colorPtr;
    XColor colorArr[MAXCOLORS];
    unsigned long int pixelValues[MAXCOLORS];
    int inUse[MAXCOLORS];
    char string[20];
    unsigned long int *indexPtr;
    int nFree;

    if (GetRealizedWindow(interp, argv[2], &tkwin) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Initially, we assume all color cells are allocated. */
    memset((char *)inUse, 0, sizeof(int) * MAXCOLORS);

    /*
     * Start allocating color cells.  This will tell us which color cells
     * haven't already been allocated in the colormap.  We'll release the
     * cells as soon as we find out how many there are.
     */
    nFree = 0;
    for (indexPtr = pixelValues, i = 0; i < MAXCOLORS; i++, indexPtr++) {
	if (!XAllocColorCells(Tk_Display(tkwin), Tk_Colormap(tkwin),
		False, NULL, 0, indexPtr, 1)) {
	    break;
	}
	inUse[*indexPtr] = TRUE;/* Indicate the cell is unallocated */
	nFree++;
    }
    XFreeColors(Tk_Display(tkwin), Tk_Colormap(tkwin), pixelValues, nFree, 0);
    for (colorPtr = colorArr, i = 0; i < MAXCOLORS; i++, colorPtr++) {
	colorPtr->pixel = i;
    }
    XQueryColors(Tk_Display(tkwin), Tk_Colormap(tkwin), colorArr, MAXCOLORS);
    for (colorPtr = colorArr, i = 0; i < MAXCOLORS; i++, colorPtr++) {
	if (!inUse[colorPtr->pixel]) {
	    sprintf(string, "#%02x%02x%02x", (colorPtr->red >> 8),
		(colorPtr->green >> 8), (colorPtr->blue >> 8));
	    Tcl_AppendElement(interp, string);
	    sprintf(string, "%ld", colorPtr->pixel);
	    Tcl_AppendElement(interp, string);
	}
    }
    return TCL_OK;
}

#endif

/*ARGSUSED*/
static int
LowerOp(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    register int i;
    Window window;
    Display *display;

    display = Tk_Display(Tk_MainWindow(interp));
    for (i = 2; i < argc; i++) {
	window = StringToWindow(interp, argv[i]);
	if (window == None) {
	    return TCL_ERROR;
	}
	XLowerWindow(display, window);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
RaiseOp(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    register int i;
    Window window;
    Display *display;

    display = Tk_Display(Tk_MainWindow(interp));
    for (i = 2; i < argc; i++) {
	window = StringToWindow(interp, argv[i]);
	if (window == None) {
	    return TCL_ERROR;
	}
	XRaiseWindow(display, window);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
MapOp(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    register int i;
    Window window;
    Display *display;

    display = Tk_Display(Tk_MainWindow(interp));
    for (i = 2; i < argc; i++) {
	if (argv[i][0] == '.') {
	    Tk_Window tkwin;
	    Tk_FakeWin *fakePtr;

	    if (GetRealizedWindow(interp, argv[i], &tkwin) != TCL_OK) {
		return TCL_ERROR;
	    }
	    fakePtr = (Tk_FakeWin *) tkwin;
	    fakePtr->flags |= TK_MAPPED;
	    window = Tk_WindowId(tkwin);
	} else {
	    int xid;

	    if (Tcl_GetInt(interp, argv[i], &xid) != TCL_OK) {
		return TCL_ERROR;
	    }
	    window = (Window)xid;
	}
	XMapWindow(display, window);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
MoveOp(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;			/* Not Used. */
    char **argv;
{
    int x, y;
    Tk_Window tkwin;
    Window window;
    Display *display;

    tkwin = Tk_MainWindow(interp);
    display = Tk_Display(tkwin);
    window = StringToWindow(interp, argv[2]);
    if (window == None) {
	return TCL_ERROR;
    }
    if (Tk_GetPixels(interp, tkwin, argv[3], &x) != TCL_OK) {
	Tcl_AppendResult(interp, ": bad window x-coordinate", (char *)NULL);
	return TCL_ERROR;
    }
    if (Tk_GetPixels(interp, tkwin, argv[4], &y) != TCL_OK) {
	Tcl_AppendResult(interp, ": bad window y-coordinate", (char *)NULL);
	return TCL_ERROR;
    }
    XMoveWindow(display, window, x, y);
    return TCL_OK;
}

/*ARGSUSED*/
static int
UnmapOp(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    register int i;
    Window window;
    Display *display;

    display = Tk_Display(Tk_MainWindow(interp));
    for (i = 2; i < argc; i++) {
	if (argv[i][0] == '.') {
	    Tk_Window tkwin;
	    Tk_FakeWin *fakePtr;

	    if (GetRealizedWindow(interp, argv[i], &tkwin) != TCL_OK) {
		return TCL_ERROR;
	    }
	    fakePtr = (Tk_FakeWin *) tkwin;
	    fakePtr->flags &= ~TK_MAPPED;
	    window = Tk_WindowId(tkwin);
	} else {
	    int xid;

	    if (Tcl_GetInt(interp, argv[i], &xid) != TCL_OK) {
		return TCL_ERROR;
	    }
	    window = (Window)xid;
	}
	XMapWindow(display, window);
    }
    return TCL_OK;
}

/* ARGSUSED */
static int
ChangesOp(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;		/* Not used. */
{
    Tk_Window tkwin;

    if (GetRealizedWindow(interp, argv[2], &tkwin) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tk_IsTopLevel(tkwin)) {
	XSetWindowAttributes attrs;
	Window window;
	unsigned int mask;

	window = Blt_GetRealWindowId(tkwin);
	attrs.backing_store = WhenMapped;
	attrs.save_under = True;
	mask = CWBackingStore | CWSaveUnder;
	XChangeWindowAttributes(Tk_Display(tkwin), window, mask, &attrs);
    }
    return TCL_OK;
}

/* ARGSUSED */
static int
ParentOp(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;			/* Not used. */
char **argv;		/* Not used. */
{
    Window window;
    int value;
    char buf[50];

    if (Tcl_GetInt(interp, argv[2], &value) != TCL_OK) {
        return TCL_ERROR;
    }
    window = Blt_GetParent(Tk_Display(Tk_MainWindow(interp)), (Window)value);
    if (window) {
        sprintf(buf, "0x%x", (int)window);    
        Tcl_AppendResult(interp, buf, 0);
    }
    return TCL_OK;
}

/* ARGSUSED */
static int
QueryOp(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;		/* Not used. */
{
    int rootX, rootY, childX, childY;
    Window root, child;
    unsigned int mask;
    Tk_Window tkwin = (Tk_Window)clientData;

    /* GetCursorPos */
    if (XQueryPointer(Tk_Display(tkwin), Tk_WindowId(tkwin), &root,
	    &child, &rootX, &rootY, &childX, &childY, &mask)) {
	char string[200];

	sprintf(string, "@%d,%d", rootX, rootY);
	Tcl_SetResult(interp, string, TCL_VOLATILE);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
WarpToOp(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_Window tkwin, mainWindow;

    mainWindow = (Tk_Window)clientData;
    if (argc > 2) {
	if (argv[2][0] == '@') {
	    int x, y;
	    Window root;

	    if (Blt_GetXY(interp, mainWindow, argv[2], &x, &y) != TCL_OK) {
		return TCL_ERROR;
	    }
	    root = RootWindow(Tk_Display(mainWindow), 
		Tk_ScreenNumber(mainWindow));
	    XWarpPointer(Tk_Display(mainWindow), None, root, 0, 0, 0, 0, x, y);
	} else {
	    if (GetRealizedWindow(interp, argv[2], &tkwin) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (!Tk_IsMapped(tkwin)) {
		Tcl_AppendResult(interp, "can't warp to unmapped window \"",
		    Tk_PathName(tkwin), "\"", (char *)NULL);
		return TCL_ERROR;
	    }
	    XWarpPointer(Tk_Display(tkwin), None, Tk_WindowId(tkwin),
		0, 0, 0, 0, Tk_Width(tkwin) / 2, Tk_Height(tkwin) / 2);
	}
    }
    return QueryOp(clientData, interp, 0, (char **)NULL);
}

#ifdef notdef
static int
ReparentOp(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char **argv;
{
    Tk_Window tkwin;

    if (GetRealizedWindow(interp, argv[2], &tkwin) != TCL_OK) {
	return TCL_ERROR;
    }
    if (argc == 4) {
	Tk_Window newParent;

	if (GetRealizedWindow(interp, argv[3], &newParent) != TCL_OK) {
	    return TCL_ERROR;
	}
	Blt_RelinkWindow2(tkwin, Blt_GetRealWindowId(tkwin), newParent, 0, 0);
    } else {
	Blt_UnlinkWindow(tkwin);
    }
    return TCL_OK;
}
#endif


/*
 * This is a temporary home for these image routines.  They will be
 * moved when a new image type is created for them.
 */
/*ARGSUSED*/
static int
ConvolveOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle srcPhoto, destPhoto;
    Blt_ColorImage srcImage, destImage;
    Filter2D filter;
    int nValues;
    char **valueArr;
    double *kernel;
    double value, sum;
    register int i;
    int dim;
    int result = TCL_ERROR;

    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    destPhoto = Blt_FindPhoto(interp, argv[3]);
    if (destPhoto == NULL) {
	Tcl_AppendResult(interp, "destination image \"", argv[3], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    if (Tcl_SplitList(interp, argv[4], &nValues, &valueArr) != TCL_OK) {
	return TCL_ERROR;
    }
    kernel = NULL;
    if (nValues == 0) {
	Tcl_AppendResult(interp, "empty kernel", (char *)NULL);
	goto error;
    }
    dim = (int)sqrt((double)nValues);
    if ((dim * dim) != nValues) {
	Tcl_AppendResult(interp, "kernel must be square", (char *)NULL);
	goto error;
    }
    kernel = Blt_Malloc(sizeof(double) * nValues);
    sum = 0.0;
    for (i = 0; i < nValues; i++) {
	if (Tcl_GetDouble(interp, valueArr[i], &value) != TCL_OK) {
	    goto error;
	}
	kernel[i] = value;
	sum += value;
    }
    filter.kernel = kernel;
    filter.support = dim * 0.5;
    filter.sum = (sum == 0.0) ? 1.0 : sum;
    filter.scale = 1.0 / nValues;

    srcImage = Blt_PhotoToColorImage(srcPhoto);
    destImage = Blt_ConvolveColorImage(srcImage, &filter);
    Blt_FreeColorImage(srcImage);
    Blt_ColorImageToPhoto(destImage, destPhoto);
    Blt_FreeColorImage(destImage);
    result = TCL_OK;
  error:
    if (valueArr != NULL) {
	Blt_Free(valueArr);
    }
    if (kernel != NULL) {
	Blt_Free(kernel);
    }
    return result;
}

/*ARGSUSED*/
static int
QuantizeOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle srcPhoto, destPhoto;
    Tk_PhotoImageBlock src, dest;
    Blt_ColorImage srcImage, destImage;
    int nColors = 1;
    int result;

    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(srcPhoto, &src);
    if ((src.width <= 1) || (src.height <= 1)) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" is empty",
	    (char *)NULL);
	return TCL_ERROR;
    }
    destPhoto = Blt_FindPhoto(interp, argv[3]);
    if (destPhoto == NULL) {
	Tcl_AppendResult(interp, "destination image \"", argv[3], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(destPhoto, &dest);
    if ((dest.width != src.width) || (dest.height != src.height)) {
	Tk_PhotoSetSize(destPhoto, src.width, src.height);
    }
    if (argc>4) {
       if (Tcl_GetInt(interp, argv[4], &nColors) != TCL_OK) {
	return TCL_ERROR;
       }
    }
    srcImage = Blt_PhotoToColorImage(srcPhoto);
    destImage = Blt_PhotoToColorImage(destPhoto);
    result = Blt_QuantizeColorImage(srcImage, destImage, nColors);
    if (result == TCL_OK) {
	Blt_ColorImageToPhoto(destImage, destPhoto);
    }
    Blt_FreeColorImage(srcImage);
    Blt_FreeColorImage(destImage);
    return result;
}

static int
GetColorPix32(Tcl_Interp *interp, Tk_Window tkwin, char *color, Pix32 *colPtr) {
    XColor *xCol;
    int red, green, blue;
    
    colPtr->Alpha = 255;
    if (*color == '#' && strlen(color) == 7 && 3 ==
        sscanf(color+1, "%02x%02x%02x", &red, &green, &blue)) {
        colPtr->Red = red;
        colPtr->Green = green;
        colPtr->Blue = blue;
        return TCL_OK;
    }
    xCol = Tk_GetColor(interp, tkwin, Tk_GetUid(color));
    if (xCol == NULL) {
        return TCL_ERROR;
    }
    colPtr->Red = (xCol->red >> 8);
    colPtr->Green = (xCol->green >> 8);
    colPtr->Blue = (xCol->blue >> 8);
    return TCL_OK;
}

static int
XColorToPix32(XColor *xCol, Pix32 *colPtr) {
    
    colPtr->Alpha = 255;
    colPtr->Red = (xCol->red >> 8);
    colPtr->Green = (xCol->green >> 8);
    colPtr->Blue = (xCol->blue >> 8);
    return TCL_OK;
}

/*ARGSUSED*/
static int
AlphaOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle srcPhoto, destPhoto;
    Tk_PhotoImageBlock src, dest;
    Blt_ColorImage srcImage, destImage;
    int result, alpha, anycolor = 0, withAlpha, hasWith = 0;
    Tk_Window tkwin = (Tk_Window)clientData;
    Pix32 oldColor;
    char *string;
    int negate = 0, shift = 0;

    alpha = 0;
    if (!strcmp("-shift", argv[2])) {
        shift = 1;
        argc -= 1;
        argv += 1;
    }
    
    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(srcPhoto, &src);
    if ((src.width <= 1) || (src.height <= 1)) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" is empty",
	    (char *)NULL);
	return TCL_ERROR;
    }
    destPhoto = Blt_FindPhoto(interp, argv[3]);
    if (destPhoto == NULL) {
	Tcl_AppendResult(interp, "destination image \"", argv[3], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(destPhoto, &dest);
    if (0 && srcPhoto == destPhoto) {
        Tcl_AppendResult(interp, "src and dest images must be different",
            (char *)NULL);
        return TCL_ERROR;
    }
    string = argv[4];
    if (string[0] == '!') {
        negate = 1;
        string++;
    }
    if (!strcmp(string, "*")) {
        anycolor = 1;
        if (argc <= 5) {
            Tcl_AppendResult(interp, "must give an alpha", 0);
            return TCL_ERROR;
        }
            
    } else {
        if (GetColorPix32(interp, tkwin, string, &oldColor) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (argc > 5) {
        if (Tcl_GetInt(interp, argv[5], &alpha) != TCL_OK) {
            return TCL_ERROR;
        }
        if (alpha<0 || alpha > 255) {
            Tcl_AppendResult(interp, "alpha must be >= 0 and <= 255", argv[3],
                (char *)NULL);
                return TCL_ERROR;
        }
    }
    if (argc > 6) {
        if (Tcl_GetInt(interp, argv[6], &withAlpha) != TCL_OK) {
            return TCL_ERROR;
        }
        hasWith = 1;
        if (withAlpha<0 || withAlpha > 255) {
            Tcl_AppendResult(interp, "withalpha must be >= 0 and <= 255", argv[3],
                (char *)NULL);
                return TCL_ERROR;
        }
    }
    if ((dest.width != src.width) || (dest.height != src.height)) {
        Tk_PhotoSetSize(destPhoto, src.width, src.height);
    }
    srcImage = Blt_PhotoToColorImage(srcPhoto);
    destImage = Blt_PhotoToColorImage(destPhoto);
    result = TCL_OK;
    {
    int width, height;
    int count, same;
    Pix32 *srcPtr, *destPtr, *endPtr, *color;
    unsigned char origAlpha;

    width = Blt_ColorImageWidth(srcImage);
    height = Blt_ColorImageHeight(srcImage);
    count = width * height;
    color = &oldColor;
    
    srcPtr = Blt_ColorImageBits(srcImage);
    destPtr = Blt_ColorImageBits(destImage);
    if (shift) {
        for (endPtr = destPtr + count; destPtr < endPtr; srcPtr++, destPtr++) {
            origAlpha = srcPtr->Alpha;
            if (origAlpha == 0) {
                destPtr->value = srcPtr->value;
                continue;
            }
            destPtr->value = color->value;
            destPtr->Alpha = srcPtr->Blue;
        }
    } else if (!anycolor) {
        color = &oldColor;
        for (endPtr = destPtr + count; destPtr < endPtr; srcPtr++, destPtr++) {
            origAlpha = srcPtr->Alpha;
            destPtr->value = srcPtr->value;
            same = (srcPtr->Red == color->Red && srcPtr->Green == color->Green &&
            srcPtr->Blue == color->Blue);
            if (hasWith && origAlpha != withAlpha) {
                
            } else if (negate) {
                if ((!same) && (origAlpha != (unsigned char)-1)) {
                    origAlpha = alpha;
                }
            } else {
                if (same) {
                    origAlpha = alpha;
                }
            }
            destPtr->Alpha = origAlpha;
        }
    } else {
        for (endPtr = destPtr + count; destPtr < endPtr; srcPtr++, destPtr++) {
            origAlpha = srcPtr->Alpha;
            destPtr->value = srcPtr->value;
            if (hasWith && origAlpha == withAlpha) {
                destPtr->Alpha = alpha;
            } else if (origAlpha == (unsigned char)-1) {
                destPtr->Alpha = alpha;
            }
        }
    }
    }

    if (result == TCL_OK) {
	Blt_ColorImageToPhoto(destImage, destPhoto);
    }
    Blt_FreeColorImage(srcImage);
    Blt_FreeColorImage(destImage);
    return result;
}

/*ARGSUSED*/
static int
RecolorOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle srcPhoto, destPhoto;
    Tk_PhotoImageBlock src, dest;
    Blt_ColorImage srcImage, destImage;
    int result, alpha;
    Tk_Window tkwin = (Tk_Window)clientData;
    Pix32 oldColor, newColor;

    alpha = -1;
    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(srcPhoto, &src);
    if ((src.width <= 1) || (src.height <= 1)) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" is empty",
	    (char *)NULL);
	return TCL_ERROR;
    }
    destPhoto = Blt_FindPhoto(interp, argv[3]);
    if (destPhoto == NULL) {
	Tcl_AppendResult(interp, "destination image \"", argv[3], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(destPhoto, &dest);
    if (GetColorPix32(interp, tkwin, argv[4], &oldColor) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetColorPix32(interp, tkwin, argv[5], &newColor) != TCL_OK) {
        return TCL_ERROR;
    }
    if (argc > 6) {
        if (Tcl_GetInt(interp, argv[6], &alpha) != TCL_OK) {
            return TCL_ERROR;
        }
        if (alpha<0 || alpha > 255) {
            Tcl_AppendResult(interp, "alpha must be >= 0 and <= 255", argv[3],
                (char *)NULL);
                return TCL_ERROR;
        }
    }
    if ((dest.width != src.width) || (dest.height != src.height)) {
        Tk_PhotoSetSize(destPhoto, src.width, src.height);
    }
    srcImage = Blt_PhotoToColorImage(srcPhoto);
    destImage = Blt_PhotoToColorImage(destPhoto);
    result = Blt_RecolorImage(srcImage, destImage, &oldColor, &newColor, alpha);
    if (result == TCL_OK) {
	Blt_ColorImageToPhoto(destImage, destPhoto);
    }
    Blt_FreeColorImage(srcImage);
    Blt_FreeColorImage(destImage);
    return result;
}

/*ARGSUSED*/
static int
ColorsOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle srcPhoto;
    Tk_PhotoImageBlock src;
    Blt_ColorImage srcImage;
    int top, x, y, isalph, iscnt, isNew, cnt;
    int i, rng[4], from = 0;
    register Pix32 *srcPtr;
    Tcl_Obj *listPtr;
    char buf[100];
    Blt_HashEntry *hPtr;
    Blt_HashTable hTbl;
    Blt_HashSearch cursor;

    top = 0;
    isalph = 0;
    iscnt = 0;
    while (argc > 3) {
        if (!strcmp(argv[2], "-alpha")) {
            isalph = 1;
        } else if (!strcmp(argv[2], "-from")) {
            from = 1;
            if (argc<7) {
                Tcl_AppendResult(interp, "expected 4 args: x1 y1 x2 y2", (char *)NULL);
                return TCL_ERROR;
            }
            for (i=0; i<4; i++) {
                if (Tcl_GetInt(interp, argv[i+3], rng+i)) {
                    return TCL_ERROR;
                }
            }
            argc -= 4;
            argv += 4;
        } else if (!strcmp(argv[2], "-count")) {
            iscnt = 1;
        } else {
            Tcl_AppendResult(interp, "expected -from, -alpha or -count", (char *)NULL);
            return TCL_ERROR;
        }
        argc--;
        argv++;
    }
    if (argc != 3) {
        Tcl_AppendResult(interp, "too few arguments", (char *)NULL);
        return TCL_ERROR;
    }
    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(srcPhoto, &src);
    if ((src.width < 1) || (src.height < 1)) {
	return TCL_OK;
    }
    srcImage = Blt_PhotoToColorImage(srcPhoto);
    srcPtr = Blt_ColorImageBits(srcImage);

    Blt_InitHashTable(&hTbl, BLT_STRING_KEYS);
    for (y = 0; y < src.height; y++) {
        if (from && (y < rng[1] || y > rng[3])) continue;
        for (x = 0; x < src.width; x++) {
            if (from && (x < rng[0] || x > rng[2])) continue;
            if (!isalph) {
                sprintf(buf, "#%02x%02x%02x", srcPtr->Red, srcPtr->Green, srcPtr->Blue);
            } else {
                sprintf(buf, "#%02x%02x%02x:%02x", srcPtr->Red, srcPtr->Green, srcPtr->Blue, srcPtr->Alpha);
            }
            hPtr = Blt_CreateHashEntry(&hTbl, buf, &isNew);
            if (isNew) {
                Blt_SetHashValue(hPtr, 1);
            } else {
                cnt = (int)Blt_GetHashValue(hPtr);
                cnt++;
                Blt_SetHashValue(hPtr, cnt);
            }
            srcPtr++;
        }
    }
    listPtr = Tcl_NewListObj(0,0);
    for (hPtr = Blt_FirstHashEntry(&hTbl, &cursor);
        hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
            
        Tcl_Obj *objPtr = Tcl_NewStringObj(Blt_GetHashKey(&hTbl, hPtr), -1);
        Tcl_ListObjAppendElement(interp, listPtr, objPtr);
        if (iscnt) {
            cnt = (int)Blt_GetHashValue(hPtr);
            sprintf(buf, "%d", cnt);
            objPtr = Tcl_NewStringObj(buf, -1);
            Tcl_ListObjAppendElement(interp, listPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listPtr);
    Blt_DeleteHashTable(&hTbl);
    return TCL_OK;
}

/*ARGSUSED*/
static int
TransOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle srcPhoto;
    Tk_PhotoImageBlock src;
    Blt_ColorImage srcImage;
    int x, y, alpha,isSet = 0;
    register Pix32 *srcPtr;
    char buf[100];

    if (argc == 6) {
        isSet = 1;
        if (Tcl_GetInt(interp, argv[5], &alpha) != TCL_OK) {
            return TCL_ERROR;
        }
        if (alpha<0 || alpha > 255) {
            Tcl_AppendResult(interp, "alpha must be >= 0 and <= 255", argv[3],
                (char *)NULL);
                return TCL_ERROR;
        }
    }
    if (Tcl_GetInt(interp, argv[3], &x) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[4], &y) != TCL_OK) {
        return TCL_ERROR;
    }
    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(srcPhoto, &src);
    if ((src.width < 1) || (src.height < 1)) {
        Tcl_AppendResult(interp, "empty image", (char *)NULL);
        return TCL_ERROR;
    }
    srcImage = Blt_PhotoToColorImage(srcPhoto);
    srcPtr = Blt_ColorImageBits(srcImage);
    if (y < 0 || y >= src.height || x < 0 || x >= src.width) {
        Tcl_AppendResult(interp, "out of range", (char *)NULL);
        return TCL_ERROR;
    } 
    srcPtr = srcPtr + y*src.width + x;
    if (isSet) {
        srcPtr->Alpha = alpha;
        Blt_ColorImageToPhoto(srcImage, srcPhoto);
    } else {
        sprintf(buf, "%d", srcPtr->Alpha);
        Tcl_AppendResult(interp, buf, 0);
    }
    return TCL_OK;
}

#define PD_SRC_OVER(srcColor,srcAlpha,dstColor,dstAlpha) \
	(srcColor*srcAlpha/255) + dstAlpha*(255-srcAlpha)/255*dstColor/255
#define PD_SRC_OVER_ALPHA(srcAlpha,dstAlpha) \
	(srcAlpha + (255-srcAlpha)*dstAlpha/255)

static
void PixBlend(
    Pix32 *srcPtr, Pix32 *src2Ptr, Pix32 *destPtr,
    unsigned char alpha, unsigned char dAlpha
) {
    dAlpha = 255-alpha;
    destPtr->Red = PD_SRC_OVER(srcPtr->Red, alpha, src2Ptr->Red, dAlpha);
    destPtr->Green = PD_SRC_OVER(srcPtr->Green, alpha, src2Ptr->Green, dAlpha);
    destPtr->Blue = PD_SRC_OVER(srcPtr->Blue, alpha, src2Ptr->Blue, dAlpha);
    destPtr->Alpha = 255; /* PD_SRC_OVER_ALPHA(alpha, dAlpha); */
}

static
void PixMerged( Pix32 *srcPtr, Pix32 *src2Ptr, Pix32 *destPtr, Pix32 *colorPtr) {
    unsigned char alpha;
    int isdead;
    
    alpha = srcPtr->Blue;
    isdead = (srcPtr->Red == 0xde && srcPtr->Green == 0xad);
    if (alpha == 0 && isdead) { /* "," inner fill */
        destPtr->value = src2Ptr->value;
    } else if (alpha == 0) {    /* "." none or transparent */
        destPtr->value = 0;
    } else if (isdead == 0) {   /* solid color with alpha */
        destPtr->value = colorPtr->value;
        destPtr->Alpha = alpha;
    } else {                    /* Blend color with inner fill */
        PixBlend( colorPtr, src2Ptr, destPtr, alpha, src2Ptr->Alpha);
    }
}


/* For RGB grab alpha from blue and substitute maskcolor for RG="#DEAD"*/
int Blt_ImageMergeInner(Tcl_Interp *interp, char *srcName, char *src2Name,
    char * destName, XColor *maskColor, int leaveMsg) {
    Tk_PhotoHandle srcPhoto, srcPhoto2, destPhoto;
    Tk_PhotoImageBlock src, src2, dest;
    Blt_ColorImage srcImage, srcImage2, destImage;
    double opacity;
    double opacity2;
    int result, isWc;
    Pix32 withColor;

    isWc = 0;
    opacity = 0.5;
    opacity2 = -1;
    srcPhoto = Blt_FindPhoto(interp, srcName);
    if (srcPhoto == NULL) {
        if (leaveMsg) 
            Tcl_AppendResult(interp, "source image \"", srcName, "\" doesn't",
            " exist or is not a photo image", (char *)NULL);
        return TCL_ERROR;
    }
    Tk_PhotoGetImage(srcPhoto, &src);
    if ((src.width <= 1) || (src.height <= 1)) {
        if (leaveMsg) 
            Tcl_AppendResult(interp, "source image \"", srcName, "\" is empty",
               (char *)NULL);
        return TCL_ERROR;
    }

    srcPhoto2 = Blt_FindPhoto(interp, src2Name);
    if (srcPhoto2 == NULL) {
        if (leaveMsg) 
            Tcl_AppendResult(interp, "source image \"", src2Name, "\" doesn't",
                " exist or is not a photo image", (char *)NULL);
        return TCL_ERROR;
    }
    Tk_PhotoGetImage(srcPhoto2, &src2);
    if ((src2.width <= 1) || (src2.height <= 1)) {
        if (leaveMsg) 
           Tcl_AppendResult(interp, "source image \"", src2Name, "\" is empty",
                (char *)NULL);
        return TCL_ERROR;
    }
    if (maskColor) {
        XColorToPix32(maskColor, &withColor);
        isWc = 1;
        goto domerge;
    }
    /*
    if (argc>5) {
        
        if (isdigit(argv[5][0]) == 0 && argv[5][0] != '.') {
            Tk_Window tkwin;
            char *colStr;
            
            tkwin = Tk_MainWindow(interp);
            isWc = 1;
            
            colStr = argv[5];
            if (colStr[0] == '!') {
                colStr++;
                isWc = 2;
            }

                return TCL_ERROR;
            }
            if (src.width < 4 || src.height < 4) {
                Tcl_AppendResult(interp, "src image too small ",  0);
                return TCL_ERROR;
            }
            if (src2.width < 4 || src2.height < 4) {
                Tcl_AppendResult(interp, "src2 image too small ",  0);
                return TCL_ERROR;
            }
            if (dest.width < 4 || dest.height < 4) {
                Tcl_AppendResult(interp, "dest image too small ",  0);
                return TCL_ERROR;
            }
            goto domerge;
        }
        
        if (Tcl_GetDouble(interp, argv[5], &opacity) != TCL_OK) {
            return TCL_ERROR;
        }
        if (opacity < 0.0 || opacity > 1.0) {
            Tcl_AppendResult(interp, "opacity must be >= 0.0 and <= 1.0: ", argv[5], (char *)NULL);
            return TCL_ERROR;
        }
    }
    if (argc>6) {
        if (Tcl_GetDouble(interp, argv[6], &opacity2) != TCL_OK) {
            return TCL_ERROR;
        }
        if (opacity2 < 0.0 || opacity2 > 1.0) {
            Tcl_AppendResult(interp, "opacity must be >= 0.0 and <= 1.0: ", argv[6], (char *)NULL);
            return TCL_ERROR;
        }
    }
    */
    if (isWc == 0 && ((src2.width != src.width) || (src2.height != src.height))) {
        int sh, sw;
        sw = (src2.width>src.width?src2.width:src.width);
        sh = (src2.height>src.height?src2.height:src.height);
        Tk_PhotoSetSize(srcPhoto2, sw, sh);
        Tk_PhotoGetImage(srcPhoto2, &src2);
        if (sw != src.width || sh != src.height) {
            Tk_PhotoSetSize(srcPhoto, sw, sh);
            Tk_PhotoGetImage(srcPhoto, &src);
        }
    }

    domerge:
    destPhoto = Blt_FindPhoto(interp, destName);
    if (destPhoto == NULL) {
        if (leaveMsg)
            Tcl_AppendResult(interp, "destination image \"", destName, "\" doesn't",
            " exist or is not a photo image", (char *)NULL);
        return TCL_ERROR;
    }
    Tk_PhotoGetImage(destPhoto, &dest);

    if ((dest.width != src.width) || (dest.height != src.height)) {
        Tk_PhotoSetSize(destPhoto, src.width, src.height);
    }
    srcImage = Blt_PhotoToColorImage(srcPhoto);
    srcImage2 = Blt_PhotoToColorImage(srcPhoto2);
    destImage = Blt_PhotoToColorImage(destPhoto);

    Tk_PhotoGetImage(destPhoto, &dest);
    if ((dest.width != src.width) || (dest.height != src.height)) {
        Tk_PhotoSetSize(destPhoto, src.width, src.height);
        destImage = Blt_PhotoToColorImage(destPhoto);
    }
    if (isWc != 1) {
        result = Blt_MergeColorImage(srcImage, srcImage2, destImage, opacity, opacity2,
            isWc ? &withColor : NULL);
    } else {
        int xs1, xs2, xd1, xd2, xsc, xdc;
        int ys1, ys2, yd1, yd2, ysc, ydc;
        Pix32 *srcPtr, *src2Ptr, *destPtr;

        srcPtr = Blt_ColorImageBits(srcImage);
        src2Ptr = Blt_ColorImageBits(srcImage2);
        destPtr = Blt_ColorImageBits(destImage);

        xsc = (src2.width/2);
        ysc = (src2.height/2);
        xdc = (dest.width/2);
        ydc = (dest.height/2);
        for (xs1 = xsc-1, xs2=xsc, xd1 = xdc-1, xd2=xd1+1;
        xd1>=0; xd1--, xs1--, xd2++, xs2++) {
            if (xs1<0) { xs1 = xsc-1; xs2 = xs1+1; }
            for (ys1 = ysc-1, ys2 = ysc, yd1 = ydc-1, yd2=yd1+1;
            yd1>=0; yd1--, ys1--, yd2++, ys2++) {
                if (ys1<0) { ys1 = ysc-1; ys2 = ys1+1; }
                
#define DDDC_O(ind,ind2) if ( withColor.value == srcPtr[ind].value) destPtr[ind].value =  src2Ptr[ind2].value
#define DDDC(ind,ind2) PixMerged(srcPtr+(ind), src2Ptr+(ind2), destPtr+(ind), &withColor)

                DDDC(yd1*dest.width+xd1, ys1*src2.width+xs1);
                if (ys2>=src2.height) { ys2--; }
                if (xs2>=src2.width) { xs2--; }
                if (yd2<dest.height && xd2<dest.width) {
                    DDDC(yd2*dest.width+xd2, ys2*src2.width+xs2);
                }
                if (xd2<dest.width) {
                    DDDC(yd1*dest.width+xd2, ys1*src2.width+xs2);
                }
                if (yd2<dest.height) {
                    DDDC(yd2*dest.width+xd1, ys2*src2.width+xs1);
                }
            }
        }
        result = TCL_OK;

    }
    if (result == TCL_OK) {
        Blt_ColorImageToPhoto(destImage, destPhoto);
    }
    Blt_FreeColorImage(srcImage);
    Blt_FreeColorImage(srcImage2);
    Blt_FreeColorImage(destImage);
    return result;
}


/*ARGSUSED*/
static int
MergeOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle srcPhoto, srcPhoto2, destPhoto;
    Tk_PhotoImageBlock src, src2, dest;
    Blt_ColorImage srcImage, srcImage2, destImage;
    double opacity;
    double opacity2;
    int result, isWc;
    Pix32 withColor;

    isWc = 0;
    opacity = 0.5;
    opacity2 = -1;
    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(srcPhoto, &src);
    if ((src.width <= 1) || (src.height <= 1)) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" is empty",
	    (char *)NULL);
	return TCL_ERROR;
    }

    srcPhoto2 = Blt_FindPhoto(interp, argv[3]);
    if (srcPhoto2 == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[3], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(srcPhoto2, &src2);
    if ((src2.width <= 1) || (src2.height <= 1)) {
	Tcl_AppendResult(interp, "source image \"", argv[3], "\" is empty",
	    (char *)NULL);
	return TCL_ERROR;
    }
    if (argc>5) {
        
        if (isdigit(argv[5][0]) == 0 && argv[5][0] != '.') {
            Tk_Window tkwin;
            char *colStr;
            
            tkwin = Tk_MainWindow(interp);
            isWc = 1;
            
            colStr = argv[5];
            if (colStr[0] == '!') {
                colStr++;
                isWc = 2;
            }

            if (GetColorPix32(interp, tkwin, colStr, &withColor) != TCL_OK) {
                return TCL_ERROR;
            }
            if (src.width < 4 || src.height < 4) {
                Tcl_AppendResult(interp, "src image too small ",  0);
                return TCL_ERROR;
            }
            if (src2.width < 4 || src2.height < 4) {
                Tcl_AppendResult(interp, "src2 image too small ",  0);
                return TCL_ERROR;
            }
            goto domerge;
        }
        
        if (Tcl_GetDouble(interp, argv[5], &opacity) != TCL_OK) {
            return TCL_ERROR;
        }
        if (opacity < 0.0 || opacity > 1.0) {
            Tcl_AppendResult(interp, "opacity must be >= 0.0 and <= 1.0: ", argv[5], (char *)NULL);
            return TCL_ERROR;
        }
    }
    if (argc>6) {
        if (Tcl_GetDouble(interp, argv[6], &opacity2) != TCL_OK) {
            return TCL_ERROR;
        }
        if (opacity2 < 0.0 || opacity2 > 1.0) {
            Tcl_AppendResult(interp, "opacity must be >= 0.0 and <= 1.0: ", argv[6], (char *)NULL);
            return TCL_ERROR;
        }
    }

    if (isWc == 0 && ((src2.width != src.width) || (src2.height != src.height))) {
	int sh, sw;
	sw = (src2.width>src.width?src2.width:src.width);
	sh = (src2.height>src.height?src2.height:src.height);
	Tk_PhotoSetSize(srcPhoto2, sw, sh);
        Tk_PhotoGetImage(srcPhoto2, &src2);
	if (sw != src.width || sh != src.height) {
	    Tk_PhotoSetSize(srcPhoto, sw, sh);
            Tk_PhotoGetImage(srcPhoto, &src);
	}
    }

domerge:
    destPhoto = Blt_FindPhoto(interp, argv[4]);
    if (destPhoto == NULL) {
	Tcl_AppendResult(interp, "destination image \"", argv[4], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(destPhoto, &dest);

    srcImage = Blt_PhotoToColorImage(srcPhoto);
    srcImage2 = Blt_PhotoToColorImage(srcPhoto2);
    destImage = Blt_PhotoToColorImage(destPhoto);

    Tk_PhotoGetImage(destPhoto, &dest);
    if ((dest.width != src.width) || (dest.height != src.height)) {
	Tk_PhotoSetSize(destPhoto, src.width, src.height);
        destImage = Blt_PhotoToColorImage(destPhoto);
    }
    if (isWc != 1) {
        result = Blt_MergeColorImage(srcImage, srcImage2, destImage, opacity, opacity2,
            isWc ? &withColor : NULL);
    } else {
        int xs1, xs2, xd1, xd2, xsc, xdc;
        int ys1, ys2, yd1, yd2, ysc, ydc;
        Pix32 *srcPtr, *src2Ptr, *destPtr;

        srcPtr = Blt_ColorImageBits(srcImage);
        src2Ptr = Blt_ColorImageBits(srcImage2);
        destPtr = Blt_ColorImageBits(destImage);

        xsc = (src2.width/2);
        ysc = (src2.height/2);
        xdc = (dest.width/2);
        ydc = (dest.height/2);
        for (xs1 = xsc-1, xs2=xs1+1, xd1 = xdc-1, xd2=xdc;
        xd1>=0; xd1--, xs1--, xd2++, xs2++) {
            if (xs1<0) { xs1 = xsc-1; xs2 = xs1+1; }
            for (ys1 = ysc-1, ys2 = ys1+1, yd1 = ydc-1, yd2=yd1+1;
            yd1>=0; yd1--, ys1--, yd2++, ys2++) {
                if (ys1<0) { ys1 = ysc-1; ys2 = ys1+1; }
                
#define DDC(ind,ind2) destPtr[ind].value = ( withColor.value == srcPtr[ind].value ? src2Ptr[ind2].value : srcPtr[ind].value)

                DDC(yd1*dest.width+xd1, ys1*src2.width+xs1);
                //destPtr[yd1*dest.width+xd1].value = srcPtr[ys1*src2.width+xs1].value;
                if (ys2>=src2.height) { ys2--; }
                if (xs2>=src2.width) { xs2--; }
                if (yd2<dest.height && xd2<dest.width)
                DDC(yd2*dest.width+xd2, ys2*src2.width+xs2);
                // destPtr[yd2*dest.width+xd2].value = srcPtr[ys2*src2.width+xs2].value;
                if (xd2<dest.width)
                DDC(yd1*dest.width+xd2, ys1*src2.width+xs2);
                //destPtr[yd1*dest.width+xd2].value = srcPtr[ys1*src2.width+xs2].value;
                if (yd2<dest.height)
                DDC(yd2*dest.width+xd1, ys2*src2.width+xs1);
                //destPtr[yd2*dest.width+xd1].value = srcPtr[ys2*src2.width+xs1].value;
                //if (ya<src.height && ya2<dest.height && xa<src.width && xa2<dest.width) {}
            }
        }
        result = TCL_OK;

    }
    if (result == TCL_OK) {
	Blt_ColorImageToPhoto(destImage, destPhoto);
    }
    Blt_FreeColorImage(srcImage);
    Blt_FreeColorImage(srcImage2);
    Blt_FreeColorImage(destImage);
    return result;
}


/*ARGSUSED*/
static int
ReadJPEGOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
#if HAVE_JPEGLIB_H
    int result;
    Tcl_DString dString;
    char *fileName;
    Tk_PhotoHandle photo;	/* The photo image to write into. */

    photo = Blt_FindPhoto(interp, argv[3]);
    if (photo == NULL) {
	Tcl_AppendResult(interp, "image \"", argv[3], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_DStringInit(&dString);
    fileName = Tcl_TranslateFileName(interp, argv[2], &dString);
    
    result = Blt_JPEGToPhoto(interp, fileName, photo);
    Tcl_DStringInit(&dString);
    return result;
#else
    Tcl_AppendResult(interp, "JPEG support not compiled", (char *)NULL);
    return TCL_ERROR;
#endif
}

static double Drand( double val) {
#ifdef HAVE_DRAND48
    return drand48();
#else
    return (double)random()/RAND_MAX;
#endif
}

#define SLANTX(x,y) (doslant==0 || slant == 0.0?x:(((int)(x + (y * slant)))%src.width))
#define ARCX(x,y) (doarc==0 || sineVal == 0.0?x:(((int)(x + (src.width*sqrt(0.5*0.5-((double)y/src.height-0.5)*((double)y/src.height-0.5)) * sineVal)))%src.width))
#define SINEX(x,y) (dosine==0 || sineVal == 0.0?x:(((int)(x + (src.width*(*tfunc)(M_PI *y/src.height) * sineVal)))%src.width))
#ifdef HAVE_DRAND48
#define RANDX(c) (dorand==0 || randVal == 0.0?c:(c+(Drand(0) * randVal - (randVal/2.0))))
#else
#define RANDX(c) c
#endif
#define SKEWX(c) ((doskew==0 || skew == 1.0)?c:(ox>=skew?1.0:c/skew))
#define CLAMP(c)	((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))

static int
GradientsOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    double range[3];
    double left[3];
    int x, y, width;
    double t;
    XColor C;
    XColor *leftPtr, *rightPtr;
    Tk_Window tkwin;
    
    tkwin = Tk_MainWindow(interp);

    leftPtr = Tk_GetColor(interp, tkwin, Tk_GetUid(argv[2]));
    if (leftPtr == NULL) {
        return TCL_ERROR;
    }
    rightPtr = Tk_GetColor(interp, tkwin, Tk_GetUid(argv[3]));
    if (rightPtr == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[4], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (width<=2) {
        Tcl_AppendResult(interp, "width must be > 2", 0);
        return TCL_ERROR;
    }
    left[0] = (double)(leftPtr->red >> 8);
    left[1] = (double)(leftPtr->green >> 8);
    left[2] = (double)(leftPtr->blue >> 8);
    range[0] = (double)((rightPtr->red - leftPtr->red) / 257.0);
    range[1] = (double)((rightPtr->green - leftPtr->green) / 257.0);
    range[2] = (double)((rightPtr->blue - leftPtr->blue) / 257.0);

    y =0;
    for (x = 0; x < width; x++) {
        char cbuf[100];
        t = ((double)( sin(M_PI_2 * (double)x / (double)width)));
        t = CLAMP(t);
        C.red = (unsigned short)(left[0] + t * range[0]);
        C.green = (unsigned short)(left[1] + t * range[1]);
        C.blue = (unsigned short)(left[2] + t * range[2]);
        sprintf(cbuf, "#%02x%02x%02x", C.red, C.green, C.blue);
        if (x) { Tcl_AppendResult(interp, " ", 0); }
        Tcl_AppendResult(interp, cbuf, 0);
    }
    return TCL_OK;
}

int
Blt_GetGradient(Tcl_Interp *interp, Tk_Window tkwin, Gradient *gradPtr) {
    XColor *leftPtr, *rightPtr;
    double range[3];
    double left[3];
    int x, y, width;
    double t;
    XColor **destPtr, C;

    leftPtr = gradPtr->color;
    rightPtr = gradPtr->color2;
    left[0] = (double)(leftPtr->red >> 8);
    left[1] = (double)(leftPtr->green >> 8);
    left[2] = (double)(leftPtr->blue >> 8);
    range[0] = (double)((rightPtr->red - leftPtr->red) / 257.0);
    range[1] = (double)((rightPtr->green - leftPtr->green) / 257.0);
    range[2] = (double)((rightPtr->blue - leftPtr->blue) / 257.0);

    y =0;
    width = gradPtr->width;
    if (gradPtr->grads) {
        Blt_FreeGradient(gradPtr);
    }
    destPtr = gradPtr->grads = (XColor**)Blt_Calloc(width+1, sizeof(XColor*));
    for (x = 0; x < width; x++) {
        char cbuf[100];
        t = ((double)( sin(M_PI_2 * (double)x / (double)width)));
        t = CLAMP(t);
        C.red = (unsigned short)(left[0] + t * range[0]);
        C.green = (unsigned short)(left[1] + t * range[1]);
        C.blue = (unsigned short)(left[2] + t * range[2]);
        /**destPtr = Tk_GetColorByValue(tkwin, &C); */
        sprintf(cbuf, "#%02x%02x%02x", C.red, C.green, C.blue);
        *destPtr = Tk_GetColor(interp, tkwin, Tk_GetUid(cbuf));
        if (*destPtr == NULL) break;
        destPtr++;
    }
    gradPtr->origColor = gradPtr->color;
    gradPtr->origColor2 = gradPtr->color2;
    gradPtr->origWidth = gradPtr->width;
    return TCL_OK;
}

int
Blt_FreeGradient(Gradient *gradPtr) {
    XColor **destPtr;
    if (gradPtr->grads) {
        destPtr = gradPtr->grads;
        while (*destPtr) {
            Tk_FreeColor(*destPtr);
            destPtr++;
        }
        Blt_Free( gradPtr->grads);
    }
    gradPtr->grads = NULL;
    return TCL_OK;

}

/*ARGSUSED*/
static int
GradientOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle photo;
    Tk_PhotoImageBlock src;
    XColor *leftPtr, *rightPtr;
    Tk_Window tkwin;
    double range[3];
    double left[3];
    Pix32 *destPtr;
    Blt_ColorImage destImage;
    double skew, randVal, slant, sineVal;
    int dorand, doslant, doskew, doarc, dosine, gtype, result, ox;
    int nWidth = 0, nHeight = 0;
    Tcl_Obj *objPtr;
    static char *types[] = { "linear", "radial", "sine", "halfsine", "rectangular",  "split", "blank", 0 };
    enum ITypeIdx {
        ILinearIdx, IRadialIdx, ISineIdx, IHalfSineIdx, IRectIdx, ISplitIdx,
        IBlankIdx
    };
    double (*tfunc)(double);
    int alpha;
    unsigned char aVal;

    aVal = 255;
    tfunc = &sin;
    dorand = 0;
    doskew = 0;
    doslant = 0;
    dosine = 0;
    doarc = 0;
    skew = 1.0;
    gtype = ISineIdx;
    tkwin = Tk_MainWindow(interp);
    photo = Blt_FindPhoto(interp, argv[2]);
    if (photo == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(photo, &src);
    leftPtr = Tk_GetColor(interp, tkwin, Tk_GetUid(argv[3]));
    if (leftPtr == NULL) {
	return TCL_ERROR;
    }
    rightPtr = Tk_GetColor(interp, tkwin, Tk_GetUid(argv[4]));
    if (rightPtr == NULL) {
	return TCL_ERROR;
    }
    while (argc > 5) {
        if (argc < 7) {
            Tcl_AppendResult(interp, "expected argument", argv[5], (char *)NULL);
            return TCL_ERROR;
        }
        if (!strcmp("-alpha", argv[5])) {
            if (Tcl_GetInt(interp, argv[6], &alpha) != TCL_OK) {
                return TCL_ERROR;
            }
            if (alpha<=0 || alpha > 255) {
                Tcl_AppendResult(interp, "alpha must be > 0 and <= 255", argv[6],
                    (char *)NULL);
                    return TCL_ERROR;
            }
            aVal = (unsigned char) alpha;

        } else if (!strcmp("-type", argv[5])) {
            objPtr = Tcl_NewStringObj(argv[6],-1);
            Tcl_IncrRefCount(objPtr);
            result = Tcl_GetIndexFromObj(interp, objPtr, types, "type", 0,
                &gtype);
            Tcl_DecrRefCount(objPtr);
            if (result != TCL_OK) {
                return TCL_ERROR;
            }
                
        } else if (!strcmp("-skew", argv[5])) {
            doskew = 1;
            if (Tcl_GetDouble(interp, argv[6], &skew) != TCL_OK) {
                return TCL_ERROR;
            }
            if (skew<0.0 || skew>1.0) {
                Tcl_AppendResult(interp, "skew must be >=0 && <=1.0: ", argv[6],
                    (char *)NULL);
                    return TCL_ERROR;
            }
        } else if (!strcmp("-width", argv[5])) {
            if (Tk_GetPixels(interp, tkwin, argv[6], &nWidth) != TCL_OK) {
                return TCL_ERROR;
            }
            if (nWidth<1 || nWidth>5000) {
                Tcl_AppendResult(interp, "width must be >=1 && <=5000: ", argv[6],
                    (char *)NULL);
                    return TCL_ERROR;
            }
        } else if (!strcmp("-height", argv[5])) {
            if (Tk_GetPixels(interp, tkwin, argv[6], &nHeight) != TCL_OK) {
                return TCL_ERROR;
            }
            if (nHeight<1 || nHeight>5000) {
                Tcl_AppendResult(interp, "width must be >=1 && <=5000: ", argv[6],
                    (char *)NULL);
                    return TCL_ERROR;
            }
        } else if (!strcmp("-rand", argv[5])) {
            dorand = 1;
            if (Tcl_GetDouble(interp, argv[6], &randVal) != TCL_OK) {
                return TCL_ERROR;
            }
            if (randVal<0.0 || randVal>0.1) {
                Tcl_AppendResult(interp, "randVal must be >= 0.0 && <= 0.1: ", argv[6],
                    (char *)NULL);
                    return TCL_ERROR;
            }
        } else if (!strcmp("-slant", argv[5])) {
            doslant = 1;
            if (Tcl_GetDouble(interp, argv[6], &slant) != TCL_OK) {
                return TCL_ERROR;
            }
            if (slant<-1000.0 || slant>1000.0) {
                Tcl_AppendResult(interp, "slant must be >= -1000.0 && <= 1000.0: ", argv[6],
                    (char *)NULL);
                    return TCL_ERROR;
            }
        } else if (!strcmp("-mathval", argv[5])) {
            dosine = 1;
            if (Tcl_GetDouble(interp, argv[6], &sineVal) != TCL_OK) {
                return TCL_ERROR;
            }
            if (sineVal<-1000.0 || sineVal>1000.0) {
                Tcl_AppendResult(interp, "mathval must be >= -1000.0 && <= 1000.0: ", argv[6],
                    (char *)NULL);
                    return TCL_ERROR;
            }
        } else if (!strcmp("-mathfunc", argv[5])) {
            if (!strcmp("circle", argv[6])) {
                dosine = 0;
                doarc = 1;
            } else if (!strcmp("sin", argv[6])) {
                tfunc = &sin;
            } else if (!strcmp("cos", argv[6])) {
                tfunc = &cos;
            } else if (!strcmp("atan", argv[6])) {
                tfunc = &atan;
            } else if (!strcmp("acos", argv[6])) {
                tfunc = &acos;
            } else if (!strcmp("asin", argv[6])) {
                tfunc = &asin;
            } else if (!strcmp("rand", argv[6])) {
                tfunc = &Drand;
            } else if (!strcmp("cosh", argv[6])) {
                tfunc = &cosh;
            } else if (!strcmp("sinh", argv[6])) {
                tfunc = &sinh;
            } else if (!strcmp("tan", argv[6])) {
                tfunc = &tan;
            } else if (!strcmp("tanh", argv[6])) {
                tfunc = &tanh;
            } else if (!strcmp("log", argv[6])) {
                tfunc = &log;
            } else if (!strcmp("log10", argv[6])) {
                tfunc = &log10;
            } else if (!strcmp("exp", argv[6])) {
                tfunc = &exp;
            } else if (!strcmp("sqrt", argv[6])) {
                tfunc = &sqrt;
            } else {
                Tcl_AppendResult(interp, "mathfunc ", argv[6], " not one of: ",
                    "sin cos tan sinh cosh tanh asin acos atan log log10 exp sqrt rand circle", (char *)NULL);
                return TCL_ERROR;
            }
        } else {
            Tcl_AppendResult(interp, "argument \"", argv[5], "\" not one of: -type -skew -rand -slant -curve -circle",
                (char *)NULL);
                return TCL_ERROR;
        }
        argc -= 2;
        argv += 2;
    }
    if ((nWidth>0 || nHeight>0) && ((nWidth != src.width) || (nHeight != src.height))) {
        if (nWidth<=0) { nWidth = src.width; }
        if (nHeight<=0) { nHeight = src.height; }
        Tk_PhotoSetSize(photo, nWidth, nHeight);
        Tk_PhotoGetImage(photo, &src);
    }
    left[0] = (double)(leftPtr->red >> 8);
    left[1] = (double)(leftPtr->green >> 8);
    left[2] = (double)(leftPtr->blue >> 8);
    range[0] = (double)((rightPtr->red - leftPtr->red) / 257.0);
    range[1] = (double)((rightPtr->green - leftPtr->green) / 257.0);
    range[2] = (double)((rightPtr->blue - leftPtr->blue) / 257.0);

    destImage = Blt_CreateColorImage(src.width, src.height);
    destPtr = Blt_ColorImageBits(destImage);
    
    if (gtype == ILinearIdx) {
	register int x, y, sx;
	double t;

	for (y = 0; y < src.height; y++) {
	    for (x = 0; x < src.width; x++) {
		ox = (double)x / src.width;
		sx = SLANTX(x,y);
		sx = ARCX(sx,y);
		sx = SINEX(sx,y);
		t = (double)sx / src.width;
		t = SKEWX(t);
		t = RANDX(t);
		t = CLAMP(t);
		destPtr->Red = (unsigned char)(left[0] + t * range[0]);
		destPtr->Green = (unsigned char)(left[1] + t * range[1]);
		destPtr->Blue = (unsigned char)(left[2] + t * range[2]);
		destPtr->Alpha = aVal;
		destPtr++;
	    }
	}
     } else if (gtype == ISineIdx) {
	register int x, y, sx;
	double t;

	for (y = 0; y < src.height; y++) {
	    for (x = 0; x < src.width; x++) {
		ox = (double)x / src.width;
		sx = SLANTX(x,y);
		sx = ARCX(sx,y);
		sx = SINEX(sx,y);
		t = ((double)(sin(2*M_PI_2 * (double)sx / (double)src.width)));
		t = SKEWX(t);
		t = RANDX(t);
 		t = CLAMP(t);
		destPtr->Red = (unsigned char)(left[0] + t * range[0]);
		destPtr->Green = (unsigned char)(left[1] + t * range[1]);
		destPtr->Blue = (unsigned char)(left[2] + t * range[2]);
		destPtr->Alpha = aVal;
		destPtr++;
	    }
	}
    } else if (gtype == IHalfSineIdx) {
	register int x, y, sx;
	double t;

	for (y = 0; y < src.height; y++) {
	    for (x = 0; x < src.width; x++) {
		ox = (double)x / src.width;
		sx = SLANTX(x,y);
		sx = ARCX(sx,y);
		sx = SINEX(sx,y);
		t = ((double)( sin(M_PI_2 * (double)sx / (double)src.width)));
		t = SKEWX(t);
		t = RANDX(t);
 		t = CLAMP(t);
		destPtr->Red = (unsigned char)(left[0] + t * range[0]);
		destPtr->Green = (unsigned char)(left[1] + t * range[1]);
		destPtr->Blue = (unsigned char)(left[2] + t * range[2]);
		destPtr->Alpha = aVal;
		destPtr++;
	    }
	}
    } else if (gtype == IRadialIdx) {
	register int x, y, sx;
	register double dx, dy;
	double dy2;
	double t;
	double midX, midY;

	midX = midY = 0.5;
	for (y = 0; y < src.height; y++) {
	    dy = (y / (double)src.height) - midY;
	    dy2 = dy * dy;
	    for (x = 0; x < src.width; x++) {
		ox = (double)x / src.width;
		sx = SLANTX(x,y);
		sx = ARCX(sx,y);
		sx = SINEX(sx,y);
		dx = (sx / (double)src.width) - midX;
		t = 1.0  - (double)sqrt(dx * dx + dy2);
		t = SKEWX(t);
		t = RANDX(t);
		t = CLAMP(t);
		destPtr->Red = (unsigned char)(left[0] + t * range[0]);
		destPtr->Green = (unsigned char)(left[1] + t * range[1]);
		destPtr->Blue = (unsigned char)(left[2] + t * range[2]);
		destPtr->Alpha = aVal;
		destPtr++;
	    }
	}
  
    } else if (gtype == IRectIdx) {
	register int x, y, sx;
	register double dx, dy;
	double t, px, py;
	double midX, midY;
	double cosTheta, sinTheta;
	double angle;

	angle = M_PI_2 * -0.3;
	cosTheta = cos(angle);
	sinTheta = sin(angle);

	midX = 0.5, midY = 0.5;
	for (y = 0; y < src.height; y++) {
	    dy = (y / (double)src.height) - midY;
	    for (x = 0; x < src.width; x++) {
		ox = (double)x / src.width;
		sx = SLANTX(x,y);
		sx = ARCX(sx,y);
		sx = SINEX(sx,y);
		dx = (sx / (double)src.width) - midX;
		px = dx * cosTheta - dy * sinTheta;
		py = dx * sinTheta + dy * cosTheta;
		t = FABS(px) + FABS(py);
		t = SKEWX(t);
		t = RANDX(t);
		t = CLAMP(t);
		destPtr->Red = (unsigned char)(left[0] + t * range[0]);
		destPtr->Green = (unsigned char)(left[1] + t * range[1]);
		destPtr->Blue = (unsigned char)(left[2] + t * range[2]);
		destPtr->Alpha = aVal;
		destPtr++;
	    }
	}
    } else if (gtype == ISplitIdx) {
	register int x, y, sx, midx;
	double t;

        midx = src.width/2;
	for (y = 0; y < src.height; y++) {
	    for (x = 0; x < src.width; x++) {
		ox = (double)x / src.width;
		sx = SLANTX(x,y);
		sx = ARCX(sx,y);
		sx = SINEX(sx,y);
	        if (x<midx) {
	            sx += (0.1 * src.width);
	        } else {
	            sx -= (0.1 * src.width);
	        }
		t = (double)sx / src.width;
		t = SKEWX(t);
		t = RANDX(t);
		t = CLAMP(t);
		destPtr->Red = (unsigned char)(left[0] + t * range[0]);
		destPtr->Green = (unsigned char)(left[1] + t * range[1]);
		destPtr->Blue = (unsigned char)(left[2] + t * range[2]);
		destPtr->Alpha = aVal;
		destPtr++;
	    }
	}
    } else if (gtype == IBlankIdx) {
	register int x, y;

	for (y = 0; y < src.height; y++) {
	    for (x = 0; x < src.width; x++) {
		destPtr->Red = (unsigned char)0xFF;
		destPtr->Green = (unsigned char)0xFF;
		destPtr->Blue = (unsigned char)0xFF;
		destPtr->Alpha = aVal;
		destPtr++;
	    }
	}
    }
    Blt_ColorImageToPhoto(destImage, photo);
    Blt_FreeColorImage(destImage);
    return TCL_OK;
}

/*ARGSUSED*/
static int
ResampleOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle srcPhoto, destPhoto;
    Tk_PhotoImageBlock src, dest;
    ResampleFilter *filterPtr, *vertFilterPtr, *horzFilterPtr;
    char *filterName;

    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    destPhoto = Blt_FindPhoto(interp, argv[3]);
    if (destPhoto == NULL) {
	Tcl_AppendResult(interp, "destination image \"", argv[3], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    filterName = (argc > 4) ? argv[4] : "none";
    if (Blt_GetResampleFilter(interp, filterName, &filterPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    vertFilterPtr = horzFilterPtr = filterPtr;
    if ((filterPtr != NULL) && (argc > 5)) {
	if (Blt_GetResampleFilter(interp, argv[5], &filterPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	vertFilterPtr = filterPtr;
    }
    Tk_PhotoGetImage(srcPhoto, &src);
    if ((src.width <= 1) || (src.height <= 1)) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" is empty",
	    (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(destPhoto, &dest);
    if ((dest.width <= 1) || (dest.height <= 1)) {
	Tk_PhotoSetSize(destPhoto, src.width, src.height);
	goto copyImage;
    }
    if ((src.width == dest.width) && (src.height == dest.height)) {
      copyImage:
	/* Source and destination image sizes are the same. Don't
	 * resample. Simply make copy of image */
	dest.width = src.width;
	dest.height = src.height;
	dest.pixelPtr = src.pixelPtr;
	dest.pixelSize = src.pixelSize;
	dest.pitch = src.pitch;
	dest.offset[0] = src.offset[0];
	dest.offset[1] = src.offset[1];
	dest.offset[2] = src.offset[2];
	Tk_PhotoPutBlock(destPhoto, &dest, 0, 0, dest.width, dest.height);
	return TCL_OK;
    }
    if (filterPtr == NULL) {
	Blt_ResizePhoto(srcPhoto, 0, 0, src.width, src.height, destPhoto);
    } else {
	Blt_ResamplePhoto(srcPhoto, 0, 0, src.width, src.height, destPhoto,
		horzFilterPtr, vertFilterPtr);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
BlurOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle srcPhoto, destPhoto;
    Tk_PhotoImageBlock src, dest;
    double radius;

    radius = 3;
    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    destPhoto = Blt_FindPhoto(interp, argv[3]);
    if (destPhoto == NULL) {
	Tcl_AppendResult(interp, "destination image \"", argv[3], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    if (argc > 4) {
        if (Tcl_GetDouble(interp, argv[4], &radius) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    Tk_PhotoGetImage(srcPhoto, &src);
    if ((src.width <= 1) || (src.height <= 1)) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" is empty",
	    (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(destPhoto, &dest);
    Tk_PhotoSetSize(destPhoto, src.width, src.height);
    return Blt_BlurColorImage(srcPhoto, destPhoto, (int)(radius + 0.5) );
}

/*ARGSUSED*/
static int
RotateOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle srcPhoto, destPhoto;
    Blt_ColorImage srcImage, destImage;
    double theta;

    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    destPhoto = Blt_FindPhoto(interp, argv[3]);
    if (destPhoto == NULL) {
	Tcl_AppendResult(interp, "destination image \"", argv[3], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    if (Tcl_ExprDouble(interp, argv[4], &theta) != TCL_OK) {
	return TCL_ERROR;
    }
    srcImage = Blt_PhotoToColorImage(srcPhoto);
    destImage = Blt_RotateColorImage(srcImage, theta);

    Blt_ColorImageToPhoto(destImage, destPhoto);
    Blt_FreeColorImage(srcImage);
    Blt_FreeColorImage(destImage);
    return TCL_OK;
}

/*ARGSUSED*/
int
Blt_ImageMirror(Tcl_Interp *interp, char *srcImg, char *dstImg, int flip, int flags) {
    Tk_PhotoHandle srcPhoto, destPhoto;
    Blt_ColorImage srcImage, destImage;
    int x, y, x1, x2, y2;
    Tk_PhotoImageBlock src, dest;
    Pix32 *destPtr, *srcPtr;

    srcPhoto = Blt_FindPhoto(interp, srcImg);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "image \"", srcImg, "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    Tk_PhotoGetImage(srcPhoto, &src);
    destPhoto = Blt_FindPhoto(interp, dstImg);
    if (destPhoto == NULL) {
	Tcl_AppendResult(interp, "destination image \"", dstImg, "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }


    if ((flip == MIRROR_TILE || flip == MIRROR_INNER || flip == MIRROR_OUTER ) && srcPhoto == destPhoto) {
        Tcl_AppendResult(interp, "image must be different", (char *)NULL);
        return TCL_ERROR;
    }
    Tk_PhotoGetImage(destPhoto, &dest);
    if (src.width <= 0 || src.height <= 0) {
        Tcl_AppendResult(interp, "src image empty ",  0);
        return TCL_ERROR;
    }

    if (flip == MIRROR_TILE) {
        if ((dest.width != (src.width *2)) || (dest.height <= (src.height*2))) {
            Tk_PhotoSetSize(destPhoto, 2*src.width, 2*src.height);
            Tk_PhotoGetImage(destPhoto, &dest);
        }
    } else if (flip != MIRROR_OUTER && flip != MIRROR_INNER) {
        if ((dest.width != src.width) || (dest.height <= src.height)) {
            Tk_PhotoSetSize(destPhoto, src.width, src.height);
            Tk_PhotoGetImage(destPhoto, &dest);
        }
    }

    srcImage = Blt_PhotoToColorImage(srcPhoto);
    destImage = Blt_PhotoToColorImage(destPhoto);
    destPtr = Blt_ColorImageBits(destImage);
    srcPtr = Blt_ColorImageBits(srcImage);

    if (flip == MIRROR_INNER) {
        /* Center  */
        int xs1, xs2, xd1, xd2, xsc, xdc;
        int ys1, ys2, yd1, yd2, ysc, ydc;
        
        if (src.width < 4 || src.height < 4) {
            Tcl_AppendResult(interp, "src image too small ",  0);
            return TCL_ERROR;
        }
        if (dest.width < 4 || dest.height < 4) {
            Tcl_AppendResult(interp, "dest image too small ",  0);
            return TCL_ERROR;
        }
        xsc = (src.width/2);
        ysc = (src.height/2);
        xdc = (dest.width/2);
        ydc = (dest.height/2);
        for (xs1 = xsc-1, xs2=xs1+1, xd1 = xdc-1, xd2=xd1+1;
             xd1>=0; xd1--, xs1--, xd2++, xs2++) {
            if (xs1<0) { xs1 = xsc-1; xs2 = xs1+1; }
            for (ys1 = ysc-1, ys2 = ys1+1, yd1 = ydc-1, yd2=yd1+1;
                 yd1>=0; yd1--, ys1--, yd2++, ys2++) {
                if (ys1<0) { ys1 = ysc-1; ys2 = ys1+1; }
                destPtr[yd1*dest.width+xd1].value = srcPtr[ys1*src.width+xs1].value;
                if (ys2>=src.height) { ys2--; }
                if (xs2>=src.width) { xs2--; }
                if (yd2<dest.height && xd2<dest.width)
                    destPtr[yd2*dest.width+xd2].value = srcPtr[ys2*src.width+xs2].value;
                if (xd2<dest.width)
                    destPtr[yd1*dest.width+xd2].value = srcPtr[ys1*src.width+xs2].value;
                if (yd2<dest.height)
                    destPtr[yd2*dest.width+xd1].value = srcPtr[ys2*src.width+xs1].value;
                //if (ya<src.height && ya2<dest.height && xa<src.width && xa2<dest.width) {}
            }
        }
    }
    if (flip == MIRROR_X) {
        /* Flip X */
        for (y = 0; y < src.height; y++) {
            for (x = 0, x1 = src.width*y, x2 = src.width*(y+1)-1;
            x < src.width; 
            x++, x1++, x2--) {
                destPtr[x2].value = srcPtr[x1].value;
            }
        }
    }
    if (flip == MIRROR_Y) {
        /* Flip Y */
        for (x = 0; x < src.width; x++) {
            for (y = 0, y2 = src.height-1; y < src.height; y++, y2--) {
                destPtr[y2*dest.width+x].value = srcPtr[y*src.width+x].value;
            }
        }
    }
    if (flip == MIRROR_XY) {
        /* Flip X and Y */
        for (x = 0, x2 = src.width-1; x < src.width; x++, x2--) {
            for (y = 0, y2 = src.height-1; y < src.height; y++, y2--) {
                destPtr[y2*dest.width+x2].value = srcPtr[y*src.width+x].value;
            }
        }
    }

    if (flip == MIRROR_TILE) {
        for (y = 0; y < src.height; y++) {
            for (x = 0; x < src.width; x++) {
                destPtr[y*dest.width+x].value = srcPtr[y*src.width+x].value;
            }
        }
        /* Flip X */
        for (y = 0; y < src.height; y++) {
            for (x = 0, x1 = src.width*y, x2 = 2*src.width*(y+1)-1;
                 x < src.width; 
                 x++, x1++, x2--) {
                destPtr[x2].value = srcPtr[x1].value;
            }
        }
        /* Flip Y */
        for (x = 0; x < src.width; x++) {
            for (y = 0, y2 = 2*src.height-1; y < src.height; y++, y2--) {
                destPtr[y2*dest.width+x].value = srcPtr[y*src.width+x].value;
            }
        }
        /* Flip X and Y */
        for (x = 0, x2 = 2*src.width-1; x < src.width; x++, x2--) {
            for (y = 0, y2 = 2*src.height-1; y < src.height; y++, y2--) {
                destPtr[y2*dest.width+x2].value = srcPtr[y*src.width+x].value;
            }
        }
    }
    
    if (flip == MIRROR_OUTER) {
        int split = (flags&1);
        int sx1, sx2, sy1, sy2,
            smx = (src.width/2), smy=(src.height/2),
            dmx = (dest.width/2), dmy=(dest.height/2);

        /* Initialize whole background to center pixel. */
        int sind = (src.height/2)*src.width+src.width/2;
        for (x = 0; x < dest.width; x++) {
            for (y = 0; y < dest.height; y++) {
                destPtr[y*dest.width+x].value =
                srcPtr[sind].value;
            }
        }

        /* Copy top & bottom pixels. */        
        for (y = 0, y2 = dest.height-1, sy1 = 0, sy2 = src.height-1;
            y < dmy;
            y++, y2--, sy1++, sy2--) {
            if (sy1 >= smy) { if (split) { sy1--; sy2++; } else { sy1 = sy2 = smy; }}
            for (x = 0, x2 = dest.width-1, sx1 = 0, sx2 = src.width-1;
                x < dmx; x++, x2--, sx1++, sx2--) {
                if (sx1 >= smx) { if (split) { sx1--; sx2++; } else { sx1 = sx2 = smx; }}
                /* Top - left & right */
                destPtr[y*dest.width+x].value = srcPtr[sy1*src.width+sx1].value;
                destPtr[y*dest.width+x2].value = srcPtr[sy1*src.width+sx2].value;
                /* Bottom - left & right */
                destPtr[y2*dest.width+x].value = srcPtr[sy2*src.width+sx1].value;
                destPtr[y2*dest.width+x2].value = srcPtr[sy2*src.width+sx2].value;
            }
        }
        /* Copy left & right pixels. */
        for (x = 0, x2 = dest.width-1, sx1 = 0, sx2 = src.width-1;
            x < smx && x < dmx;
            x++, x2--, sx1++, sx2--) {
            if (sx1 >= smx) { if (split) { sx1--; sx2++; } else { sx1 = sx2 = smx; }}
            for (y = 0, y2 = dest.height-1, sy1 = 0, sy2 = src.height-1;
                y < dmy; y++, y2--, sy1++, sy2--) {
                if (sy1 >= smy) { if (split) { sy1--; sy2++; } else { sy1 = sy2 = smy; }}
                /* Left - top & bottom */
                destPtr[y*dest.width+x].value = srcPtr[sy1*src.width+sx1].value;
                destPtr[y*dest.width+x2].value = srcPtr[sy1*src.width+sx2].value;
                /* Right - top & bottom */
                destPtr[y2*dest.width+x].value = srcPtr[sy2*src.width+sx1].value;
                destPtr[y2*dest.width+x2].value = srcPtr[sy2*src.width+sx2].value;
            }
        }
    }

    Blt_ColorImageToPhoto(destImage, destPhoto);
    Blt_FreeColorImage(srcImage);
    Blt_FreeColorImage(destImage);
    return TCL_OK;
}


/*ARGSUSED*/
static int
MirrorOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_PhotoHandle srcPhoto, destPhoto;
    int flip, halo;

    flip = 3;
    halo = 0;
    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    destPhoto = Blt_FindPhoto(interp, argv[3]);
    if (destPhoto == NULL) {
	Tcl_AppendResult(interp, "destination image \"", argv[3], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    if (argc>4) {
        if (!strcmp(argv[4],"x")) {
            flip = MIRROR_X; /* 1 */
        } else if (!strcmp(argv[4],"y")) {
            flip = MIRROR_Y; /* 2 */
        } else if (!strcmp(argv[4],"xy")) {
            flip = MIRROR_XY; /* 3 */
        } else if (!strcmp(argv[4],"tile")) {
            flip = MIRROR_TILE; /* 4 */
        } else if (!strcmp(argv[4],"outer")) {
            flip = MIRROR_OUTER; /* 5 */
        } else if (!strcmp(argv[4],"inner")) {
            flip = MIRROR_INNER; /* 6 */
        } else {
            Tcl_AppendResult(interp, "direction ", argv[4],
                " must be \"x\", \"y\", \"xy\",\"tile\", \"inner\", or  \"outer\"", (char *)NULL);
            return TCL_ERROR;
        }
    }
    if (argc>5) {
        if (flip != MIRROR_OUTER) {
            Tcl_AppendResult(interp, "halo is for outer only", 0);
            return TCL_ERROR;
        }
        if (Tcl_GetInt(interp, argv[5], &halo) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return Blt_ImageMirror(interp, argv[2], argv[3], flip, halo);
}
    

/*
 * --------------------------------------------------------------------------
 *
 * SnapOp --
 *
 *	Snaps a picture of a window and stores it in a designated
 *	photo image.  The window must be completely visible or
 *	the snap will fail.
 *
 * Results:
 *	Returns a standard Tcl result.  interp->result contains
 *	the list of the graph coordinates. If an error occurred
 *	while parsing the window positions, TCL_ERROR is returned,
 *	then interp->result will contain an error message.
 *
 * -------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SnapOp(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_Window tkwin;
    int width, height, destWidth, destHeight;
    Window window;

    tkwin = Tk_MainWindow(interp);
    window = StringToWindow(interp, argv[2]);
    if (window == None) {
	return TCL_ERROR;
    }
    if (GetWindowSize(interp, window, &width, &height) != TCL_OK) {
	Tcl_AppendResult(interp, "can't get window geometry of \"", argv[2],
		 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    destWidth = width, destHeight = height;
    if ((argc > 4) && (Blt_GetPixels(interp, tkwin, argv[4], PIXELS_POSITIVE,
		&destWidth) != TCL_OK)) {
	return TCL_ERROR;
    }
    if ((argc > 5) && (Blt_GetPixels(interp, tkwin, argv[5], PIXELS_POSITIVE,
		&destHeight) != TCL_OK)) {
	return TCL_ERROR;
    }
    return Blt_SnapPhoto(interp, tkwin, window, 0, 0, width, height, destWidth,
	destHeight, argv[3], GAMMA);
}

/*ARGSUSED*/
static int
SubsampleOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window of the interpreter. */
    Tcl_Interp *interp;
    int argc;			/* Not used. */
    char **argv;
{
    Tk_Window tkwin;
    Tk_PhotoHandle srcPhoto, destPhoto;
    Tk_PhotoImageBlock src, dest;
    ResampleFilter *filterPtr, *vertFilterPtr, *horzFilterPtr;
    char *filterName;
    int flag;
    int x, y;
    int width, height;

    srcPhoto = Blt_FindPhoto(interp, argv[2]);
    if (srcPhoto == NULL) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    destPhoto = Blt_FindPhoto(interp, argv[3]);
    if (destPhoto == NULL) {
	Tcl_AppendResult(interp, "destination image \"", argv[3], "\" doesn't",
	    " exist or is not a photo image", (char *)NULL);
	return TCL_ERROR;
    }
    tkwin = (Tk_Window)clientData;
    flag = PIXELS_NONNEGATIVE;
    if (Blt_GetPixels(interp, tkwin, argv[4], flag, &x) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_GetPixels(interp, tkwin, argv[5], flag, &y) != TCL_OK) {
	return TCL_ERROR;
    }
    flag = PIXELS_POSITIVE;
    if (Blt_GetPixels(interp, tkwin, argv[6], flag, &width) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_GetPixels(interp, tkwin, argv[7], flag, &height) != TCL_OK) {
	return TCL_ERROR;
    }
    filterName = (argc > 8) ? argv[8] : "none";
    if (Blt_GetResampleFilter(interp, filterName, &filterPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    vertFilterPtr = horzFilterPtr = filterPtr;
    if ((filterPtr != NULL) && (argc > 9)) {
	if (Blt_GetResampleFilter(interp, argv[9], &filterPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	vertFilterPtr = filterPtr;
    }
    Tk_PhotoGetImage(srcPhoto, &src);
    Tk_PhotoGetImage(destPhoto, &dest);
    if ((src.width <= 1) || (src.height <= 1)) {
	Tcl_AppendResult(interp, "source image \"", argv[2], "\" is empty",
	    (char *)NULL);
	return TCL_ERROR;
    }
    if (((x + width) > src.width) || ((y + height) > src.height)) {
	Tcl_AppendResult(interp, "nonsensical dimensions for subregion: x=",
	    argv[4], " y=", argv[5], " width=", argv[6], " height=",
	    argv[7], (char *)NULL);
	return TCL_ERROR;
    }
    if ((dest.width <= 1) || (dest.height <= 1)) {
	Tk_PhotoSetSize(destPhoto, width, height);
    }
    if (filterPtr == NULL) {
	Blt_ResizePhoto(srcPhoto, x, y, width, height, destPhoto);
    } else {
	Blt_ResamplePhoto(srcPhoto, x, y, width, height, destPhoto, 
		horzFilterPtr, vertFilterPtr);
    }
    return TCL_OK;
}

static Blt_OpSpec imageOps[] =
{
    {"alpha", 2, (Blt_Op)AlphaOp, 6, 10, "?-shift? srcPhoto destPhoto Color ?alpha? ?withalpha?",},
    {"blur", 2, (Blt_Op)BlurOp, 5, 6, "srcPhoto destPhoto ?radius?",},
    {"colors", 3, (Blt_Op)ColorsOp, 4, 0, "?-alpha? ?-count? ?-from x1 y1 x2 y2? srcPhoto",},
    {"convolve", 3, (Blt_Op)ConvolveOp, 6, 6, "srcPhoto destPhoto filter",},
    {"gradient", 1, (Blt_Op)GradientOp, 6, 0, "photo left right ?-type lines|normal|radial|rectangular|linear|sine|halfsine? ?-skew val? ?-slant val? ?-rand val? ?-mathval val? ?-mathfunc circle|sin|cos|tan|asin|acos|atan|sinh|cosh|tanh|exp|sqrt|log|log10|rand?",},
    {"merge", 2, (Blt_Op)MergeOp, 6, 7, "srcPhoto1 srcPhoto2 destPhoto ?opacity? ?opacity2?",},
    {"mirror", 3, (Blt_Op)MirrorOp, 5, 7, "srcPhoto destPhoto ?x|y|xy|tile|outer|innert? ?halo?",},
    {"quantize", 2, (Blt_Op)QuantizeOp, 4, 5, "srcPhoto destPhoto ?nColors?",},
    {"readjpeg", 3, (Blt_Op)ReadJPEGOp, 5, 5, "fileName photoName",},
    {"recolor", 3, (Blt_Op)RecolorOp, 7, 8, "srcPhoto destPhoto oldColor newColor ?alpha?",},
    {"resample", 3, (Blt_Op)ResampleOp, 5, 7, 
	"srcPhoto destPhoto ?horzFilter vertFilter?",},
    {"rotate", 2, (Blt_Op)RotateOp, 6, 6, "srcPhoto destPhoto angle",},
    {"subsample", 2, (Blt_Op)SubsampleOp, 9, 11,
	"srcPhoto destPhoto x y width height ?horzFilter? ?vertFilter?",},
    {"trans", 2, (Blt_Op)TransOp, 6, 7, "image x y ?alpha?",},
};

static int nImageOps = sizeof(imageOps) / sizeof(Blt_OpSpec);

/* ARGSUSED */
static int
ImageOp(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window of interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOp(interp, nImageOps, imageOps, BLT_OP_ARG2, argc, argv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    clientData = (ClientData)Tk_MainWindow(interp);
    result = (*proc) (clientData, interp, argc - 1, argv + 1);
    return result;
}

static Blt_OpSpec winOps[] =
{
    {"changes", 3, (Blt_Op)ChangesOp, 3, 3, "window",},
#ifndef WIN32
    {"colormap", 3, (Blt_Op)ColormapOp, 3, 3, "window",},
#endif
    {"gradients", 1, (Blt_Op)GradientsOp, 5, 5, "color1 color2 width",},
    {"image", 1, (Blt_Op)ImageOp, 2, 0, "args",},
    {"lower", 1, (Blt_Op)LowerOp, 2, 0, "window ?window?...",},
    {"map", 2, (Blt_Op)MapOp, 2, 0, "window ?window?...",},
    {"move", 2, (Blt_Op)MoveOp, 5, 5, "window x y",},
    {"parent", 3, (Blt_Op)ParentOp, 3, 3, "window",},
    {"query", 3, (Blt_Op)QueryOp, 2, 2, "",},
    {"raise", 2, (Blt_Op)RaiseOp, 2, 0, "window ?window?...",},
#ifdef notdef
    {"reparent", 3, (Blt_Op)ReparentOp, 3, 4, "window ?parent?",},
#endif
    {"snap", 2, (Blt_Op)SnapOp, 4, 8, "window photoName ?width height?",},
    {"unmap", 1, (Blt_Op)UnmapOp, 2, 0, "window ?window?...",},
    {"warpto", 1, (Blt_Op)WarpToOp, 2, 5, "?window?",},
};

static int nWinOps = sizeof(winOps) / sizeof(Blt_OpSpec);

/* ARGSUSED */
static int
WinopCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window of interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOp(interp, nWinOps, winOps, BLT_OP_ARG1,  argc, argv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    clientData = (ClientData)Tk_MainWindow(interp);
    result = (*proc) (clientData, interp, argc, argv);
    return result;
}

int
Blt_WinopInit(interp)
    Tcl_Interp *interp;
{
    static Blt_CmdSpec cmdSpec = {"winop", WinopCmd,};

    if (Blt_InitCmd(interp, "blt", &cmdSpec) == NULL) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

#endif /* NO_WINOP */
