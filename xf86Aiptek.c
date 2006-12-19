/*
 * xf86Aiptek
 *
 * This driver assumes Linux Input Device support, available for USB devices.
 * 
 * Copyright 2003 by Bryan W. Headley. <bwheadley@earthlink.net>
 *
 * Lineage: This driver is based on both the xf86HyperPen and xf86Wacom tablet
 *          drivers.
 *
 *      xf86HyperPen -- modified from xf86Summa (c) 1996 Steven Lang
 *          (c) 2000 Roland Jansen
 *          (c) 2000 Christian Herzog (button & 19200 baud support)
 *
 *      xf86Wacom -- (c) 1995-2001 Frederic Lepied
 *
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Bryan W. Headley not be used in 
 * advertising or publicity pertaining to distribution of the software 
 * without specific, written prior permission.  Bryan W. Headley makes no 
 * representations about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * BRYAN W. HEADLEY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL BRYAN W. HEADLEY BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTIONS, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Version 0.0, 1-Jan-2003, Bryan W. Headley
 * - Initial release.
 *
 * Version 1.0, 21-Aug-2003 - 20-Sep-2003, Bryan W. Headley
 * o Implement support for the Wheel device
 * o Implement driver parameter dynamic reconfiguration mechanism,
 *   allowing a client application to both query and program the
 *   driver. The communication layer uses the XInput PtrFeedbackControl
 *   mechanism.
 * o Bug fixes from Adrzej Szombierksi <qq@kuku.eu.org> and 
 *   martin.schneebacher <masc@theaterzentrum.at>.
 * o Way too many issues with definition of TRUE|FALSE, versus
 *   Success|BadMatch. Cannot rely on 'if (so)' syntax to work;
 *   basically refactored all if statements as a matter of policy.
 *   Also got rid of 'implied bools' by comparing bitmasks != 0.
 *   Bryan W. Headley, Jan-3-2004.
 *
 * Version 1.1, 16-Feb-2004, Bryan W. Headley
 * o Added stub routines for hotplug
 * o Deal with logical and physical device paths.
 *
 * Version 1.2, 6-June-2004, Bryan W. Headley
 * o Get rid of some kruft. Some dangling pointers repaired, if statements
 *   fixed, and AiptekIsValidDevice logic fixed.
 */

/* $Header$ */

/*
 *
 * Section "InputDevice"
 *      Identifier  "stylus"                        # a name of your choosing
 *      Driver      "aiptek"
 *      Option      "Type"          "string"        # {stylus|cursor|eraser}
 *      Option      "Device"        "pathname"      # {/dev/input/event0}
 *      Option      "Mode"          "string"        # {absolute|relative}
 *      Option      "Cursor"        "string"        # {stylus|puck}
 *      Option      "USB"           "bool"          # {on|off}
 *      Option      "ScreenNo"      "int"
 *      Option      "KeepShape"     "bool"          # {on|off}
 *      Option      "AlwaysCore"    "bool"          # {on|off}
 *      Option      "DebugLevel"    "int"
 *      Option      "HistorySize"   "int"
 *
 *      # This defines whether we are doing smoothing on pressure inputs.
 *      # 'linear' infers no smoothing; there is also a soft and hard
 *      # algorithm.
 *      Option      "Pressure"      "string"        # {soft|hard|linear}
 *      Option      "PressCurve"    "y0,..,y511"
 *
 *      # ------------------------------------------------------------------
 *      # These parameters describe active area. There are three different
 *      # sets of parameters you can use. Use of one set of parameters
 *      # precludes using the others. Forgetting that will cause undue
 *      # confusion.
 *      #
 *      # 1) xMax/yMax: The drawing area is assumed to begin at (0,0); what you
 *      # are therefore doing is describing the maximum X and Y coordinate 
 *      # values of the opposite corner of the rectangle.
 *
 *      Option      "XMax"          "int"
 *      Option      "YMax"          "int"
 *
 *      # 2) xOffset/yOffset, xSize/ySize. The coordinate pair, xOffset, yOffset
 *      # describes the physical location of what will be considered to be 
 *      # coordinate (0,0); xSize/ySize describes the coordinate of the 
 *      # opposite corner of the rectangle, using relative coordinates 
 *      # (e.g., with width and height.)
 *
 *      Option      "XSize"         "int"
 *      Option      "YSize"         "int"
 *      Option      "XOffset"       "int"
 *      Option      "YOffset"       "int"
 *      
 *      # 3) xTop/yTop, xBottom, yBottom. xTop/yTop describes the physical
 *      # location of what will be considered coordinate (0,0); 
 *      # xBottom/yBottom describes the coordinate of the opposite corner 
 *      # of the rectangle, using physical coordinates.
 *
 *      Option      "XTop"          "int"
 *      Option      "YTop"          "int"
 *      Option      "XBottom"       "int"
 *      Option      "YBottom"       "int"
 *
 *      # End of 'Active Area' parameters
 *      # ------------------------------------------------------------------
 *
 *
 *      # Minimal and maximal pressure values that will be accepted.
 *      # Totally unrelated to thesholds, described below.
 *      Option      "ZMax"          "int"
 *      Option      "ZMin"          "int"
 *
 *      # Threshold describes the minimal delta between this reading
 *      # and the previous reading. For example, if xThreshold is set to
 *      # 5 and you move the x coordinate by 2, that movement willl not
 *      # be reported to the X server. Movement will only be reported should
 *      # you move the stylus/mouse >= 5.
 *      Option      "XThreshold"    "int"
 *      Option      "YThreshold"    "int"
 *      Option      "ZThreshold"    "int"
 *
 * EndSection
 *
 *----------------------------------------------------------------------
 *  Commentary:
 *  1.  Identifier: what you name your input device is not significant to the
 *      X-Server.  But it does afford you the opportunity to use names
 *      that infer what the device type is. It would be advantageous
 *      to use names like 'AiptekCursor', but we impose no standards on
 *      you with respect to that.
 *  2.  Type: you may have sections for each driver type ('stylus', 
 *      'eraser', and 'cursor'.) Since all three types may concurrently be 
 *      associated with the same tablet, you'll likely associate
 *      all three with the same device (e.g., /dev/input/eventX.) Unless,
 *      you own more than one tablet, and only want the second one to
 *      operate as the stylus...
 *  3.  Device: obviously, if you have more than one tablet, then each
 *      will be mapped to different /dev/input/eventX paths. Use unique
 *      Identifiers for all devices.
 *  4.  Multiple tablet support works if each tablet is mapped to a different
 *      X-Server screen (ScreenNo.) There's nothing in place for multiple
 *      tablets to service the same screen at the same time (e.g., Xinerama
 *      supporting multiple display drivers.) If you want that, feel free
 *      to write it, and submit patches :-)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86Aiptek.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/***********************************************************************
 * Function/Macro keys variables.
 *
 * This is a list of X keystrokes the macro keys can send. We're supporting
 * up to 32 function keys, plus the escape key. Even with the 12000U, there
 * are only 24 keys (and 12 macro keys on the 8000U and lesser.) So, we're
 * supporting more keys than your tablet actually has. Call it future
 * capacity planning, if you must.
 */
static KeySym aiptek_map[] =
{
    /* 0x00 .. 0x07 */
    NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol,
    NoSymbol,
    /* 0x08 .. 0x0f */
    XK_F1, XK_F2, XK_F3, XK_F4, XK_F5, XK_F6, XK_F7, XK_F8,
    /* 0x10 .. 0x17 */
    XK_F9, XK_F10, XK_F11, XK_F12, XK_F13, XK_F14, XK_F15, XK_F16,
    /* 0x18 .. 0x1f */
    XK_F17, XK_F18, XK_F19, XK_F20, XK_F21, XK_F22, XK_F23, XK_F24,
    /* 0x20 .. 0x27 */
    XK_F25, XK_F26, XK_F27, XK_F28, XK_F29, XK_F30, XK_F31, XK_F32
};

/* This is the map of Linux Event Input system keystrokes sent for
 * the macro keys. There are gaps in the integral values of these
 * 'defines', so we cannot do a numeric transformation process.
 * We therefore are left with a lookup process which finds the offset
 * from the base element of this array, then applying same to aiptekKeysymMap.
 * Oh, and there's the issue of the first 8 codes not being (re)definable
 * in X. Just as well: X wants an offset into aiptekKeysymMap, instead
 * of the KeySym code.
 */
static int linux_inputDevice_keyMap[] =
{
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8,
    KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_F13, KEY_F14, KEY_F15, KEY_F16,
    KEY_F17, KEY_F18, KEY_F19, KEY_F20, KEY_F21, KEY_F22, KEY_F23, KEY_F24,
    KEY_STOP, KEY_AGAIN, KEY_PROPS, KEY_UNDO, KEY_FRONT, KEY_COPY,
    KEY_OPEN, KEY_PASTE
};

/* minKeyCode = 8 because this is the min legal key code */
static KeySymsRec keysyms =
{
    /* map        minKeyCode  maxKC   width */
    aiptek_map,    8,         0x27,    1
};

/* These are the parameter settings, and their default values.
 * We use this to make a 'fakeLocal'...
 */
static const char *default_options[] =
{
    "BaudRate", "9600",
    NULL
};

static const char* pStylusDevice = "stylus";
static const char* pCursorDevice = "cursor";
static const char* pEraserDevice = "eraser";
static const char* pUnknownDevice = "unknown";

/***********************************************************************
 * AiptekIsValidFileDescriptor
 *
 * Determines whether a given fd is valid/open by abusing
 * the UNIX notion that anything >= 0 is valid, and our
 * vapid notion that VALUE_NA is a legal value for a closed
 * handle. But it does get rid of stupidity of 'if (fd < 0)', which
 * defeats the purpose of a #define...
 */
static Bool
AiptekIsValidFileDescriptor(
        int fd)
{
    if (fd == VALUE_NA) 
    {
        return BadMatch;
    }
    return Success;
}

/***********************************************************************
 * AiptekDispatcher
 *
 * Call dispatcher for this driver.
 */
static Bool
AiptekDispatcher(
        DeviceIntPtr dev,
        int requestCode)
{
    const char *methodName = "AiptekDispatcher";
    LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
    AiptekDevicePtr device = (AiptekDevicePtr) PRIVATE (dev);

    CHECK_DEVICE_INT_PTR (dev, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);
  
    DBG (1, xf86Msg (X_INFO, "%s: entry\n", methodName));
  
    switch (requestCode)
    {
        case DEVICE_INIT:
        {
            DBG(1, xf86Msg(X_INFO,
            "%s: type=%s DeviceIntPtr.id=%d request=DEVICE_INIT\n",
                methodName, 
                (DEVICE_ID (device->deviceId) == STYLUS_ID) ? pStylusDevice : 
                (DEVICE_ID (device->deviceId) == CURSOR_ID) ? pCursorDevice :
                (DEVICE_ID (device->deviceId) == ERASER_ID) ? pEraserDevice : 
                    pUnknownDevice, dev->id));

            if (AiptekAllocateDriverClasses (dev) == BadMatch ||
                AiptekOpenDriver (dev) == BadMatch)
            {
                return BadMatch;
            }
        }
        break;
        
        case DEVICE_ON:
        {
            DBG(1, xf86Msg(X_INFO, 
            "%s device type=%s, DeviceIntPtr.id=%d, request=DEVICE_ON\n",
                methodName, 
                (DEVICE_ID (device->deviceId) == STYLUS_ID) ? pStylusDevice : 
                (DEVICE_ID (device->deviceId) == CURSOR_ID) ? pCursorDevice :
                (DEVICE_ID (device->deviceId) == ERASER_ID) ? pEraserDevice : 
                   pUnknownDevice, dev->id));

            if (AiptekIsValidFileDescriptor (local->fd) == BadMatch &&
                AiptekOpenDriver (dev) == BadMatch)
            {
                xf86Msg (X_ERROR, "%s: type=%s, Unable to open Aiptek Device\n",
                    methodName,
                    (DEVICE_ID (device->deviceId) == STYLUS_ID) ? pStylusDevice : 
                    (DEVICE_ID (device->deviceId) == CURSOR_ID) ? pCursorDevice :
                    (DEVICE_ID (device->deviceId) == ERASER_ID) ? pEraserDevice : 
                        pUnknownDevice);
                dev->inited = FALSE;
                return BadMatch;
            }
            xf86AddEnabledDevice (local);
            dev->public.on = TRUE;
        }
        break;
        
        case DEVICE_OFF:
        {
            DBG (1, xf86Msg(X_INFO,
            "%s: device type=%s, DeviceIntPtr.id=%d, request=DEVICE_OFF\n",
                    methodName,
                    (DEVICE_ID (device->deviceId) == STYLUS_ID) ? pStylusDevice : 
                    (DEVICE_ID (device->deviceId) == CURSOR_ID) ? pCursorDevice :
                    (DEVICE_ID (device->deviceId) == ERASER_ID) ? pEraserDevice : 
                    pUnknownDevice, dev->id));

            if (AiptekIsValidFileDescriptor (local->fd) == Success)
            {
                xf86RemoveEnabledDevice (local);
                /* AiptekCloseDriver(local); */
            }
            dev->public.on = FALSE;
        }
        break;
        
        case DEVICE_CLOSE:
        {
            DBG (1, xf86Msg(X_INFO,
            "%s DEVICE_CLOSE, device type=%s, DeviceIntPtr.id=%d, request=DEVICE_CLOSE\n",
                methodName,
                (DEVICE_ID (device->deviceId) == STYLUS_ID) ? pStylusDevice : 
                (DEVICE_ID (device->deviceId) == CURSOR_ID) ? pCursorDevice :
                (DEVICE_ID (device->deviceId) == ERASER_ID) ? pEraserDevice : 
                    pUnknownDevice, dev->id));
           
            if (AiptekIsValidFileDescriptor (local->fd) == Success)
            {
                xf86RemoveEnabledDevice (local);
                AiptekCloseDriver (local);
            }
            dev->public.on = FALSE;
        }
        break;
        
        default:
        {
            xf86Msg (X_ERROR,
             "%s: device type=%s, DeviceIntPtr.id=%d, Unsupported request=%d\n",
                   methodName,
                   (DEVICE_ID (device->deviceId) == STYLUS_ID) ? pStylusDevice :
                   (DEVICE_ID (device->deviceId) == CURSOR_ID) ? pCursorDevice :
                   (DEVICE_ID (device->deviceId) == ERASER_ID) ? pEraserDevice :
                   pUnknownDevice, dev->id, requestCode);
            return BadMatch;
        }
        break;
    }
    DBG (1, xf86Msg (X_INFO, "%s: type=%s, ends successfully\n",
           methodName,
           (DEVICE_ID (device->deviceId) == STYLUS_ID) ? pStylusDevice :
           (DEVICE_ID (device->deviceId) == CURSOR_ID) ? pCursorDevice :
           (DEVICE_ID (device->deviceId) == ERASER_ID) ? pEraserDevice :
           pUnknownDevice));
    return Success;
}

/***********************************************************************
 * AiptekAllocateDriverStructs --
 *
 * Helper to AiptekDispatcher -- performs the 'DEVICE_INIT' function.
 */
