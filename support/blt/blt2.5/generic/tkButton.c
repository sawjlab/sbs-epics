/*
 * tkButton.c --
 *
 *	This module implements a collection of button-like
 *	widgets for the Tk toolkit.  The widgets implemented
 *	include labels, buttons, check buttons, and radio
 *	buttons.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tkButton.c 1.128 96/03/01 17:34:49
 */

#include "bltInt.h"
#include "bltTree.h"

#ifndef NO_TILEBUTTON

#define INDICATOR_WIDTH		28
#define INDICATOR_HEIGHT	17


#include "bltTile.h"
#include "bltImage.h"

extern Tk_CustomOption bltTileOption;
/*
 * The definitions below provide symbolic names for the default colors.
 * NORMAL_BG -		Normal background color.
 * ACTIVE_BG -		Background color when widget is active.
 * SELECT_BG -		Background color for selected text.
 * TROUGH -		Background color for troughs in scales and scrollbars.
 * INDICATOR -		Color for indicator when button is selected.
 * DISABLED -		Foreground color when widget is disabled.
 */

#define NORMAL_BG	"#d9d9d9"
#define ACTIVE_BG	"#d9d9d9"
#define SELECT_BG	"#c3c3c3"
#define TROUGH		"#c3c3c3"
#define INDICATOR	"#b03060"
#define DISABLED	"#a3a3a3"

static Tk_Uid tkNormalUid, tkActiveUid, tkDisabledUid;

#define DEF_BUTTON_ANCHOR		"center"
#define DEF_BUTTON_ACTIVE_BACKGROUND	"#ececec"
#define DEF_BUTTON_ACTIVE_BG_MONO	RGB_BLACK
#define DEF_BUTTON_ACTIVE_FOREGROUND	RGB_BLACK
#define DEF_BUTTON_ACTIVE_FG_MONO	RGB_WHITE
#define DEF_BUTTON_BACKGROUND		STD_NORMAL_BACKGROUND
#define DEF_BUTTON_BG_MONO		RGB_WHITE
#define DEF_BUTTON_BITMAP		""
#define DEF_BUTTON_BORDERWIDTH		"1"
#define DEF_BUTTON_CURSOR		""
#define DEF_BUTTON_COMMAND		""
#define DEF_BUTTON_COMPOUND		"none"
#define DEF_DIRECTION_DEFAULT "below"
#define DEF_BUTTON_DEFAULT              "disabled"
#define DEF_BUTTON_DISABLED_FOREGROUND	STD_DISABLE_FOREGROUND
#define DEF_BUTTON_DISABLED_FG_MONO	""
#define DEF_BUTTON_FG			RGB_BLACK
#define DEF_BUTTON_FONT			"TkDefaultFont"
#define DEF_BUTTON_HEIGHT		"0"
#define DEF_BUTTON_HIGHLIGHT_BG		STD_NORMAL_BACKGROUND
#define DEF_BUTTON_HIGHLIGHT		RGB_BLACK
#define DEF_LABEL_HIGHLIGHT_WIDTH	"0"
#define DEF_BUTTON_HIGHLIGHT_WIDTH	"1"
#define DEF_BUTTON_IMAGE		(char *) NULL
#define DEF_BUTTON_INDICATOR		"1"
#define DEF_BUTTON_INDICATORSIZE		"10"
#define DEF_BUTTON_JUSTIFY		"center"
#define DEF_BUTTON_OFF_VALUE		"0"
#define DEF_BUTTON_ON_VALUE		"1"
#define DEF_BUTTON_OVER_RELIEF		"raised"
#define DEF_BUTTON_PADX			"3m"
#define DEF_LABCHKRAD_PADX		"1"
#define DEF_BUTTON_PADY			"1m"
#define DEF_LABCHKRAD_PADY		"1"
#define DEF_BUTTON_RELIEF		"raised"
#define DEF_BUTTON_REPEAT_DELAY		"0"
#define DEF_LABCHKRAD_RELIEF		"flat"
#define DEF_BUTTON_SELECT_COLOR		STD_INDICATOR_COLOR
#define DEF_BUTTON_SELECT_MONO		RGB_BLACK
#define DEF_BUTTON_SELECT_IMAGE		(char *) NULL
#define DEF_BUTTON_STATE		"normal"
#define DEF_LABEL_TAKE_FOCUS		"0"
#define DEF_BUTTON_TAKE_FOCUS		""
#define DEF_BUTTON_TEXT			""
#define DEF_BUTTON_TEXT_VARIABLE	""
#define DEF_BUTTON_UNDERLINE		"-1"
#define DEF_BUTTON_VALUE		""
#define DEF_BUTTON_WIDTH		"0"
#define DEF_BUTTON_WRAP_LENGTH		"0"
#define DEF_RADIOBUTTON_VARIABLE	"selectedButton"
#define DEF_CHECKBUTTON_VARIABLE	""
#define DEF_SHADOW		RGB_BLACK
#define DEF_ROTATE		"0.0"

/*
 * A data structure of the following type is kept for each
 * widget managed by this file:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the button.  NULL
				 * means that the window has been destroyed. */
    Display *display;		/* Display containing widget.  Needed to
				 * free up resources after tkwin is gone. */
    Tcl_Interp *interp;		/* Interpreter associated with button. */
    Tcl_Command widgetCmd;	/* Token for button's widget command. */
    int type;			/* Type of widget:  restricts operations
				 * that may be performed on widget.  See
				 * below for possible values. */

    /*
     * Information about what's in the button.
     */

    char *textPtr;			/* Text to display in button (malloc'ed)
				 * or NULL. */
    int numChars;		/* # of characters in text. */
    int underline;		/* Index of character to underline.  < 0 means
				 * don't underline anything. */
    char *textVarName;		/* Name of variable (malloc'ed) or NULL.
				 * If non-NULL, button displays the contents
				 * of this variable. */
    Pixmap bitmap;		/* Bitmap to display or None.  If not None
				 * then text and textVar are ignored. */
    char *imageString;		/* Name of image to display (malloc'ed), or
				 * NULL.  If non-NULL, bitmap, text, and
				 * textVarName are ignored. */
    char *activeImageString;
    char *disabledImageString;
    Tk_Image image;		/* Image to display in window, or NULL if
				 * none. */
    Tk_Image activeImage;
    Tk_Image disabledImage;
    Tk_Image tristateImage;
    char * tristateImageString;
    char *selectImageString;	/* Name of image to display when selected
				 * (malloc'ed), or NULL. */
    Tk_Image selectImage;	/* Image to display in window when selected,
				 * or NULL if none.  Ignored if image is
				 * NULL. */

    /*
     * Information used when displaying widget:
     */

    int state;		/* State of button for display purposes:
				 * normal, active, or disabled. */
    Tk_3DBorder normalBorder;	/* Structure used to draw 3-D
				 * border and background when window
				 * isn't active.  NULL means no such
				 * border exists. */
    Tk_3DBorder activeBorder;	/* Structure used to draw 3-D
				 * border and background when window
				 * is active.  NULL means no such
				 * border exists. */
    int borderWidth;		/* Width of border. */
    int relief;			/* 3-d effect: TK_RELIEF_RAISED, etc. */
    int overRelief;		/* Value of -overrelief option: specifies a 3-d
				 * effect for the border, such as
				 * TK_RELIEF_RAISED, to be used when the mouse
				 * is over the button. */
    int offRelief;
    int highlightWidth;		/* Width in pixels of highlight to draw
				 * around widget when it has the focus.
				 * <= 0 means don't draw a highlight. */
    XColor *highlightBgColorPtr;
    /* Color for drawing traversal highlight
				 * area when highlight is off. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    int inset;			/* Total width of all borders, including
				 * traversal highlight and 3-D border.
				 * Indicates how much interior stuff must
				 * be offset from outside edges to leave
				 * room for borders. */
    Tk_Font tkfont;		/* Information about text font, or NULL. */
    XColor *normalFg;		/* Foreground color in normal mode. */
    XColor *activeFg;		/* Foreground color in active mode.  NULL
				 * means use normalFg instead. */
    XColor *disabledFg;		/* Foreground color when disabled.  NULL
				 * means use normalFg with a 50% stipple
				 * instead. */
    GC normalTextGC;		/* GC for drawing text in normal mode.  Also
				 * used to copy from off-screen pixmap onto
				 * screen. */
    GC activeTextGC;		/* GC for drawing text in active mode (NULL
				 * means use normalTextGC). */
    Pixmap gray; 		/* Pixmap for displaying disabled text if
				 * disabledFg is NULL. */
    GC disabledGC;		/* Used to produce disabled effect.  If
				 * disabledFg isn't NULL, this GC is used to
				 * draw button text or icon.  Otherwise
				 * text or icon is drawn with normalGC and
				 * this GC is used to stipple background
				 * across it.  For labels this is None. */
    GC copyGC;			/* Used for copying information from an
				 * off-screen pixmap to the screen. */
    char *widthString;		/* Value of -width option.  Malloc'ed. */
    char *heightString;		/* Value of -height option.  Malloc'ed. */
    int width, height;		/* If > 0, these specify dimensions to request
				 * for window, in characters for text and in
				 * pixels for bitmaps.  In this case the actual
				 * size of the text string or bitmap is
				 * ignored in computing desired window size. */
    int wrapLength;		/* Line length (in pixels) at which to wrap
				 * onto next line.  <= 0 means don't wrap
				 * except at newlines. */
    int padX, padY;		/* Extra space around text (pixels to leave
				 * on each side).  Ignored for bitmaps and
				 * images. */
    Tk_Anchor anchor;		/* Where text/bitmap should be displayed
				 * inside button region. */
    Tk_Justify justify;		/* Justification to use for multi-line text. */
    int indicatorOn;		/* True means draw indicator, false means
				 * don't draw it. */
    Tk_3DBorder selectBorder;	/* For drawing indicator background, or perhaps
				 * widget background, when selected. */
    int textWidth;		/* Width needed to display text as requested,
				 * in pixels. */
    int textHeight;		/* Height needed to display text as requested,
				 * in pixels. */
#if (TK_MAJOR_VERSION > 4)
    Tk_TextLayout textLayout;	/* Saved text layout information. */
#endif
    int indicatorSpace;		/* Horizontal space (in pixels) allocated for
				 * display of indicator. */
    int indicatorDiameter;	/* Diameter of indicator, in pixels. */
    int indicatorSize;    /* Fixed the size of indicator. */

    int defaultState;	/* Used in 8.0 (not here)  */

    /*
     * For check and radio buttons, the fields below are used
     * to manage the variable indicating the button's state.
     */

    char *selVarName;		/* Name of variable used to control selected
				 * state of button.  Malloc'ed (if
				 * not NULL). */
    char *onValue;		/* Value to store in variable when
				 * this button is selected.  Malloc'ed (if
				 * not NULL). */
    char *offValue;		/* Value to store in variable when this
				 * button isn't selected.  Malloc'ed
				 * (if not NULL).  Valid only for check
				 * buttons. */

    /*
     * Miscellaneous information:
     */

    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    char *takeFocus;		/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */
    char *command;		/* Command to execute when button is
				 * invoked; valid for buttons only.
				 * If not NULL, it's malloc-ed. */
    int compound;		/* Value of -compound option; specifies whether
				 * the button should show both an image and
				 * text, and, if so, how. */
    int repeatDelay;		/* Value of -repeatdelay option; specifies
				 * the number of ms after which the button will
				 * start to auto-repeat its command. */
    int repeatInterval;		/* Value of -repeatinterval option; specifies
				 * the number of ms between auto-repeat
				 * invocataions of the button command. */
    int flags;			/* Various flags;  see below for
				 * definitions. */
    Blt_Tile innerTile, disabledTile;
    Blt_Tile tile, activeTile, activeInnerTile, disabledInnerTile;
    Shadow shadow;
    Shadow activeShadow;
    double rotate;
    GC focusGC;
    char *tristateValue;
    Tk_Image icons[9];
    char *iconStr;
    int iconCnt;
    Tk_3DBorder innerBorder;
    char *bdImageString;		/* Name of image for border. */
    char *activeBdImageString;		/* Name of image for border. */
    char *disabledBdImageString;		/* Name of image for border. */
    Tk_Image bdImage;
    Tk_Image activeBdImage;
    Tk_Image disabledBdImage;
    Tk_Image bdImageSized;
    int bdHalo;
    Blt_Tree tree;
    int node;
    Blt_TreeTrace textTrace;
    Blt_TreeTrace selTrace;
    Blt_Tile inBdTile;
    int xOffset, yOffset;
    XColor *bdMaskColor;		/* Mask color for bdImage */
    char *menuName;
    int direction;
    int indicatorWidth;
    int indicatorHeight;
    Blt_Dashes dashes;
    int bgTileTop;
    char *classString;
} Button;

/*
 * Possible "type" values for buttons.  These are the kinds of
 * widgets supported by this file.  The ordering of the type
 * numbers is significant:  greater means more features and is
 * used in the code.
 */

#define TYPE_FRAME		0
#define TYPE_LABEL		1
#define TYPE_BUTTON		2
#define TYPE_CHECK_BUTTON	3
#define TYPE_RADIO_BUTTON	4
#define TYPE_MENU_BUTTON	5
#define CHOOSE(default, override)	\
	(((override) == NULL) ? (default) : (override))

extern Tk_CustomOption bltShadowOption;
extern Tk_CustomOption bltGradientOption;

/* static char *defaultStrings[] = {
    "active", "disabled", "normal", NULL
}; */

/* enum { STATE_NORMAL, STATE_ACTIVE, STATE_DISABLED } StateEnum; */
static char *stateStrings[] = {
    "normal", "active", "disabled", NULL
};

static char *directionStrings[] = {
    "above", "below", "flush", "left", "right", NULL
};

enum { COMPOUND_NONE, COMPOUND_BOTTOM, COMPOUND_CENTER, COMPOUND_LEFT, COMPOUND_RIGHT, COMPOUND_TOP } CompoundEnum;
static char *compoundStrings[] = {
    "none", "bottom", "center", "left", "right", "top", NULL
};

static int
StringToName( ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
    char *string, char *widgRec, int offset, char *strList[], char *label )
{
    int *namePtr = (int *)(widgRec + offset);
    char c;
    int i;
    
    for (i=0; strList[i]; i++) {
        c = string[0];
        if (strcmp(string, strList[i]) == 0) {
            *namePtr = i;
            return TCL_OK;
        }
    }
     Tcl_AppendResult(interp, "bad ", label, " \"", string,
	    "\": should be one of: ", (char *)NULL);
    for (i=0; strList[i]; i++) {
        if (i>0) Tcl_AppendResult(interp, ", ", (char *)NULL);
 	Tcl_AppendResult(interp, strList[i], (char *)NULL);
    }
    return TCL_ERROR;
}

static
char * NameToString( ClientData clientData, Tk_Window parent,
    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr,
    char *strList[], char *msg )
{
    int value = *(int *)(widgRec + offset);
    int i;
    
    for (i=0; strList[i]; i++) ;
    if (value < i) {
        return strList[value];
    }
    return msg;
}

static int
StringToState( ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
    char *string, char *widgRec, int offset)
{
    return StringToName( clientData, interp, parent,
        string, widgRec, offset, stateStrings, "state" );
}

static char *
StateToString( ClientData clientData, Tk_Window parent,
    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr)
{
    return NameToString(clientData, parent,
        widgRec, offset, freeProcPtr, stateStrings, "unknown state value");
}

static int
StringToDirection( ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
    char *string, char *widgRec, int offset)
{
    return StringToName( clientData, interp, parent,
        string, widgRec, offset, directionStrings, "direction" );
}

static char *
DirectionToString( ClientData clientData, Tk_Window parent,
    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr)
{
    return NameToString(clientData, parent,
        widgRec, offset, freeProcPtr, directionStrings, "unknown direction value");
}


static Tk_CustomOption directionOption =
{
    StringToDirection, DirectionToString, (ClientData)0,
};

static Tk_CustomOption stateOption =
{
    StringToState, StateToString, (ClientData)0,
};

static int
StringToCompound( ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
    char *string, char *widgRec, int offset)
{
    return StringToName( clientData, interp, parent,
        string, widgRec, offset, compoundStrings, "compound" );
}

static char *
CompoundToString( ClientData clientData, Tk_Window parent,
    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr)
{
    return NameToString(clientData, parent,
    widgRec, offset, freeProcPtr, compoundStrings, "unknown compound value");
}


