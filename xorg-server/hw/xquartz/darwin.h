/*
 * Copyright (C) 2008 Apple, Inc.
 * Copyright (c) 2001-2004 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#ifndef _DARWIN_H
#define _DARWIN_H

#include <IOKit/IOTypes.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include <X11/extensions/XKB.h>
#include <assert.h>

#include "darwinfb.h"

// From darwin.c
void DarwinPrintBanner(void);
int DarwinParseModifierList(const char *constmodifiers, int separatelr);
void DarwinAdjustScreenOrigins(ScreenInfo *pScreenInfo);

#define SCREEN_PRIV(pScreen) ((DarwinFramebufferPtr) \
    dixLookupPrivate(&pScreen->devPrivates, darwinScreenKey))

/*
 * Global variables from darwin.c
 */
extern DevPrivateKeyRec darwinScreenKeyRec;
#define darwinScreenKey (&darwinScreenKeyRec)
extern int              darwinScreensFound;
extern io_connect_t     darwinParamConnect;
extern int              darwinEventReadFD;
extern int              darwinEventWriteFD;
extern DeviceIntPtr     darwinPointer;
extern DeviceIntPtr     darwinTabletCursor;
extern DeviceIntPtr     darwinTabletStylus;
extern DeviceIntPtr     darwinTabletEraser;
extern DeviceIntPtr     darwinKeyboard;

// User preferences
extern int              darwinMouseAccelChange;
extern int              darwinFakeButtons;
extern int              darwinFakeMouse2Mask;
extern int              darwinFakeMouse3Mask;
extern unsigned int     darwinAppKitModMask;
extern unsigned int     windowItemModMask;
extern int              darwinSyncKeymap;
extern int              darwinDesiredDepth;

// location of X11's (0,0) point in global screen coordinates
extern int              darwinMainScreenX;
extern int              darwinMainScreenY;

// bundle-main.c
extern char *bundle_id_prefix;

#define ENABLE_DEBUG_LOG 1

#ifdef ENABLE_DEBUG_LOG
extern FILE *debug_log_fp;
#define DEBUG_LOG_NAME "x11-debug.txt"
#define DEBUG_LOG(msg, args...) if (debug_log_fp) fprintf(debug_log_fp, "%s:%s:%d " msg, __FILE__, __FUNCTION__, __LINE__, ##args ); fflush(debug_log_fp);
#else
#define DEBUG_LOG(msg, args...) 
#endif

#define TRACE() DEBUG_LOG("\n")

#endif  /* _DARWIN_H */