static Bool
AiptekAllocateDriverClasses(
        DeviceIntPtr dev)
{
    const char *methodName = "AiptekAllocateDeviceClasses";
    LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
    AiptekDevicePtr device = (AiptekDevicePtr) PRIVATE (dev);
    CARD8 map[(32 << 4) + 1];
    int numAxes;
    int numButtons;
    int loop;

    CHECK_DEVICE_INT_PTR (dev, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    DBG (1, xf86Msg (X_INFO, "%s: begins, device=%s\n", methodName, local->name));

    numAxes = 6;           /* X, Y, Z, xTilt, yTilt, wheel */
    numButtons = 5;        /* three mice; max 5 stylus */

    memset (map, 0, sizeof (map));
    for (loop = 1; loop <= numButtons; ++loop)
    {
        map[loop] = loop;
    }

    /* These routines are typedef'd for Bool, yet return
    * TRUE|FALSE, which is polar opposite of definition
    * for Success|BadMatch (1,0 versus 0,1)
    */
    if (InitButtonClassDeviceStruct(dev, numButtons, map) == FALSE ||
        InitFocusClassDeviceStruct(dev) == FALSE ||
        InitPtrFeedbackClassDeviceStruct(dev, AiptekPtrFeedbackHandler) == FALSE || 
        InitProximityClassDeviceStruct(dev) == FALSE || 
        InitKeyClassDeviceStruct(dev, &keysyms, NULL) == FALSE ||
        InitValuatorClassDeviceStruct(dev, numAxes, xf86GetMotionEvents,
            local->history_size, ((device->flags & ABSOLUTE_FLAG) != 0 
                    ? Absolute
                    : Relative) | OutOfProximity) == FALSE)
    {
        xf86Msg (X_ERROR, "%s: abends: Cannot allocate Class Device Struct(s)\n",
            methodName);
        return BadMatch;
    }

    /* Allocate the motion history buffer if needed
    */
    xf86MotionHistoryAllocate(local);

    DBG (1, xf86Msg (X_INFO, "%s: ends successfully\n", methodName));
    return Success;
}

/***********************************************************************
 * AiptekUninit --
 *
 * Called when the driver instance is unloaded.
 */
static void
AiptekUninit(
        InputDriverPtr drv,
        LocalDevicePtr local,
        int flags)
{
    const char *methodName = "AiptekUninit";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;

    CHECK_INPUT_DRIVER_PTR (drv, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    DBG (1, xf86Msg (X_INFO, "%s: entry\n", methodName));

    if (AiptekDispatcher (local->dev, DEVICE_OFF) == BadMatch)
    {
        return;
    }

    if (device->devicePath != NULL)
    {
        xfree (device->devicePath);
        device->devicePath = NULL;
    }
    if (device != NULL)
    {
        xfree (device);
        device = NULL;
    }
    xf86DeleteInput (local, 0);
    DBG (1, xf86Msg (X_INFO, "%s: ends successfully\n", methodName));
}

/***********************************************************************
 * AiptekInit --
 *
 * Called when the module subsection is found in XF86Config4.
 * This parses the parameters found in the given section of the file.
 */
static InputInfoPtr
AiptekInit(
        InputDriverPtr drv,
        IDevPtr dev,
        int flags)
{
    const char *methodName = "AiptekInit";
    LocalDevicePtr local = NULL;
    LocalDevicePtr fakeLocal;
    AiptekDevicePtr device;
    char *devicePath;
    char *tempStr;

    CHECK_INPUT_DRIVER_PTR (drv, __LINE__);
    CHECK_I_DEV_PTR (dev, __LINE__);

    xf86Msg (X_INFO, "%s: begins for '%s'\n", methodName, dev->identifier);

    fakeLocal = (LocalDevicePtr) xcalloc (1, sizeof (LocalDeviceRec));
    if (fakeLocal == NULL)
    {
        xf86Msg (X_ERROR, "%s: cannot allocate a LocalDevicePtr struct\n",
            methodName);
        return NULL;
    }

    fakeLocal->conf_idev = dev;

    /* We use fakeLocal here because we need something to parse
     * what driver type is being asked for (stylus, eraser, cursor.)
     * But, building a 'real' LocalDevicePtr means knowing which type
     * of device we're building, which we don't yet know :-) Hence,
     * the 'fake' struct.
     */
    xf86CollectInputOptions (fakeLocal, default_options, NULL);

    /* Device
     */
    devicePath = xf86FindOptionValue (fakeLocal->options, "Device");
    if (devicePath == NULL)
    {
        xf86Msg (X_ERROR, "%s: '%s': No Device specified.\n", methodName,
            dev->identifier);
        goto SetupProc_fail;
    }

    /* Type
    */
    tempStr = xf86FindOptionValue (fakeLocal->options, "Type");
    if (tempStr != NULL && xf86NameCmp (tempStr, "stylus") == 0)
    {
        local = AiptekAllocateLocal (drv, dev->identifier, devicePath,
            STYLUS_ID, 0);
    }
    else if (tempStr != NULL && xf86NameCmp (tempStr, "cursor") == 0)
    {
        local = AiptekAllocateLocal (drv, dev->identifier, devicePath,
            CURSOR_ID, 0);
    }
    else if (tempStr != NULL && xf86NameCmp (tempStr, "eraser") == 0)
    {
        local = AiptekAllocateLocal (drv, dev->identifier, devicePath,
            ERASER_ID, ABSOLUTE_FLAG);
    }
    else if (tempStr != NULL && xf86NameCmp (tempStr, "rubber") == 0)
    {
        local = AiptekAllocateLocal (drv, dev->identifier, devicePath,
            ERASER_ID, ABSOLUTE_FLAG);
    }
    else
    {
        xf86Msg (X_ERROR, "%s: '%s': No type or invalid type specified.\n" 
                           "Must be one of 'stylus', 'cursor', or 'eraser'\n",
             methodName, dev->identifier);
    }

    /* Capture errors allocating LocalDevicePtr and AiptekDevicePtr
    */
    if (local == NULL)
    {
        xfree (fakeLocal);
        return NULL;
    }

    /* Paranoia is a good thing :-)
     */
    device = (AiptekDevicePtr) local->private;
    if (device == NULL)
    {
        xfree (local);
        return NULL;
    }

    /* Move the options from fakeLocal over to local, now that we
     * have a 'real' LocalDevicePtr.
     */
    local->options = fakeLocal->options;
    local->conf_idev = fakeLocal->conf_idev;
    local->name = dev->identifier;
    xfree (fakeLocal);

    /* If there are no other devices sharing this device, then we also have
     * no AiptekCommonPtr, either. So, let's build one of those..
     */
    device->common = AiptekAssignCommon (local, devicePath);
    if (device->common == NULL)
    {
        xf86Msg (X_ERROR, "%s: '%s': No Device specified.\n", methodName,
            dev->identifier);
        goto SetupProc_fail;
    }
    /* Process the common options
     */
    xf86ProcessCommonOptions (local, local->options);

    /* Optional configuration items, now..
    */
    xf86Msg (X_CONFIG, "%s: '%s' path='%s'\n", methodName, dev->identifier,
        devicePath);

    /* DebugLevel
    */
    device->debugLevel = xf86SetIntOption (local->options, "DebugLevel",
        INI_DEBUG_LEVEL);
    if (device->debugLevel > 0)
    {
        xf86Msg (X_CONFIG, "%s: Debug level set to %d\n", methodName,
            device->debugLevel);
    }

    /* HistorySize
    */
    local->history_size = xf86SetIntOption (local->options, "HistorySize", 0);
    if (local->history_size > 0)
    {
        xf86Msg (X_CONFIG, "%s: History size set to %d\n", methodName,
            local->history_size);
    }

    /* ScreenNo
    */
    device->screenNo = xf86SetIntOption (local->options, "ScreenNo", VALUE_NA);
    if (device->screenNo != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: '%s': attached to screen number %d\n",
            methodName, dev->identifier, device->screenNo);
    }

    /* KeepShape
    */
    if (xf86SetBoolOption (local->options, "KeepShape", FALSE) == TRUE)
    {
        device->flags |= KEEP_SHAPE_FLAG;
        xf86Msg (X_CONFIG, "%s: '%s': keeps shape\n", methodName,
            dev->identifier);
    }

    /* AlwaysCore
    */
    if (xf86SetBoolOption (local->options, "AlwaysCore", FALSE) == TRUE)
    {
        device->flags |= ALWAYS_CORE_FLAG;
        xf86Msg (X_CONFIG, "%s: '%s': always core\n", methodName,
            dev->identifier);
    }

    /* Pressure
     */
    device->zMode = PRESSURE_MODE_LINEAR;
    tempStr = xf86FindOptionValue (local->options, "PressureCurveType");
    if (tempStr != NULL && xf86NameCmp (tempStr, "hard") == 0)
    {
        device->zMode = PRESSURE_MODE_HARD_SMOOTH;
    }
    else if (tempStr != NULL && xf86NameCmp (tempStr, "soft") == 0)
    {
        device->zMode = PRESSURE_MODE_SOFT_SMOOTH;
    }
    else if (tempStr != NULL && xf86NameCmp (tempStr, "normal") == 0)
    {
        device->zMode = PRESSURE_MODE_LINEAR;
    }
    else if (tempStr != NULL && xf86NameCmp (tempStr, "custom") == 0)
    {
        device->zMode = PRESSURE_MODE_CUSTOM;
    }
    else if (tempStr != NULL)
    {
        xf86Msg (X_ERROR, "%s: '%s': Invalid Mode (normal|soft|hard|custom).\n",
            methodName, dev->identifier);
    }
    AiptekProjectCurvePoints(device);

    tempStr = xf86FindOptionValue (local->options, "PressureCurve");
    if (device->zMode == PRESSURE_MODE_CUSTOM && tempStr == NULL)
    {
        xf86Msg (X_ERROR, 
        "%s: '%s': Pressure=custom requires the PressureCurve parameter.\n",
                methodName, dev->identifier);
    }
    /* Only parse the PressureCurve if we're set for a custom pressure
     * curve.
     */
    if (device->zMode == PRESSURE_MODE_CUSTOM && tempStr != NULL)
    {
        AiptekParsePressureCurve (device, tempStr);
    }

    /* Mode
    */
    tempStr = xf86FindOptionValue (local->options, "Mode");
    if (tempStr != NULL && xf86NameCmp (tempStr, "absolute") == 0)
    {
        device->flags |= ABSOLUTE_FLAG;
    }
    else if (tempStr != NULL && xf86NameCmp (tempStr, "relative") == 0)
    {
        device->flags &= ~ABSOLUTE_FLAG;
    }
    else if (tempStr != NULL)
    {
        xf86Msg (X_ERROR, "%s: %s: Invalid Mode (absolute|relative).\n",
            methodName, dev->identifier);
        device->flags |= ABSOLUTE_FLAG;
    }
    xf86Msg (X_CONFIG, "%s: %s coordinates is in %s mode\n", methodName,
        local->name,
       (device->flags & ABSOLUTE_FLAG) != 0 ? "absolute" : "relative");

    /* Cursor
     */
    tempStr = xf86FindOptionValue (local->options, "Cursor");
    if (tempStr != NULL && xf86NameCmp (tempStr, "stylus") == 0)
    {
        device->flags |= CURSOR_STYLUS_FLAG;
    }
    else if (tempStr != NULL && xf86NameCmp (tempStr, "puck") == 0)
    {
        device->flags &= ~CURSOR_STYLUS_FLAG;
    }
    xf86Msg (X_CONFIG, "%s: %s cursor is in %s mode\n", methodName, local->name,
       (device->flags & CURSOR_STYLUS_FLAG) != 0 ? "stylus" : "relative");

#ifdef linux
#ifndef LINUX_INPUT
#define LINUX_INPUT
#endif
#endif

#ifdef LINUX_INPUT
  /* The define-name is accurate; the Xorg keyword is not. We are
   * reading from a Linux kernel "Input" device. The Input device
   * layer generally supports mice, joysticks, and keyboards. As
   * an extension, the Input device layer also supports HID devices.
   * HID is a standard specified by the USB Implementors Forum. Ergo,
   * 99.9% of HID devices are USB devices.
   *
   * This option is misnamed, misunderstood, misanthrope. Maybe.
   */
    if (xf86SetBoolOption (local->options, "USB", TRUE) == TRUE)
    {
        xf86Msg (X_CONFIG, "%s: %s: USB: True\n", methodName, dev->identifier);
    }
#else
    #error(The Aiptek USB Driver is not available for your platform);
#endif

  /* These dsylexic parameter names are candidates for deprecation.
   * We support 'xSize' and 'sizeX' because of the dual lineage with
   * the wacom driver and the hyperpen driver. However, we prefer
   * only supporting 'xSize' and other names where the coordinate
   * precedes the fieldName. With the exception of 'invX/invY'
   * which we cannot tolerate being called  'xInv'... :-/
   */

  /* XSize
   */
    device->xSize = xf86SetIntOption (local->options, "XSize", device->xSize);
    device->xSize = xf86SetIntOption (local->options, "SizeX", device->xSize);
    if (device->xSize != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: XSize/SizeX = %d\n", methodName,
            dev->identifier, device->xSize);
    }

    /* YSize
     */
    device->ySize = xf86SetIntOption (local->options, "YSize", device->ySize);
    device->ySize = xf86SetIntOption (local->options, "SizeY", device->ySize);
    if (device->ySize != VALUE_NA) 
    {
        xf86Msg (X_CONFIG, "%s: %s: YSize/SizeY = %d\n", methodName,
            dev->identifier, device->ySize);
    }

    /* XOffset
     */
    device->xOffset = xf86SetIntOption (local->options, "XOffset",
        device->xOffset);
    device->xOffset = xf86SetIntOption (local->options, "OffsetX",
        device->xOffset);
    if (device->xOffset != VALUE_NA) 
    {
        xf86Msg (X_CONFIG, "%s: %s: XOffset/OffsetX = %d\n", methodName,
            dev->identifier, device->xOffset);
    }

    /* YOffset
     */
    device->yOffset = xf86SetIntOption (local->options, "YOffset",
        device->yOffset);
    device->yOffset = xf86SetIntOption (local->options, "OffsetY",
        device->yOffset);
    if (device->yOffset != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: YOffset/OffsetY = %d\n", methodName,
            dev->identifier, device->yOffset);
    }

    /* XThreshold
    */
    device->xThreshold = xf86SetIntOption (local->options, "XThreshold",
        device->xThreshold);
    device->xThreshold = xf86SetIntOption (local->options, "ThresholdX",
        device->xThreshold);
    if (device->xThreshold != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: XThreshold/ThresholdX = %d\n", methodName,
            dev->identifier, device->xThreshold);
    }

    /* YThreshold
    */
    device->yThreshold = xf86SetIntOption (local->options, "YThreshold",
        device->yThreshold);
    device->yThreshold = xf86SetIntOption (local->options, "ThresholdY",
        device->yThreshold);
    if (device->yThreshold != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: YThreshold/ThresholdY = %d\n",
            methodName, dev->identifier, device->yThreshold);
    }

    /* ZThreshold
    */
    device->zThreshold = xf86SetIntOption (local->options, "ZThreshold",
        device->zThreshold);
    device->zThreshold = xf86SetIntOption (local->options, "ThresholdZ",
        device->zThreshold);
    if (device->zThreshold != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: ZThreshold/ThresholdZ = %d\n",
            methodName, dev->identifier, device->zThreshold);
    }

    /* XMax
    */
    device->xMax = xf86SetIntOption (local->options, "XMax", device->xMax);
    device->xMax = xf86SetIntOption (local->options, "MaxX", device->xMax);
    if (device->xMax != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: XMax/MaxX = %d\n",
            methodName, dev->identifier, device->xMax);
    }

    /* YMax
    */
    device->yMax = xf86SetIntOption (local->options, "YMax", device->yMax);
    device->yMax = xf86SetIntOption (local->options, "MaxY", device->yMax);
    if (device->yMax != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: YMax/MaxY = %d\n",
            methodName, dev->identifier, device->yMax);
    }

    /* ZMax
    */
    device->zMax = xf86SetIntOption (local->options, "ZMax", device->zMax);
    device->zMax = xf86SetIntOption (local->options, "MaxZ", device->zMax);
    if (device->zMax != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: ZMax/MaxZ = %d\n",
            methodName, dev->identifier, device->zMax);
    }

    /* ZMin
    */
    device->zMin = xf86SetIntOption (local->options, "ZMin", device->zMin);
    device->zMin = xf86SetIntOption (local->options, "MinZ", device->zMin);
    if (device->zMin != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: ZMin/MinZ = %d\n",
            methodName, dev->identifier, device->zMin);
    }

    /* TopX
    */
    device->xTop = xf86SetIntOption (local->options, "TopX", device->xTop);
    device->xTop = xf86SetIntOption (local->options, "XTop", device->xTop);
    if (device->xTop != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: TopX/XTop = %d\n",
            methodName, dev->identifier, device->xTop);
    }

    /* TopY
    */
    device->yTop = xf86SetIntOption (local->options, "TopY", device->yTop);
    device->yTop = xf86SetIntOption (local->options, "YTop", device->yTop);
    if (device->yTop != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: TopY/YTop = %d\n",
            methodName, dev->identifier, device->yTop);
    }

    /* BottomX
    */
    device->xBottom = xf86SetIntOption (local->options, "BottomX",
        device->xBottom);
    device->xBottom = xf86SetIntOption (local->options, "XBottom",
        device->xBottom);
    if (device->xBottom != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: BottomX/XBottom = %d\n",
            methodName, dev->identifier, device->xBottom);
    }

    /* BottomY
    */
    device->yBottom = xf86SetIntOption (local->options, "BottomY",
        device->yBottom);
    device->yBottom = xf86SetIntOption (local->options, "YBottom",
        device->yBottom);
    if (device->yBottom != VALUE_NA)
    {
        xf86Msg (X_CONFIG, "%s: %s: BottomY/YBottom = %d\n", methodName,
            dev->identifier, device->yBottom);
    }

    /* InvX
    */
    if (xf86SetBoolOption (local->options, "InvX", FALSE) == TRUE)
    {
        device->flags |= INVX_FLAG;
        xf86Msg (X_CONFIG, "%s: %s: InvX\n", methodName, dev->identifier);
    }

    /* InvY
    */
    if (xf86SetBoolOption (local->options, "InvY", FALSE) == TRUE)
    {
        device->flags |= INVY_FLAG;
        xf86Msg (X_CONFIG, "%s: %s: InvY\n", methodName, dev->identifier);
    }

    /* If we've created a new common struct, it's unopened. So let's
    * do a kludge, quietly open the tablet, and gather it's physical
    * capacites...
    */
    if (device->common->fd == VALUE_NA &&
        AiptekGetTabletCapacity (local) == BadMatch)
    {
        xf86Msg (X_ERROR, "%s: abends: AiptekGetTabletCapacity fails\n",
            methodName);
        goto SetupProc_fail;
    }

    /* ... With said capacities, we can tune the active area parameters
    * for their sanity, which we'll need done before we later call
    * AiptekAllocateDriverAxes().
    */
    if (AiptekNormalizeSizeParameters (local) == BadMatch)
    {
        xf86Msg (X_ERROR, "%s: abends: AiptekNormalizeSizeParameters fails\n",
            methodName);
        goto SetupProc_fail;
    }

    xf86Msg (X_CONFIG, "%s: ends successfully for %s\n", methodName,
        dev->identifier);

    /* Mark the device as configured
    */
    local->flags |= (XI86_POINTER_CAPABLE | XI86_CONFIGURED);

    /* Return the LocalDevice
     */
    return local;

SetupProc_fail:
    /* Else we have to do a clean-up.
    */
    if (local != NULL)
    {
        device = (AiptekDevicePtr) local->private;
        if (device != NULL)
        {
            AiptekDetachCommon (local);
            xfree (device);
            device = NULL;
        }
        xfree (local);
        local = NULL;
    }
    return NULL;
}

/***********************************************************************
 * AiptekAllocateLocal --
 *
 * Allocates the device structures for the Aiptek pseudo-driver instance.
 */
static LocalDevicePtr
AiptekAllocateLocal(
        InputDriverPtr drv,
        char *name,
        char *devicePath,
        int deviceId,
        int flags)
{
    const char *methodName = "AiptekAllocateLocal";
    AiptekDevicePtr device;
    LocalDevicePtr local;

    CHECK_INPUT_DRIVER_PTR (drv, __LINE__);

    xf86Msg (X_INFO, "%s: begins with %s (%s, flags %d)\n", methodName, name,
        devicePath, flags);

    device = (AiptekDevicePtr) xalloc (sizeof (AiptekDeviceRec));
    if (device == NULL)
    {
        xf86Msg(X_ERROR, "%s: failed to allocate AiptekDevicePtr\n",methodName);
        return NULL;
    }

    local = xf86AllocateInput (drv, 0);
    if (local == NULL)
    {
        xf86Msg (X_ERROR, "%s: failed at xf86AllocateInput()\n", methodName);
        xfree (device);
        return NULL;
    }

    local->name = name;
    local->type_name = XI_TABLET;
    local->flags = (XI86_POINTER_CAPABLE | XI86_CONFIGURED);
    local->device_control = AiptekDispatcher;
    local->read_input = AiptekReadInput;
    local->control_proc = AiptekChangeControl;
    local->close_proc = AiptekCloseDriver;
    local->switch_mode = AiptekSwitchMode;
    local->conversion_proc = AiptekConvert;
    local->reverse_conversion_proc = AiptekReverseConvert;

    local->fd = VALUE_NA;
    local->atom = 0;
    local->dev = NULL;
    local->private = device;
    local->private_flags = 0;
    local->history_size = 0;
    local->old_x = VALUE_NA;
    local->old_y = VALUE_NA;

    /* Of note: devicePath is a deep copy; AiptekCommonPtr is NULL.
    */
    AiptekInitializeDevicePtr (device, xstrdup (devicePath), deviceId, flags,
        NULL);

    xf86Msg (X_INFO, "%s: ends successfully\n", methodName);
    return local;
}

/***********************************************************************
 * AiptekAssignCommon --
 *
 * Finds a common structure for this Aiptek Driver instance that's been
 * assigned to another Aiptek Driver instance (yet for the same physical
 * tablet.) If none exist, we'll create a new common struct for the user.
 */