static Tk_CustomOption compoundOption =
{
    StringToCompound, CompoundToString, (ClientData)0,
};


static char *
IconsToString( ClientData clientData, Tk_Window parent,
    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr)
{
    char *imgStr = *(char **)(widgRec + offset);
    return imgStr;
}

static int
StringToIcons( ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
    char *string, char *widgRec, int offset);

static char *
TreeToString( ClientData clientData, Tk_Window parent,
    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr)
{
    Blt_Tree *treePtr = (Blt_Tree *)(widgRec + offset);
    if (*treePtr == NULL) {
        return NULL;
    }
    return Blt_TreeName(*treePtr);
}

static int
StringToTree( ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
    char *string, char *widgRec, int offset)
{
    Blt_Tree *treePtr = (Blt_Tree *)(widgRec + offset);
    if (string[0] == 0) {
        if (*treePtr) Blt_TreeReleaseToken(*treePtr);
        *treePtr = NULL;
        return TCL_OK;
    }
    return Blt_TreeGetToken(interp, string, treePtr);
}


/* WARNING: hardcoded to butPtr->icons. */
static Tk_CustomOption iconsOption =
{
    StringToIcons, IconsToString, (ClientData)0,
};

Tk_CustomOption bltTreeOption =
{
    StringToTree, TreeToString, (ClientData)0,
};


/*
 * Class names for buttons, indexed by one of the type values above.
 */

#define BBOFS 6

static char *classNames[] =
{"Frame", "Label", "Button", "Checkbutton", "Radiobutton", "Menubutton",
 "BFrame", "BLabel", "BButton", "BCheckbutton", "BRadiobutton", "BMenubutton"
};

static void widgetWorldChanged(ClientData clientData);

static Tk_ClassProcs butClass = {
    sizeof(Tk_ClassProcs),	/* size */
    widgetWorldChanged,		/* worldChangedProc */
};

/*
 * Flag bits for buttons:
 *
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler
 *				has already been queued to redraw
 *				this window.
 * SELECTED:			Non-zero means this button is selected,
 *				so special highlight should be drawn.
 * GOT_FOCUS:			Non-zero means this button currently
 *				has the input focus.
 */

#define REDRAW_PENDING          (1 << 0)
#define SELECTED                (1 << 1)
#define GOT_FOCUS               (1 << 2)
#define BUTTON_DELETED          (1 << 3)
#define TRISTATED               (1 << 4)
#define HAS_ICONS               (1 << 5)
#define BUTTON_LAYOUT           (1 << 6)
#define BUTTON_DIRTYBD          (1 << 7)


/*
 * Mask values used to selectively enable entries in the
 * configuration specs:
 */

#define FRAME_MASK		TK_CONFIG_USER_BIT
#define LABEL_MASK		TK_CONFIG_USER_BIT << 1
#define BUTTON_MASK		TK_CONFIG_USER_BIT << 2
#define CHECK_BUTTON_MASK	TK_CONFIG_USER_BIT << 3
#define RADIO_BUTTON_MASK	TK_CONFIG_USER_BIT << 4
#define MENU_BUTTON_MASK	TK_CONFIG_USER_BIT << 5
#define NALL_MASK		(LABEL_MASK | BUTTON_MASK \
	| CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | MENU_BUTTON_MASK)
#define ALL_MASK		(FRAME_MASK | NALL_MASK )
#define NONLABEL_MASK		(BUTTON_MASK \
         | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | MENU_BUTTON_MASK)

static int configFlags[] =
{FRAME_MASK, LABEL_MASK, BUTTON_MASK,
    CHECK_BUTTON_MASK, RADIO_BUTTON_MASK, MENU_BUTTON_MASK};

extern Tk_CustomOption bltDashesOption;

/*
 * Information used for parsing configuration specs:
 */

static Tk_ConfigSpec configSpecs[] =
{
    {TK_CONFIG_BORDER, "-activebackground", "activeBackground", "Foreground",
	DEF_BUTTON_ACTIVE_BACKGROUND, Tk_Offset(Button, activeBorder),
	ALL_MASK | TK_CONFIG_COLOR_ONLY | TK_CONFIG_NULL_OK},
    {TK_CONFIG_BORDER, "-activebackground", "activeBackground", "Foreground",
	DEF_BUTTON_ACTIVE_BG_MONO, Tk_Offset(Button, activeBorder),
	ALL_MASK | TK_CONFIG_MONO_ONLY  | TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-activeforeground", "activeForeground", "Background",
	DEF_BUTTON_ACTIVE_FOREGROUND, Tk_Offset(Button, activeFg),
	ALL_MASK | TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_COLOR, "-activeforeground", "activeForeground", "Background",
	DEF_BUTTON_ACTIVE_FG_MONO, Tk_Offset(Button, activeFg),
	ALL_MASK | TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_CUSTOM, "-activeshadow", "activeShadow", "ActiveShadow",
	NULL, Tk_Offset(Button, activeShadow),
        ALL_MASK | TK_CONFIG_NULL_OK, &bltShadowOption},
    {TK_CONFIG_CUSTOM, "-activetile", "activeTile", "Tile",
	(char *)NULL, Tk_Offset(Button, activeTile),
	ALL_MASK | TK_CONFIG_NULL_OK, &bltTileOption},
    {TK_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",
	DEF_BUTTON_ANCHOR, Tk_Offset(Button, anchor), NALL_MASK},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_BUTTON_BACKGROUND, Tk_Offset(Button, normalBorder),
	ALL_MASK | TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_BUTTON_BG_MONO, Tk_Offset(Button, normalBorder),
	ALL_MASK | TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL,
	(char *)NULL, 0, ALL_MASK},
    {TK_CONFIG_SYNONYM, "-bg", "background", (char *)NULL,
	(char *)NULL, 0, ALL_MASK},
   {TK_CONFIG_INT, "-bdflags", "bdFlags", "BdFlags",
	"0", Tk_Offset(Button, bdHalo),
	ALL_MASK | TK_CONFIG_NULL_OK},
   {TK_CONFIG_STRING, "-bdimage", "bdImage", "Image",
	DEF_BUTTON_IMAGE, Tk_Offset(Button, bdImageString),
	ALL_MASK | TK_CONFIG_NULL_OK},
   {TK_CONFIG_STRING, "-activebdimage", "activeBdImage", "Image",
	DEF_BUTTON_IMAGE, Tk_Offset(Button, activeBdImageString),
	ALL_MASK | TK_CONFIG_NULL_OK},
   {TK_CONFIG_STRING, "-class", "class", "Class",
	NULL, Tk_Offset(Button, classString),
	FRAME_MASK | TK_CONFIG_NULL_OK},
   {TK_CONFIG_CUSTOM, "-dashes", "dashes", "Dashes",
	"dot", Tk_Offset(Button, dashes),
	TK_CONFIG_NULL_OK | NONLABEL_MASK, &bltDashesOption},
   {TK_CONFIG_STRING, "-disabledbdimage", "disabledBdImage", "Image",
	DEF_BUTTON_IMAGE, Tk_Offset(Button, disabledBdImageString),
	ALL_MASK | TK_CONFIG_NULL_OK},
    /*{TK_CONFIG_COLOR, "-bdmaskcolor", "bdMaskColor", "BdMaskColor",
	(char *)NULL, Tk_Offset(Button, bdMaskColor),
	ALL_MASK| TK_CONFIG_NULL_OK}, */
    {TK_CONFIG_BITMAP, "-bitmap", "bitmap", "Bitmap",
	DEF_BUTTON_BITMAP, Tk_Offset(Button, bitmap),
	NALL_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BUTTON_BORDERWIDTH, Tk_Offset(Button, borderWidth), ALL_MASK},
    {TK_CONFIG_STRING, "-command", "command", "Command",
	DEF_BUTTON_COMMAND, Tk_Offset(Button, command),
	BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-compound", "compound", "Compound",
	DEF_BUTTON_COMPOUND, Tk_Offset(Button, compound), 
	ALL_MASK | TK_CONFIG_NULL_OK, &compoundOption},
    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_BUTTON_CURSOR, Tk_Offset(Button, cursor),
	ALL_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-default", "default", "Default",
	DEF_BUTTON_DEFAULT, Tk_Offset(Button, defaultState), BUTTON_MASK, &stateOption},
    {TK_CONFIG_CUSTOM, "-direction", "direction", "Direction",
	DEF_DIRECTION_DEFAULT, Tk_Offset(Button, direction), MENU_BUTTON_MASK, &directionOption},
    {TK_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
	"DisabledForeground", DEF_BUTTON_DISABLED_FOREGROUND,
	Tk_Offset(Button, disabledFg), ALL_MASK | TK_CONFIG_COLOR_ONLY | TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
	"DisabledForeground", DEF_BUTTON_DISABLED_FG_MONO,
	Tk_Offset(Button, disabledFg), ALL_MASK | TK_CONFIG_MONO_ONLY | TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-disabledtile", "disabledTile", "Tile",
	(char *)NULL, Tk_Offset(Button, disabledTile),
	ALL_MASK | TK_CONFIG_NULL_OK, &bltTileOption},
    {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL,
	(char *)NULL, 0, NALL_MASK},
    {TK_CONFIG_FONT, "-font", "font", "Font",
	DEF_BUTTON_FONT, Tk_Offset(Button, tkfont),
	NALL_MASK},
    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	DEF_BUTTON_FG, Tk_Offset(Button, normalFg), NALL_MASK},
    {TK_CONFIG_STRING, "-height", "height", "Height",
	DEF_BUTTON_HEIGHT, Tk_Offset(Button, heightString), ALL_MASK},
    {TK_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_BUTTON_HIGHLIGHT_BG,
	Tk_Offset(Button, highlightBgColorPtr), NALL_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_BUTTON_HIGHLIGHT, Tk_Offset(Button, highlightColorPtr),
	NALL_MASK},
    {TK_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness",
	DEF_LABEL_HIGHLIGHT_WIDTH, Tk_Offset(Button, highlightWidth),
	LABEL_MASK| MENU_BUTTON_MASK},
    {TK_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness",
	DEF_BUTTON_HIGHLIGHT_WIDTH, Tk_Offset(Button, highlightWidth),
	BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK  },
    {TK_CONFIG_CUSTOM, "-icons", "icons", "icons",
	NULL, Tk_Offset(Button, iconStr),
         TK_CONFIG_NULL_OK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | MENU_BUTTON_MASK, &iconsOption},
    {TK_CONFIG_STRING, "-image", "image", "Image",
	DEF_BUTTON_IMAGE, Tk_Offset(Button, imageString),
	NALL_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-activeimage", "activeImage", "Image",
	DEF_BUTTON_IMAGE, Tk_Offset(Button, activeImageString),
	ALL_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_BOOLEAN, "-indicatoron", "indicatorOn", "IndicatorOn",
	DEF_BUTTON_INDICATOR, Tk_Offset(Button, indicatorOn),
	CHECK_BUTTON_MASK | RADIO_BUTTON_MASK },
    {TK_CONFIG_BOOLEAN, "-indicatoron", "indicatorOn", "IndicatorOn",
	"0", Tk_Offset(Button, indicatorOn),
        MENU_BUTTON_MASK },
    {TK_CONFIG_INT, "-checksize", "checkSize", "CheckSize",
	DEF_BUTTON_INDICATORSIZE, Tk_Offset(Button, indicatorSize),
	CHECK_BUTTON_MASK | RADIO_BUTTON_MASK},
    {TK_CONFIG_JUSTIFY, "-justify", "justify", "Justify",
	DEF_BUTTON_JUSTIFY, Tk_Offset(Button, justify), NALL_MASK},
   /* {TK_CONFIG_BORDER, "-innerbg", "innerBg", "Background",
	NULL, Tk_Offset(Button, innerBorder),
	ALL_MASK |TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-innertile", "innerTile", "InnerTile",
	(char *)NULL, Tk_Offset(Button, innerTile),
	ALL_MASK | TK_CONFIG_NULL_OK, &bltTileOption},
    {TK_CONFIG_CUSTOM, "-activeinnertile", "activeInnerTile", "ActiveInnerTile",
	(char *)NULL, Tk_Offset(Button, activeInnerTile),
	ALL_MASK | TK_CONFIG_NULL_OK, &bltTileOption},
    {TK_CONFIG_CUSTOM, "-disabledinnertile", "disabledInnerTile", "DisabledInnerTile",
	(char *)NULL, Tk_Offset(Button, disabledInnerTile),
	ALL_MASK | TK_CONFIG_NULL_OK, &bltTileOption}, */
    {TK_CONFIG_STRING, "-menu", "menu", "Menu",
	(char *)NULL, Tk_Offset(Button, menuName),
	MENU_BUTTON_MASK},
    {TK_CONFIG_STRING, "-offvalue", "offValue", "Value",
	DEF_BUTTON_OFF_VALUE, Tk_Offset(Button, offValue),
	CHECK_BUTTON_MASK},
    {TK_CONFIG_STRING, "-onvalue", "onValue", "Value",
	DEF_BUTTON_ON_VALUE, Tk_Offset(Button, onValue),
	CHECK_BUTTON_MASK},
    {TK_CONFIG_RELIEF, "-overrelief", "overRelief", "OverRelief",
	DEF_BUTTON_OVER_RELIEF, Tk_Offset(Button, overRelief),
	BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_PIXELS, "-padx", "padX", "Pad",
	DEF_BUTTON_PADX, Tk_Offset(Button, padX), BUTTON_MASK | MENU_BUTTON_MASK},
    {TK_CONFIG_PIXELS, "-padx", "padX", "Pad",
	DEF_LABCHKRAD_PADX, Tk_Offset(Button, padX),
	FRAME_MASK | LABEL_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK},
    {TK_CONFIG_PIXELS, "-pady", "padY", "Pad",
	DEF_BUTTON_PADY, Tk_Offset(Button, padY), BUTTON_MASK | MENU_BUTTON_MASK},
    {TK_CONFIG_PIXELS, "-pady", "padY", "Pad",
	DEF_LABCHKRAD_PADY, Tk_Offset(Button, padY),
	FRAME_MASK | LABEL_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK},
    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_BUTTON_RELIEF, Tk_Offset(Button, relief), BUTTON_MASK},
    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_LABCHKRAD_RELIEF, Tk_Offset(Button, relief),
	FRAME_MASK | LABEL_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | MENU_BUTTON_MASK},
    {TK_CONFIG_RELIEF, "-offrelief", "offRelief", "Relief",
	DEF_BUTTON_RELIEF, Tk_Offset(Button, offRelief), CHECK_BUTTON_MASK | RADIO_BUTTON_MASK},
    {TK_CONFIG_INT, "-repeatdelay", "repeatDelay", "RepeatDelay",
	DEF_BUTTON_REPEAT_DELAY, Tk_Offset(Button, repeatDelay),
 	BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK},
    {TK_CONFIG_DOUBLE, "-rotate", "rotate", "Rotate",
	DEF_ROTATE, Tk_Offset(Button, rotate),
	TK_CONFIG_DONT_SET_DEFAULT|ALL_MASK},
    {TK_CONFIG_BORDER, "-selectcolor", "selectColor", "Background",
	DEF_BUTTON_SELECT_COLOR, Tk_Offset(Button, selectBorder),
	CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | TK_CONFIG_COLOR_ONLY
	| TK_CONFIG_NULL_OK},
    {TK_CONFIG_BORDER, "-selectcolor", "selectColor", "Background",
	DEF_BUTTON_SELECT_MONO, Tk_Offset(Button, selectBorder),
	CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | TK_CONFIG_MONO_ONLY
	| TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-selectimage", "selectImage", "Image",
	DEF_BUTTON_SELECT_IMAGE, Tk_Offset(Button, selectImageString),
	CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-shadow", "shadow", "Shadow",
	NULL, Tk_Offset(Button, shadow),
        ALL_MASK | TK_CONFIG_NULL_OK, &bltShadowOption},
    {TK_CONFIG_CUSTOM, "-state", "state", "State",
	DEF_BUTTON_STATE, Tk_Offset(Button, state),
	ALL_MASK, &stateOption},
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_LABEL_TAKE_FOCUS, Tk_Offset(Button, takeFocus),
	LABEL_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_BUTTON_TAKE_FOCUS, Tk_Offset(Button, takeFocus),
	BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | MENU_BUTTON_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-text", "text", "Text",
	DEF_BUTTON_TEXT, Tk_Offset(Button, textPtr), NALL_MASK},
    {TK_CONFIG_STRING, "-textvariable", "textVariable", "Variable",
	DEF_BUTTON_TEXT_VARIABLE, Tk_Offset(Button, textVarName),
	NALL_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-tile", "tile", "Tile",
	(char *)NULL, Tk_Offset(Button, tile),
	ALL_MASK | TK_CONFIG_NULL_OK, &bltTileOption},
    {TK_CONFIG_CUSTOM, "-tree", "tree", "Tree", 
	(char *)NULL, Tk_Offset(Button, tree), NALL_MASK | TK_CONFIG_NULL_OK, 
	&bltTreeOption},
    {TK_CONFIG_INT, "-node", "node", "Node",
	"0", Tk_Offset(Button, node), NALL_MASK},
    {TK_CONFIG_INT, "-tiletop", "tileTop", "TileTop",
	"0", Tk_Offset(Button, bgTileTop),
	FRAME_MASK | LABEL_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-tristateimage", "tristateImage", "Image",
	DEF_BUTTON_IMAGE, Tk_Offset(Button, tristateImageString),
        CHECK_BUTTON_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-tristatevalue", "tristateValue", "TristateValue",
	DEF_BUTTON_TEXT_VARIABLE, Tk_Offset(Button, tristateValue),
         CHECK_BUTTON_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_INT, "-underline", "underline", "Underline",
	DEF_BUTTON_UNDERLINE, Tk_Offset(Button, underline), NALL_MASK},
    {TK_CONFIG_STRING, "-value", "value", "Value",
	DEF_BUTTON_VALUE, Tk_Offset(Button, onValue),
	RADIO_BUTTON_MASK},
    {TK_CONFIG_STRING, "-variable", "variable", "Variable",
	DEF_RADIOBUTTON_VARIABLE, Tk_Offset(Button, selVarName),
	RADIO_BUTTON_MASK},
    {TK_CONFIG_STRING, "-variable", "variable", "Variable",
	DEF_CHECKBUTTON_VARIABLE, Tk_Offset(Button, selVarName),
	CHECK_BUTTON_MASK | TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-width", "width", "Width",
	DEF_BUTTON_WIDTH, Tk_Offset(Button, widthString), ALL_MASK},
    {TK_CONFIG_PIXELS, "-wraplength", "wrapLength", "WrapLength",
	DEF_BUTTON_WRAP_LENGTH, Tk_Offset(Button, wrapLength), NALL_MASK},
    {TK_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset",
	DEF_BUTTON_WRAP_LENGTH, Tk_Offset(Button, xOffset), NALL_MASK},
    {TK_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset",
	DEF_BUTTON_WRAP_LENGTH, Tk_Offset(Button, yOffset), NALL_MASK},
    {TK_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

/*
 * String to print out in error messages, identifying options for
 * widget commands for different types of labels or buttons:
 */

static char *optionStrings[] =
{
    "cget or configure",
    "cget, configure, flash, or invoke",
    "cget, configure, deselect, flash, invoke, select, or toggle",
    "cget, configure, deselect, flash, invoke, or select"
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void ButtonCmdDeletedProc _ANSI_ARGS_((
	ClientData clientData));
static int ButtonCreate _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int argc, char **argv,
	int type));