static AiptekCommonPtr
AiptekAssignCommon(
        LocalDevicePtr local,
        char *devicePath)
{
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    AiptekCommonPtr common;
    LocalDevicePtr localIter;
    int i;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    /* Go through linked list of local device drivers and find instances
    * of the aiptek driver. For those that refer to the same driverPath,
    * get the common structure and return it.
    *
    * If said common structure does not yet exist, we'll proceed to
    * create one.
    */
    for (localIter = xf86FirstLocalDevice ();
         localIter != NULL;
         localIter = localIter->next)
     {
        common = (localIter->device_control == AiptekDispatcher)
            ?  ((AiptekDevicePtr) localIter->private)->common
            : NULL;

        if (local != localIter &&
            common != NULL &&
            common->devicePath != NULL &&
            strcmp (common->devicePath, devicePath) == 0)
        {
            /* Return the common with it having a reference back to local.
            */
            for (i = 0; i < AIPTEK_MAX_DEVICES; ++i)
            {
                if (common->deviceArray[i] == NULL)
                {
                    common->deviceArray[i] = local;
                    device->common = common;
                    break;
                }
            }
            return common;
        }
    }
    return AiptekAllocateCommon (local, devicePath);
}

/***********************************************************************
 * AiptekAllocateCommon -
 *
 * This routine creates a 'common info' struct. There's only one
 * per physical device path.
 *
 * This routine will also establish a reference to it's local struct.
 * 
 * DO NOT CALL THIS METHOD DIRECTLY; go through AiptekAssignCommon.
 */
static AiptekCommonPtr
AiptekAllocateCommon(
        LocalDevicePtr local,
        char *devicePath)
{
    const char *methodName = "AiptekAllocateCommon";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    AiptekCommonPtr common;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    xf86Msg (X_INFO, "%s: begins\n", methodName);

    common = (AiptekCommonPtr) xalloc (sizeof (AiptekCommonRec));
    if (common == NULL)
    {
        xf86Msg (X_ERROR, "%s: failed to allocate AiptekCommonPtr\n", methodName);
        return NULL;
    }

    /* Record of the event currently being read of the queue
    */
    common->currentValues.eventType = 0;    /* Device event is for, e.g., */
                                            /* STYLUS, RUBBER, CURSOR */
    common->currentValues.x = 0;            /* X coordinate */
    common->currentValues.y = 0;            /* Y coordinate */
    common->currentValues.z = 0;            /* Z (pressure) */
    common->currentValues.xTilt = 0;        /* XTilt */
    common->currentValues.yTilt = 0;        /* YTilt */
    common->currentValues.proximity = 0;    /* proximity bit */
    common->currentValues.macroKey = VALUE_NA;    /* tablet macro key code */
    common->currentValues.button = 0;       /* bitmask of buttons pressed */
    common->currentValues.wheel = 0;        /* wheel */
    common->currentValues.distance = 0;     /* currently unsupported */

    /* Record of the event previously read off of the queue. It's a
     * dup of currentValues.
    */
    memmove (&common->previousValues, &common->currentValues,
       sizeof (AiptekStateRec));

    common->devicePath = xstrdup (devicePath);
                                        /* kernel driver path */
    common->flags = 0;                  /* various flags */
    common->fd = VALUE_NA;              /* common fd */
    common->deviceArray[0] = local;     /* local as element */
    common->deviceArray[1] = NULL;      /* zap other elements */
    common->deviceArray[2] = NULL;      /* zap other elements */
    common->xMinCapacity = VALUE_NA;    /* tablet's min X value */
    common->xMaxCapacity = VALUE_NA;    /* tablet's max X value */
    common->yMinCapacity = VALUE_NA;    /* tablet's min Y value */
    common->yMaxCapacity = VALUE_NA;    /* tablet's max Y value */
    common->zMinCapacity = VALUE_NA;    /* tablet's min Z value */
    common->zMaxCapacity = VALUE_NA;    /* tablet's max Z value */
    common->xtMinCapacity = VALUE_NA;   /* tablet's min Xtilt */
    common->xtMaxCapacity = VALUE_NA;   /* tablet's max XTilt */
    common->ytMinCapacity = VALUE_NA;   /* tablet's min YTilt */
    common->ytMaxCapacity = VALUE_NA;   /* tablet's max YTilt */
    common->wMinCapacity = VALUE_NA;    /* tablet's min Wheel */
    common->wMaxCapacity = VALUE_NA;    /* tablet's max Wheel */

    device->common = common;    /* set up info ptr */

    xf86Msg (X_INFO, "%s: ends\n", methodName);

    return common;
}

/***********************************************************************
 * AiptekDetachCommonPtr --
 * 
 * Remove the common ptr reference from any closed devices.
 */
static Bool
AiptekDetachCommon(
        LocalDevicePtr local)
{
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    AiptekCommonPtr common = device->common;
    int i, j;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

  /* Paranoia is good :-)
   */
    if (common == NULL)
    {
    return 0;
    }

  /* Go through all devices referenced by the common struct.
   * 1. When a device is closed, it should no longer reference
   *    a common struct.
   * 2. The device struct should have no reference to common.
   * 3. When there no longer are devices that reference the
   *    common struct, we need to delete the common struct.
   */
    for (i = 0, j = 0; i < AIPTEK_MAX_DEVICES; ++i)
    {
        /* Remove the association
         */
        if (common->deviceArray[i] == local)
        {
            common->deviceArray[i] = NULL;
            device->common = NULL;
        }
        /* Now count the existing references.
        */
        if (common->deviceArray[i] != NULL)
        {
            ++j;
        }
    }
    /* If there are no more references, get rid of the common struct.
    */
    if (j == 0)
    {
        if (common->devicePath != NULL)
        {
            xfree (common->devicePath);
            common->devicePath = NULL;
        }
        xfree (common);
        common = NULL;
    }
    return j;
}

/***********************************************************************
 * AiptekInitializeDevicePtr
 *
 * Initializes the AiptekDevicePtr to known 'N/A' values. Except for,
 * of course, devicePath, deviceId, flags and common. Broken out as a
 * separate routine as we call this also from the device driver callback
 * when reprogramming the driver's state.
 */
static void
AiptekInitializeDevicePtr(
        AiptekDevicePtr device,
        char *devicePath,
        int deviceId,
        int flags,
        AiptekCommonPtr common)
{
    device->devicePath = devicePath;    /* kernel's driver path */
    device->deviceId = deviceId;        /* device type      */
    device->flags = flags;              /* flags for various settings */
    device->xSize = VALUE_NA;           /* Active Area X */
    device->ySize = VALUE_NA;           /* Active Area Y */
    device->xOffset = VALUE_NA;         /* Active area offset X */
    device->yOffset = VALUE_NA;         /* Active area offset Y */
    device->xMax = VALUE_NA;            /* Max allowed X value */
    device->yMax = VALUE_NA;            /* Max allowed Y value */
    device->zMin = VALUE_NA;            /* Min allowed Z value */
    device->zMax = VALUE_NA;            /* Max allowed Z value */
    device->xTop = VALUE_NA;            /* Upper Left X coordinate */
    device->yTop = VALUE_NA;            /* Upper Left Y coordinate */
    device->xBottom = VALUE_NA;         /* Lower Right X coordinate */
    device->yBottom = VALUE_NA;         /* Lower Right Y coordinate */
    device->xThreshold = VALUE_NA;      /* X threshold */
    device->yThreshold = VALUE_NA;      /* Y threshold */
    device->zThreshold = VALUE_NA;      /* Z threshold  */
    device->zMode = VALUE_NA;           /* Z: linear, soft, hard log */
    device->screenNo = VALUE_NA;        /* Attached to X screen */
    device->common = common;            /* The commonPtr        */
    device->message.messageBuffer = NULL;
    device->pPressCurve = NULL;         /* initially no pressure curve. */
}

/***********************************************************************
 * AiptekOpenDriver --
 *
 * Opens the file descriptor associated with the Aiptek driver;
 * also performs axis allocations and normalization of drawing area 
 * parameters.
 *
 * This should be the only 'public' method invoked to open an Aiptek
 * Device Driver.
 */
static Bool
AiptekOpenDriver(
        DeviceIntPtr dev)
{
    const char *methodName = "AiptekOpenDriver";
    LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
    AiptekDevicePtr device = (AiptekDevicePtr) PRIVATE (dev);

    CHECK_DEVICE_INT_PTR (dev, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    DBG (2, xf86Msg (X_INFO, "%s: begins (%s)\n", methodName, local->name));

    if (AiptekOpenFileDescriptor (local) == BadMatch ||
        AiptekNormalizeSizeParameters (local) == BadMatch ||
        AiptekAllocateDriverAxes (dev) == BadMatch)
    {
        xf86Msg (X_ERROR, "%s: abends: failure opening/allocating driver\n",
            methodName);
        return BadMatch;
    }

    DBG (2, xf86Msg (X_INFO, "%s: ends successfully (%s)\n", methodName,
        local->name));
    return Success;
}

/***********************************************************************
 * AiptekOpenFileDescriptor
 *
 * Opens the UNIX file descriptor associated with an Aiptek Driver instance.
 * Deals with the issue of commonality of Aiptek Drivers' file descriptors
 * to their physical Aiptek tablet (e.g., the drivers all share the same
 * fd, a copy of which is kept in the common struct.)
 *
 * DO NOT CALL THIS ROUTINE DIRECTLY, with the exception of 
 * from AiptekOpenDriver().
 */
static Bool
AiptekOpenFileDescriptor(
        LocalDevicePtr local)
{
    const char *methodName = "AiptekOpenFileDescriptor";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    AiptekCommonPtr common = device->common;
    int i;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);
    CHECK_AIPTEK_COMMON_PTR (common, __LINE__);

    DBG (2, xf86Msg (X_INFO, "%s: begins (%s)\n", methodName, local->name));

    /* We're being asked to open a driver that already has an opened
     * file handle? Must be a redundency; play along.
     */
    if (AiptekIsValidFileDescriptor (local->fd) == Success)
    {
        DBG (2, xf86Msg (X_INFO, "%s: ends successfully (%s,fd=%d)\n",
             methodName, local->name, local->fd));
        return Success;
    }

    /* If we are the first device being opened for a physical tablet,
     * then we'll have no common struct assign. So, let's build one.
     */
    if (common == NULL)
    {
        if ((common = AiptekAssignCommon(local, device->devicePath)) == NULL)
        {
            xf86Msg (X_ERROR, "%s: abends on AiptekAssignCommon\n", methodName);
            return BadMatch;
        }
        device->common = common;
    }

  /* The common struct will have a valid fd if one of the other Aiptek 
   * driver instances associated with the physical tablet have already
   * been Open'ed. If so, we'll reuse that fd. Else, we're the first
   * Aiptek driver, and we have to do the opening.
   */
    if (AiptekIsValidFileDescriptor(common->fd) == Success)
    {
        local->fd = common->fd;
        DBG (2, xf86Msg (X_INFO, "%s: ends successfully (%s,fd=%d)\n",
                 methodName, local->name, local->fd));
        return Success;
    }

  /* Okay, we've done everything we could to prevent an actual UNIX
   * 'open' call being invoked. We've now no choice :-)
   */
    if (AiptekLowLevelOpen(local) != Success)
    {
        xf86Msg (X_ERROR, "%s: abends on AiptekLowLevelOpen\n", methodName);
        return BadMatch;
    }

  /* Move the file descriptor over to the common struct to prevent
   * subsequent opens of the physical tablet by other Aiptek driver
   * instances.
   */
    if (AiptekIsValidFileDescriptor(common->fd) == BadMatch)
    {
        common->fd = local->fd;
    }

  /* Copy the opened file descriptor over to any other Aiptek Driver
   * instances who share the same physical device, and have already
   * been assigned to this common struct.
   */
    for (i = 0; i < AIPTEK_MAX_DEVICES; ++i)
    {
        if (common->deviceArray[i] != NULL)
        {
            common->deviceArray[i]->fd = local->fd;
            common->deviceArray[i]->flags |= (XI86_CONFIGURED | XI86_POINTER_CAPABLE);
        }
    }

    DBG (2, xf86Msg (X_INFO, "%s: ends successfully (%s,fd=%d)\n",
           methodName, local->name, local->fd));
    return Success;
}

/***********************************************************************
 * AiptekGetTabletCapacity --
 *
 * Kludge alert - we allocate Axis before we know the tablet's abilities,
 * indeed, before the tablet is "officially" opened. So, what
 * we do is quietly open the tablet device driver, determine the physical
 * size, and close the file handle before the tablet is officially opened.
 */
static Bool
AiptekGetTabletCapacity(
        LocalDevicePtr local)
{
    const char *methodName = "AiptekGetTabletCapacity";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    AiptekCommonPtr common = device->common;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);
    CHECK_AIPTEK_COMMON_PTR (common, __LINE__);

    DBG (1, xf86Msg (X_INFO, "%s: entry (%s)\n", methodName, local->name));

    if (AiptekLowLevelOpen (local) == BadMatch ||
        close (local->fd) < 0)
    {
        xf86Msg (X_ERROR, "%s: abends (%s)\n", methodName, local->name);
        return BadMatch;
    }

  /* We're not open, we've never been opened, we know
   * nothing about any opened tablets :-)
   */
    local->fd = VALUE_NA;
    common->fd = VALUE_NA;
    DBG (1, xf86Msg (X_INFO, "%s: ends successfully (%s)\n", methodName,
        local->name));
    return Success;
}

/***********************************************************************
 * AiptekLowLevelOpen --
 *
 * This is the low-level call to open the file descriptor associated
 * with an Aiptek Device Driver instance. Not only do we open the
 * file descriptor, but we also perform the ioctl commands to learn
 * the physical tablet's capacities, which we store in the common
 * struct.
 *
 * DO NOT CALL THIS ROUTINE DIRECTLY -- use AiptekOpenFileDescriptor.
 * EXCEPTION: AiptekGetTabletCapacity().
 */