static void ButtonEventProc _ANSI_ARGS_((ClientData clientData,
	XEvent *eventPtr));
static void ButtonImageProc _ANSI_ARGS_((ClientData clientData,
	int x, int y, int width, int height,
	int imgWidth, int imgHeight));
static void ButtonImageBdProc _ANSI_ARGS_((ClientData clientData,
	int x, int y, int width, int height,
	int imgWidth, int imgHeight));
static void ButtonSelectImageProc _ANSI_ARGS_((
	ClientData clientData, int x, int y, int width,
	int height, int imgWidth, int imgHeight));
static char *ButtonTextVarProc _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, char *name1, char *name2,
	int flags));
static char *ButtonVarProc _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, char *name1, char *name2,
	int flags));
static int TreeTraceProc(ClientData clientData, 
    Tcl_Interp *interp, Blt_TreeNode node, Blt_TreeKey key, 
    unsigned int flags);
static int TreeTextTraceProc(ClientData clientData, 
    Tcl_Interp *interp, Blt_TreeNode node, Blt_TreeKey key, 
    unsigned int flags);
static int ButtonWidgetCmd _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int argc, char **argv));
static void ComputeButtonGeometry _ANSI_ARGS_((Button *butPtr));
static int ConfigureButton _ANSI_ARGS_((Tcl_Interp *interp,
	Button *butPtr, int argc, char **argv,
	int flags));
static void DestroyButton _ANSI_ARGS_((Button *butPtr));
static void DisplayButton _ANSI_ARGS_((ClientData clientData));
static int InvokeButton _ANSI_ARGS_((Button *butPtr));

static Blt_TileChangedProc TileChangedProc;
static Tcl_CmdProc ButtonCmd, LabelCmd, CheckbuttonCmd, RadiobuttonCmd;

EXTERN int TkCopyAndGlobalEval _ANSI_ARGS_((Tcl_Interp *interp, char *script));

#if (TK_MAJOR_VERSION > 4)
EXTERN void TkComputeAnchor _ANSI_ARGS_((Tk_Anchor anchor, Tk_Window tkwin, 
	int padX, int padY, int innerWidth, int innerHeight, int *xPtr, 
	int *yPtr));
#endif

static void
EventuallyRedraw(butPtr)
    Button *butPtr;
{
    if ((butPtr->tkwin != NULL) && !(butPtr->flags & REDRAW_PENDING)) {
        butPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayButton, butPtr);
    }
}


static void
widgetWorldChanged(ClientData clientData)
{
    Button *butPtr = (Button *)clientData;
    butPtr->flags |= (BUTTON_LAYOUT);

    EventuallyRedraw(butPtr);
}


/* TODO: make this a hash table to avoid allocating images for each button???. */
static int
StringToIcons( ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
    char *string, char *widgRec, int offset)
{
    Button *butPtr = (Button*)widgRec;;
    int argc = 0;
    int result,  i;
    char **argv;
    Tk_Image imgs[9], image;

    result = TCL_OK;
    if (string && Tcl_SplitList(interp, string, &argc, &argv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (argc == 0) {
        ckfree( (char*)argv);
        for (i = 0; i < 9; i++) {
            if (butPtr->icons[i]) Tk_FreeImage(butPtr->icons[i]);
            butPtr->icons[i] = NULL;
        }
        if (butPtr->iconStr) {
            ckfree(butPtr->iconStr);
        }
        butPtr->iconStr = NULL;
        butPtr->iconCnt = 0;
        return TCL_OK;
    }
    if (argc <2 || argc >9) {
        ckfree( (char*)argv);
        Tcl_AppendResult(interp, "expected 0, or 2-9 icons", 0);
        return TCL_ERROR;
    }
    for (i = 0; i < 9; i++) {
        imgs[i] = NULL;
    }
    for (i = 0; i < argc; i++) {
        image = Tk_GetImage(interp, butPtr->tkwin, argv[i], ButtonImageProc, (ClientData)butPtr);
        if (image == NULL) {
            for (i = i-1; i >=0; i--) {
                Tk_FreeImage(imgs[i]);
            }
            ckfree( (char*)argv);
            return TCL_ERROR;
        }
        imgs[i] = image;
    }
    ckfree( (char*)argv);
    butPtr->iconCnt = argc;
    for (i = 0; i < 9; i++) {
        if (butPtr->icons[i]) {  Tk_FreeImage(butPtr->icons[i]); }
        butPtr->icons[i] = imgs[i];
    }
    if (butPtr->iconStr) {
        ckfree(butPtr->iconStr);
    }
    butPtr->iconStr = strdup(string);
    return TCL_OK;
}

static void
ComputeAnchor( Tk_Anchor anchor, Tk_Window tkwin, int padX, int padY,
    int innerWidth, int innerHeight, int *xPtr, int *yPtr, int xAdj, int yAdj)
{
    int width, height;
    
    width = Tk_Width(tkwin)-xAdj;
    height = Tk_Height(tkwin)-yAdj;
    /*
     * Handle the horizontal parts.
     */

    switch (anchor) {
    case TK_ANCHOR_NW:
    case TK_ANCHOR_W:
    case TK_ANCHOR_SW:
	*xPtr = Tk_InternalBorderLeft(tkwin) + padX;
	break;

    case TK_ANCHOR_N:
    case TK_ANCHOR_CENTER:
    case TK_ANCHOR_S:
	*xPtr = (width - innerWidth - Tk_InternalBorderLeft(tkwin) -
		Tk_InternalBorderRight(tkwin)) / 2 +
		Tk_InternalBorderLeft(tkwin);
	break;

    default:
	*xPtr = width - Tk_InternalBorderRight(tkwin) - padX
		- innerWidth;
	break;
    }

    /*
     * Handle the vertical parts.
     */

    switch (anchor) {
    case TK_ANCHOR_NW:
    case TK_ANCHOR_N:
    case TK_ANCHOR_NE:
	*yPtr = Tk_InternalBorderTop(tkwin) + padY;
	break;

    case TK_ANCHOR_W:
    case TK_ANCHOR_CENTER:
    case TK_ANCHOR_E:
	*yPtr = (height - innerHeight- Tk_InternalBorderTop(tkwin) -
		Tk_InternalBorderBottom(tkwin)) / 2 +
		Tk_InternalBorderTop(tkwin);
	break;

    default:
	*yPtr = height - Tk_InternalBorderBottom(tkwin) - padY
		- innerHeight;
	break;
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_ButtonCmd, Tk_CheckbuttonCmd, Tk_LabelCmd, Tk_RadiobuttonCmd --
 *
 *	These procedures are invoked to process the "button", "label",
 *	"radiobutton", and "checkbutton" Tcl commands.  See the
 *	user documentation for details on what they do.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.  These procedures are just wrappers;
 *	they call ButtonCreate to do all of the real work.
 *
 *--------------------------------------------------------------
 */

static int
ButtonCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_BUTTON);
}

static int
CheckbuttonCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_CHECK_BUTTON);
}

static int
LabelCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_LABEL);
}

static int
BFrameCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_FRAME+BBOFS);
}

static int
FrameCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_FRAME);
}

static int
MenubuttonCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_MENU_BUTTON);
}

static int
RadiobuttonCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_RADIO_BUTTON);
}
static int
BButtonCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_BUTTON+BBOFS);
}

static int
BCheckbuttonCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_CHECK_BUTTON+BBOFS);
}

static int
BLabelCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_LABEL+BBOFS);
}

static int
BMenubuttonCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_MENU_BUTTON+BBOFS);
}

static int
BRadiobuttonCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    return ButtonCreate(clientData, interp, argc, argv, TYPE_RADIO_BUTTON+BBOFS);
}

/*
 *--------------------------------------------------------------
 *
 * ButtonCreate --
 *
 *	This procedure does all the real work of implementing the
 *	"button", "label", "radiobutton", and "checkbutton" Tcl
 *	commands.  See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

/*ARGSUSED*/
static int
ButtonCreate(clientData, interp, argc, argv, type)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
    int type;			/* Type of button to create: TYPE_LABEL,
				 * TYPE_BUTTON, TYPE_CHECK_BUTTON, or
				 * TYPE_RADIO_BUTTON. */
{
    register Button *butPtr;
    Tk_Window tkwin;
    int oType = type;
    char *className;

    if (type >= BBOFS) type -= BBOFS;
    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
	    argv[0], " pathName ?options?\"", (char *)NULL);
	return TCL_ERROR;
    }
    /*
     * Create the new window.
     */

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), argv[1],
	(char *)NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    /*
     * Initialize the data structure for the button.
     */

    butPtr = Blt_Calloc(1, sizeof(Button));
    butPtr->tkwin = tkwin;
    butPtr->display = Tk_Display(tkwin);
    butPtr->widgetCmd = Tcl_CreateCommand(interp, Tk_PathName(butPtr->tkwin),
	ButtonWidgetCmd, butPtr, ButtonCmdDeletedProc);
#ifdef ITCL_NAMESPACES
    Itk_SetWidgetCommand(butPtr->tkwin, butPtr->widgetCmd);
#endif /* ITCL_NAMESPACES */

    /* Since calloc was used, could skip most of the following ... */
    butPtr->interp = interp;
    butPtr->type = type;
    butPtr->textPtr = NULL;
    butPtr->numChars = 0;
    butPtr->underline = -1;
    butPtr->textVarName = NULL;
    butPtr->bitmap = None;
    butPtr->imageString = NULL;
    butPtr->image = NULL;
    butPtr->tristateImageString = NULL;
    butPtr->selectImageString = NULL;
    butPtr->selectImage = NULL;
    butPtr->state = STATE_NORMAL;
    butPtr->normalBorder = NULL;
    butPtr->activeBorder = NULL;
    butPtr->borderWidth = 0;
    butPtr->relief = TK_RELIEF_FLAT;
    butPtr->highlightWidth = 0;
    butPtr->highlightBgColorPtr = NULL;
    butPtr->highlightColorPtr = NULL;
    butPtr->inset = 0;
    butPtr->tkfont = NULL;
    butPtr->normalFg = NULL;
    butPtr->activeFg = NULL;
    butPtr->disabledFg = NULL;
    butPtr->normalTextGC = None;
    butPtr->activeTextGC = None;
    /*butPtr->gray = None;*/
    butPtr->disabledGC = None;
    butPtr->copyGC = None;
    butPtr->widthString = NULL;
    butPtr->heightString = NULL;
    butPtr->width = 0;
    butPtr->height = 0;
    butPtr->wrapLength = 0;
    butPtr->padX = 0;
    butPtr->padY = 0;
    butPtr->anchor = TK_ANCHOR_CENTER;
    butPtr->justify = TK_JUSTIFY_CENTER;
#if (TK_MAJOR_VERSION > 4)
    butPtr->textLayout = NULL;
#endif
    butPtr->indicatorOn = 0;
    butPtr->selectBorder = NULL;
    butPtr->indicatorSpace = 0;
    butPtr->indicatorDiameter = 0;
    butPtr->selVarName = NULL;
    butPtr->onValue = NULL;
    butPtr->offValue = NULL;
    butPtr->cursor = None;
    butPtr->command = NULL;
    butPtr->takeFocus = NULL;
    butPtr->flags = 0;
    butPtr->tile = butPtr->activeTile = butPtr->innerTile = butPtr->disabledTile = NULL;
    butPtr->defaultState = STATE_DISABLED;
    butPtr->compound = 0;
    butPtr->repeatDelay = 0;
    butPtr->overRelief = TK_RELIEF_RAISED;

    
    className = classNames[oType];
    if (type == TYPE_FRAME) {
        int length, i;
        for (i = 2; i < argc; i += 2) {
            length = strlen(argv[i]);
            if (length < 2) {
                continue;
            }
            if ((argv[i][1] == 'c') && (length >= 3)
                && (strncmp(argv[i], "-class", (unsigned) length) == 0)) {
                className = argv[i+1];
            }
        }
    }


    Tk_SetClass(tkwin, className);
    Tk_SetClassProcs(tkwin, &butClass, (ClientData)butPtr);
    Tk_CreateEventHandler(butPtr->tkwin,
	ExposureMask | StructureNotifyMask | FocusChangeMask,
	ButtonEventProc, butPtr);
    if (ConfigureButton(interp, butPtr, argc - 2, argv + 2,
	    configFlags[type]) != TCL_OK) {
	Tk_DestroyWindow(butPtr->tkwin);
	return TCL_ERROR;
    }
    Tcl_SetResult(interp, Tk_PathName(butPtr->tkwin), TCL_VOLATILE);
    return TCL_OK;
}

static char *ButtonGetValue(Button *butPtr) {
    Blt_TreeNode nodePtr;
    Tcl_Obj *valuePtr;
    CONST char *value;
    Tcl_Interp *interp = butPtr->interp;
    
    value = NULL;
    
    if (butPtr->tree == NULL) {
        value = Tcl_GetVar(interp, butPtr->selVarName, TCL_GLOBAL_ONLY);
    } else {
        nodePtr = Blt_TreeGetNode(butPtr->tree, butPtr->node);
        if (nodePtr == NULL) {
            nodePtr = Blt_TreeCreateNode(butPtr->tree, Blt_TreeGetNode(butPtr->tree, 0), NULL, -1);
        }
        if (nodePtr == NULL) {
            return NULL;
        }
        if (Blt_TreeGetValue(NULL, butPtr->tree, 
            nodePtr, butPtr->selVarName, &valuePtr) == TCL_OK && valuePtr != NULL) {
                value = Tcl_GetString(valuePtr);
        }
    }
    return value;
}

static int ButtonSetValue(Button *butPtr, char *value, int warn) {
    Blt_TreeNode nodePtr;
    Tcl_Obj *valuePtr;
    Tcl_Interp *interp = butPtr->interp;

    if (butPtr->tree == NULL) {
        if (Tcl_SetVar(interp, butPtr->selVarName, value,
            TCL_GLOBAL_ONLY | (warn?TCL_LEAVE_ERR_MSG:0)) == NULL) {
            return TCL_ERROR;
        }
        return TCL_OK;
    }
    nodePtr = Blt_TreeGetNode(butPtr->tree, butPtr->node);
    if (nodePtr == NULL) {
        nodePtr = Blt_TreeCreateNode(butPtr->tree, Blt_TreeGetNode(butPtr->tree, 0), NULL, -1);
    }
    if (nodePtr == NULL) {
        return TCL_ERROR;
    }
    valuePtr = Tcl_NewStringObj(value, -1);
    return Blt_TreeSetValue(interp, butPtr->tree, 
        nodePtr, butPtr->selVarName, valuePtr);
}


/*
 *--------------------------------------------------------------
 *
 * ButtonWidgetCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
ButtonWidgetCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Information about button widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    register Button *butPtr = clientData;
    int result = TCL_OK;
    size_t length;
    int c;

    if ((butPtr->flags & BUTTON_DELETED)) {
        return TCL_OK;
    }

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
	    " option ?arg arg ...?\"", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_Preserve(butPtr);
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'c') && (strncmp(argv[1], "cget", length) == 0)
	&& (length >= 2)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " cget option\"",
		(char *)NULL);
	    goto error;
	}
	result = Tk_ConfigureValue(interp, butPtr->tkwin, configSpecs,
	    (char *)butPtr, argv[2], configFlags[butPtr->type]);
    } else if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)
	&& (length >= 2)) {
	if (argc == 2) {
	    result = Tk_ConfigureInfo(interp, butPtr->tkwin, configSpecs,
		(char *)butPtr, (char *)NULL, configFlags[butPtr->type]);
	} else if (argc == 3) {
	    result = Tk_ConfigureInfo(interp, butPtr->tkwin, configSpecs,
		(char *)butPtr, argv[2],
		configFlags[butPtr->type]);
	} else {
	    result = ConfigureButton(interp, butPtr, argc - 2, argv + 2,
		configFlags[butPtr->type] | TK_CONFIG_ARGV_ONLY);
	}
    } else if ((c == 'd') && (strncmp(argv[1], "deselect", length) == 0)
	&& (butPtr->type >= TYPE_CHECK_BUTTON) && (butPtr->type < TYPE_MENU_BUTTON)) {
	if (argc > 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" deselect\"", (char *)NULL);
	    goto error;
	}
	if (butPtr->type == TYPE_CHECK_BUTTON) {
	    result = ButtonSetValue(butPtr, butPtr->offValue, 1);
	} else if (butPtr->flags & SELECTED) {
	    result = ButtonSetValue(butPtr, "", 1);
	}
    } else if ((c == 'f') && (strncmp(argv[1], "flash", length) == 0)
	&& (butPtr->type > TYPE_LABEL)) {
	int i;

	if (argc > 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" flash\"", (char *)NULL);
	    goto error;
	}
	if (butPtr->state != STATE_DISABLED) {
	    for (i = 0; i < 4; i++) {
		butPtr->state = (butPtr->state == STATE_NORMAL)
		    ? STATE_ACTIVE : STATE_NORMAL;
		Tk_SetBackgroundFromBorder(butPtr->tkwin,
		    (butPtr->state == STATE_ACTIVE && butPtr->activeBorder) ? butPtr->activeBorder
		    : butPtr->normalBorder);
		DisplayButton(butPtr);

		/*
		 * Special note: must cancel any existing idle handler
		 * for DisplayButton;  it's no longer needed, and DisplayButton
		 * cleared the REDRAW_PENDING flag.
		 */

		Tcl_CancelIdleCall(DisplayButton, butPtr);
#ifndef WIN32
		XFlush(butPtr->display);