static Bool
AiptekLowLevelOpen(
        LocalDevicePtr local)
{
    int abs[5];
    unsigned long bit[EV_MAX][NBITS (KEY_MAX)];
    int i, j;
    int version;
    int err;
    const char *methodName = "AiptekLowLevelOpen";
    char name[256] = "Unknown";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    AiptekCommonPtr common = device->common;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);
    CHECK_AIPTEK_COMMON_PTR (common, __LINE__);

    DBG (1, xf86Msg (X_INFO, "%s: begins, (%s,path=%s)\n",
           methodName, local->name, common->devicePath));

    /* Do the low-level open(2) call.
    */
    local->fd = xf86OpenSerial (local->options);
    if (local->fd < 0)
    {
        xf86Msg (X_ERROR, "%s: abends: (%s,path=%s): %s\n",
             methodName, local->name, common->devicePath, strerror (errno));
        local->fd = VALUE_NA;
        common->fd = VALUE_NA;
        return BadMatch;
    }

    DBG (1, xf86Msg (X_INFO, "%s: Getting tablet info\n", methodName));

    SYSCALL (err, ioctl (local->fd, EVIOCGNAME (sizeof (name)), name));
    if (err < 0)
    {
        xf86Msg (X_ERROR, "%s: abends: (%s,path=%s) ioctl: %s\n",
         methodName, local->name, common->devicePath, strerror (errno));
        close (local->fd);
        local->fd = VALUE_NA;
        common->fd = VALUE_NA;
        return BadMatch;
    }

    if (strcmp (name, "Aiptek") != 0)
    {
        xf86Msg (X_ERROR,
             "%s: abends: (%s,path=%s) '%s' is NOT an Aiptek Tablet! "
             "(perhaps not the correct device path?)\n",
             methodName, local->name, common->devicePath, name);
        close (local->fd);
        local->fd = VALUE_NA;
        common->fd = VALUE_NA;
        return BadMatch;
    }

    SYSCALL (err, ioctl (local->fd, EVIOCGVERSION, &version));
    if (err < 0)
    {
        xf86Msg (X_ERROR, "%s: abends: (%s,path=%s) ioctl: %s\n",
             methodName, local->name, common->devicePath, strerror (errno));
        close(local->fd);
        local->fd = VALUE_NA;
        common->fd = VALUE_NA;
        return BadMatch;
    }
    xf86Msg (X_INFO, "%s: Linux Input Driver Version: %d.%d.%d\n",
       methodName, version >> 16, (version >> 8) & 0xff, version & 0xff);

    /* Pull up the tablet's capacities.
     */
    memset (bit, 0, sizeof (bit));
    SYSCALL (err, ioctl (local->fd, EVIOCGBIT (0, EV_MAX), bit[0]));
    if (err < 0)
    {
        xf86Msg (X_ERROR, "%s: abends: (%s,path=%s) ioctl: %s\n",
            methodName, local->name, common->devicePath, strerror (errno));
        close (local->fd);
        local->fd = VALUE_NA;
        common->fd = VALUE_NA;
        return BadMatch;
    }

    for (i = 0; i < EV_MAX; ++i)
    {
        if (TEST_BIT (i, bit[0]) != 0)
        {
#if 0
      /* Yes, we should check for error codes here. Mandrake 92
       * however returns an error code when we run i==4 by it.
       * That's because the array of event types, from MIN to MAX
       * are not contiguous values. Vanilla 2.4 and 2.6 do not
       * throw errors.
       */
            SYSCALL (err, ioctl (local->fd, EVIOCGBIT (i, KEY_MAX), bit[i]));
            if (err < 0) 
            {
                xf86Msg (X_ERROR, "%s: abends: (%s,path=%s) ioctl: %s\n",
                    methodName, local->name, common->devicePath,
                    strerror (errno));
                close (local->fd);
                local->fd = VALUE_NA;
                common->fd = VALUE_NA;
                return BadMatch;
            }
#else
            ioctl (local->fd, EVIOCGBIT (i, KEY_MAX), bit[i]);
#endif
            for (j = 0; j < KEY_MAX; ++j)
            {
                if (TEST_BIT (j, bit[i]))
                {
                    if (i == EV_ABS)
                    {
                        SYSCALL (err, ioctl (local->fd, EVIOCGABS (j), abs));
                        if (err < 0)
                        {
                            xf86Msg (X_ERROR,
                                "%s: abends: (%s,path=%s) ioctl: %s\n",
                                methodName, local->name,
                                common->devicePath, strerror (errno));
                            close (local->fd);
                            local->fd = VALUE_NA;
                            common->fd = VALUE_NA;
                            return BadMatch;
                        }
                        switch (j)
                        {
                            case ABS_X:
                            {
                                xf86Msg (X_INFO, "%s: Tablet's xMinCapacity=%d\n", methodName, abs[1]);
                                xf86Msg (X_INFO, "%s: Tablet's xMaxCapacity=%d\n", methodName, abs[2]);
                                common->xMinCapacity = abs[1];
                                common->xMaxCapacity = abs[2];
                            }
                            break;

                            case ABS_Y:
                            {
                                xf86Msg (X_INFO, "%s: Tablet's yMinCapacity=%d\n", methodName, abs[1]);
                                xf86Msg (X_INFO, "%s: Tablet's yMaxCapacity=%d\n", methodName, abs[2]);
                                common->yMinCapacity = abs[1];
                                common->yMaxCapacity = abs[2];
                            }
                            break;

                            case ABS_PRESSURE:
                            {
                                xf86Msg (X_INFO, "%s: Tablet's zMinCapacity=%d\n", methodName, abs[1]);
                                xf86Msg (X_INFO, "%s: Tablet's zMaxCapacity=%d\n", methodName, abs[2]);
                                common->zMinCapacity = abs[1];
                                common->zMaxCapacity = abs[2];
                            }
                            break;

                            case ABS_TILT_X:
                            {
                                xf86Msg (X_INFO, "%s: Tablet's xtiltMinCapacity=%d\n", methodName, abs[1]);
                                xf86Msg (X_INFO, "%s: Tablet's xtiltMaxCapacity=%d\n", methodName, abs[2]);
                                common->xtMinCapacity = abs[1];
                                common->xtMaxCapacity = abs[2];
                            }
                            break;

                            case ABS_TILT_Y:
                            {
                                xf86Msg (X_INFO, "%s: Tablet's ytiltMinCapacity=%d\n", methodName, abs[1]);
                                xf86Msg (X_INFO, "%s: Tablet's ytiltMaxCapacity=%d\n", methodName, abs[2]);
                                common->ytMinCapacity = abs[1];
                                common->ytMaxCapacity = abs[2];
                            }
                            break;

                            case ABS_WHEEL:
                            {
                                xf86Msg (X_INFO, "%s: Tablet's wheelMinCapacity=%d\n", methodName, abs[1]);
                                xf86Msg (X_INFO, "%s: Tablet's wheelMaxCapacity=%d\n", methodName, abs[2]);
                                common->wMinCapacity = abs[1];
                                common->wMaxCapacity = abs[2];
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    DBG (2, xf86Msg (X_INFO, "%s: ends successfully (%s,path=%s)\n",
           methodName, local->name, common->devicePath));
    return Success;
}

/***********************************************************************
 * AiptekNormalizeSizeParameters --
 *
 * Normalize active area parameters so they actually fit on the tablet.
 */
static Bool
AiptekNormalizeSizeParameters(
        LocalDevicePtr local)
{
    const char *methodName = "AiptekNormalizeSizeParameters";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    AiptekCommonPtr common = device->common;
    double tabletRatio, screenRatio, xFactor, yFactor;
    int gap, activeAreaType, message, xDiff, yDiff;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);
    CHECK_AIPTEK_COMMON_PTR (common, __LINE__);

    DBG (2, xf86Msg (X_INFO, "%s: begins\n", methodName));

  /* Check our active area parameters. We support the following
   * three sets of mutually exclusive parameter sets:
   *
   * 1) xMax/yMax. The origin {0,0} of the active area is the origin
   *    of the physical tablet. You therefore are describing the
   *    width and height of that active area.
   *
   * 2) xOffset/xSize,yOffset/ySize. The origin (0,0) of the active
   *    area is defined as (xOffset,yOffset) (which we'll report as
   *    {0,0}). The size of the active area in width and height are
   *    expressed in coordinates as xSize/ySize. That is to say,
   *    if xOffset=5; yOffset=5, and xSize=10; ySize=10, then we will 
   *    have an active area beginning at {5,5} and ending at {15,15}.
   *    Physical coordinate {5,5} is reported as {0,0}; {15,15} is 
   *    reported as {10,10}. The rest of the tablet is inert, as far as
   *    drawing area goes,
   *
   * 3) xTop/xBottom,yTop/yBottom. The difference between this and
   *    #2 above is that all four parameters are physical coordinates
   *    on the tablet. Using the example above, xTop=5; yTop=5, and
   *    xBottom=15; yBottom=15. It is inferred mathematically that
   *    the overall active area is 10x10 coordinates.
   */
    activeAreaType = AIPTEK_ACTIVE_AREA_NONE;
    if (device->xMax != VALUE_NA ||
        device->yMax != VALUE_NA)
    {
        activeAreaType |= AIPTEK_ACTIVE_AREA_MAX;
    }
    if (device->xSize != VALUE_NA ||
        device->ySize != VALUE_NA ||
        device->xOffset != VALUE_NA ||
        device->yOffset != VALUE_NA)
    {
        activeAreaType |= AIPTEK_ACTIVE_AREA_SIZE;
    }

    if (device->xTop != VALUE_NA ||
        device->yTop != VALUE_NA ||
        device->xBottom != VALUE_NA ||
        device->yBottom != VALUE_NA)
    {
        activeAreaType |= AIPTEK_ACTIVE_AREA_TOP;
    }

    /* Okay, we have a mask of which active area parameter(s)
    * you've used by their type. What we now test is whether
    * you have one and only one bit set.
    */
    if (activeAreaType != AIPTEK_ACTIVE_AREA_MAX &&
        activeAreaType != AIPTEK_ACTIVE_AREA_SIZE &&
        activeAreaType != AIPTEK_ACTIVE_AREA_TOP &&
        activeAreaType != AIPTEK_ACTIVE_AREA_NONE)
    {
        xf86Msg (X_ERROR,
             "%s: abends: Mutually exclusive active area parms!\n",
             methodName);
        return BadMatch;
    }

    /* We're now going to deal with the active area parameters specifying
    * an area larger than the actual tablet. We do this in three passes,
    * as there are three sets of active area parameters the user could've
    * used.
    *
    * The first of the bunch are the xMax/yMax set.
    */
    if (device->xMax != VALUE_NA ||
        device->yMax != VALUE_NA)
    {
        /* Deal with larger-than-tablet and NA values.
         */
        if (device->xMax > common->xMaxCapacity ||
            device->xMax == VALUE_NA)
        {
            device->xMax = common->xMaxCapacity;
            xf86Msg (X_CONFIG, "%s: xMax value unspecified/invalid; now %d\n",
               methodName, device->xMax);
        }
        if (device->yMax > common->yMaxCapacity ||
            device->yMax == VALUE_NA)
        {
            device->yMax = common->yMaxCapacity;
            xf86Msg (X_CONFIG, "%s: yMax value unspecified/invalid; now %d\n",
               methodName, device->yMax);
        }

    /* Internally we use xTop/yTop/xBottom/yBottom
     * for everything. It's the easiest for us to work
     * with, vis-a-vis filtering.
     */
        device->xTop = 0;
        device->yTop = 0;
        device->xBottom = device->xMax;
        device->yBottom = device->yMax;
    }

  /* We're now doing the same 'active area greater than the tablet'
   * tests as above, but now assuming the user has given us the
   * xOffset/yOffset; xSize/ySize parameters.
   */
    if (device->xSize != VALUE_NA ||
        device->ySize != VALUE_NA ||
        device->xOffset != VALUE_NA ||
        device->yOffset != VALUE_NA)
    {
        message = 0;
    /* Simple sanity tests: nothing larger than the
     * tablet; nothing negative, except for an NA value.
     */
        if ( device->xOffset != VALUE_NA &&
            (device->xOffset > common->xMaxCapacity ||
             device->xOffset < common->xMinCapacity))
        {
            message = 1;
            device->xOffset = 0;
        }
        if ( device->yOffset != VALUE_NA &&
            (device->yOffset > common->yMaxCapacity ||
             device->yOffset < common->yMinCapacity))
        {
            message = 1;
            device->yOffset = 0;
        }
        if ( device->xSize != VALUE_NA &&
            (device->xSize > common->xMaxCapacity ||
             device->xSize < common->yMinCapacity))
        {
            message = 1;
            device->xSize = common->xMaxCapacity;
        }
        if ( device->ySize != VALUE_NA &&
            (device->ySize > common->yMaxCapacity ||
             device->ySize < common->yMinCapacity))
        {
            message = 1;
            device->ySize = common->yMaxCapacity;
        }

        /* If one parameter is set but not the other, we'll
         * guess at something reasonable for the missing one.
         */
        if (device->xOffset == VALUE_NA ||
            device->xSize == VALUE_NA)
        {
            if (device->xOffset == VALUE_NA)
            {
                message = 1;
                device->xOffset = 0;
            }
            else
            {
                message = 1;
                device->xSize = common->xMaxCapacity - device->xOffset;
            }
        }
        if (device->yOffset == VALUE_NA ||
            device->ySize == VALUE_NA)
        {
            if (device->yOffset == VALUE_NA)
            {
                message = 1;
                device->yOffset = 0;
            }
            else
            {
                message = 1;
                device->ySize = common->yMaxCapacity - device->yOffset;
             }
        }

    /* Do not allow the active area to exceed the size of the
     * tablet. To do this, we have to consider both parameters.
     * Assumption: xOffset/yOffset is always correct; deliver less
     * of the tablet than they asked for, if the user asked for 
     * larger area than tablet can provide.
     */
        if ((device->xOffset + device->xSize) > common->xMaxCapacity)
        {
            message = 1;
            device->xSize = common->xMaxCapacity - device->xOffset;
        }
        if ((device->yOffset + device->ySize) > common->yMaxCapacity)
        {
            message = 1;
            device->ySize = common->yMaxCapacity - device->yOffset;
        }

    /* 'message' is used to indicate that we've changed some parameter
     * during our filtration process. It's conceivable that we may
     * have changed parameters several times, so we without commentary
     * to the very end.
     */
        if (message == 1)
        {
            xf86Msg (X_CONFIG,
                "%s: xOffset/yOffset: xSize/ySize are wrong/unspecified.\n",
                methodName);
            xf86Msg (X_CONFIG, "%s: xOffset now %d\n", methodName,
                device->xOffset);
            xf86Msg (X_CONFIG, "%s: yOffset now %d\n", methodName,
                device->yOffset);
            xf86Msg (X_CONFIG, "%s: xSize now %d\n", methodName, device->xSize);
            xf86Msg (X_CONFIG, "%s: ySize now %d\n", methodName, device->ySize);
        }

    /* Internally we use xTop/yTop/xBottom/yBottom
     * for everything. It's the easiest for us to work
     * with, vis-a-vis filtering.
     */
        device->xTop = device->xOffset;
        device->yTop = device->yOffset;
        device->xBottom = device->xOffset + device->xSize;
        device->yBottom = device->yOffset + device->ySize;
    }

  /* We're now doing the same 'active area greater than the tablet'
   * tests as above, but now assuming checking for the third set
   * of parameters the user can specify, the xTop/yTop/xBottom/yBottom
   * parameters.
   *
   * As all tablet size parameters are internally expressed as xTop/yTop,
   * etc., we'll do our 'greater than tablet' tests on the xTop/yTop
   * parameters we synthesized from the above tests as well. So, these
   * tests are being done NO MATTER WHAT 'size' parameters you actually
   * specified.
   */
    if (device->xTop == VALUE_NA ||
        device->xTop < common->xMinCapacity ||
        device->xTop > common->xMaxCapacity)
    {
        device->xTop = 0;
        xf86Msg (X_CONFIG, "%s: xTop invalid/unspecified: now %d\n",
             methodName, device->xTop);
    }
    if (device->yTop == VALUE_NA ||
        device->yTop < common->yMinCapacity ||
        device->yTop > common->yMaxCapacity)
    {
        device->yTop = 0;
        xf86Msg (X_CONFIG, "%s: yTop invalid/unspecified: now %d\n",
            methodName, device->yTop);
    }
    if (device->xBottom == VALUE_NA ||
        device->xBottom < common->xMinCapacity ||
        device->xBottom > common->xMaxCapacity)
    {
        device->xBottom = common->xMaxCapacity;
        xf86Msg (X_CONFIG, "%s: xBottom invalid/unspecified: now %d\n",
             methodName, device->xBottom);
    }
    if (device->yBottom == VALUE_NA ||
        device->yBottom < common->yMinCapacity ||
        device->yBottom > common->yMaxCapacity)
    {
        device->yBottom = common->yMaxCapacity;
        xf86Msg (X_CONFIG, "%s: yBottom invalid/unspecified: now %d\n",
             methodName, device->yBottom);
    }

  /* Determine the X screen we're going to be using.
   * If NA, or larger than the number of screens we
   * have, or negative, we've going for screen 0, e.g.,
   * 'default' screen.
   */
    if (device->screenNo >= screenInfo.numScreens ||
        device->screenNo == VALUE_NA ||
        device->screenNo < 0)
    {
        device->screenNo = 0;
        xf86Msg (X_CONFIG, "%s: ScreenNo invalid/unknown; now %d\n",
             methodName, device->screenNo);
    }

  /* Calculate the ratio according to KeepShape, TopX and TopY
   */
    if ((device->flags & KEEP_SHAPE_FLAG) != 0)
    {
        xDiff = common->xMaxCapacity - device->xTop;
        yDiff = common->yMaxCapacity - device->yTop;

        tabletRatio = (double) xDiff / (double) yDiff;
        screenRatio = (double) screenInfo.screens[device->screenNo]->width /
          (double) screenInfo.screens[device->screenNo]->height;

        DBG (2, xf86Msg (X_INFO,
             "%s: Screen %d: screenRatio = %.3g, tabletRatio = %.3g\n",
             methodName, device->screenNo, screenRatio, tabletRatio));

        if (screenRatio > tabletRatio)
        {
            gap = (int) ((double) common->yMaxCapacity *
                   (1.0 - tabletRatio / screenRatio));
            device->xBottom = common->xMaxCapacity;
            device->yBottom = common->yMaxCapacity - gap;
            DBG (2,
                xf86Msg (X_INFO, "%s: Screen %d: 'Y' Gap of %d computed\n",
                    methodName, device->screenNo, gap));
        }
        else
        {
            gap = (int) ((double) common->xMaxCapacity *
                    (1.0 - screenRatio / tabletRatio));
            device->xBottom = common->xMaxCapacity - gap;
            device->yBottom = common->yMaxCapacity;
            DBG (2,
                xf86Msg (X_INFO, "%s: Screen %d: 'X' Gap of %d computed\n",
                    methodName, device->screenNo, gap));
        }
    }

    xFactor = (double) screenInfo.screens[device->screenNo]->width /
                (double) (device->xBottom - device->xTop);
    yFactor = (double) screenInfo.screens[device->screenNo]->height /
                (double) (device->yBottom - device->yTop);

  /* Check threshold correctness
   */
    if (device->xThreshold > common->xMaxCapacity ||
        device->xThreshold < common->xMinCapacity ||
        device->xThreshold == VALUE_NA)
    {
        device->xThreshold = 0;
    }

    if (device->yThreshold > common->yMaxCapacity ||
        device->yThreshold < common->yMinCapacity ||
        device->yThreshold == VALUE_NA)
    {
        device->yThreshold = 0;
    }

    if (device->zThreshold > common->zMaxCapacity ||
        device->zThreshold < common->zMinCapacity ||
        device->zThreshold == VALUE_NA)
    {
        device->zThreshold = 0;
    }

    if (device->zMin < common->zMinCapacity ||
        device->zMin == VALUE_NA)
    {
        device->zMin = common->zMinCapacity;
    }

    if (device->zMax > common->zMaxCapacity ||
        device->zMax == VALUE_NA)
    {
        device->zMax = common->zMaxCapacity;
    }

    DBG (2, xf86Msg (X_INFO, "%s: ends successfully\n", methodName));
    return Success;
}

/***********************************************************************
 * AiptekAllocateDriverStructs --
 *
 * This method is used to allocate Axis Structs and set up callbacks.
 */
static Bool
AiptekAllocateDriverAxes(
        DeviceIntPtr dev)
{
    const char *methodName = "AiptekAllocateDriverAxes";
    LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    AiptekCommonPtr common = device->common;

    CHECK_DEVICE_INT_PTR (dev, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);
    CHECK_AIPTEK_COMMON_PTR (common, __LINE__);

    DBG (1, xf86Msg (X_INFO, "%s: begins\n", methodName));

    if (common == NULL)
    {
        xf86Msg (X_ERROR, "%s: abends - No common struct\n", methodName);
        return BadMatch;
    }

    /* Create axisStructs for every axis we support.
     * NOTE: min_resolution and max_resolution infers to
     * me a programmability to increase/decrease resolution.
     * We don't support that, so min & max = current_resolution.
     */
      InitValuatorAxisStruct (dev,              /* X resolution */
              0,                                /* axis_id */
              common->xMinCapacity,             /* min value */
              device->xBottom - device->xTop,   /* max value */
              LPI2CPM (500),                    /* resolution */
              0,                                /* min_resolution */
              LPI2CPM (500));                   /* max_resolution */

      InitValuatorAxisStruct (dev,              /* Y Resolution */
              1,                                /* axis_id */
              common->yMinCapacity,             /* min value */
              device->yBottom - device->yTop,   /* max value */
              LPI2CPM (500),                    /* resolution */
              0,                                /* min_resolution */
              LPI2CPM (500));                   /* max_resolution */

      InitValuatorAxisStruct (dev,              /* Pressure */
              2,                                /* axis_id */
              common->zMinCapacity,             /* min value */
              common->zMaxCapacity + 1,         /* max value */
              1,                                /* resolution */
              1,                                /* min resolution */
              1);                               /* max resolution */

      InitValuatorAxisStruct (dev,              /* xTilt */
              3,                                /* axis id */
              common->xtMinCapacity,            /* min value */
              common->xtMaxCapacity,            /* max value */
              1,                                /* resolution */
              1,                                /* min resolution */
              1);                               /* max resolution */

      InitValuatorAxisStruct (dev,              /* yTilt */
              4,                                /* axis id */
              common->ytMinCapacity,            /* min value */
              common->ytMaxCapacity,            /* max value */
              1,                                /* resolution */
              1,                                /* min resolution */
              1);                               /* max resolution */

      InitValuatorAxisStruct (dev,              /* wheel */
              5,                                /* axis id */
              common->wMinCapacity,             /* min value */
              common->wMaxCapacity,             /* max value */
              1,                                /* resolution */
              1,                                /* min resolution */
              1);                               /* max resolution */

    DBG (1, xf86Msg (X_INFO, "%s: ends successfully\n", methodName));
    return Success;
}

/***********************************************************************
 * AiptekDeleteDriverClasses --
 *
 * This beast frees the allocation of axis structs.
 */
static Bool
AiptekDeleteDriverClasses(
        DeviceIntPtr dev)
{
    const char *methodName = "AiptekDeleteDriverClasses";
    LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
    AxisInfoPtr axis;
    int i;

    CHECK_DEVICE_INT_PTR (dev, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);

    xf86Msg (X_INFO, "%s: begins\n", methodName);

  /* Axes are not allocated by InitValuatorAxisStruct, merely populated.
   * So, we'll clean the settings.
   */
    for (i = 0; i < 6; ++i)
    {
        axis = dev->valuator->axes + i;
        axis->min_value = 0;
        axis->max_value = 0;
        axis->resolution = 0;
        axis->min_resolution = 0;
        axis->max_resolution = 0;
    }
#if 0
    /* Drop the motion history buffer
     */
    if (local->motion_history != NULL)
    {
        xfree (local->motion_history);
        local->motion_history = NULL;
    }
    /* Now delete the different ClassPtr's owned by 'dev'
     */
    if (dev->key != NULL)
    {
        xfree (dev->key);
        dev->key = NULL;
    }
    if (dev->valuator != NULL)
    {
        xfree (dev->valuator);
        dev->valuator = NULL;
    }
    if (dev->button != NULL)
    {
        xfree (dev->button);
        dev->button = NULL;
    }
    if (dev->focus != NULL)
    {
        xfree (dev->focus);
        dev->focus = NULL;
    }
    if (dev->proximity != NULL)
    {
        xfree (dev->proximity);
        dev->proximity = NULL;
    }
    if (dev->kbdfeed != NULL)
    {
        xfree (dev->kbdfeed);
        dev->kbdfeed = NULL;
    }
    if (dev->ptrfeed != NULL)
    {
        xfree (dev->ptrfeed);
        dev->ptrfeed = NULL;
    }
    if (dev->intfeed != NULL)
    {
        xfree (dev->intfeed);
        dev->intfeed = NULL;
    }
    if (dev->stringfeed != NULL)
    {
        xfree (dev->stringfeed);
        dev->stringfeed = NULL;
    }
    if (dev->bell != NULL)
    {
        xfree (dev->bell);
        dev->bell = NULL;
    }
    if (dev->leds != NULL)
    {
        xfree (dev->leds);
        dev->leds = NULL;
    }
#endif
    xf86Msg (X_INFO, "%s: ends successfully\n", methodName);
    return Success;
}

/***********************************************************************
 * AiptekCloseDriver --
 *
 * Close the device by disassociating the Aiptek Driver instance from its
 * common struct. If no Drivers are subsequently associated with the common 
 * struct, then we want to also do a low-level close() on the file descriptor.
 */
static void
AiptekCloseDriver(
        LocalDevicePtr local)
{
    const char *methodName = "AiptekCloseDriver";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;

    /* Shut up the compiler's warnings */
    if (device)
        ;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    xf86Msg (X_INFO, "%s: begins\n", methodName);

    /* If we're asked to close a Driver that already has been
     * closed (e.g., the file descriptor is invalid), assume it is
     *  a paranoid redundency, and let it pass without comment.
     */
    if (AiptekIsValidFileDescriptor (local->fd) == Success)
    {
        /* Disassociate the Driver instance from it's common struct,
         * returning the number of still-attached Drivers. Only if that
         * number goes to zero, should we proceed to call the OS and
         * close the file descriptor.
         */
        if (AiptekDetachCommon (local) == 0)
        {
            close (local->fd);
        }
    }
    local->fd = VALUE_NA;
    /* xfree(device->devicePath);
     * device->devicePath = NULL;
     */

    xf86Msg (X_INFO, "%s: ends\n", methodName);
}

/***********************************************************************
 * AiptekChangeControl --
 *
 * Allow the user to change the tablet resolution -- we have an issue
 * insofar as we don't know how to write to the tablet. And furthermore,
 * even if we DID know how to write to the tablet, it doesn't support
 * a "change resolution" call. We tried to avoid this by claiming when
 * creating axisStructs that minRes = curRes = maxRes. So, we should never
 * get dispatched.
 */
static Bool
AiptekChangeControl(
        LocalDevicePtr local,
        xDeviceCtl * control)
{
    const char *methodName = "AiptekChangeControl";
    AiptekDevicePtr device;
    xDeviceResolutionCtl *res;
    int *resolutions;

    device = (AiptekDevicePtr) local->private;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    DBG (1, xf86Msg (X_INFO, "%s: begins\n", methodName));

    res = (xDeviceResolutionCtl *) control;

    if ((control->control != DEVICE_RESOLUTION) ||
        (res->num_valuators < 1))
    {
        xf86Msg (X_ERROR, "%s: abends\n", methodName);
        return BadMatch;
    }
    resolutions = (int *) (res + 1);

    DBG (3, xf86Msg (X_INFO, "%s: changing to resolution %d\n",
           methodName, resolutions[0]));

    DBG (1, xf86Msg (X_INFO, "%s: ends successfully\n", methodName));
    return Success;
}

/***********************************************************************
 * AiptekSwitchMode --
 *
 * Switches the mode between absolute or relative.
 */
static Bool
AiptekSwitchMode(
        ClientPtr client,
        DeviceIntPtr dev,
        int mode)
{
    const char *methodName = "AiptekSwitchMode";
    LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
    AiptekDevicePtr device = (AiptekDevicePtr) (local->private);

    CHECK_DEVICE_INT_PTR (dev, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    DBG (1, xf86Msg (X_INFO, "%s: begins: dev=0x%p mode=%d\n",
           methodName, (void *) dev, mode));

    if (mode == Absolute)
    {
        device->flags |= ABSOLUTE_FLAG;
    }
    else if (mode == Relative)
    {
        device->flags &= ~ABSOLUTE_FLAG;
    }
    else
    {
        xf86Msg (X_ERROR, "%s: dev=0x%p invalid mode=%d\n",
            methodName, (void *) dev, mode);
        return BadMatch;
    }

    DBG (1, xf86Msg (X_INFO, "%s: ends successfully\n", methodName));
    return Success;
}

/***********************************************************************
 * AiptekConvert
 * Convert valuators to X and Y. We deal with multiple X screens, adjusting
 * for xTop/xBottom/yTop/yBottom (or xSize/ySize).
 */
static Bool
AiptekConvert(
        LocalDevicePtr local,
        int first,
        int num,
        int v0,
        int v1,
        int v2,
        int v3,
        int v4,
        int v5,
        int *x,
        int *y)
{
    const char *methodName = "AiptekConvert";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    int xSize, ySize;
    int width, height;

  ScreenPtr pScreen = miPointerCurrentScreen();

  /* Change the screen number if it differs from that which
   * the pointer is currently on
   */
    if (pScreen->myNum != device->screenNo)
    {
        device->screenNo = pScreen->myNum;
    }

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    DBG (18, xf86Msg (X_INFO, "%s: begins, with: first=%d, num=%d, v0=%d, "
           "v1=%d, v2=%d, v3=%d,, v4=%d, v5=%d, x=%d, y=%d\n",
           methodName, first, num, v0, v1, v2, v3, v4, v5, *x, *y));

    if (first != 0 || num == 1)
    {
        return BadMatch;
    }

    xSize = device->xBottom - device->xTop;
    ySize = device->yBottom - device->yTop;

    width = screenInfo.screens[device->screenNo]->width;
    height = screenInfo.screens[device->screenNo]->height;

    *x = (v0 * width) / xSize;
    *y = (v1 * height) / ySize;

    /* Deal with coordinate inversion
    */
    if ((device->flags & INVX_FLAG) != 0)
    {
        *x = width - *x;
    }
    if ((device->flags & INVY_FLAG) != 0)
    {
        *y = height - *y;
    }

    /* Normalize the adjusted sizes.
    */
    if (*x < 0)
    {
        *x = 0;
    }
    if (*x > width)
    {
        *x = width;
    }

    if (*y < 0)
    {
        *y = 0;
    }
    if (*y > height)
    {
        *y = height;
    }

    xf86XInputSetScreen (local, device->screenNo, *x, *y);
    DBG (18, xf86Msg (X_INFO, "%s: ends successfully, with: x=%d, y=%d\n",
           methodName, *x, *y));

    /* return Success; */
    return 1;
}

/***********************************************************************
 * AiptekReverseConvert --
 *
 * Convert X and Y to valuators.
 */
static Bool
AiptekReverseConvert(
        LocalDevicePtr local,
        int x,
        int y,
        int *valuators)
{
    const char *methodName = "AiptekReverseConvert";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    int xSize, ySize;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    DBG (18, xf86Msg (X_INFO, "%s: begins, with: x=%d, y=%d, "
           "valuators[0]=%d, valuators[1]=%d\n",
           methodName, x, y, valuators[0], valuators[1]));

    /* Adjust by tablet ratio
     */
    xSize = device->xBottom - device->xTop;
    ySize = device->yBottom - device->yTop;

    valuators[0] = (x * xSize) / screenInfo.screens[device->screenNo]->width;
    valuators[1] = (y * ySize) / screenInfo.screens[device->screenNo]->height;

    DBG (18, xf86Msg (X_INFO, "%s: Converted x,y (%d, %d) to (%d, %d)\n",
           methodName, x, y, valuators[0], valuators[1]));

    xf86XInputSetScreen (local, device->screenNo, valuators[0], valuators[1]);
    DBG (18, xf86Msg (X_INFO, "%s: ends successfully, with valuators(%d,%d)\n",
           methodName, valuators[0], valuators[1]));

    return Success;
}

/***************************************************************************
 * AiptekReadInput --
 *
 * Read the new events from the device, and enqueue them. Since a singular
 * file descriptor is associated with up to three X devices, any
 * of those three drivers may be awaken and dispatched by X's select loop.
 *
 * 1) The Cursor device therefore might receive events for the Eraser;
 *    it must be aware of how to dispatch to more devices than itself.
 * 2) The Cursor needs to the aware that if there is no Eraser device
 *    associated with this physical tablet, it needs to throw out Eraser
 *    events.
 */
static void
AiptekReadInput(
        LocalDevicePtr local)
{
    const char *methodName = "AiptekReadInput";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    AiptekCommonPtr common = device->common;
    struct input_event *event;
    ssize_t currRead;
    ssize_t totalRead;
    int i, id;
    char eventBuf[sizeof (struct input_event) * MAX_EVENTS];
    int eventsInMessage;
    int deviceIdArray[] = 
    {
        STYLUS_ID, CURSOR_ID, ERASER_ID,
        STYLUS_ID, CURSOR_ID, ERASER_ID
    };
    int start, j;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);
    CHECK_AIPTEK_COMMON_PTR (common, __LINE__);

  /* Read up to MAX_EVENTS from the tablet. Disallow any partial reads
   * (e.g., we don't want any fractional input_events). We'll continue
   * to read until we get at least one full input_event. The likelihood
   * of being caught in here not reading a full input_event is low, given
   * that we're dispatched by select(), and we know that on the kernel
   * side, only full input_events are emitted. Good paranoia.
   */
    totalRead = 0;
    do
    {
        SYSCALL (currRead, 
            read (local->fd,
                  eventBuf + totalRead,
                  sizeof (eventBuf) - totalRead));

        if (currRead < 0)
        {
            xf86Msg (X_ERROR, "%s: abends: read: %s\n", methodName,
                strerror (errno));
            return;
        }
        totalRead += currRead;
    }
    while ((totalRead % sizeof (struct input_event)) != 0);

  /* Parse the events read from the tablet.
   *
   * We switch from one device to another through the auspices
   * of button events. We'll allow that from the standpoint that
   * we'll allow device-selecting events only if the underlying
   * driver for that device is loaded/opened.
   *
   * All positional reports are handled as absolute reports.
   * If they are relative, we adjust them into absolute.
   *
   * Later, we'll convert those reports back to relative if Xorg
   * has the driver configured in relative mode. This allows for the
   * kernel driver and the X driver to be out of sync WRT
   * absolute/relative coordinates, and yet still work.
   *
   * Actually, it allows the X user to set his tablet to relative
   * mode without the worry of also going to the kernel driver and
   * configuring it to agree with the Xorg driver.
   */
    eventsInMessage = 0;
    for (event = (struct input_event *) (eventBuf);
         event < (struct input_event *) (eventBuf + totalRead);
         ++event)
    {
        /* Unprocessed events:
         * ABS_RZ       - Rotate Stylus
         * ABS_DISTANCE - Related to the Wheel?
         * ABS_THROTTLE - Related to the Wheel?
         *
         * Synthesized events
         * ABS_X_TILT - The aiptek tablet does not report these,
         * ABS_Y_TILT - but the Linux kernel driver sends synthetic values.
         * ABS_WHEEL  - Ditto.
         * REL_WHEEL  - Ditto.
         */
        switch (event->type)
        {
            case EV_ABS:
            {
                eventsInMessage = AiptekHandleAbsEvents(
                        eventsInMessage, event, common);
            }
            break;

            case EV_REL:
            {
                eventsInMessage = AiptekHandleRelEvents(
                        eventsInMessage, event, common);
            }
            break;

            case EV_KEY:
            {
                eventsInMessage = AiptekHandleKeyEvents(
                        eventsInMessage, event, common);
            }
            break;
        }

        /* We have two potential event terminators. EV_MSC was used
         * by (unwritten) convention to indicate the end-of-report.
         * Problem is, EV_MSC is supposed to actually report data,
         * so a new event type, EV_SYN, was created in Linux 2.5.x
         * expressively for this purpose.
         *
         * Theoretically, if EV_SYN is defined, we should only terminate
         * the population of device->currentValues struct IFF we receive
         * that event. The fact of the matter is, the EV_MSC is assumed
         * to be an ugliness that will take some time to be deprecated.
         * For the nonce, we'll support both. But, if you have a tablet
         * that's actually supplying something interesting with EV_MSC,
         * this is obviously some code that will require modifications.
         */
        if ((event->type == EV_MSC || event->type == EV_SYN) &&
            eventsInMessage)
        {
            /* We've seen EV_MSCs in the incoming data trail with no
             * other message types in-between. We use 'eventsInMessage'
             * to count all 'real' messages in-between. If there were none,
             * do NOT copy common->currentValues to common->previousValues
             * (as this will make the jitter filter useless). Just go and
             * read the subsequent events.
             *
             * This filter throws out reports that do not meet minimum threshold
             * requirements for movement along that axis.
             *
             * Presently, we discard the entire report if any dimension of the
             * currentValues struct does not meet it's minimum threshold.
             *
             * Also, we only do the comparison IFF a threshold has been set
             * for that given dimension.
             */

            /* If this report somehow has (almost) exactly the same readings
             * as the previous report for all dimensions, throw the 
             * report out.
             */
            if (abs(common->currentValues.x - common->previousValues.x) <= 
                            device->xThreshold &&
                abs(common->currentValues.y - common->previousValues.y) <= 
                            device->yThreshold &&
                abs(common->currentValues.z - common->previousValues.z) <=
                            device->zThreshold &&
                (common->currentValues.proximity == 
                            common->previousValues.proximity) &&
                (common->currentValues.button ==
                            common->previousValues.button) &&
                (common->currentValues.macroKey ==
                            common->previousValues.macroKey))
            {
                DBG (13, xf86Msg (X_INFO, "%s: Event Filtered Out\n",
                    methodName));
                continue;
            }

            /* What do we do if we have an event for a device that
             * is closed? Do we throw it out or make an attempt to reroute
             * to another device? We will attempt to reroute it. Otherwise, the
             * tablet becomes silently inert, which isn't good.
             *
             * As we're using powers of 2 for values in deviceIdArray,
             * figuring out the starting element thus works fine.
             */
            start = common->currentValues.eventType / 2;

            /* This allows us to track that all devices are closed.
             */
            common->currentValues.eventType = 0;

            for (i = deviceIdArray[start], j = 0;
                 j < AIPTEK_MAX_DEVICES;
                 i = deviceIdArray[j], ++j)
            {
                if (AiptekIsEventValid (common, i) == Success)
                {
                    common->currentValues.eventType = /* 1 << */ i;
                    break;
                }
            }
            /* What if ALL devices are closed? Well, pretend it's a stylus,
             * then...
             */
            if (common->currentValues.eventType == 0)
            {
                common->currentValues.eventType = STYLUS_ID;
            }
            /* Put up a message if we re-route a message.
             */
            if (deviceIdArray[start] != deviceIdArray[j])
            {
                DBG (3, xf86Msg (X_INFO, 
                    "%s: Event for closed device! Rerouted\n", methodName));
            }

            /* Dispatch events to one of our configured devices.
             */
            for (i = 0; i < AIPTEK_MAX_DEVICES; ++i)
            {
                /* Do not consider processing null device slots or
                 * device drivers that are not open.
                 */
                if (common->deviceArray[i] != NULL &&
                    AiptekIsValidFileDescriptor(common->deviceArray[i]->fd)
                        == Success)
                {
                    id = ((AiptekDevicePtr)common->deviceArray[i]->
                            private)->deviceId;

                    if (DEVICE_ID (common->currentValues.eventType) == 
                            DEVICE_ID (id))
                    {
                        AiptekSendEvents (common->deviceArray[i]);
                        break;
                    }
                }
            }

            /* Move the events we've just parsed/dispatched into
             * previous values, and go read some more.
             */
            memmove (&common->previousValues, &common->currentValues,
                sizeof (AiptekStateRec));
            common->currentValues.macroKey = VALUE_NA;
        }
    }
}

/***********************************************************************
 * Parse and handle Absolute Events into the common->currentValues
 * struct. This effectively makes AiptekReadInput a more manageable function
 * than by having nested switch-case's in a while loop.
 */

static int
AiptekHandleAbsEvents(
        int eventsInMessage,
        struct input_event *event,
        AiptekCommonPtr common)
{
    switch (event->code)
    {
        case ABS_X:
        {
            ++eventsInMessage;
            common->currentValues.x = event->value;
        }
        break;

        case ABS_Y:
        {
            ++eventsInMessage;
            common->currentValues.y = event->value;
        }
        break;

        case ABS_PRESSURE:
        {
            ++eventsInMessage;
            common->currentValues.z = event->value;
        }
        break;

        case ABS_TILT_X:
        case ABS_RZ:
        {
            ++eventsInMessage;
            common->currentValues.xTilt = event->value;
        }
        break;

        case ABS_TILT_Y:
        {
            ++eventsInMessage;
            common->currentValues.yTilt = event->value;
        }
        break;

        case ABS_DISTANCE:
        {
            ++eventsInMessage;
            common->currentValues.distance = event->value;
        }
        break;

        case ABS_WHEEL:
        case ABS_THROTTLE:
        {
            ++eventsInMessage;
            common->currentValues.wheel = event->value;
        }
        break;

        case ABS_MISC:
        {
            /* We have an agreement with the
             * Linux Aiptek HID driver (which we also wrote) to send
             * the proximity bit as bit 0 of the ABS_MISC value.
             *
             * The tool through which we get the report,
             * the tablet's Stylus or Mouse device, is mapped
             * to bits 4..7. Presently, the X driver does not
             * use that information. Which means, if the driver
             * is presently routing all events to, say,
             * CURSOR_ID, then we don't mind the user actually
             * using the stylus tool on the tablet for it.
             */
            ++eventsInMessage;
            common->currentValues.proximity = 
                PROXIMITY(event->value) != 0 ? 1 : 0;
        }
        break;
    }
    return eventsInMessage;
}

/***********************************************************************
 * Parse and handle Relative Events into the common->currentValues
 * struct. This effectively makes AiptekReadInput a more manageable function
 * than by having nested switch-case's in a while loop.
 */
static int
AiptekHandleRelEvents(
        int eventsInMessage,
        struct input_event *event,
        AiptekCommonPtr common)
{
    switch (event->code)
    {
        case REL_X:
        {
            /* Normalize all relative events into absolute
             * coordinates.
             */
            ++eventsInMessage;
            common->currentValues.x = common->previousValues.x + event->value;
        }
        break;

        case REL_Y:
        {
            /* Normalize all relative events into absolute
             * coordinates.
             */
            ++eventsInMessage;
            common->currentValues.y = common->previousValues.y + event->value;
        }
        break;

        case REL_WHEEL:
        {
            /* Normalize all relative events into absolute
             * coordinates.
             */
            ++eventsInMessage;
            common->currentValues.wheel = 
                common->previousValues.wheel + event->value;
        }
        break;

        case REL_MISC:
        {
            /* We have an agreement with the
             * Linux Aiptek HID driver to send
             * the proximity bit through REL_MISC.
             * We do this solely if proximity is
             * being reported through the Mouse tool;
             * else, if stylus, we'll get proximity through
             * bits 0..3 of the ABS_MISC report.
             *
             * The tool through which we get the report,
             * the tablet's Stylus or Mouse device, is mapped
             * to bits 4..7. Presently, the X driver does not
             * use that information. Which means, if the driver
             * is presently routing all events to, say,
             * CURSOR_ID, then we don't mind the user actually
             * using the stylus tool on the tablet for it.
             */
            ++eventsInMessage;
            common->currentValues.proximity =
                PROXIMITY (event->value) != 0 ? 1 : 0;
        }
        break;
    }
    return eventsInMessage;
}

/***********************************************************************
 * Parse and handle Keyboard Events into the common->currentValues
 * struct. This effectively makes AiptekReadInput a more manageable function
 * than by having nested switch-case's in a while loop.
 */
static int
AiptekHandleKeyEvents(
        int eventsInMessage,
        struct input_event *event,
        AiptekCommonPtr common)
{
    switch (event->code)
    {
        /* Events that begin with btn_tool_pen .. 
         * btn_tool_airbrush are from the stylus. Further, they
         * indicate that the user wants input to be dispatched
         * to the STYLUS_ID device. Which, we'll do, IF said device
         * is presently open. Otherwise, no, we won't, we'll stay
         * with the current device.
         */
        case BTN_TOOL_PEN:
        case BTN_TOOL_PENCIL:
        case BTN_TOOL_BRUSH:
        case BTN_TOOL_AIRBRUSH:
        {
            if (AiptekIsEventValid (common, STYLUS_ID) == Success)
            {
                ++eventsInMessage;
                common->currentValues.eventType = STYLUS_ID;
            }
        }
        break;

       /* Events that begin with btn_tool_rubber are from the
        * eraser. Further, they indicate that the user wants
        * input to be dispatched to the ERASER_ID device. Which,
        * we'll do, IF said device is presently open. Otherwise,
        * no, we won't, we'll stay with the current device.
        */
        case BTN_TOOL_RUBBER:
        {
            if (AiptekIsEventValid (common, ERASER_ID) == Success)
            {
                ++eventsInMessage;
                common->currentValues.eventType = ERASER_ID;
            }
        }
        break;

       /* Events that begin with btn_tool_mouse-btn_tool_lens 
        * are from the cursor. Further, they indicate that the
        * user wants input to be dispatched to the CURSOR_ID
        * device. Which, we'll do, IF said device is presently
        * open. Otherwise, no, we won't, we'll stay with the
        * current device.
        */
        case BTN_TOOL_MOUSE:
        case BTN_TOOL_LENS:
        {
            if (AiptekIsEventValid (common, CURSOR_ID) == Success)
            {
                ++eventsInMessage;
                common->currentValues.eventType = CURSOR_ID;
            }
        }
        break;

       /* Remaining button events are just that -- button events.
        * We get them, we report them.
        */
        case BTN_TOUCH:
        {
            ++eventsInMessage;
            if (event->value > 0)
            {
                common->currentValues.button |= BUTTONS_EVENT_TOUCH;
            }
            else
            {
                common->currentValues.button &= ~BUTTONS_EVENT_TOUCH;
            }
        }
        break;

        case BTN_STYLUS:
        {
            ++eventsInMessage;
            if (event->value != 0)
            {
                common->currentValues.button |= BUTTONS_EVENT_STYLUS;
            }
            else
            {
                common->currentValues.button &= ~BUTTONS_EVENT_STYLUS;
            }
        }
        break;

        case BTN_STYLUS2:
        {
            ++eventsInMessage;
            if (event->value != 0)
            {
                common->currentValues.button |= BUTTONS_EVENT_STYLUS2;
            }
            else
            {
                common->currentValues.button &= ~BUTTONS_EVENT_STYLUS2;
            }
        }
        break;

       /* Normal Mouse button handling: LEFT, RIGHT and
        * MIDDLE all buttons that we'll report to X
        * as normal buttons. Note that the damned things
        * re-use the same bitmasks as the Stylus buttons,
        * above.
        */
        case BTN_LEFT:
        {
            ++eventsInMessage;
            if (event->value != 0)
            {
                common->currentValues.button |= BUTTONS_EVENT_MOUSE_LEFT;
            }
            else
            {
                common->currentValues.button &= ~BUTTONS_EVENT_MOUSE_LEFT;
            }
        }
        break;

        case BTN_MIDDLE:
        {
            ++eventsInMessage;
            if (event->value != 0)
            {
                common->currentValues.button |= BUTTONS_EVENT_MOUSE_MIDDLE;
            }
            else
            {
                common->currentValues.button &= ~BUTTONS_EVENT_MOUSE_MIDDLE;
            }
        }
        break;

        case BTN_RIGHT:
        {
            ++eventsInMessage;
            if (event->value != 0)
            {
                common->currentValues.button |= BUTTONS_EVENT_MOUSE_RIGHT;
            }
            else
            {
                common->currentValues.button &= ~BUTTONS_EVENT_MOUSE_RIGHT;
            }
        }
        break;

        case BTN_SIDE:
        {
            ++eventsInMessage;
            if (event->value != 0)
            {
                common->currentValues.button |= BUTTONS_EVENT_SIDE_BTN;
            }
            else
            {
                common->currentValues.button &= ~BUTTONS_EVENT_SIDE_BTN;
            }
        }
        break;

        case BTN_EXTRA:
        {
            ++eventsInMessage;
            if (event->value != 0)
            {
                common->currentValues.button |= BUTTONS_EVENT_EXTRA_BTN;
            }
            else
            {
                common->currentValues.button &= ~BUTTONS_EVENT_EXTRA_BTN;
            }
        }
        break;

        /* Any other EV_KEY event is assumed to be
         * the pressing of a macro key from the tablet.
         */
        default:
        {
            ++eventsInMessage;
            common->currentValues.macroKey = event->value;
        }
        break;
    }
    return eventsInMessage;
}

/***********************************************************************
 * AiptekIsEventValid --
 *
 * Return indication that the event for device 'x' is valid. Validity is
 * defined as an Xorg driver instance for the given device, and that said
 * driver is in an open state.
 */
static Bool
AiptekIsEventValid(
        AiptekCommonPtr common,
        int code)
{
    int i, id;

    CHECK_AIPTEK_COMMON_PTR (common, __LINE__);

    for (i = 0; i < AIPTEK_MAX_DEVICES; ++i)
    {
        if (common->deviceArray[i] != NULL &&
            AiptekIsValidFileDescriptor(common->deviceArray[i]->fd) == Success)
        {
            id = ((AiptekDevicePtr)common->deviceArray[i]->private)->deviceId;
            if (DEVICE_ID (code) == DEVICE_ID (id))
            {
                return Success;
            }
        }
    }
    return BadMatch;
}

/**********************************************************************
 * AiptekSendEvents --
 *
 * Send events according to the device state.
 */
static void
AiptekSendEvents(
        LocalDevicePtr local)
{
    const char *methodName = "AiptekSendEvents";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    AiptekCommonPtr common = device->common;
    int bCorePointer, bAbsolute;
    int i, x, y, z, xTilt, yTilt, wheel;
    int delta, id;

    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);
    CHECK_AIPTEK_COMMON_PTR (common, __LINE__);

    if (DEVICE_ID (device->deviceId) !=
        DEVICE_ID (common->currentValues.eventType))
    {
        xf86Msg (X_ERROR, "%s: not the same device type (%u,%u)\n",
            methodName,
            DEVICE_ID (device->deviceId), common->currentValues.eventType);
        return;
    }

    bAbsolute = device->flags & ABSOLUTE_FLAG;
    bCorePointer = xf86IsCorePointer (local->dev);

    /* Normalize X and Y coordinates. This includes dealing
    * with absolute/relative coordinate mode.
    */
    if (bAbsolute != 0)
    {
        x = common->currentValues.x;
        y = common->currentValues.y;

        /* Translate coordinates according to Top and Bottom points.
        */
        if (x > device->xBottom)
        {
            x = device->xBottom;
        }

        if (y > device->yBottom)
        {
            y = device->yBottom;
        }

        if (device->xTop > 0)
        {
            DBG (3, xf86Msg (X_INFO, "%s: Adjusting x, with xTop=%d\n",
                 methodName, device->xTop));
            x = device->xTop;
        }

        if (device->yTop > 0)
        {
            DBG (3, xf86Msg (X_INFO, "%s: Adjusting y, with yTop=%d\n",
                 methodName, device->yTop));
            y = device->yTop;
        }

        if (x < 0)
        {
            x = 0;
        }
        if (y < 0)
        {
            y = 0;
        }
    }
    else
    {
        if (common->previousValues.proximity != 0)
        {
            x = common->currentValues.x - common->previousValues.x;
            y = common->currentValues.y - common->previousValues.y;
        }
        else
        {
            x = 0;
            y = 0;
        }
    }
    z = common->currentValues.z;
    xTilt = common->currentValues.xTilt;
    yTilt = common->currentValues.yTilt;
    wheel = common->currentValues.wheel;

    /* Deal with pressure min..max, which differs from threshold.
     */
    if (z < device->zMin)
    {
        z = 0;
    }

    if (z > device->zMax)
    {
        z = device->zMax;
    }

    /* Transform the pressure with the presscurve
     */
    AiptekLookupPressureValue (local, &(common->currentValues));

    /* First, handle the macro keys.
     */
    if (common->currentValues.macroKey != VALUE_NA)
    {
        /* This is a little silly, but: The Linux Event Input
         * system uses a slightly different keymap than does X 
         * (it also has more keys defined). So, we have to go
         * through a translation process. It's made sillier than
         * required because X wants an offset to it's KeySym table,
         * rather than an event key -- it'll do it's own lookup.
         * It DOES support arbitrary ordering of key events, and
         * partial keyboard matrices, so that speaks in favor of this
         * scheme.
         */
        for (i = 0;
            i < sizeof (linux_inputDevice_keyMap) / 
                sizeof (linux_inputDevice_keyMap[0]);
            ++i)
        {
            if (linux_inputDevice_keyMap[i] == common->currentValues.macroKey)
            {
                /* First available Keycode begins at 8 => macro+7.
                 */
                /* Keyboard 'make' (press) event */
                xf86PostKeyEvent (local->dev, i + 7, 1,
                                  bAbsolute != 0, 0, 6,
                                  x, y, /* common->currentValues.button, */ z,
                                  xTilt, yTilt, wheel);

                /* Keyboard 'break' (depress) event */
                xf86PostKeyEvent (local->dev, i + 7, 0,
                                  bAbsolute != 0, 0, 6,
                                  x, y, /* common->currentValues.button, */ z,
                                  xTilt, yTilt, wheel);
                break;
            }
        }
    }

    /* As the coordinates are ready, we can send events to X 
     */
    if (common->currentValues.proximity != 0)
    {
        if (common->previousValues.proximity == 0)
        {
            if (bCorePointer == 0)
            {
                xf86PostProximityEvent (local->dev, 1, 0, 6, x, y, z,
                                        xTilt, yTilt, wheel);
            }
        }

        if ((bAbsolute != 0 &&
            (common->previousValues.x != common->currentValues.x ||
             common->previousValues.y != common->currentValues.y ||
             common->previousValues.z != common->currentValues.z)) ||
            bAbsolute == 0)
        {
            if (bAbsolute == 0)
            {
                /* Weirdly, X needs it to be 1,1 for the pointer
                 * not to move in relative mode
                 */
                ++x;
                ++y;
            }

            xf86PostMotionEvent (local->dev, bAbsolute != 0, 0, 6,
                                 x, y, z, xTilt, yTilt, wheel);
        }
    
        delta = common->currentValues.button ^ common->previousValues.button;
        while (delta)
        {
            id = ffs (delta);

            DBG( 10, xf86Msg(X_INFO, 
                "arguments %p, %i, %i, %i, %i, %i, %i, %i, %i, %i\n", 
                (void*)(local->dev), bAbsolute != 0, id,
                (common->currentValues.button & (1 << (id - 1))) != 0, 
                x, y, z, xTilt, yTilt, wheel));
      
            delta &= ~(1 << (id - 1));
            xf86PostButtonEvent (local->dev, bAbsolute != 0, id,
                    (common->currentValues.button & (1 << (id - 1))) != 0, 
                    0, 6, x, y, z, xTilt, yTilt, wheel);
        }
    }
    else
    {
        if (bCorePointer == 0)
        {
            if (common->previousValues.proximity != 0)
            {
                xf86PostProximityEvent (local->dev, 0, 0, 6, x, y, z,
                    xTilt, yTilt, wheel);
            }
        }
        common->previousValues.proximity = 0;
    }
}

/*======================================================================
 *
 * Bidirectional Callback methods are located here
 */

/***********************************************************************
 * AiptekPtrFeedbackHandler --
 *
 * This routine is called whenever the client sends a PtrFeedback
 * message to the driver. Now, Aiptek overrides the meaning of the
 * PtrCtrl struct, so as to implement bidirectional communication
 * between the client and this driver. This is the entry point
 * for all communications.
 */
static void
AiptekPtrFeedbackHandler(
        DeviceIntPtr dev,
        PtrCtrl * ptrFeedback)
{
    LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    AiptekFeedbackPtr feedback = (AiptekFeedbackPtr) & ptrFeedback->num;
    AiptekMessagePtr message = &device->message;

    CHECK_DEVICE_INT_PTR (dev, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    /* First, we have a problem insofar as there's nothing in the
    * X driver identifier to ID the device driver associated with it.
    * name is used for the device identifier; type_name contains info
    * on whether it's a stylus, eraser, cursor. So, how do we it's an
    * Aiptek? We send a stupid handshake, and expect another one in
    * reply...
    *
    * EXPECTS: "DRIVER"
    * REPLIES: "AIPTEK"
    *
    * And, oh! cute! they're both exactly 6 bytes in length, which happens
    * to be the exact size of usable characters in the PtrCtrl buffer! How
    * did that happen? :-)
    */
    AiptekDemarshallRawPacket (feedback, message);

    if (strncmp (message->marshallBuf, "DRIVER", 6) == 0)
    {
        /* Build a reply message and marshall it back into 'feedback'
         */
        memcpy (message->marshallBuf, "AIPTEK", 6);
        AiptekMarshallRawPacket (feedback, message);
        return;
    }

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    * Now we're busy determining our handshake state between self
    * and the client. Check for error conditions first, in numbing
    * detail :-)
    */

    /* Deal with startup condition (no previous command nor reply)
     */
    if (message->messageStatus == MESSAGE_STATUS_OFF) 
    {
        AiptekHandleMessageBegin(feedback, message);
    }
    /* Deal with situation where we have a reply ready for the client;
     * hopefully it's coming in asking to download it.
     */
    else if (message->messageStatus == MESSAGE_STATUS_AWAIT_REPLY) 
    {
        AiptekHandleAwaitReplyStatus(feedback, message);
    }

    /* MESSAGE_STATUS enums have the same values/meaning
     * as FEEDBACK_DIRECTION enums. The significance is, we'll
     * very quickly know when the conversation is out of phase.
     */
    if (message->messageStatus != 
        feedback->manifest.controlPacket.feedbackDirection)
    {
        /* We're out of sync -- send nastigram message back
         */
        AiptekInitMessageStruct(message, MESSAGE_STATUS_OFF);
        feedback->manifest.controlPacket.feedbackStatus = FEEDBACK_STATUS_NAK;
        return;
    }

    /* Now let's see if we have an incoming command or an outgoing reply.
     * Because we cannot send multiple REPLY messages of our own volition,
     * the client "knows to request REPLY packets from us until we're 
     * unwound. Then we free our buffer and await a new COMMAND.
     */
    if (feedback->manifest.controlPacket.feedbackDirection == 
        FEEDBACK_DIRECTION_COMMAND)
    {
        AiptekHandleDirectionCommandPacket(dev, feedback, message);
    }
    else if (feedback->manifest.controlPacket.feedbackDirection ==
        FEEDBACK_DIRECTION_REPLY)
    {
        AiptekHandleDirectionReplyPacket(dev, feedback, message);
    }
}

/***********************************************************************
 * Handle the beginning of a message sequence. Basically, we're concerned
 * that the client doesn't begin a sequence requesting data -- it first has
 * to ask for the data.
 */
static void
AiptekHandleMessageBegin(
        AiptekFeedbackPtr feedback,
        AiptekMessagePtr message)
{
    /* Await_reply is our transitive state indicator which signals
     * that we've been sent a query command, have built the reply string,
     * and am now awaiting for the client to being request/download the
     * reply string. So, if we're at packet 0, and the client is asking for
     * reply string, but we're not awaiting a reply request, something's gone
     * wrong. Else, we're cool.
     */
    if (feedback->manifest.controlPacket.feedbackDirection == 
                FEEDBACK_DIRECTION_REPLY)
    {
        /* Client is asking for a reply, but we don't know what we're
         * replying to. Send an error code.
         */
        AiptekInitMessageStruct(message, MESSAGE_STATUS_OFF);
        feedback->manifest.controlPacket.feedbackStatus = FEEDBACK_STATUS_NAK;
    }
    else
    {
        message->messageStatus = MESSAGE_STATUS_COMMAND;
    }
}

/***********************************************************************
 * Handle the transitive state between the client sending a command, which
 * happens to be a query. The driver has completed making the response
 * buffer, and we now expect the client to begin retrieving the response.
 */
static void
AiptekHandleAwaitReplyStatus(
        AiptekFeedbackPtr feedback,
        AiptekMessagePtr message)
{
    if (feedback->manifest.controlPacket.feedbackDirection !=
        FEEDBACK_DIRECTION_REPLY)
    {
        /* We think we have data for you, but you are not asking for it?!?
         * That's bad; signify an error.
         */
        AiptekInitMessageStruct(message, MESSAGE_STATUS_OFF);
        feedback->manifest.controlPacket.feedbackStatus = FEEDBACK_STATUS_NAK;
    }
    else
    {
        message->packetNo      = 0;
        message->messageStatus = MESSAGE_STATUS_REPLY;
    }
}

/***********************************************************************
 * Handle an incoming command. Note that a command may be a request for
 * information, e.g., the driver will be replying to the client.
 */
static void
AiptekHandleDirectionCommandPacket(
        DeviceIntPtr dev,
        AiptekFeedbackPtr feedback,
        AiptekMessagePtr message)
{
    LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    const char* methodName = "AiptekHandleDirectionCommandPacket";
    size_t i;

    /* If packet 0, we need to know the length of the incoming buffer
     * and allocate space for it.
     */
    if (feedback->manifest.controlPacket.packetNo == 0)
    {
        message->messageStatus = MESSAGE_STATUS_COMMAND;
        message->messageOffset = 0;
        message->messageLength = feedback->manifest.controlPacket.numBytes;
        message->packetNo      = 0;

        /* Determine how big the buffer should be. Always round the buffer
         * a packet up, if not at an even boundary.
         */
        i  = message->messageLength % FEEDBACK_MESSAGE_LENGTH == 0 ? 0 : 1;
        i += message->messageLength / FEEDBACK_MESSAGE_LENGTH;
        i *= FEEDBACK_MESSAGE_LENGTH;

        if (message->messageBuffer != NULL)
        {
            xfree (message->messageBuffer);
        }
        message->messageBuffer = xalloc (i);
        if (message->messageBuffer == NULL)
        {
            AiptekInitMessageStruct(message, MESSAGE_STATUS_OFF);
            feedback->manifest.controlPacket.feedbackStatus =
                    FEEDBACK_STATUS_NAK;
            return;
        }
        /* Happygram return status
         */
        feedback->manifest.controlPacket.feedbackStatus = FEEDBACK_STATUS_ACK;
        return;
    }
    message->packetNo++;

    /* Unpack the datapacket, and put into marshallBuf for later
     * inspection.
     */
    AiptekDemarshallDataPacket (feedback, message);

    /* Check to see that we haven't received a packet out of order.
     */
    if (feedback->manifest.controlPacket.packetNo == message->packetNo)
    {
        memmove(message->messageBuffer + message->messageOffset,
                message->marshallBuf,
                FEEDBACK_MESSAGE_LENGTH);
        message->messageOffset += FEEDBACK_MESSAGE_LENGTH;
    }
    else
    {
        /* Packets out of sync-- send nastigram message back
         */
        AiptekInitMessageStruct(message, MESSAGE_STATUS_OFF);
        feedback->manifest.controlPacket.feedbackStatus = FEEDBACK_STATUS_NAK;
        return;
    }

    /* We rounded the sizeof messageBuffer up a packet, 
     * so the >= is an okay comparison.
     */
    if (message->messageOffset >= message->messageLength) 
    {
        /* Process the message. If there's an error,
         * return the Nastigram.
         */

        DBG (3, xf86Msg (X_INFO, "%s: received '%s' from client\n",
                methodName, message->messageBuffer));

        if (AiptekFeedbackDispatcher(dev) == BadMatch)
        {
            AiptekInitMessageStruct(message, MESSAGE_STATUS_OFF);
            feedback->manifest.controlPacket.feedbackStatus =
                    FEEDBACK_STATUS_NAK;
        }
        else
        {
            /* The downstream method is responsible for doing
             * an AiptekInitMessageStruct() -- we don't know
             * if we have a reply message for the client or not.
             */
            feedback->manifest.controlPacket.feedbackStatus =
                    FEEDBACK_STATUS_ACK;
        }
    }
}

/***********************************************************************
 * Handle the client's request for a reply buffer. As noted above, the client
 * can send a command, requesting data from the driver. Once the driver has
 * finished creating the reply string, the client will then send reply packets
 * to retrieve that data.
 */
static void
AiptekHandleDirectionReplyPacket(
        DeviceIntPtr dev,
        AiptekFeedbackPtr feedback,
        AiptekMessagePtr message)
{
    LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;
    const char* methodName = "AiptekHandleDirectionReplyPacket";

    /* If packet 0, we need to know the length of the incoming buffer
     * and allocate space for it.
     */
    if (message->packetNo == 0)
    {
        message->messageStatus = MESSAGE_STATUS_REPLY;

        feedback->manifest.controlPacket.packetNo = 0;
        feedback->manifest.controlPacket.numBytes = message->messageLength;
        feedback->manifest.controlPacket.feedbackDirection = 
            FEEDBACK_DIRECTION_REPLY;

        DBG (3, xf86Msg (X_INFO, "%s: sending '%s' to client\n", methodName,
            message->messageBuffer));
        message->packetNo++;
        return;
    }
    /* Otherwise, send the reply, one message at a time (should
     * be several :-)
     *
     * Move over 4 bytes from messageBuffer into the feedback struct.
     */
    memmove (message->marshallBuf,
             message->messageBuffer + message->messageOffset,
             FEEDBACK_MESSAGE_LENGTH);

    feedback->manifest.controlPacket.feedbackStatus    = FEEDBACK_STATUS_ACK;
    feedback->manifest.controlPacket.feedbackDirection =
        FEEDBACK_DIRECTION_REPLY;
    feedback->manifest.controlPacket.packetNo = message->packetNo;

    AiptekMarshallDataPacket (feedback, message);

    message->packetNo++;
    message->messageOffset += FEEDBACK_MESSAGE_LENGTH;

    /* If we're finished sending the message,
     * set the message structure into idle mode, and free
     * our buffer.
     */
    if (message->messageOffset >= message->messageLength)
    {
        AiptekInitMessageStruct(message, MESSAGE_STATUS_OFF); 
    }
}

/***********************************************************************
 * There are several places that reset the message struct back to
 * an 'unused' state. E.g., free the buffers, reset the internal
 * flags.
 */
static void AiptekInitMessageStruct(
    AiptekMessagePtr message,
    int messageStatus)
{
    if (message->messageBuffer != NULL)
    {
        xfree (message->messageBuffer);
    }
    message->messageStatus = messageStatus;
    message->messageBuffer = NULL;
    message->messageLength = 0;
    message->messageOffset = 0;
    message->packetNo      = 0;
}

/***********************************************************************
 * AiptekMarshallRawPacket --
 *
 * Packs a payload of 6 bytes into 3 integers, understanding that
 * only the least significant 16 bits of the integers are significant/
 * usable.
 */
static void
AiptekMarshallRawPacket(
        AiptekFeedbackPtr feedback,
        AiptekMessagePtr message)
{
    int i, j;
    for (i = 0; i < 3; ++i)
    {
        j = ((int) message->marshallBuf[(i * 2) + 0]) << 8;
        j += (int) message->marshallBuf[(i * 2) + 1];
        feedback->manifest.rawPacket.packetBuffer[i] = j & 0xffff;
    }
}

/***********************************************************************
 * AiptekDemarshallRawPacket --
 *
 * Unpacks a payload of 6 bytes from 3 integers, understanding that
 * only the least significant 16 bits of the integers are significant/
 * usable.
 */
static void
AiptekDemarshallRawPacket(
        AiptekFeedbackPtr feedback,
        AiptekMessagePtr message)
{
    int i, j;
    for (i = 0; i < 3; ++i)
    {
        j = feedback->manifest.rawPacket.packetBuffer[i] & 0xffff;
        message->marshallBuf[(i * 2) + 0] = (char) ((j >> 8) & 0xff);
        message->marshallBuf[(i * 2) + 1] = (char) (j & 0xff);
    }
}

/***********************************************************************
 * AiptekMarshallDataPacket --
 *
 * Packs a payload of 4 bytes into 2 integers, understanding that
 * only the least significant 16 bits of the integers are significant/
 * usable. This is the primary payload transfer mechanism.
 */
static void
AiptekMarshallDataPacket(
        AiptekFeedbackPtr feedback,
        AiptekMessagePtr message)
{
    int i, j;
    for (i = 0; i < 2; ++i)
    {
        j = ((int) message->marshallBuf[(i * 2) + 0]) << 8;
        j += (int) message->marshallBuf[(i * 2) + 1];
        feedback->manifest.dataPacket.packetBuffer[i] = j & 0xffff;
    }
}

/***********************************************************************
 * AiptekDemarshallDataPacket --
 *
 * Unpacks a payload of 4 bytes from 2 integers, understanding that
 * only the least significant 16 bits of the integers are significant/
 * usable. This is the primary payload transfer mechanism.
 */
static void
AiptekDemarshallDataPacket(
        AiptekFeedbackPtr feedback,
        AiptekMessagePtr message)
{
    int i, j;
    for (i = 0; i < 2; ++i)
    {
        j = feedback->manifest.dataPacket.packetBuffer[i] & 0xffff;
        message->marshallBuf[(i * 2) + 0] = (char) ((j >> 8) & 0xff);
        message->marshallBuf[(i * 2) + 1] = (char) (j & 0xff);
    }
}

/***********************************************************************
 * AiptekFeedbackDispatcher --
 *
 * This routine determines what to do with the incoming packet.
 * We support three commands,
 *
 * 1) Query (return driver state),
 * 2) Program (set driver to state indicated in remainder of message),
 * 3) Identify (returns whether driver controls a Cursor, Eraser, or Pointer)
 *
 * The first line of the incoming buffer will contain the keyword 
 * to ID the request.
 */
static Bool
AiptekFeedbackDispatcher(
        DeviceIntPtr dev)
{
    const char *methodName = "AiptekFeedbackDispatcher";
    AiptekDevicePtr device = (AiptekDevicePtr) PRIVATE (dev);
    LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
    AiptekMessagePtr message = &device->message;
    int length;
    char *temp1;
    char keyword[32];
    Bool retval;

    CHECK_DEVICE_INT_PTR (dev, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);

    DBG (1, xf86Msg (X_INFO, "%s: entry\n", methodName));

    /* The first line of the buffer has the type of request: either
     * a Query, Program or Identify.
     */
    temp1  = strchr (message->messageBuffer, '\n');
    length = temp1 - message->messageBuffer;
    if (length >= sizeof (keyword))
    {
        length = sizeof (keyword);
    }

    strncpy (keyword, message->messageBuffer, length);
    keyword[length] = '\0';
    temp1++;

    /* Now that we have the keyword, parse the sucker and let's roll.
     */
    if (strcmp (keyword, FEEDBACK_CMD_QUERY_TOKEN) == 0)
    {
        retval = AiptekFeedbackQueryHandler(dev, device, local);

        /* If an error, we do not have a reply message built that
         * we immediately expect the client to ask for in its
         * subsequent request.
         */
        if (retval != BadMatch)
        {
            message->messageStatus = MESSAGE_STATUS_AWAIT_REPLY;
        }
    }
    else if (strcmp(keyword, FEEDBACK_CMD_COMMAND_TOKEN) == 0)
    {
        retval = AiptekFeedbackCommandHandler(dev, device, local, temp1);
        message->messageStatus = MESSAGE_STATUS_OFF;
    }
    else if (strcmp(keyword, FEEDBACK_CMD_IDENTIFY_TOKEN) == 0)
    {
        retval = AiptekFeedbackIdentifyHandler (dev, device, local);

        /* If an error, we do not have a reply message built that
         * we immediately expect the client to ask for in its
         * subsequent request.
         */
        if (retval != BadMatch)
        {
            message->messageStatus = MESSAGE_STATUS_AWAIT_REPLY;
        }
    }
    else if (strcmp(keyword, FEEDBACK_CMD_CLOSE_TOKEN) == 0)
    {
        retval = AiptekFeedbackCloseHandler(dev, device, local);
    }
    else if (strcmp(keyword, FEEDBACK_CMD_OPEN_TOKEN) == 0)
    {
        retval = AiptekFeedbackOpenHandler(dev, device, local, temp1);
    }
    else
    {
        xf86Msg (X_ERROR, "%s: abends - token not found\n", methodName);
        retval = BadMatch;
    }

    if (retval == BadMatch)
    {
        AiptekInitMessageStruct(message, MESSAGE_STATUS_OFF);
    }
    return retval;
}

/***********************************************************************
 * Hotplug routines for the X driver -- this routine deals with a device
 * unplug event, which should close the device entry. I'm thinking, too,
 * that I should like to deallocate all resources associated with this
 * device, but if the clod plugs the tablet right into the exact-same
 * usb socket, is it cheaper for me to leave remnants behind?
 *
 * Along those lines, X will non-desructively close a device. So with that
 * in mind, I ought to send an open/close attribute...
 */
static Bool
AiptekFeedbackCloseHandler(
        DeviceIntPtr dev,
        AiptekDevicePtr device,
        LocalDevicePtr local)
{
    return Success;
}

/***********************************************************************
 * Hotplug routines for the X driver -- we close and reopen...
 */
static Bool
AiptekFeedbackOpenHandler(
        DeviceIntPtr dev,
        AiptekDevicePtr device,
        LocalDevicePtr local,
        char *incomingMessage)
{
    return Success;
}

/***********************************************************************
 * AiptekFeedbackQueryHandler --
 * 
 * Returns the current state of the driver's configuration as an ASCII
 * buffer to the returned to the client application.
 */
static Bool
AiptekFeedbackQueryHandler(
        DeviceIntPtr dev,
        AiptekDevicePtr device,
        LocalDevicePtr local)
{
    const char *methodName = "AiptekFeedbackQueryHandler";
    AiptekCommonPtr common = device->common;
    AiptekMessagePtr message = &device->message;
    char *s, *t;
    int len, i;

    CHECK_DEVICE_INT_PTR (dev, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_COMMON_PTR (common, __LINE__);

    DBG (1, xf86Msg (X_INFO, "%s: entry\n", methodName));

    /* We don't need the incoming buffer anymore.
    */
    if (message->messageBuffer != NULL)
    {
        xfree (message->messageBuffer);
    }
    message->messageBuffer = NULL;

    /* Now build the reply message with the driver's current
    * configuration state.
    */
    s = xalloc (4096);
    if (s == NULL)
    {
        return BadMatch;
    }
    t = s;

    /* Identifier
    */
    t += sprintf (t, "identifier=%s\n", local->name);

    /* deviceType, aka  "Type"
    */
    t += sprintf (t, "deviceType=%s\n",
        (DEVICE_ID (device->deviceId) == STYLUS_ID) ? pStylusDevice :
        (DEVICE_ID (device->deviceId) == CURSOR_ID) ? pCursorDevice :
        (DEVICE_ID (device->deviceId) == ERASER_ID) ? pEraserDevice :
        pUnknownDevice);

    /* physicalDevicePath, which we don't know.
    */
    t += sprintf (t, "physicalDevicePath=unknown\n");

    /* logicalDevicePath, aka "Device"
    */
    t += sprintf (t, "logicalDevicePath=%s\n", common->devicePath);

    /* coordinateType, aka "Mode"
    */
    if ((device->flags & ABSOLUTE_FLAG) != 0) 
    {
        t += sprintf (t, "coordinateType=absolute\n");
    }
    else
    {
        t += sprintf (t, "coordinateType=relative\n");
    }

    /* cursorType, aka "Cursor"
    */
    if ((device->flags & CURSOR_STYLUS_FLAG) != 0)
    {
        t += sprintf (t, "cursorType=stylus\n");
    }
    else 
    {
        t += sprintf (t, "cursorType=puck\n");
    }

    /* There is no reason to emit "USB" option
    */

    /* screenNo, aka "ScreenNo"
    */
    t += sprintf (t, "screenNo=%d\n", device->screenNo);

    /* keepShape , aka "KeepShape"
    */
    t += sprintf (t, "keepShape=%s\n",
        (device->flags & KEEP_SHAPE_FLAG) != 0 ? "true" : "false");

    /* alwaysCore, aka "AlwaysCore"
    */
    t += sprintf (t, "alwaysCore=%s\n",
        (device->flags & ALWAYS_CORE_FLAG) != 0 ? "true" : "false");

    /* debugLevel, aka "DebugLevel"
    */
    t += sprintf (t, "debugLevel=%d\n", device->debugLevel);

    /* historySize, aka "HistorySize"
    */
    t += sprintf (t, "historySize=%d\n", local->history_size);

    /* pressureType, aka "Pressure"
    */
    t += sprintf (t, "pressureCurveType=%s\n",
        (device->zMode == PRESSURE_MODE_LINEAR) ? "normal" :
        (device->zMode == PRESSURE_MODE_SOFT_SMOOTH) ? "soft" :
        (device->zMode == PRESSURE_MODE_HARD_SMOOTH) ? "hard" :
        (device->zMode == PRESSURE_MODE_CUSTOM) ? "custom" :
        "unknown");

    if (device->zMode == PRESSURE_MODE_CUSTOM)
    {
       t += sprintf(t, "pressureCurve=%s\n", AiptekPrintPressureCurve(device));
    }
    /* xMax, aka "XMax" or "MaxX"
    */
    t += sprintf (t, "xMax=%d\n", device->xMax);

    /* yMax, aka "YMax" or "MaxY"
    */
    t += sprintf (t, "yMax=%d\n", device->yMax);

    /* xSize, aka "XSize" or "SizeX"
    */
    t += sprintf (t, "xSize=%d\n", device->xSize);

    /* ySize, aka "YSize" or "SizeY"
    */
    t += sprintf (t, "ySize=%d\n", device->ySize);

    /* xOffset, aka "XOffset" or "OffsetX"
    */
    t += sprintf (t, "xOffset=%d\n", device->xOffset);

    /* yOffset, aka "YOffset" or "OffsetY"
    */
    t += sprintf (t, "yOffset=%d\n", device->yOffset);

    /* xTop, aka "XTop" or "TopX"
    */
    t += sprintf (t, "xTop=%d\n", device->xTop);

    /* yTop, aka "YTop" or "TopY"
    */
    t += sprintf (t, "yTop=%d\n", device->yTop);

    /* xBottom, aka "XBottom" or "BottomX"
    */
    t += sprintf (t, "xBottom=%d\n", device->xBottom);

    /* yBottom, aka "YBottom" or "BottomY"
    */
    t += sprintf (t, "yBottom=%d\n", device->yBottom);

    /* invX, aka "InvX"
    */
    t += sprintf (t, "invX=%s\n",
        (device->flags & INVX_FLAG) != 0 ? "true" : "false");

    /* invY, aka "InvY"
    */
    t += sprintf (t, "invY=%s\n",
        (device->flags & INVY_FLAG) != 0 ? "true" : "false");

    /* zMin, aka "zMin"
    */
    t += sprintf (t, "zMin=%d\n", device->zMin);

    /* zMax, aka "ZMax"
    */
    t += sprintf (t, "zMax=%d\n", device->zMax);

    /* xThreshold, aka "xThreshold"
    */
    t += sprintf (t, "xThreshold=%d\n", device->xThreshold);

    /* yThreshold, aka "yThreshold"
    */
    t += sprintf (t, "yThreshold=%d\n", device->yThreshold);

    /* zThreshold, aka "zThreshold"
    */
    t += sprintf (t, "zThreshold=%d\n", device->zThreshold);

    /* Round the size of the reply buffer to the nextmost message packet,
    * and also send the trailing \0.
    */
    len = strlen (s) + 1;
    i  = len % FEEDBACK_MESSAGE_LENGTH == 0 ? 0 : 1;
    i += len / FEEDBACK_MESSAGE_LENGTH;
    i *= FEEDBACK_MESSAGE_LENGTH;

    message->messageBuffer = xalloc (i);
    if (message->messageBuffer == NULL)
    {
        xfree (s);
        return BadMatch;
    }

    /* We have the reply ready. Now all we need is for the client
    * to begin asking for it. Which it should do following our
    * return.
    */
    message->messageStatus = MESSAGE_STATUS_AWAIT_REPLY;
    strcpy (message->messageBuffer, s);
    message->messageLength = i;
    message->messageOffset = 0;
    message->packetNo = 0;

    /* We're done with our tempy buffer.
    */
    xfree (s);

    DBG (1, xf86Msg (X_INFO, "%s: ends successfully\n", methodName));
    return Success;
}

/***********************************************************************
 * AiptekFeedbackIdentifyHandler --
 *
 * This handler allows us to identify that the driver is an Aiptek Driver,
 * what path is it getting input from, whether it is a cursor, stylus, or
 * eraser.
 */
static Bool
AiptekFeedbackIdentifyHandler(
        DeviceIntPtr dev,
        AiptekDevicePtr device,
        LocalDevicePtr local)
{
    const char *methodName = "AiptekFeedbackIdentifyHandler";
    AiptekCommonPtr common = device->common;
    AiptekMessagePtr message = &device->message;
    char *s, *t;
    int len, i;

    CHECK_DEVICE_INT_PTR (dev, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);
    CHECK_AIPTEK_COMMON_PTR (common, __LINE__);

    DBG (1, xf86Msg (X_INFO, "%s: entry\n", methodName));

    /* We don't need the incoming buffer anymore.
     */
    if (message->messageBuffer != NULL)
    {
        xfree (message->messageBuffer);
    }
    message->messageBuffer = NULL;

    s = xalloc (1024);
    if (s == NULL)
    {
        xf86Msg (X_INFO, "%s: no xalloc workie\n", methodName);
        return BadMatch;
    }
    t = s;

    /* Our biggest problem is, we're not able to ID a driver
     * as an 'Aiptek' driver, nor, if known, whether it is
     * driving a stylus, cursor, or eraser device.
     */
    t += sprintf (t, "driverName=%s\n", local->name);
    t += sprintf (t, "driverType=%s\n",
        (DEVICE_ID (device->deviceId) == STYLUS_ID) ? pStylusDevice :
        (DEVICE_ID (device->deviceId) == CURSOR_ID) ? pCursorDevice :
        (DEVICE_ID (device->deviceId) == ERASER_ID) ? pEraserDevice :
        pUnknownDevice);
    t += sprintf (t, "physicalDevicePath=unknown\n");
    t += sprintf (t, "logicalDevicePath=%s\n", common->devicePath);

  /* Round the size of the buffer to the nextmost message packet,
   * And yes, send the \0 always!
   */
    len = strlen (s) + 1;
    i =  len % FEEDBACK_MESSAGE_LENGTH  ? 0 : 1; 
    i += len / FEEDBACK_MESSAGE_LENGTH;
    i *= FEEDBACK_MESSAGE_LENGTH;

    message->messageBuffer = xalloc (i);
    if (message->messageBuffer == NULL)
    {
        xfree (s);
        return BadMatch;
    }

  /* We have the reply ready. Now all we need is for the client
   * to begin asking for it. Which it should do following our
   * return.
   */
    message->messageStatus = MESSAGE_STATUS_AWAIT_REPLY;
    strcpy (message->messageBuffer, s);
    message->messageLength = i;
    message->messageOffset = 0;
    message->packetNo = 0;

    xfree (s);

    DBG (1, xf86Msg (X_INFO, "%s: ends successfully\n", methodName));
    return Success;
}

/***********************************************************************
 * AiptekFeedbackCommandHandler --
 *
 * This method deals with the user dynamically reprogramming driver
 * attributes. Said attributes are in the form of an ASCII string,
 * which we parse.
 */
static Bool
AiptekFeedbackCommandHandler (
        DeviceIntPtr dev,
        AiptekDevicePtr device,
        LocalDevicePtr local,
        char *incomingMessage)
{
    const char *methodName = "AiptekFeedbackCommandHandler";
    AiptekMessagePtr message = &device->message;
    char *scan = incomingMessage;
    char *keyword = NULL;
    char *value = NULL;

    CHECK_DEVICE_INT_PTR (dev, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR (device, __LINE__);
    CHECK_LOCAL_DEVICE_PTR (local, __LINE__);

    DBG (1, xf86Msg (X_INFO, "%s: begins\n", methodName));

    /* Zap the DevicePtr back to N/A values so we can properly set all
     * parameters.
     */
    AiptekInitializeDevicePtr(device, device->devicePath, device->deviceId,
        device->flags, device->common);

    /* Now scan for 'keyword=value' pairs and interpret their meaning.
     */
    while (*scan != '\0')
    {
        if (*scan == '\n' || *scan == '\0')
        {
            if (*scan == '\n')
            {
                *scan = '\0';
                scan++;
            }

            if (keyword != NULL && value != NULL)
            {
                if (AiptekFeedbackCommandParser(dev, local, keyword, value)
                    == BadMatch)
                {
                    return BadMatch;
                }
            }
            keyword = NULL;
            value = NULL;
            continue;
        }

        if (*scan != '=' && keyword == NULL)
        {
            keyword = scan;
        }
        else if (*scan == '=')
        {
            *scan++ = '\0';
            value = scan;
        }
        scan++;
    }

    if (keyword != NULL && value != NULL)
    {
        if (AiptekFeedbackCommandParser(dev, local, keyword, value) == BadMatch)
        {
            return BadMatch;
        }
    }

    /* If we've been given a new devicePath, AiptekFeedbackCommandParser
     * will set NEW_PATH_FLAG for us. That's because we want to control
     * the timing of the device close/re-opening.
     */
    if (AiptekDeleteDriverClasses(dev) == BadMatch ||
        AiptekNormalizeSizeParameters(local) == BadMatch ||
        AiptekAllocateDriverAxes(dev) == BadMatch)
    {
        xf86Msg(X_ERROR, "%s: abends: Dispatcher or DeleteStructs fails\n",
            methodName);
        return BadMatch;
    }

    /* We're done with the incoming message buffer.
     */
    AiptekInitMessageStruct(message, MESSAGE_STATUS_OFF);
    DBG (1, xf86Msg (X_INFO, "%s: ends successfully\n", methodName));

    return Success;
}

/***********************************************************************
 * AiptekFeedbackCommandParser --
 *
 * Given a keyword and value pair, will determine the meaning and set
 * the appropriate attribute in local. This is a helper method to
 * AiptekFeedbackCommandHandler.
 */
static Bool
AiptekFeedbackCommandParser(
        DeviceIntPtr dev,
        LocalDevicePtr local,
        char *keyword,
        char *value)
{
    const char *methodName = "AiptekFeedbackCommandParser";
    AiptekDevicePtr device = (AiptekDevicePtr) local->private;

    CHECK_DEVICE_INT_PTR(dev, __LINE__);
    CHECK_AIPTEK_DEVICE_PTR(device, __LINE__);
    CHECK_LOCAL_DEVICE_PTR(local, __LINE__);

    DBG (1, xf86Msg(X_INFO, "%s: entry\n", methodName));

    /* Now begin the keyword parsing.
    */
    if (strcmp(keyword, "coordinateType") == 0)
    {
        if (strcmp(value, "relative") == 0)
        {
            device->flags &= ~ABSOLUTE_FLAG;
        }
        else if (strcmp(value, "absolute") == 0)
        {
            device->flags |= ABSOLUTE_FLAG;
        }
    }
    else if (strcmp(keyword, "physicalDevicePath") == 0)
    {
        /* Emit a hand, disconnect the tablet, connect it
         * to another USB port. Or, myabe not :-)
         */
        ;
    }
    else if (strcmp (keyword, "logicalDevicePath") == 0 &&
             strcmp (device->devicePath, value) != 0)
    {
        xfree (device->devicePath);
        device->devicePath = xstrdup (value);
    }
    else if (strcmp (keyword, "cursorType") == 0)
    {
        if (strcmp (value, "stylus") == 0)
        {
            device->flags |= CURSOR_STYLUS_FLAG;
        }
        else if (strcmp (value, "puck") == 0)
        {
            device->flags &= ~CURSOR_STYLUS_FLAG;
        }
    }
    else if (strcmp (keyword, "pressureCurveType") == 0)
    {
        if (strcmp (value, "normal") == 0)
        {
            device->zMode = PRESSURE_MODE_LINEAR;
        }
        else if (strcmp (value, "soft") == 0)
        {
            device->zMode = PRESSURE_MODE_SOFT_SMOOTH;
        }
        else if (strcmp (value, "hard") == 0)
        {
            device->zMode = PRESSURE_MODE_HARD_SMOOTH;
        }
        else if (strcmp (value, "custom") == 0)
        {
            device->zMode = PRESSURE_MODE_CUSTOM;
        }
        AiptekProjectCurvePoints(device);
    }
    else if (strcmp(keyword, "pressureCurve") == 0)
    {
        if (device->zMode == PRESSURE_MODE_CUSTOM)
        {
            AiptekParsePressureCurve(device, value);
        }
    }
    else if (strcmp (keyword, "screenNo") == 0)
    {
        device->screenNo = atoi (value);
    }
    else if (strcmp (keyword, "keepShape") == 0)
    {
        if (strcmp (value, "true") == 0)
        {
            device->flags |= KEEP_SHAPE_FLAG;
        }
        else if (strcmp (value, "false") == 0)
        {
            device->flags &= ~KEEP_SHAPE_FLAG;
        }
    }
    else if (strcmp (keyword, "alwaysCore") == 0)
    {
        if (strcmp (value, "true") == 0)
        {
            device->flags |= ALWAYS_CORE_FLAG;
        }
        else if (strcmp (value, "false") == 0)
        {
            device->flags &= ~ALWAYS_CORE_FLAG;
        }
    }
    else if (strcmp (keyword, "debugLevel") == 0)
    {
        device->debugLevel = atoi (value);
    }
    else if (strcmp (keyword, "historySize") == 0)
    {
        local->history_size = atoi (value);
    }
    else if (strcmp (keyword, "invX") == 0)
    {
        if (strcmp (value, "true") == 0)
        {
            device->flags |= INVX_FLAG;
        }
        else if (strcmp (value, "false") == 0)
        {
            device->flags &= ~INVX_FLAG;
        }
    }
    else if (strcmp (keyword, "invY") == 0)
    {
        if (strcmp (value, "true") == 0)
        {
            device->flags |= INVY_FLAG;
        }
        else if (strcmp (value, "false") == 0)
        {
            device->flags &= ~INVY_FLAG;
        }
    }
    else if (strcmp (keyword, "xSize") == 0)
    {
        device->xSize = atoi (value);
    }
    else if (strcmp (keyword, "ySize") == 0)
    {
        device->ySize = atoi (value);
    }
    else if (strcmp (keyword, "xOffset") == 0)
    {
        device->xOffset = atoi (value);
    }
    else if (strcmp (keyword, "yOffset") == 0)
    {
        device->yOffset = atoi (value);
    }
    else if (strcmp (keyword, "xMax") == 0) {
        device->xMax = atoi (value);
    }
    else if (strcmp (keyword, "yMax") == 0) {
        device->yMax = atoi (value);
    }
    else if (strcmp (keyword, "xTop") == 0) {
        device->xTop = atoi (value);
    }
    else if (strcmp (keyword, "yTop") == 0) {
        device->yTop = atoi (value);
    }
    else if (strcmp (keyword, "xBottom") == 0) {
        device->xBottom = atoi (value);
    }
    else if (strcmp (keyword, "yBottom") == 0) {
        device->yBottom = atoi (value);
    }
    else if (strcmp (keyword, "zMin") == 0) {
        device->zMin = atoi (value);
    }
    else if (strcmp (keyword, "zMax") == 0) {
        device->zMax = atoi (value);
    }
    else if (strcmp (keyword, "xThreshold") == 0) {
        device->xThreshold = atoi (value);
    }
    else if (strcmp (keyword, "yThreshold") == 0) {
        device->yThreshold = atoi (value);
    }
    else if (strcmp (keyword, "zThreshold") == 0)
    {
        device->zThreshold = atoi (value);
    }

    DBG (1, xf86Msg (X_INFO, "%s: ends successfully\n", methodName));
    return Success;
}

/***********************************************************************
 * Pressure Curve Routines
 *
 * AiptekProjectCurvePoints
 *
 * Given a known curve type, we'll project out the curve points in a
 * 512-element array. This affords us flexibility to handle custom curves,
 * insofar as all pressure readings will be inferred from the curve points.
 */
static void
AiptekProjectCurvePoints(
        AiptekDevicePtr pDev)
{
    float f, i;
    int j;
    if (pDev->pPressCurve)
    {
        xfree( pDev->pPressCurve );
    }
    pDev->pPressCurve = (int*) xalloc(sizeof(int) * 512);
    if (pDev->zMode == PRESSURE_MODE_LINEAR)
    {
        for (i = 0; i < 512; ++i)
        {
            j = (int) i;
            pDev->pPressCurve[j] = j;
        }
    }
    else if (pDev->zMode == PRESSURE_MODE_HARD_SMOOTH)
    {
        for (i = 0; i < 512; ++i)
        {
            f = (512.0 * sqrt(i / 512.0)) + 0.5;
            j = (int) i;
            pDev->pPressCurve[j] = (int) f;
        }
    }
    else if (pDev->zMode == PRESSURE_MODE_SOFT_SMOOTH)
    {
        for (i = 0; i < 512; ++i)
        {
            f = ((i * i / 512.0) + 0.5);
            j = (int) i;
            pDev->pPressCurve[j] = (int) f;
        }
    }
    else if (pDev->zMode == PRESSURE_MODE_CUSTOM)
    {
        /* The custom curve will receive an array of all-zero elements.
         * We expect the user to momentarily provide us with a curve
         * that will overwrite these values.
         */
        for (i = 0; i < 512; ++i)
        {
            pDev->pPressCurve[(int)i] = 0;
        }
    }
}

/***********************************************************************
 * AiptekLookupPressureValues --
 *
 * Given a pressure value read from the tablet, lookup and return 
 * its appropriate value on the pressure curve.
 */
static void
AiptekLookupPressureValue(
        LocalDevicePtr local,
        AiptekStatePtr pState)
{
    AiptekDevicePtr pDev = (AiptekDevicePtr) local->private;
    if (pDev->pPressCurve)
    {
        int p = pState->z;
        if (((pDev->common)->zMaxCapacity) <= 0)
        {
            ((pDev->common)->zMaxCapacity) = 512;
        }

        p = (p < 0)
                ? 0
                : (p > (pDev->common)->zMaxCapacity)
                    ? ((pDev->common)->zMaxCapacity)
                    : p;

        pState->z = pDev->pPressCurve[p];
    }
}
/***********************************************************************
 * AiptekParsePressureCurve -- 
 *
 * Parses a string containing the points for the pressure curve. Points
 * will be in 512 pairs of x1,y1 values (e.g., '0,13'). This routine
 * parses the tokens/curvePoints one at a time, and sends them off to
 * populated in the curve.
 */
static void
AiptekParsePressureCurve(
        AiptekDevicePtr pDev,
        char* curvePoints)
{
    char *scan = curvePoints;
    char *value = scan;
    int i = 0;

    /* We now scan for "value0,..value511" dataPoints.
    */
    while (*scan != '\0')
    {
        if (*scan == ',')
        {
            *scan++ = '\0';
            AiptekSetPressureCurvePoint(pDev, i, value);
            value = scan;
            i++;
        }
        scan++;
    }
    AiptekSetPressureCurvePoint(pDev, i, value);
}

/*********************************************************************** 
 * AiptekSetPressureCurvePoint--
 *
 * Simple curvePoint parse & insertion routine.
 */
static void
AiptekSetPressureCurvePoint(
        AiptekDevicePtr pDev,
        int curveOffset,
        char* value)
{
    int curveValue  = atoi(value);

    if (pDev->pPressCurve != NULL)
    {
        pDev->pPressCurve[curveOffset] = curveValue;
    }
}

/***********************************************************************
 * AiptekPrintPressureCurve --
 *
 * Streams out the pressureCurve in pairs of x1,y1 values.
 */
static char*
AiptekPrintPressureCurve(
        AiptekDevicePtr pDev)
{
    int i;
    char* scan;
    char* ret;
    ret = xalloc(2048);
    if (ret == NULL)
    {
        return ret;
    }
    scan = ret;
    for (i = 0; i < 512; ++i)
    {
        scan += sprintf(scan, "%d%c", pDev->pPressCurve[i],
                (i==511) ? '\0' : ',');
    }
    return ret;
}
/***********************************************************************
 * Plug, unplug, and structs are put at the end of the driver. Why is 
 * unknown at present time.
 */
static InputDriverRec AIPTEK =
{
    1,               /* driver version */
    "aiptek",        /* driver name */
    NULL,            /* identify */
    AiptekInit,      /* pre-init */
    AiptekUninit,    /* un-init */
    NULL,            /* module */
    0                /* ref count */
};

/***********************************************************************
 * AiptekUnplug --
 *
 * Should remove all vestiges of the input driver. TODO.
 */
static void
AiptekUnplug(
        pointer p)
{
    ErrorF("AiptekUnplug\n");
}

/***********************************************************************
 * AiptekPlug --
 *
 * Called to load our driver into memory, do registration, etc., processing
 */
static pointer
AiptekPlug(
        pointer module,
        pointer options,
        int *errmaj,
        int *errmin)
{
    xf86AddInputDriver(&AIPTEK, module, 0);
    xf86Msg(X_INFO,
       "AiptekPlug: Aiptek USB Tablet Driver Version 1.9.1 (20-November-2005)\n");
    return module;
}

/* Version ID and module identification "stuff"
 */
static XF86ModuleVersionInfo AiptekVersionRec =
{
    "aiptek",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    1, 9, 1,
    ABI_CLASS_XINPUT,
    ABI_XINPUT_VERSION,
    MOD_CLASS_XINPUT,
    {0, 0, 0, 0}            /* A signature that we ignore */
};

XF86ModuleData aiptekModuleData =
{
    &AiptekVersionRec,
    AiptekPlug,
    AiptekUnplug
};


/*======================================================================
 *
 * End of xf86Aiptek.c
 */