#endif
		Tcl_Sleep(50);
	    }
	}
    } else if ((c == 'i') && (strncmp(argv[1], "invoke", length) == 0)
	&& (butPtr->type > TYPE_LABEL)) {
	if (argc > 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		" invoke\"", (char *)NULL);
	    goto error;
	}
	if (butPtr->state != STATE_DISABLED) {
	    result = InvokeButton(butPtr);
	}
    } else if ((c == 's') && (strncmp(argv[1], "select", length) == 0)
	&& (butPtr->type >= TYPE_CHECK_BUTTON) && (butPtr->type < TYPE_MENU_BUTTON)) {
	if (argc > 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" select\"", (char *)NULL);
	    goto error;
	}
         result = ButtonSetValue(butPtr,  butPtr->onValue, 1);
    } else if ((c == 't') && (strncmp(argv[1], "toggle", length) == 0)
	&& (length >= 2) && (butPtr->type == TYPE_CHECK_BUTTON)) {
	if (argc > 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" toggle\"", (char *)NULL);
	    goto error;
	}
	if (butPtr->flags & SELECTED) {
             result = ButtonSetValue(butPtr,  butPtr->offValue, 0);
	} else {
             result = ButtonSetValue(butPtr,  butPtr->onValue, 0);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1], "\": must be ",
	    optionStrings[butPtr->type], (char *)NULL);
	goto error;
    }
    Tcl_Release(butPtr);
    return result;

  error:
    Tcl_Release(butPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyButton --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a button at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the widget is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyButton(butPtr)
    Button *butPtr;		/* Info about button widget. */
{
    int i;
    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeOptions handle all the standard option-related
     * stuff.
     */
    butPtr->flags = BUTTON_DELETED;
    if (butPtr->image != NULL) {
	Tk_FreeImage(butPtr->image);
    }
    if (butPtr->activeImage != NULL) {
        Tk_FreeImage(butPtr->activeImage);
    }
    if (butPtr->disabledImage != NULL) {
        Tk_FreeImage(butPtr->disabledImage);
    }
    if (butPtr->bdImage != NULL) {
        Tk_FreeImage(butPtr->bdImage);
    }
    if (butPtr->activeBdImage != NULL) {
        Tk_FreeImage(butPtr->activeBdImage);
    }
    if (butPtr->disabledBdImage != NULL) {
        Tk_FreeImage(butPtr->disabledBdImage);
    }
    if (butPtr->bdImageSized != NULL) {
        Tk_FreeImage(butPtr->bdImageSized);
    }
    if (butPtr->selectImage != NULL) {
	Tk_FreeImage(butPtr->selectImage);
    }
    if (butPtr->normalTextGC != None) {
	Tk_FreeGC(butPtr->display, butPtr->normalTextGC);
    }
    if (butPtr->focusGC != None) {
        Blt_FreePrivateGC(butPtr->display, butPtr->focusGC);
    }
    if (butPtr->activeTextGC != None) {
        Tk_FreeGC(butPtr->display, butPtr->activeTextGC);
    }
    if (butPtr->gray != None) {
        Tk_FreeBitmap(butPtr->display, butPtr->gray);
    }
    if (butPtr->disabledGC != None) {
	Tk_FreeGC(butPtr->display, butPtr->disabledGC);
    }
    if (butPtr->copyGC != None) {
	Tk_FreeGC(butPtr->display, butPtr->copyGC);
    }
    if (butPtr->selVarName != NULL && butPtr->tree == NULL) {
        Tcl_UntraceVar(butPtr->interp, butPtr->selVarName,
            TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
            ButtonVarProc, (ClientData)butPtr);
    }
    if (butPtr->textVarName != NULL && butPtr->tree == NULL) {
        Tcl_UntraceVar(butPtr->interp, butPtr->textVarName,
            TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
            ButtonTextVarProc, butPtr);
    }
    if (butPtr->selTrace != NULL) {
        Blt_TreeDeleteTrace(butPtr->selTrace);
    }
    if (butPtr->textTrace != NULL) {
        Blt_TreeDeleteTrace(butPtr->textTrace);
    }
    if (butPtr->tree) {
        Blt_TreeReleaseToken(butPtr->tree);
    }
    if (butPtr->inBdTile != NULL) {
        Blt_FreeTile(butPtr->inBdTile);
    }
    if (butPtr->activeTile != NULL) {
	Blt_FreeTile(butPtr->activeTile);
    }
    if (butPtr->disabledTile != NULL) {
        Blt_FreeTile(butPtr->disabledTile);
    }
    if (butPtr->innerTile != NULL) {
        Blt_FreeTile(butPtr->innerTile);
    }
    if (butPtr->activeInnerTile != NULL) {
        Blt_FreeTile(butPtr->activeInnerTile);
    }
    if (butPtr->disabledInnerTile != NULL) {
        Blt_FreeTile(butPtr->disabledInnerTile);
    }
    if (butPtr->tile != NULL) {
	Blt_FreeTile(butPtr->tile);
    }
    if (butPtr->shadow.color != NULL) {
        Tk_FreeColor(butPtr->shadow.color);
    }
    if (butPtr->activeShadow.color != NULL) {
        Tk_FreeColor(butPtr->activeShadow.color);
    }

    for (i = 0; i < 9; i++) {
        if (butPtr->icons[i]) Tk_FreeImage(butPtr->icons[i]);
        butPtr->icons[i] = NULL;
    }
#if (TK_MAJOR_VERSION > 4)
    if (butPtr->textLayout) {
        Tk_FreeTextLayout(butPtr->textLayout);
    }
#endif
    Tk_FreeOptions(configSpecs, (char *)butPtr, butPtr->display,
	configFlags[butPtr->type]);
    Tcl_EventuallyFree((ClientData)butPtr, TCL_DYNAMIC);
}

/*ARGSUSED*/
static void
TileChangedProc(clientData, tile)
    ClientData clientData;
    Blt_Tile tile;		/* Not used. */
{
    Button *butPtr = clientData;

    if (butPtr->tkwin != NULL) {
	/*
	 * Arrange for the button to be redisplayed.
	 */
	if (Tk_IsMapped(butPtr->tkwin) && !(butPtr->flags & REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(DisplayButton, (ClientData)butPtr);
	    butPtr->flags |= REDRAW_PENDING;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureButton --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a button widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for butPtr;  old resources get freed, if there
 *	were any.  The button is redisplayed.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureButton(interp, butPtr, argc, argv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register Button *butPtr;	/* Information about widget;  may or may
				 * not already have values for some fields. */
    int argc;			/* Number of valid entries in argv. */
    char **argv;		/* Arguments. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    Tk_Image image;
    char *oldTextVar, *oldSelVar;
    Blt_Tree oldTree;
    int oldNode, result = TCL_OK;
    char * oldABdStr = butPtr->activeBdImageString;
    char * oldDBStr = butPtr->disabledBdImageString;
    char * oldBDStr = butPtr->bdImageString;
    int oldState = butPtr->state, fmask;
    Tk_FakeWin *winPtr;

    winPtr = (Tk_FakeWin *) (butPtr->tkwin);
    oldTree = butPtr->tree;
    oldNode = butPtr->node;
    
    if (oldTree == NULL) {
        oldTextVar = (butPtr->textVarName?strdup(butPtr->textVarName):NULL);
        oldSelVar = (butPtr->selVarName?strdup(butPtr->selVarName):NULL);
    }
    
    if (Tk_ConfigureWidget(interp, butPtr->tkwin, configSpecs,
	    argc, argv, (char *)butPtr, flags) != TCL_OK) {
        if (oldTextVar) ckfree(oldTextVar);
        if (oldTextVar) ckfree(oldSelVar);
	return TCL_ERROR;
    }
    /*
     * Eliminate any existing trace on variables monitored by the button.
     */
 
     if (oldTree == NULL) {
         if (oldTextVar != NULL) {
             Tcl_UntraceVar(interp, oldTextVar,
                 TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 ButtonTextVarProc, (ClientData)butPtr);
                 ckfree(oldTextVar);
         }
         if (oldSelVar != NULL) {
             Tcl_UntraceVar(interp, oldSelVar,
                 TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 ButtonVarProc, (ClientData)butPtr);
                 ckfree(oldSelVar);
         }
     } else {
         if (butPtr->textTrace != NULL) {
             Blt_TreeDeleteTrace(butPtr->textTrace);
             butPtr->textTrace = NULL;
         }
         if (butPtr->selTrace != NULL) {
             Blt_TreeDeleteTrace(butPtr->selTrace);
             butPtr->selTrace = NULL;
         }


     }
    /*
     * A few options need special processing, such as setting the
     * background from a 3-D border, or filling in complicated
     * defaults that couldn't be specified to Tk_ConfigureWidget.
     */
     

    if (butPtr->bgTileTop) {
        winPtr->flags |= TK_BGTILE_TOP;
    } else {
        winPtr->flags &= ~TK_BGTILE_TOP;
    }

    if ((butPtr->state == STATE_ACTIVE) && !Tk_StrictMotif(butPtr->tkwin)) {
        Tk_SetBackgroundFromBorder(butPtr->tkwin, CHOOSE(butPtr->normalBorder,butPtr->activeBorder));
    }

    if (butPtr->highlightWidth < 0) {
	butPtr->highlightWidth = 0;
    }
    fmask = 0;
    if (butPtr->tkfont) {
        fmask = (GCForeground | GCBackground | GCFont);
        gcValues.font = Tk_FontId(butPtr->tkfont);
        gcValues.foreground = butPtr->normalFg->pixel;
        gcValues.background = Tk_3DBorderColor(butPtr->normalBorder)->pixel;
    }
    if (butPtr->activeTile != NULL) {
	Blt_SetTileChangedProc(butPtr->activeTile, TileChangedProc,
		(ClientData)butPtr);
    }
    if (butPtr->disabledTile != NULL) {
        Blt_SetTileChangedProc(butPtr->disabledTile, TileChangedProc,
            (ClientData)butPtr);
    }
    if (butPtr->innerTile != NULL) {
        Blt_SetTileChangedProc(butPtr->innerTile, TileChangedProc,
            (ClientData)butPtr);
    }
    if (butPtr->activeInnerTile != NULL) {
        Blt_SetTileChangedProc(butPtr->activeInnerTile, TileChangedProc,
            (ClientData)butPtr);
    }
    if (butPtr->disabledInnerTile != NULL) {
        Blt_SetTileChangedProc(butPtr->disabledInnerTile, TileChangedProc,
            (ClientData)butPtr);
    }
    if (butPtr->tile != NULL) {
        Blt_SetTileChangedProc(butPtr->tile, TileChangedProc,
            (ClientData)butPtr);
    }
    /*
     * Note: GraphicsExpose events are disabled in normalTextGC because it's
     * used to copy stuff from an off-screen pixmap onto the screen (we know
     * that there's no problem with obscured areas).
     */
    gcValues.graphics_exposures = False;
    newGC = Tk_GetGC(butPtr->tkwin,
	fmask | GCGraphicsExposures,
	&gcValues);
    if (butPtr->normalTextGC != None) {
	Tk_FreeGC(butPtr->display, butPtr->normalTextGC);
    }
    butPtr->normalTextGC = newGC;

    mask = (GCForeground | GCLineWidth );
    gcValues.line_width = 1;
    if (LineIsDashed(butPtr->dashes)) {
        mask |= GCLineStyle;
        gcValues.line_style = LineOnOffDash;
    }
    newGC = Blt_GetPrivateGC(butPtr->tkwin, mask, &gcValues);
    if (LineIsDashed(butPtr->dashes)) {
        butPtr->dashes.offset = 2;
        Blt_SetDashes(butPtr->display, newGC, &(butPtr->dashes));
    }
    if (butPtr->focusGC != None) {
        Blt_FreePrivateGC(butPtr->display, butPtr->focusGC);
    }
    butPtr->focusGC = newGC;
    
    if (butPtr->activeFg != NULL && butPtr->tkfont) {
	gcValues.font = Tk_FontId(butPtr->tkfont);
	gcValues.foreground = butPtr->activeFg->pixel;
         gcValues.background = Tk_3DBorderColor(CHOOSE(butPtr->normalBorder,butPtr->activeBorder))->pixel;
	newGC = Tk_GetGC(butPtr->tkwin,
	    fmask, &gcValues);
	if (butPtr->activeTextGC != None) {
	    Tk_FreeGC(butPtr->display, butPtr->activeTextGC);
	}
	butPtr->activeTextGC = newGC;
    }
    
    if (butPtr->type > TYPE_LABEL) {
	gcValues.font = Tk_FontId(butPtr->tkfont);
	gcValues.background = Tk_3DBorderColor(butPtr->normalBorder)->pixel;
         if ((butPtr->disabledFg != NULL) && ((butPtr->imageString == NULL) || butPtr->compound != COMPOUND_NONE)) {
	    gcValues.foreground = butPtr->disabledFg->pixel;
	    mask = GCForeground | GCBackground | GCFont;
         } else {
             gcValues.foreground = gcValues.background;
             mask = GCForeground;
             if (butPtr->gray == None) {
                 butPtr->gray = Tk_GetBitmap(NULL, butPtr->tkwin, "gray50");
             }
             if (butPtr->gray != None) {
                 gcValues.fill_style = FillStippled;
                 gcValues.stipple = butPtr->gray;
                 mask |= GCFillStyle | GCStipple;
             }
        }
	newGC = Tk_GetGC(butPtr->tkwin, mask, &gcValues);
	if (butPtr->disabledGC != None) {
	    Tk_FreeGC(butPtr->display, butPtr->disabledGC);
	}
	butPtr->disabledGC = newGC;
    }
    if (butPtr->copyGC == None) {
	butPtr->copyGC = Tk_GetGC(butPtr->tkwin, 0, &gcValues);
    }
    if (butPtr->padX < 0) {
	butPtr->padX = 0;
    }
    if (butPtr->padY < 0) {
	butPtr->padY = 0;
    }
    if (butPtr->tree && butPtr->textVarName != NULL && strchr(butPtr->textVarName,'(') != NULL) {
        Tcl_AppendResult(interp, "can not use array", 0);
        ckfree(butPtr->textVarName);
        butPtr->textVarName = NULL;
        result = TCL_ERROR;
    }

    if (butPtr->type >= TYPE_CHECK_BUTTON && (butPtr->type < TYPE_MENU_BUTTON)) {
	CONST char *value;

         if (butPtr->tree && butPtr->selVarName != NULL && strchr(butPtr->selVarName,'(') != NULL) {
             Tcl_AppendResult(interp, "can not use array", 0);
             ckfree(butPtr->selVarName);
             butPtr->selVarName = Blt_Malloc(strlen(Tk_Name(butPtr->tkwin)) + 1);
             strcpy(butPtr->selVarName, Tk_Name(butPtr->tkwin));
             return TCL_ERROR;
         }

	if (butPtr->selVarName == NULL) {
	    butPtr->selVarName = Blt_Malloc(strlen(Tk_Name(butPtr->tkwin)) + 1);
	    strcpy(butPtr->selVarName, Tk_Name(butPtr->tkwin));
	}
	/*
	 * Select the button if the associated variable has the
	 * appropriate value, initialize the variable if it doesn't
	 * exist, then set a trace on the variable to monitor future
	 * changes to its value.
	 */

        butPtr->flags &= ~SELECTED;
        if (butPtr->tree == NULL) {
            value = Tcl_GetVar(interp, butPtr->selVarName, TCL_GLOBAL_ONLY);
            if (value != NULL) {
                if (strcmp(value, butPtr->onValue) == 0) {
                    butPtr->flags |= SELECTED;
                }
            } else {
                if (Tcl_SetVar(interp, butPtr->selVarName,
                    (butPtr->type == TYPE_CHECK_BUTTON) ? butPtr->offValue : "",
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                        return TCL_ERROR;
                }
            }
            Tcl_TraceVar(interp, butPtr->selVarName,
                TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                ButtonVarProc, (ClientData)butPtr);
        } else {
            Tcl_Obj *valuePtr;
            Blt_TreeNode nodePtr;
            
            nodePtr = Blt_TreeGetNode(butPtr->tree, butPtr->node);
            if (nodePtr == NULL) {
                nodePtr = Blt_TreeCreateNode(butPtr->tree, Blt_TreeGetNode(butPtr->tree, 0), NULL, -1);
            }
            if (nodePtr == NULL) {
                return TCL_ERROR;
            }
            if (Blt_TreeGetValue(NULL, butPtr->tree, 
                nodePtr, butPtr->selVarName, &valuePtr) == TCL_OK) {
            } else {
                valuePtr = Tcl_NewStringObj(((butPtr->type == TYPE_CHECK_BUTTON) ? butPtr->offValue : ""),-1);
                Tcl_IncrRefCount(valuePtr);
                if (Blt_TreeSetValue(interp, butPtr->tree, 
                    nodePtr, butPtr->selVarName, valuePtr) != TCL_OK) {
                    Tcl_DecrRefCount(valuePtr);
                    return TCL_ERROR;
                }
                Tcl_DecrRefCount(valuePtr);
            }
            value = (valuePtr?Tcl_GetString(valuePtr):"");
            if (strcmp(value, butPtr->onValue) == 0) {
                butPtr->flags |= SELECTED;
            }
            butPtr->selTrace = Blt_TreeCreateTrace(butPtr->tree, 
                nodePtr, butPtr->selVarName, NULL,
                TREE_TRACE_UNSET|TREE_TRACE_WRITE, TreeTraceProc, (ClientData)butPtr);
        }
    }
    /*
     * Get the images for the widget, if there are any.  Allocate the
     * new images before freeing the old ones, so that the reference
     * counts don't go to zero and cause image data to be discarded.
     */

    if (oldABdStr != butPtr->activeBdImageString ||
         oldDBStr != butPtr->disabledBdImageString ||
         oldBDStr != butPtr->bdImageString ||
         oldState != butPtr->state) {
        butPtr->flags |= BUTTON_DIRTYBD;
    }

    if (butPtr->imageString != NULL) {
	image = Tk_GetImage(butPtr->interp, butPtr->tkwin,
	    butPtr->imageString, ButtonImageProc, (ClientData)butPtr);
	if (image == NULL) {
	    return TCL_ERROR;
	}
    } else {
	image = NULL;
    }
    if (butPtr->image != NULL) {
	Tk_FreeImage(butPtr->image);
    }
    butPtr->image = image;
    
    if (butPtr->disabledImageString != NULL) {
        image = Tk_GetImage(butPtr->interp, butPtr->tkwin,
            butPtr->disabledImageString, ButtonImageProc, (ClientData)butPtr);
            if (image == NULL) {
            return TCL_ERROR;
        }
    } else {
        image = NULL;
    }
    if (butPtr->disabledImage != NULL) {
        Tk_FreeImage(butPtr->disabledImage);
    }
    butPtr->disabledImage = image;

    if (butPtr->activeImageString != NULL) {
        image = Tk_GetImage(butPtr->interp, butPtr->tkwin,
            butPtr->activeImageString, ButtonImageProc, (ClientData)butPtr);
            if (image == NULL) {
            return TCL_ERROR;
        }
    } else {
        image = NULL;
    }
    if (butPtr->activeImage != NULL) {
        Tk_FreeImage(butPtr->activeImage);
    }
    butPtr->activeImage = image;

    if (butPtr->bdImageString != NULL) {
        image = Tk_GetImage(butPtr->interp, butPtr->tkwin,
            butPtr->bdImageString, ButtonImageBdProc, (ClientData)butPtr);
            if (image == NULL) {
            return TCL_ERROR;
        }
    } else {
        image = NULL;
    }
    if (butPtr->bdImage != NULL) {
        Tk_FreeImage(butPtr->bdImage);
    }
    butPtr->bdImage = image;

    if (butPtr->activeBdImageString != NULL) {
        image = Tk_GetImage(butPtr->interp, butPtr->tkwin,
            butPtr->activeBdImageString, ButtonImageBdProc, (ClientData)butPtr);
            if (image == NULL) {
            return TCL_ERROR;
        }
    } else {
        image = NULL;
    }
    if (butPtr->activeBdImage != NULL) {
        Tk_FreeImage(butPtr->activeBdImage);
    }
    butPtr->activeBdImage = image;

    if (butPtr->disabledBdImageString != NULL) {
        image = Tk_GetImage(butPtr->interp, butPtr->tkwin,
            butPtr->disabledBdImageString, ButtonImageBdProc, (ClientData)butPtr);
            if (image == NULL) {
            return TCL_ERROR;
        }
    } else {
        image = NULL;
    }
    if (butPtr->disabledBdImage != NULL) {
        Tk_FreeImage(butPtr->disabledBdImage);
    }
    butPtr->disabledBdImage = image;

    if (butPtr->selectImageString != NULL) {
	image = Tk_GetImage(butPtr->interp, butPtr->tkwin,
	    butPtr->selectImageString, ButtonSelectImageProc,
	    (ClientData)butPtr);
	if (image == NULL) {
	    return TCL_ERROR;
	}
    } else {
	image = NULL;
    }
    if (butPtr->selectImage != NULL) {
	Tk_FreeImage(butPtr->selectImage);
    }
    butPtr->selectImage = image;

    if (butPtr->tristateImageString != NULL) {
        image = Tk_GetImage(butPtr->interp, butPtr->tkwin,
            butPtr->tristateImageString, ButtonSelectImageProc,
            (ClientData)butPtr);
        if (image == NULL) {
            return TCL_ERROR;
        }
    } else {
        image = NULL;
    }
    if (butPtr->tristateImage != NULL) {
        Tk_FreeImage(butPtr->tristateImage);
    }
    butPtr->tristateImage = image;

    if ((butPtr->textVarName != NULL)) {
	/*
	 * The button must display the value of a variable: set up a trace
	 * on the variable's value, create the variable if it doesn't
	 * exist, and fetch its current value.
	 */

	CONST char *value;

         if (butPtr->tree == NULL) {
             value = Tcl_GetVar(interp, butPtr->textVarName, TCL_GLOBAL_ONLY);
             if (value == NULL) {
                 if (Tcl_SetVar(interp, butPtr->textVarName, butPtr->textPtr,
                     TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                         return TCL_ERROR;
                 }
             } else {
                 if (butPtr->textPtr != NULL) {
                     Blt_Free(butPtr->textPtr);
                 }
                 butPtr->textPtr = Blt_Malloc(strlen(value) + 1);
                 strcpy(butPtr->textPtr, value);
             }
             Tcl_TraceVar(interp, butPtr->textVarName,
                 TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 ButtonTextVarProc, (ClientData)butPtr);
         } else {
             Tcl_Obj *valuePtr;
             Blt_TreeNode nodePtr;
            
             nodePtr = Blt_TreeGetNode(butPtr->tree, butPtr->node);
             if (nodePtr == NULL) {
                 nodePtr = Blt_TreeCreateNode(butPtr->tree, Blt_TreeGetNode(butPtr->tree, 0), NULL, -1);
             }
             if (nodePtr == NULL) {
                 return TCL_ERROR;
             }
             if (Blt_TreeGetValue(NULL, butPtr->tree, 
                 nodePtr, butPtr->textVarName, &valuePtr) == TCL_OK) {
             } else {
                 valuePtr = Tcl_NewStringObj("",-1);
                 Tcl_IncrRefCount(valuePtr);
                 if (Blt_TreeSetValue(interp, butPtr->tree, 
                     nodePtr, butPtr->textVarName, valuePtr) != TCL_OK) {
                         Tcl_DecrRefCount(valuePtr);
                         return TCL_ERROR;
                 }
                 Tcl_DecrRefCount(valuePtr);
             }
             
             value = (valuePtr? Tcl_GetString(valuePtr):"");
             if (butPtr->textPtr != NULL) {
                 Blt_Free(butPtr->textPtr);
             }
             butPtr->textPtr = Blt_Malloc(strlen(value) + 1);
             strcpy(butPtr->textPtr, value);

             butPtr->textTrace = Blt_TreeCreateTrace(butPtr->tree, 
                 nodePtr, butPtr->textVarName, NULL,
                 TREE_TRACE_UNSET|TREE_TRACE_WRITE, TreeTextTraceProc, (ClientData)butPtr);
         }
    }

    if ((butPtr->bitmap != None) || (butPtr->image != NULL)) {
	if (Tk_GetPixels(interp, butPtr->tkwin, butPtr->widthString,
		&butPtr->width) != TCL_OK) {
	  widthError:
	    Tcl_AddErrorInfo(interp, "\n    (processing -width option)");
	    return TCL_ERROR;
	}
	if (Tk_GetPixels(interp, butPtr->tkwin, butPtr->heightString,
		&butPtr->height) != TCL_OK) {
	  heightError:
	    Tcl_AddErrorInfo(interp, "\n    (processing -height option)");
	    return TCL_ERROR;
	}
    } else {
	if (Tcl_GetInt(interp, butPtr->widthString, &butPtr->width)
	    != TCL_OK) {
	    goto widthError;
	}
	if (Tcl_GetInt(interp, butPtr->heightString, &butPtr->height)
	    != TCL_OK) {
	    goto heightError;
	}
    }
    ComputeButtonGeometry(butPtr);

    /*
     * Lastly, arrange for the button to be redisplayed.
     */

    if (Tk_IsMapped(butPtr->tkwin) && !(butPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayButton, (ClientData)butPtr);
	butPtr->flags |= REDRAW_PENDING;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayButton --
 *
 *	This procedure is invoked to display a button widget.  It is
 *	normally invoked as an idle handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the button in its
 *	current mode.  The REDRAW_PENDING flag is cleared.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayButton(clientData)
    ClientData clientData;	/* Information about widget. */
{
    register Button *butPtr = clientData;
    GC gc;
    Tk_3DBorder border;
    Pixmap pixmap;
    int x = 0;			/* Initialization only needed to stop
				 * compiler warning. */
    int y, relief;
    register Tk_Window tkwin = butPtr->tkwin;
    int offset = 0;			/* 0 means this is a label widget.  1 means
				 * it is a flavor of button, so we offset
				 * the text to make the button appear to
				 * move up and down as the relief changes. */
    Blt_Tile tile, bgTile, inTile;
    int borderWidth, drawBorder;
    int textXOffset, textYOffset;
    int haveImage = 0, haveText = 0;
    int imageWidth, imageHeight;
    int imageXOffset = 0, imageYOffset = 0;
    int fullWidth, fullHeight;
    int incFull = 0, xAdj = 0, yAdj = 0, inMirror = 0, dirty = 0, iSpc;
    Tk_Image image, bdImage, curImage;
    
    bdImage = butPtr->bdImage;
    if (butPtr->state == STATE_ACTIVE && butPtr->activeBdImage != NULL) {
        bdImage = butPtr->activeBdImage;
    } else if (butPtr->state == STATE_DISABLED && butPtr->disabledBdImage != NULL) {
        bdImage = butPtr->disabledBdImage;
    }
    image = butPtr->image;
    curImage = butPtr->image;
    if (butPtr->state == STATE_DISABLED && butPtr->disabledImage != NULL) {
        curImage = butPtr->disabledImage;
    }
    if (butPtr->state == STATE_ACTIVE && butPtr->activeImage != NULL) {
        curImage = butPtr->activeImage;
    }
    if ((butPtr->flags & BUTTON_DELETED)) {
        return;
    }
    if ((butPtr->flags & BUTTON_DIRTYBD)) {
        butPtr->flags &= ~BUTTON_DIRTYBD;
        dirty = 1;
    }

    if ((butPtr->selectImage != NULL) && (butPtr->flags & SELECTED)) {
         image = butPtr->selectImage;
    } else if ((butPtr->tristateImage != NULL) && (butPtr->flags & TRISTATED)) {
        image = butPtr->tristateImage;
    } else {
        image = curImage;
    }
    butPtr->flags &= ~REDRAW_PENDING;
    if ((butPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }
    
    if (butPtr->flags & BUTTON_LAYOUT) {
        butPtr->flags &= ~BUTTON_LAYOUT;
        ComputeButtonGeometry(butPtr);
    }

    iSpc = butPtr->indicatorSpace;
    if (butPtr->type == TYPE_MENU_BUTTON) {
        iSpc = 0;
        xAdj = butPtr->indicatorSpace;
    }

    drawBorder = 1;

    if (image != NULL) {
        Tk_SizeOfImage(image, &imageWidth, &imageHeight);
        haveImage = 1;
    } else if (butPtr->bitmap != None) {
        Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &imageWidth, &imageHeight);
        haveImage = 1;
    }
    haveText = (butPtr->textPtr != NULL && butPtr->textPtr[0]);
    relief = butPtr->relief;
    if ((butPtr->type >= TYPE_CHECK_BUTTON)  && (butPtr->type < TYPE_MENU_BUTTON) && !butPtr->indicatorOn) {
        if (butPtr->flags & SELECTED) {
            relief = TK_RELIEF_SUNKEN;
        } else if (butPtr->overRelief != relief) {
            relief = butPtr->offRelief;
        }
    }
    
    if ((butPtr->type == TYPE_BUTTON) && (bdImage == NULL) && !Tk_StrictMotif(butPtr->tkwin)) {
        offset = 1;
    }

#define TILECHOOSE(t2,t1) (Blt_HasTile(t1) ? t1 : (Blt_HasTile(t2) ? t2 : NULL))

    tile = inTile = butPtr->innerTile;
    bgTile =( Blt_HasTile(butPtr->tile) ? butPtr->tile : NULL);
    if (butPtr->bdMaskColor == NULL) {
        tile = TILECHOOSE(tile,butPtr->innerTile);
        bgTile = TILECHOOSE(bgTile,butPtr->innerTile);
    }
    border = butPtr->normalBorder;
    borderWidth = butPtr->borderWidth;

    gc = butPtr->normalTextGC;
        
    if (butPtr->state == STATE_DISABLED) {
        if (Blt_HasTile(butPtr->disabledTile)) {
            tile = butPtr->disabledTile;
            bgTile = butPtr->disabledTile;
        }
        if (butPtr->disabledFg != NULL && butPtr->disabledGC) {
            gc = butPtr->disabledGC;
        }
        if (Blt_HasTile(butPtr->disabledInnerTile)) {
            inTile = butPtr->disabledInnerTile;
        }
    } else if ((butPtr->state == STATE_ACTIVE) && butPtr->activeFg != NULL
	&& !Tk_StrictMotif(butPtr->tkwin)) {
	gc = butPtr->activeTextGC;
        border = CHOOSE(butPtr->normalBorder,butPtr->activeBorder);
	tile = TILECHOOSE(tile,butPtr->activeTile);
        bgTile = TILECHOOSE(bgTile,butPtr->activeTile);
        if (Blt_HasTile(butPtr->activeInnerTile)) {
            inTile = butPtr->activeInnerTile;
        }
    }
    if ((butPtr->flags & SELECTED) && (butPtr->state != STATE_ACTIVE)
	&& (butPtr->selectBorder != NULL) && !butPtr->indicatorOn
	&& butPtr->type != TYPE_MENU_BUTTON) {
	border = butPtr->selectBorder;
    }
    /*
     * Override the relief specified for the button if this is a
     * checkbutton or radiobutton and there's no indicator.
     */

    relief = butPtr->relief;
    if ((butPtr->type >= TYPE_CHECK_BUTTON) && (butPtr->type < TYPE_MENU_BUTTON) && !butPtr->indicatorOn) {
	relief = (butPtr->flags & SELECTED) ? TK_RELIEF_SUNKEN
	    : TK_RELIEF_RAISED;
    }

    /*
     * In order to avoid screen flashes, this procedure redraws
     * the button in a pixmap, then copies the pixmap to the
     * screen in a single operation.  This means that there's no
     * point in time where the on-sreen image has been cleared.
     */

     pixmap = Tk_GetPixmap(butPtr->display, Tk_WindowId(tkwin),
	   Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));

    if (Blt_HasTile(bgTile)) {
        Blt_SetTileOrigin(tkwin, bgTile, 0, 0);
        Blt_TileRectangle(tkwin, pixmap, bgTile, 0, 0, Tk_Width(tkwin),
            Tk_Height(tkwin));
    } else {
        Tk_3DBorder curBorder = border;
        Blt_Fill3DRectangle(tkwin, pixmap, curBorder, 0, 0, Tk_Width(tkwin),
            Tk_Height(tkwin), 0, TK_RELIEF_FLAT);
    }
    if (bdImage) {
        if (butPtr->bdImageSized == NULL) {
            /* TODO: would be nice if we could get rid of Tcl visible image for photo */
            butPtr->bdImageSized = Blt_CreateTemporaryImage(butPtr->interp, tkwin, butPtr);
        }
        if (butPtr->bdImageSized != NULL) {
            int width = 0, height = 0, bdWid = butPtr->borderWidth + butPtr->highlightWidth;
            int wWidth = Tk_Width(tkwin)-bdWid*2, wHeight = Tk_Height(tkwin)-bdWid*2;
            int result = TCL_OK;
            Tk_SizeOfImage(butPtr->bdImageSized, &width, &height);
            /*if (butPtr->bdHalo>=0 && butPtr->state != STATE_ACTIVE) { drawBorder = 0; } */
            if (width != wWidth || height != wHeight || dirty) {
                Tk_PhotoHandle destPhoto;
                char *dName, *sName;
                dName = Blt_NameOfImage(butPtr->bdImageSized);
                if (dName == NULL) {
                    Tk_FreeImage(butPtr->bdImageSized);
                    butPtr->bdImageSized = Blt_CreateTemporaryImage(butPtr->interp, tkwin, butPtr);
                    dName = Blt_NameOfImage(butPtr->bdImageSized);
                }
                sName = Blt_NameOfImage(bdImage);
                destPhoto = Tk_FindPhoto(butPtr->interp, dName);
                if (sName && dName && destPhoto) {
                    int nWidth, nHeight;
                    nWidth = ((wWidth+1) & ~0x1);
                    nHeight = ((wHeight+1) & ~0x1);
                    Tk_PhotoSetSize(destPhoto, nWidth, nHeight);
                    result = Blt_ImageMirror(butPtr->interp, sName, dName, butPtr->bdHalo<0?MIRROR_INNER:MIRROR_OUTER, butPtr->bdHalo<0?0:butPtr->bdHalo);
                    if (result == TCL_OK && butPtr->bdMaskColor && inTile) {
                        char *tName = Blt_NameOfTile(inTile);
                        result = Blt_ImageMergeInner(butPtr->interp, dName, tName, dName, butPtr->bdMaskColor, 0);
                    }
                }
            }
            if (butPtr->bdHalo<0 && result == TCL_OK) {
                inMirror = 1;
            }
            if (result == TCL_OK && inMirror == 0) {
                Tk_RedrawImage(butPtr->bdImageSized, 0, 0, wWidth, wHeight, pixmap, bdWid, bdWid);
            }
        }
    }
    textXOffset = 0;
    textYOffset = 0;

    if (butPtr->compound != COMPOUND_NONE && haveImage && haveText) {
        fullWidth = 0;
        fullHeight = 0;

        switch (butPtr->compound) {
        case COMPOUND_TOP:
        case COMPOUND_BOTTOM:
            /*
            * Image is above or below text.
            */

            if (butPtr->compound == COMPOUND_TOP) {
                textYOffset = imageHeight + butPtr->padY + butPtr->yOffset;
            } else {
                imageYOffset = butPtr->textHeight + butPtr->padY + butPtr->yOffset;
            }
            fullHeight = imageHeight + butPtr->textHeight + butPtr->padY;
            fullWidth = (imageWidth > butPtr->textWidth ? imageWidth : butPtr->textWidth);
            textXOffset = (fullWidth - butPtr->textWidth)/2 + butPtr->xOffset;
            imageXOffset = (fullWidth - imageWidth)/2 + butPtr->xOffset;
            break;
        case COMPOUND_LEFT:
        case COMPOUND_RIGHT:
            /*
            * Image is left or right of text.
            */

            if (butPtr->compound == COMPOUND_LEFT) {
                textXOffset = imageWidth + butPtr->padX + butPtr->xOffset;
            } else {
                imageXOffset = butPtr->textWidth + butPtr->padX  + butPtr->xOffset;
            }
            fullWidth = butPtr->textWidth + butPtr->padX + imageWidth;
            fullHeight = (imageHeight > butPtr->textHeight ? imageHeight : butPtr->textHeight);
            textYOffset = (fullHeight - butPtr->textHeight)/2  + butPtr->yOffset;
            imageYOffset = (fullHeight - imageHeight)/2 + butPtr->yOffset;
            break;
        case COMPOUND_CENTER:
            /*
            * Image and text are superimposed.
            */

            fullWidth = (imageWidth > butPtr->textWidth ? imageWidth :
            butPtr->textWidth);
            fullHeight = (imageHeight > butPtr->textHeight ? imageHeight :
            butPtr->textHeight);
            textXOffset = (fullWidth - butPtr->textWidth)/2 + butPtr->xOffset;
            imageXOffset = (fullWidth - imageWidth)/2 + butPtr->xOffset;
            textYOffset = (fullHeight - butPtr->textHeight)/2 + butPtr->yOffset;
            imageYOffset = (fullHeight - imageHeight)/2 + butPtr->yOffset;
            break;
        case COMPOUND_NONE:
            imageXOffset += butPtr->xOffset;
            imageYOffset += butPtr->yOffset;
            break;
        }
	
        ComputeAnchor(butPtr->anchor, tkwin, butPtr->padX, butPtr->padY,
            iSpc + fullWidth, fullHeight, &x, &y, xAdj, yAdj);

            x += iSpc;

            x += offset;
            y += offset;
            if (relief == TK_RELIEF_RAISED) {
            x -= offset;
            y -= offset;
        } else if (relief == TK_RELIEF_SUNKEN) {
            x += offset;
            y += offset;
        }

        imageXOffset += x;
        imageYOffset += y;

        if (image != NULL) {
            /*
            * Do boundary clipping, so that Tk_RedrawImage is passed valid
            * coordinates. [Bug 979239]
            */

            if (imageXOffset < 0) {
                imageXOffset = 0;
            }
            if (imageYOffset < 0) {
                imageYOffset = 0;
            }
            if (imageWidth > Tk_Width(tkwin)) {
                imageWidth = Tk_Width(tkwin);
            }
            if (imageHeight > Tk_Height(tkwin)) {
                imageHeight = Tk_Height(tkwin);
            }
            if ((imageWidth + imageXOffset) > Tk_Width(tkwin)) {
                imageXOffset = Tk_Width(tkwin) - imageWidth;
            }
            if ((imageHeight + imageYOffset) > Tk_Height(tkwin)) {
                imageYOffset = Tk_Height(tkwin) - imageHeight;
            }

            Tk_RedrawImage(image, 0, 0, imageWidth, imageHeight, pixmap, imageXOffset,
                imageYOffset);
        } else {
            XSetClipOrigin(butPtr->display, gc, imageXOffset, imageYOffset);
            XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, gc,
                0, 0, (unsigned int) imageWidth, (unsigned int) imageHeight,
                imageXOffset, imageYOffset, 1);
                XSetClipOrigin(butPtr->display, gc, 0, 0);
        }
        incFull = 1;
        goto drawtext;
    } else {
        if (haveImage) {
            ComputeAnchor(butPtr->anchor, tkwin, 0, 0,
                iSpc + imageWidth, imageHeight, &x, &y, xAdj, yAdj);
                x += iSpc;

                x += offset;
                y += offset;
                if (relief == TK_RELIEF_RAISED) {
                    x -= offset;
                    y -= offset;
                } else if (relief == TK_RELIEF_SUNKEN) {
                    x += offset;
                    y += offset;
                }
                imageXOffset += (x + butPtr->xOffset);
                imageYOffset += (y + butPtr->yOffset);
                if (curImage != NULL) {
                    /*
                    * Do boundary clipping, so that Tk_RedrawImage is passed
                    * valid coordinates. [Bug 979239]
                    */

                    if (imageXOffset < 0) {
                        imageXOffset = 0;
                    }
                    if (imageYOffset < 0) {
                        imageYOffset = 0;
                    }
                    if (imageWidth > Tk_Width(tkwin)) {
                        imageWidth = Tk_Width(tkwin);
                    }
                    if (imageHeight > Tk_Height(tkwin)) {
                        imageHeight = Tk_Height(tkwin);
                    }
                    if ((imageWidth + imageXOffset) > Tk_Width(tkwin)) {
                        imageXOffset = Tk_Width(tkwin) - imageWidth;
                    }
                    if ((imageHeight + imageYOffset) > Tk_Height(tkwin)) {
                        imageYOffset = Tk_Height(tkwin) - imageHeight;
                    }

                    Tk_RedrawImage(image, 0, 0, imageWidth, imageHeight, pixmap,
                        imageXOffset, imageYOffset);
                } else {
                    XSetClipOrigin(butPtr->display, gc, x, y);
                    XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, gc, 0, 0,
                        (unsigned int) imageWidth, (unsigned int) imageHeight, x, y, 1);
                        XSetClipOrigin(butPtr->display, gc, 0, 0);
                }
                y += imageHeight/2;
            } else {
                ComputeAnchor(butPtr->anchor, tkwin, butPtr->padX, butPtr->padY,
                    iSpc + butPtr->textWidth,
                    butPtr->textHeight, &x, &y, xAdj, yAdj);

                    x += iSpc;

                    x += offset;
                    y += offset;
                    if (relief == TK_RELIEF_RAISED) {
                    x -= offset;
                    y -= offset;
                } else if (relief == TK_RELIEF_SUNKEN) {
                    x += offset;
                    y += offset;
                }
                if ((butPtr->type <= TYPE_LABEL)) {
                    x -= 2;
                }
#if 1
drawtext:
         {
             TextStyle ts;
             Shadow *shadowPtr;
             int selected = 0, active = 0;
             x += butPtr->xOffset;
             y += butPtr->yOffset;
             if (butPtr->state == STATE_ACTIVE) {
                 shadowPtr = &butPtr->activeShadow;
             } else {
                 shadowPtr = &butPtr->shadow;
             }
             Blt_SetDrawTextStyle(&ts, butPtr->tkfont, gc ? gc : butPtr->normalTextGC,
                (butPtr->state == STATE_DISABLED && butPtr->disabledFg) ?
                 butPtr->disabledFg: butPtr->normalFg, butPtr->activeFg,
                 shadowPtr->color, butPtr->rotate, TK_ANCHOR_NW, butPtr->justify,
                 0, shadowPtr->offset);
             ts.underline = butPtr->underline;
             ts.state = (butPtr->state==STATE_ACTIVE?STATE_ACTIVE:0);
             ts.border = border;
             ts.padX.side1 = ts.padX.side2 = 2;
             if ((selected) || (active)) {
                 ts.state |= STATE_ACTIVE;
             }
             Blt_DrawText(butPtr->tkwin, pixmap, butPtr->textPtr, &ts, x + textXOffset, y + textYOffset);
             if (incFull) {
                 y += fullHeight/2;
             } else {
                 y = Tk_Height(tkwin)/2;
             }

         }
#else
            Tk_DrawTextLayout(butPtr->display, pixmap, gc, butPtr->textLayout,
                x, y, 0, -1);
                Tk_UnderlineTextLayout(butPtr->display, pixmap, gc,
                butPtr->textLayout, x, y, butPtr->underline);
                y += butPtr->textHeight/2;
#endif                

    }
    
    }

    /*
     * Draw the indicator for check buttons and radio buttons.  At this
     * point x and y refer to the top-left corner of the text or image
     * or bitmap.
     */

    if ((butPtr->type == TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
	int dim;
        int bd = 1; /* butPtr->borderWidth */

        if (butPtr->icons[0] && (butPtr->flags & HAS_ICONS)) {
            goto drawInd;
        }
        dim = butPtr->indicatorDiameter;
	x -= butPtr->indicatorSpace;
	y -= dim / 2;
	if (y < 2) { y = 2; }
	if (dim > 2 * butPtr->borderWidth) {
	    Blt_Draw3DRectangle(tkwin, pixmap, border, x, y, dim, dim,
		bd,
		(butPtr->flags & SELECTED) ? TK_RELIEF_SUNKEN :
		TK_RELIEF_RAISED);
	    x += bd;
	    y += bd;
	    dim -= 2 * bd;
	    if (butPtr->flags & SELECTED) {
		GC borderGC;

		borderGC = Tk_3DBorderGC(tkwin, (butPtr->selectBorder != NULL)
		    ? butPtr->selectBorder : butPtr->normalBorder,
		    TK_3D_FLAT_GC);
		XFillRectangle(butPtr->display, pixmap, borderGC, x, y,
		    (unsigned int)dim, (unsigned int)dim);
	    } else {
		Blt_Fill3DRectangle(tkwin, pixmap, butPtr->normalBorder, x, y,
		    dim, dim, butPtr->borderWidth, TK_RELIEF_FLAT);
	    }
	}
    } else if ((butPtr->type == TYPE_RADIO_BUTTON) && butPtr->indicatorOn) {
	XPoint points[4];
	int radius;
        int bd = 1; /* butPtr->borderWidth */

        if (butPtr->icons[0] && (butPtr->flags & HAS_ICONS)) {
            goto drawInd;
        }
        radius = butPtr->indicatorDiameter / 2;
	points[0].x = x - butPtr->indicatorSpace;
	points[0].y = y;
	points[1].x = points[0].x + radius;
	points[1].y = points[0].y + radius;
	points[2].x = points[1].x + radius;
	points[2].y = points[0].y;
	points[3].x = points[1].x;
	points[3].y = points[0].y - radius;
	if (butPtr->flags & SELECTED) {
	    GC borderGC;

	    borderGC = Tk_3DBorderGC(tkwin, (butPtr->selectBorder != NULL)
		? butPtr->selectBorder : butPtr->normalBorder,
		TK_3D_FLAT_GC);
	    XFillPolygon(butPtr->display, pixmap, borderGC, points, 4, Convex,
		CoordModeOrigin);
	} else {
	    Tk_Fill3DPolygon(tkwin, pixmap, butPtr->normalBorder, points,
		4, bd, TK_RELIEF_FLAT);
	}
	Tk_Draw3DPolygon(tkwin, pixmap, border, points, 4, bd,
	    (butPtr->flags & SELECTED) ? TK_RELIEF_SUNKEN :
	    TK_RELIEF_RAISED);
    }
    
    if (butPtr->type == TYPE_MENU_BUTTON && butPtr->indicatorOn) {
        int mborderWidth, bsub = 0;

        if (butPtr->icons[0] && (butPtr->flags & HAS_ICONS)) {
            goto drawInd;
        }
        if (bdImage) {
            bsub = 7;
        }
        mborderWidth = (butPtr->indicatorHeight + 1) / 3;
        if (mborderWidth < 1) {
            mborderWidth = 1;
        }

        Blt_Fill3DRectangle(tkwin, pixmap, border,
            Tk_Width(tkwin) - butPtr->inset - butPtr->indicatorWidth + 2 - bsub,
            butPtr->inset + (Tk_Height(tkwin) - butPtr->highlightWidth -
            butPtr->indicatorHeight)/2,
            butPtr->indicatorWidth - mborderWidth - 2,
            butPtr->indicatorHeight,
            mborderWidth, TK_RELIEF_RAISED);
    }

    goto goon;
    
    /* Handle drawing the indicator for Check/Radio/Menu buttons. */
    {
        Tk_Image icon;
        int w1, h1, sind;
drawInd:
        
        sind = 0;

        if (butPtr->state == STATE_ACTIVE) {
            sind = 3;
        } else if (butPtr->state == STATE_DISABLED) {
            sind = 6;
        }
#define STATEICON(n) (butPtr->icons[sind+n] ? butPtr->icons[sind+n] : butPtr->icons[n]) 
        icon = NULL;
        if ((butPtr->flags & SELECTED)) {
            icon = 	STATEICON(1);
        } else if (((butPtr->flags & TRISTATED))) {
            icon = STATEICON(2);
        }
        if (icon == NULL) {
            icon = 	STATEICON(0);
        }
        Tk_SizeOfImage(icon, &w1, &h1);
        x -= butPtr->indicatorSpace;
        y -= butPtr->indicatorSpace/2;

        Tk_RedrawImage(icon, 0, 0, w1, h1, pixmap, x, y);
    }
    
goon:
    
    /*
     * If the button is disabled with a stipple rather than a special
     * foreground color, generate the stippled effect.  If the widget
     * is selected and we use a different background color when selected,
     * must temporarily modify the GC.
     */

    if ((butPtr->state == STATE_DISABLED) && (Blt_HasTile(butPtr->disabledTile) == 0)
	&& ((butPtr->disabledFg == NULL) || (curImage != NULL)) &&
	bdImage == NULL && Blt_HasTile(butPtr->tile) == 0) {
	if ((butPtr->flags & SELECTED) && butPtr->indicatorOn == 0
	    && (butPtr->selectBorder != NULL)) {
	    XSetForeground(butPtr->display, butPtr->disabledGC,
		Tk_3DBorderColor(butPtr->selectBorder)->pixel);
	}
	XFillRectangle(butPtr->display, pixmap, butPtr->disabledGC,
	    butPtr->inset, butPtr->inset,
	    (unsigned)(Tk_Width(tkwin) - 2 * butPtr->inset),
	    (unsigned)(Tk_Height(tkwin) - 2 * butPtr->inset));
	if ((butPtr->flags & SELECTED) && butPtr->indicatorOn == 0
	    && (butPtr->selectBorder != NULL)) {
	    XSetForeground(butPtr->display, butPtr->disabledGC,
		Tk_3DBorderColor(butPtr->normalBorder)->pixel);
	}
    }
    /*
     * Draw the border and traversal highlight last.  This way, if the
     * button's contents overflow they'll be covered up by the border.
     */

    if (relief != TK_RELIEF_FLAT && drawBorder  && butPtr->type > TYPE_LABEL ) {
	int inset = butPtr->highlightWidth;
	if (butPtr->defaultState == STATE_ACTIVE) {
	    inset += 2;
	    Blt_Draw3DRectangle(tkwin, pixmap, border, inset, inset,
		Tk_Width(tkwin) - 2 * inset, Tk_Height(tkwin) - 2 * inset,
		1, TK_RELIEF_SUNKEN);
	    inset += 3;
	}
	Blt_Draw3DRectangle(tkwin, pixmap, border, inset, inset,
	    Tk_Width(tkwin) - 2 * inset, Tk_Height(tkwin) - 2 * inset,
	    butPtr->borderWidth, relief);
    }
    if (butPtr->highlightWidth > 0 && butPtr->highlightBgColorPtr != NULL  && butPtr->type > TYPE_LABEL ) {
	GC highlightGC;

	if (butPtr->flags & GOT_FOCUS) {
	    highlightGC = Tk_GCForColor(butPtr->highlightColorPtr, pixmap);
	} else {
	    highlightGC = Tk_GCForColor(butPtr->highlightBgColorPtr, pixmap);
	}
	Tk_DrawFocusHighlight(tkwin, highlightGC, butPtr->highlightWidth, pixmap);
    }
    if ((butPtr->flags & GOT_FOCUS) && Tk_Width(tkwin) > 11 && Tk_Height(tkwin) > 11 && bdImage  && butPtr->type > TYPE_LABEL ) {
        int dx;
        dx = 5 + butPtr->highlightWidth;
        XDrawRectangle(butPtr->display, pixmap, butPtr->focusGC,
            dx, dx, Tk_Width(tkwin) - 2*dx, Tk_Height(tkwin) - 2*dx);
    }
    /*
     * Copy the information from the off-screen pixmap onto the screen,
     * then delete the pixmap.
     */

    XCopyArea(butPtr->display, pixmap, Tk_WindowId(tkwin),
	butPtr->copyGC, 0, 0, (unsigned)Tk_Width(tkwin),
	(unsigned)Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(butPtr->display, pixmap);
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeButtonGeometry --
 *
 *	After changes in a button's text or bitmap, this procedure
 *	recomputes the button's geometry and passes this information
 *	along to the geometry manager for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The button's window may change size.
 *
 *----------------------------------------------------------------------
 */
#define STR(s) ((s)?s:"")
static void
ComputeButtonGeometry(butPtr)
    register Button *butPtr;	/* Button whose geometry may have changed. */
{
    int width, height, avgWidth, txtWidth, txtHeight;
    int haveImage = 0, haveText = 0, isRadio = 0;
    Tk_FontMetrics fm;
    Tk_Image curImage;

    curImage = butPtr->image;
    butPtr->inset = butPtr->highlightWidth + butPtr->borderWidth;
    isRadio = (butPtr->type == TYPE_RADIO_BUTTON);
    /*
    * Leave room for the default ring if needed.
    */

    if (butPtr->type == TYPE_CHECK_BUTTON && butPtr->tristateValue &&
        butPtr->selVarName) {
        char *value;
        
        value = ButtonGetValue(butPtr);

        if (value == NULL) {
            value = "";
        }

        if (strcmp(value,butPtr->tristateValue)==0) {
            butPtr->flags |= TRISTATED;
        } else {
            butPtr->flags &= ~TRISTATED;
        }
    }
    if (butPtr->defaultState != STATE_DISABLED) {
        butPtr->inset += 5;
    }
    butPtr->indicatorSpace = 0;

    width = 0;
    height = 0;
    txtWidth = 0;
    txtHeight = 0;
    avgWidth = 0;

    if (butPtr->state == STATE_DISABLED && butPtr->disabledImage != NULL) {
        curImage = butPtr->disabledImage;
    }
    if (butPtr->state == STATE_ACTIVE && butPtr->activeImage != NULL) {
        curImage = butPtr->activeImage;
    }
    if (curImage != NULL) {
        Tk_SizeOfImage(curImage, &width, &height);
        haveImage = 1;
    } else if (butPtr->bitmap != None) {
        Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
        haveImage = 1;
    }

    if (butPtr->type != TYPE_FRAME && (haveImage == 0 || butPtr->compound != COMPOUND_NONE)) {

        if (butPtr->rotate > 0.0) {
            double rotWidth, rotHeight;
            int labelWidth, labelHeight;
            TextStyle ts;

            Blt_InitTextStyle(&ts);
            ts.font = butPtr->tkfont;
            if (butPtr->state == STATE_ACTIVE) {
                ts.shadow.offset = butPtr->shadow.offset;
            } else {
                ts.shadow.offset = butPtr->activeShadow.offset;
            }
            ts.padX.side1 = ts.padX.side2 = 2;
           
            Blt_GetTextExtents(&ts, STR(butPtr->textPtr), &labelWidth, &labelHeight);
            Blt_GetBoundingBox(labelWidth, labelHeight, butPtr->rotate,
                &rotWidth, &rotHeight, (Point2D *)NULL);
            butPtr->textWidth = ROUND(rotWidth);
            butPtr->textHeight = ROUND(rotHeight);

        } else {
            if (butPtr->textLayout) {
                Tk_FreeTextLayout(butPtr->textLayout);
            }
 
            butPtr->textLayout = Tk_ComputeTextLayout(butPtr->tkfont,
                STR(butPtr->textPtr), -1, butPtr->wrapLength,
                butPtr->justify, 0, &butPtr->textWidth, &butPtr->textHeight);
        }

        txtWidth = butPtr->textWidth;
        txtHeight = butPtr->textHeight;
        avgWidth = Tk_TextWidth(butPtr->tkfont, "0", 1);
        Tk_GetFontMetrics(butPtr->tkfont, &fm);
        haveText = (txtWidth != 0 && txtHeight != 0);
    }

    /*
    * If the button is compound (i.e., it shows both an image and text), the
    * new geometry is a combination of the image and text geometry. We only
    * honor the compound bit if the button has both text and an image,
    * because otherwise it is not really a compound button.
    */

    if (butPtr->compound != COMPOUND_NONE && haveImage && haveText) {
        int isVert = 0;
        switch (butPtr->compound) {
            case COMPOUND_TOP:
            case COMPOUND_BOTTOM:
            /*
            * Image is above or below text.
            */
            isVert = 1;
            height += txtHeight + butPtr->padY;
            width = (width > txtWidth ? width : txtWidth);
            break;
            case COMPOUND_LEFT:
            case COMPOUND_RIGHT:
            /*
            * Image is left or right of text.
            */

            width += txtWidth + butPtr->padX;
            height = (height > txtHeight ? height : txtHeight);
            break;
            case COMPOUND_CENTER:
            /*
            * Image and text are superimposed.
            */

            width = (width > txtWidth ? width : txtWidth);
            height = (height > txtHeight ? height : txtHeight);
            break;
            case COMPOUND_NONE:
            break;
        }
        if (butPtr->width < 0) {
            width = -butPtr->width;
        } else if (butPtr->width > 0) {
            width = butPtr->width;
        }
        if (butPtr->height < 0) {
            height = -butPtr->height;
        } else if (butPtr->height > 0) {
            height = butPtr->height;
        }


        if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
            if (butPtr->rotate <= 0 && isVert==0) {
                butPtr->indicatorSpace = height;
            } else {
                if (butPtr->indicatorSize>0) {
                    butPtr->indicatorSpace = butPtr->indicatorSize+6;
                } else {
                    butPtr->indicatorSpace = height;
                }
            }
            if (butPtr->indicatorSize>0) {
                butPtr->indicatorDiameter = butPtr->indicatorSize + (isRadio*2);
            } else if (butPtr->type == TYPE_CHECK_BUTTON) {
                butPtr->indicatorDiameter = (65*height)/100;
            } else {
                butPtr->indicatorDiameter = (75*height)/100;
            }
        }

        width += 2*butPtr->padX;
        height += 2*butPtr->padY;
    } else {
        if (haveImage) {
            if (butPtr->width < 0) {
                width = -butPtr->width;
            } else if (butPtr->width > 0) {
                width = butPtr->width;
            }
            if (butPtr->height < 0) {
                height = -butPtr->height;
            } else if (butPtr->height > 0) {
                height = butPtr->height;
            }

            if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
                butPtr->indicatorSpace = height;
                if (butPtr->indicatorSize>0) {
                    butPtr->indicatorDiameter = butPtr->indicatorSize + (isRadio*2);
                } else if (butPtr->type == TYPE_CHECK_BUTTON) {
                    butPtr->indicatorDiameter = (65*height)/100;
                } else {
                    butPtr->indicatorDiameter = (75*height)/100;
                }
            }
        } else {
            width = txtWidth;
            height = txtHeight;

            if (butPtr->width < 0) {
                width = -butPtr->width;
            } else if (butPtr->width > 0) {
                if (butPtr->type >= TYPE_FRAME) {
                    width = butPtr->width;
                } else {
                    width = butPtr->width * avgWidth;
                }
            }
            if (butPtr->height < 0) {
                height = -butPtr->height;
             } else if (butPtr->height > 0) {
                 height = butPtr->height;
            }
            if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
                if (butPtr->indicatorSize>0) {
                    butPtr->indicatorDiameter = butPtr->indicatorSize + (isRadio*2);
                } else {
                    butPtr->indicatorDiameter = fm.linespace;
                    if (butPtr->type == TYPE_CHECK_BUTTON) {
                        butPtr->indicatorDiameter =
                        (80*butPtr->indicatorDiameter)/100;
                    }
                }
                butPtr->indicatorSpace = butPtr->indicatorDiameter + avgWidth;
            }
        }
    }
    butPtr->flags &= ~HAS_ICONS;

    if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn && butPtr->icons[0]) {
        int h1, h2, h3, w1, w2, w3;
        Tk_SizeOfImage(butPtr->icons[0], &w1, &h1);
        Tk_SizeOfImage(butPtr->icons[1], &w2, &h2);
        if (butPtr->icons[2]) {
            Tk_SizeOfImage(butPtr->icons[2], &w3, &h3);
            if (w3>w1) { w1 = w3; }
            if (h3>h1) { h1 = h3; }
        }
        if (w1>0 && h1>0 && w2>0 && h2>0) {
            butPtr->flags |= HAS_ICONS;
            if (w2>w1) { w1 = w2; }
            if (h2>h1) { h1 = h2; }
            butPtr->indicatorSpace = w1;
            if (h1>butPtr->indicatorSpace) { butPtr->indicatorSpace = h1; }
            butPtr->indicatorSpace += 4;
        }

    }
    if (butPtr->type == TYPE_MENU_BUTTON) {
        if (butPtr->indicatorOn) {
            if (butPtr->indicatorOn) {
                int mm, pixels;
                mm = WidthMMOfScreen(Tk_Screen(butPtr->tkwin));
                pixels = WidthOfScreen(Tk_Screen(butPtr->tkwin));
                butPtr->indicatorHeight = (INDICATOR_HEIGHT * pixels) / (10 * mm);
                butPtr->indicatorWidth = (INDICATOR_WIDTH * pixels) / (10 * mm)
                + 2 * butPtr->indicatorHeight;
                butPtr->indicatorSpace = butPtr->indicatorWidth;
            } else {
                butPtr->indicatorHeight = 0;
                butPtr->indicatorWidth = 0;
                butPtr->indicatorSpace = 0;
            }
        }
    }

    if ((curImage == NULL) && (butPtr->bitmap == None)) {
        width += 2*butPtr->padX;
        height += 2*butPtr->padY;
    }
    if ((butPtr->type == TYPE_BUTTON) && !Tk_StrictMotif(butPtr->tkwin)) {
        width += 2;
        height += 2;
    }
    if ((butPtr->type <= TYPE_LABEL)) {
        width += 4;
    }
    if (width%2) width++;
    if (height%2) height++;
    Tk_GeometryRequest(butPtr->tkwin, (int) (width + butPtr->indicatorSpace
        + 2*butPtr->inset), (int) (height + 2*butPtr->inset));
    Tk_SetInternalBorder(butPtr->tkwin, butPtr->inset);
}

/*
 *--------------------------------------------------------------
 *
 * ButtonEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on buttons.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
ButtonEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    Button *butPtr = clientData;
    if ((butPtr->flags & BUTTON_DELETED)) {
        return;
    }

    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
	goto redraw;
    } else if (eventPtr->type == ConfigureNotify) {
	/*
	 * Must redraw after size changes, since layout could have changed
	 * and borders will need to be redrawn.
	 */

	goto redraw;
    } else if (eventPtr->type == DestroyNotify) {
	if (butPtr->tkwin != NULL) {
	    butPtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(butPtr->interp, butPtr->widgetCmd);
	}
	if (butPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayButton, (ClientData)butPtr);
	}
	/* This is a hack to workaround a bug in 8.3.3. */
	DestroyButton((ClientData)butPtr);
	/* Tcl_EventuallyFree((ClientData)butPtr, (Tcl_FreeProc *)Blt_Free); */
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    butPtr->flags |= GOT_FOCUS;
	    if (1 || butPtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
             butPtr->flags &= ~GOT_FOCUS;
	    if (1 || butPtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    }
    return;

  redraw:
    if ((butPtr->tkwin != NULL) && !(butPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayButton, (ClientData)butPtr);
	butPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ButtonCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
ButtonCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    Button *butPtr = clientData;
    Tk_Window tkwin = butPtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	butPtr->tkwin = NULL;
#ifdef ITCL_NAMESPACES
	Itk_SetWidgetCommand(tkwin, (Tcl_Command) NULL);
#endif
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * InvokeButton --
 *
 *	This procedure is called to carry out the actions associated
 *	with a button, such as invoking a Tcl command or setting a
 *	variable.  This procedure is invoked, for example, when the
 *	button is invoked via the mouse.
 *
 * Results:
 *	A standard Tcl return value.  Information is also left in
 *	interp->result.
 *
 * Side effects:
 *	Depends on the button and its associated command.
 *
 *----------------------------------------------------------------------
 */

static int
InvokeButton(butPtr)
    register Button *butPtr;	/* Information about button. */
{
    if ((butPtr->flags & BUTTON_DELETED)) {
        return TCL_OK;
    }

    if (butPtr->type == TYPE_CHECK_BUTTON) {
	if (butPtr->flags & SELECTED) {
	    if (ButtonSetValue(butPtr, butPtr->offValue, 1) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    if (ButtonSetValue(butPtr, butPtr->onValue, 1) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    } else if (butPtr->type == TYPE_RADIO_BUTTON) {
        if (ButtonSetValue(butPtr, butPtr->onValue, 1) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if ((butPtr->type > TYPE_LABEL) && (butPtr->command != NULL)) {
	return TkCopyAndGlobalEval(butPtr->interp, butPtr->command);
    }
    return TCL_OK;
}

/* Trace tree operation. */
static int TreeTraceProc(ClientData clientData, 
    Tcl_Interp *interp, Blt_TreeNode node, Blt_TreeKey key, 
    unsigned int flags) {
    register Button *butPtr = clientData;
    Blt_TreeNode nodePtr;
    Tcl_Obj *valuePtr;
    CONST char *value;
    int doUpd = 0;
    nodePtr = Blt_TreeGetNode(butPtr->tree, butPtr->node);
    /* printf("TRC(%d)\n", (flags & TREE_TRACE_UNSET));  */
    if (nodePtr == NULL) {
        nodePtr = Blt_TreeCreateNode(butPtr->tree, Blt_TreeGetNode(butPtr->tree, 0), NULL, -1);
    }
    if (nodePtr == NULL) {
        return TCL_ERROR;
    }

    if (flags & TREE_TRACE_UNSET) {
        valuePtr = Tcl_NewStringObj("",-1);
        Blt_TreeSetValue(NULL, butPtr->tree, 
            nodePtr, butPtr->selVarName, valuePtr);
        Tcl_AppendResult(interp, "can not delete node", 0);
        return TCL_ERROR;
        /*goto redisplay; */
    }
    
    if (Blt_TreeGetValue(NULL, butPtr->tree, 
        nodePtr, butPtr->selVarName, &valuePtr) == TCL_OK) {
        value = Tcl_GetString(valuePtr);
    } else {
	value = "";
    }
    if (butPtr->type == TYPE_CHECK_BUTTON && butPtr->tristateValue &&
        strcmp(value,butPtr->tristateValue)==0) {
        if ((butPtr->flags&TRISTATED) == 0) doUpd = 1;
        butPtr->flags |= TRISTATED;
    } else {
        if ((butPtr->flags&TRISTATED) != 0) doUpd = 1;
        butPtr->flags &= ~TRISTATED;
    }
    if (strcmp(value, butPtr->onValue) == 0) {
	if (butPtr->flags & SELECTED) {
	    if (doUpd) goto redisplay;
	    return TCL_OK;
	}
	butPtr->flags |= SELECTED;
    } else if (butPtr->flags & SELECTED) {
	butPtr->flags &= ~SELECTED;
    } else {
        if (doUpd) goto redisplay;
        return TCL_OK;
    }
    
 redisplay:
    if ((butPtr->tkwin != NULL) && Tk_IsMapped(butPtr->tkwin)
    && !(butPtr->flags & REDRAW_PENDING)) {
        Tcl_DoWhenIdle(DisplayButton, (ClientData)butPtr);
        butPtr->flags |= REDRAW_PENDING;
    }
    return TCL_OK;
}



/*
 *--------------------------------------------------------------
 *
 * ButtonVarProc --
 *
 *	This procedure is invoked when someone changes the
 *	state variable associated with a radio button.  Depending
 *	on the new value of the button's variable, the button
 *	may be selected or deselected.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The button may become selected or deselected.
 *
 *--------------------------------------------------------------
 */

 /* ARGSUSED */
static char *
ButtonVarProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Information about button. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* Name of variable. */
    char *name2;		/* Second part of variable name. */
    int flags;			/* Information about what happened. */
{
    register Button *butPtr = clientData;
    CONST char *value;
    int doUpd = 0;

    if ((butPtr->flags & BUTTON_DELETED)) {
        return NULL;
    }

    /*
     * If the variable is being unset, then just re-establish the
     * trace unless the whole interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
	butPtr->flags &= ~SELECTED;
	if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
	    Tcl_TraceVar(interp, butPtr->selVarName,
		TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		ButtonVarProc, clientData);
	}
	goto redisplay;
    }
    /*
     * Use the value of the variable to update the selected status of
     * the button.
     */

    value = Tcl_GetVar(interp, butPtr->selVarName, TCL_GLOBAL_ONLY);
    if (value == NULL) {
	value = "";
    }
    if (butPtr->type == TYPE_CHECK_BUTTON && butPtr->tristateValue &&
        strcmp(value,butPtr->tristateValue)==0) {
        if ((butPtr->flags&TRISTATED) == 0) doUpd = 1;
        butPtr->flags |= TRISTATED;
    } else {
        if ((butPtr->flags&TRISTATED) != 0) doUpd = 1;
        butPtr->flags &= ~TRISTATED;
    }
    if (strcmp(value, butPtr->onValue) == 0) {
	if (butPtr->flags & SELECTED) {
	    if (doUpd) goto redisplay;
	    return (char *) NULL;
	}
	butPtr->flags |= SELECTED;
    } else if (butPtr->flags & SELECTED) {
	butPtr->flags &= ~SELECTED;
    } else {
        if (doUpd) goto redisplay;
        return (char *) NULL;
    }

  redisplay:
    if ((butPtr->tkwin != NULL) && Tk_IsMapped(butPtr->tkwin)
	&& !(butPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayButton, (ClientData)butPtr);
	butPtr->flags |= REDRAW_PENDING;
    }
    return (char *) NULL;
}
/* Trace tree operation. */
static int TreeTextTraceProc(ClientData clientData, 
    Tcl_Interp *interp, Blt_TreeNode node, Blt_TreeKey key, 
    unsigned int flags) {
    register Button *butPtr = clientData;
    Blt_TreeNode nodePtr;
    Tcl_Obj *valuePtr;
    CONST char *value;
    
    if ((butPtr->flags & BUTTON_DELETED)) {
        return TCL_OK;
    }

    /* printf("TRC TXT(%d)\n", (flags & TREE_TRACE_UNSET));  */
    nodePtr = Blt_TreeGetNode(butPtr->tree, butPtr->node);
    if (nodePtr == NULL) {
        nodePtr = Blt_TreeCreateNode(butPtr->tree, Blt_TreeGetNode(butPtr->tree, 0), NULL, -1);
    }
    if (nodePtr == NULL) {
        return TCL_ERROR;
    }

    if (flags & TREE_TRACE_UNSET) {
        Tcl_AppendResult(interp, "can not delete node", 0);
        valuePtr = Tcl_NewStringObj(butPtr->textPtr,-1);
        Blt_TreeSetValue(NULL, butPtr->tree, 
            nodePtr, butPtr->textVarName, valuePtr);
        return TCL_ERROR;
        /* goto redisplay; */
    }
    
    if (Blt_TreeGetValue(NULL, butPtr->tree, 
        nodePtr, butPtr->textVarName, &valuePtr) == TCL_OK) {
        value = Tcl_GetString(valuePtr);
    } else {
	value = "";
    }
    if (butPtr->textPtr != NULL) {
        Blt_Free(butPtr->textPtr);
    }
    butPtr->textPtr = Blt_Malloc(strlen(value) + 1);
    strcpy(butPtr->textPtr, value);
    ComputeButtonGeometry(butPtr);

    
    if ((butPtr->tkwin != NULL) && Tk_IsMapped(butPtr->tkwin)
    && !(butPtr->flags & REDRAW_PENDING)) {
        Tcl_DoWhenIdle(DisplayButton, (ClientData)butPtr);
        butPtr->flags |= REDRAW_PENDING;
    }
    return TCL_OK;
}



/*
 *--------------------------------------------------------------
 *
 * ButtonTextVarProc --
 *
 *	This procedure is invoked when someone changes the variable
 *	whose contents are to be displayed in a button.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The text displayed in the button will change to match the
 *	variable.
 *
 *--------------------------------------------------------------
 */

 /* ARGSUSED */
static char *
ButtonTextVarProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Information about button. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* Not used. */
    char *name2;		/* Not used. */
    int flags;			/* Information about what happened. */
{
    register Button *butPtr = clientData;
    CONST char *value;

    if ((butPtr->flags & BUTTON_DELETED)) {
        return NULL;
    }

    /*
     * If the variable is unset, then immediately recreate it unless
     * the whole interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
	if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
	    Tcl_SetVar(interp, butPtr->textVarName, butPtr->textPtr,
		TCL_GLOBAL_ONLY);
	    Tcl_TraceVar(interp, butPtr->textVarName,
		TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		ButtonTextVarProc, clientData);
	}
	return (char *) NULL;
    }
    value = Tcl_GetVar(interp, butPtr->textVarName, TCL_GLOBAL_ONLY);
    if (value == NULL) {
	value = "";
    }
    if (butPtr->textPtr != NULL) {
	Blt_Free(butPtr->textPtr);
    }
    butPtr->textPtr = Blt_Malloc(strlen(value) + 1);
    strcpy(butPtr->textPtr, value);
    ComputeButtonGeometry(butPtr);

    if ((butPtr->tkwin != NULL) && Tk_IsMapped(butPtr->tkwin)
	&& !(butPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayButton, (ClientData)butPtr);
	butPtr->flags |= REDRAW_PENDING;
    }
    return (char *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ButtonImageProc --
 *
 *	This procedure is invoked by the image code whenever the manager
 *	for an image does something that affects the size of contents
 *	of an image displayed in a button.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the button to get redisplayed.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
ButtonImageProc(clientData, x, y, width, height, imgWidth, imgHeight)
    ClientData clientData;	/* Pointer to widget record. */
    int x, y;			/* Upper left pixel (within image)
					 * that must be redisplayed. */
    int width, height;		/* Dimensions of area to redisplay
					 * (may be <= 0). */
    int imgWidth, imgHeight;	/* New dimensions of image. */
{
    register Button *butPtr = clientData;

    if ((butPtr->flags & BUTTON_DELETED)) {
        return;
    }

    if (butPtr->tkwin != NULL) {
	ComputeButtonGeometry(butPtr);
	if (Tk_IsMapped(butPtr->tkwin) && !(butPtr->flags & REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(DisplayButton, (ClientData)butPtr);
	    butPtr->flags |= REDRAW_PENDING;
	}
    }
}
static void
ButtonImageBdProc(clientData, x, y, width, height, imgWidth, imgHeight)
    ClientData clientData;	/* Pointer to widget record. */
    int x, y;			/* Upper left pixel (within image)
					 * that must be redisplayed. */
    int width, height;		/* Dimensions of area to redisplay
					 * (may be <= 0). */
    int imgWidth, imgHeight;	/* New dimensions of image. */
{
    register Button *butPtr = clientData;

    if ((butPtr->flags & BUTTON_DELETED)) {
        return;
    }

    if (butPtr->tkwin != NULL) {
        butPtr->flags |= BUTTON_DIRTYBD;
	ComputeButtonGeometry(butPtr);
	if (Tk_IsMapped(butPtr->tkwin) && !(butPtr->flags & REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(DisplayButton, (ClientData)butPtr);
	    butPtr->flags |= REDRAW_PENDING;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ButtonSelectImageProc --
 *
 *	This procedure is invoked by the image code whenever the manager
 *	for an image does something that affects the size of contents
 *	of the image displayed in a button when it is selected.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May arrange for the button to get redisplayed.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
ButtonSelectImageProc(clientData, x, y, width, height, imgWidth, imgHeight)
    ClientData clientData;	/* Pointer to widget record. */
    int x, y;			/* Upper left pixel (within image)
					 * that must be redisplayed. */
    int width, height;		/* Dimensions of area to redisplay
					 * (may be <= 0). */
    int imgWidth, imgHeight;	/* New dimensions of image. */
{
    register Button *butPtr = clientData;

    if ((butPtr->flags & BUTTON_DELETED)) {
        return;
    }

    /*
     * Don't recompute geometry:  it's controlled by the primary image.
     */

    if ((butPtr->flags & SELECTED) && (butPtr->tkwin != NULL)
	&& Tk_IsMapped(butPtr->tkwin)
	&& !(butPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayButton, (ClientData)butPtr);
	butPtr->flags |= REDRAW_PENDING;
    }
}

int
Blt_ButtonInit(interp)
    Tcl_Interp *interp;
{
    static Blt_CmdSpec cmdSpecs[6] =
    {
	{"frame", FrameCmd,},
	{"button", ButtonCmd,},
	{"checkbutton", CheckbuttonCmd,},
	{"radiobutton", RadiobuttonCmd,},
	{"label", LabelCmd,},
        {"menubutton", MenubuttonCmd,},
    };
    static Blt_CmdSpec cmdBSpecs[6] =
    {
	{"frame", BFrameCmd,},
	{"button", BButtonCmd,},
	{"checkbutton", BCheckbuttonCmd,},
	{"radiobutton", BRadiobuttonCmd,},
	{"label", BLabelCmd,},
        {"menubutton", BMenubuttonCmd,},
    };
    int result;
    tkNormalUid = Tk_GetUid("normal");
    tkDisabledUid = Tk_GetUid("disabled");
    tkActiveUid = Tk_GetUid("active");
    result = Blt_InitCmds(interp, "blt::tile", cmdSpecs, 6);
    if (result == TCL_OK) {
        result = Blt_InitCmds(interp, "blt::widget", cmdBSpecs, 6);
    }
    return result;
}

#endif /* NO_TILEBUTTON */
