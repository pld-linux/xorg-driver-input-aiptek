/*
 * xf86Aiptek
 *
 * Lineage: This driver is based on both the xf86HyperPen and xf86Wacom tablet
 *          drivers.
 *
 * xf86HyperPen -- modified from xf86Summa (c) 1996 Steven Lang
 *  (c) 2000 Roland Jansen
 *  (c) 2000 Christian Herzog (button & 19200 baud support)
 *
 * xf86Wacom -- (c) 1995-2001 Frederic Lepied
 *
 * This driver assumes Linux HID support, available for USB devices.
 * 
 * Version 0.0, 1-Jan-2003, Bryan W. Headley
 * 
 * Copyright 2003 by Bryan W. Headley. <bwheadley@earthlink.net>
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
 */

#ifndef _AIPTEK_H_
#define _AIPTEK_H_

#ifdef LINUX_INPUT
#include <asm/types.h>
#include <linux/input.h>

/* Am using absence of HAVE_CONFIG_H, for now, to cater for non-modular 
   X build. */
#ifndef HAVE_CONFIG_H
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#   ifndef EV_MSC
#	define EV_MSC 0x04
#   endif

/* keithp - a hack to avoid redefinitions of these in xf86str.h */
#ifdef BUS_PCI
#       undef BUS_PCI
#endif
#ifdef BUS_ISA
#       undef BUS_ISA
#endif
#endif

#include <xf86Version.h>

#ifndef XFree86LOADER
#   include <unistd.h>
#   include <errno.h>
#endif

#include <misc.h>
#include <xf86.h>
#ifndef HAVE_CONFIG_H
#include <xf86_ansic.h>
#endif

#ifndef NEED_XF86_TYPES
#define NEED_XF86_TYPES
#endif

/* #include <xf86_ansic.h> */
#if !defined(DGUX)
#include <xisb.h>
#endif

#include <xf86_OSproc.h>
#include <xf86Xinput.h>
/* #include <extinit.h> */
#include <exevents.h>        /* Needed for InitValuator/Proximity stuff */
#include <xf86Module.h>
#include <X11/keysym.h>
#include <mipointer.h>

#define xf86Verbose 1

/***********************************************************************
 * If you are using Linux 2.6, your distribution may have kernel headers
 * from Linux 2.4.x. The issue there is that there are symbols we refer
 * to from the 2.6 headers.
 */
#ifndef EV_SYN
#define EV_SYN                  0x00
#endif

#undef PRIVATE
#define PRIVATE(x) XI_PRIVATE(x)

/******************************************************************************
 * Bitmasks used for active area parameters
 */
#define AIPTEK_ACTIVE_AREA_NONE     0x00
#define AIPTEK_ACTIVE_AREA_MAX      0x01
#define AIPTEK_ACTIVE_AREA_SIZE     0x02
#define AIPTEK_ACTIVE_AREA_TOP      0x04

/***********************************************************************
 * We ship the proximity bit through EV_MISC, ORed with information
 * as to whether report came from the stylus or tablet mouse.
 */
#define PROXIMITY(flags)            ((flags) & 0x0F)


/* macro from counts/inch to counts/meter */
#define LPI2CPM(res)    (res * 1000 / 25.4)

/* max number of input events to read in one read call */
#define MAX_EVENTS 50

#define BITS_PER_LONG           (sizeof(long) * 8)
#define NBITS(x)                ((((x)-1)/BITS_PER_LONG)+1)
#define TEST_BIT(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)
#define OFF(x)                  ((x)%BITS_PER_LONG)
#define LONG(x)                 ((x)/BITS_PER_LONG)

/******************************************************************************
 * Debugging macros
 */
#ifdef DBG
#   undef DBG
#endif

#ifdef DEBUG
#   undef DEBUG
#endif

#ifndef INI_DEBUG_LEVEL
#   define INI_DEBUG_LEVEL 0
#endif

#define DEBUG 1
#if DEBUG
#   define     DBG(lvl, f)     {if (device && (lvl) <= device->debugLevel) f;}
#else
#   define     DBG(lvl, f)
#endif

#define DEFAULT_BTN_TOUCH_THRESHOLD 100

/******************************************************************************
 * AiptekDeviceState 'buttons' bitmasks
 */

    /* Stylus events */
#define BUTTONS_EVENT_TOUCH         0x01
#define BUTTONS_EVENT_STYLUS        0x02
#define BUTTONS_EVENT_STYLUS2       0x04
#define BUTTONS_EVENT_SIDE_BTN      0x08
#define BUTTONS_EVENT_EXTRA_BTN     0x10

    /* Mouse events */
#define BUTTONS_EVENT_MOUSE_LEFT    0x01
#define BUTTONS_EVENT_MOUSE_RIGHT   0x02
#define BUTTONS_EVENT_MOUSE_MIDDLE  0x04

/******************************************************************************
 * AiptekDeviceRec 'zMode' settings
 */
#define PRESSURE_MODE_LINEAR        0
#define PRESSURE_MODE_SOFT_SMOOTH   1
#define PRESSURE_MODE_HARD_SMOOTH   2
#define PRESSURE_MODE_CUSTOM        3

/******************************************************************************
 * Used throughout to indicate numeric options
 * whose value has not been set (e.g., do not enable it's functionality,
 * or resort to default behavior).
 */

#define VALUE_NA     -1
/******************************************************************************
 * This describes the control message passed between the client and the
 * server. When sending packet #0, manifest.controlPacket is active; else
 * manifest.packetBuffer sends the partial buffer, sizeof(int) *2 bytes
 * at a time.
 */

#define FEEDBACK_CMD_QUERY_TOKEN    "Query"
#define FEEDBACK_CMD_COMMAND_TOKEN  "Command"
#define FEEDBACK_CMD_IDENTIFY_TOKEN "Identify"
#define FEEDBACK_CMD_CLOSE_TOKEN    "Close"
#define FEEDBACK_CMD_OPEN_TOKEN     "Open"

/* Size of the PtrCtrl message buffer that's usable for non-control
 * data.
 */
#define FEEDBACK_MESSAGE_LENGTH     4

/*PressCurve*/
#define FILTER_PRESSURE_RES 512        /* maximum points in pressure curve */

typedef struct
{
    /* These defines are used in feedbackDirection, which appears in both
     * struct controlPacket and struct dataPacket.
     */
#define FEEDBACK_DIRECTION_COMMAND   1  /* Receiving a command from client  */
#define FEEDBACK_DIRECTION_REPLY     2  /* Replying to client               */

    /* These defines are used in feedbackStatus, which appears in both
     * struct controlPacket and struct replyPacket.
     */
#define FEEDBACK_STATUS_ACK          0  /* Message received; no errors      */
#define FEEDBACK_STATUS_NAK          1  /* Message received; choked         */

    /* This union describes how X's PtrCtrl struct is defined, and how
     * we re-use said struct (without affecting it's size.)
     */
    union
    {
        /* The original X PtrCtrl definition.
         */
        struct
        {
            int     num;
            int     den;
            int     threshold;
        } originalXPacket;

        /* Said definition is actually an array of ints, of which
         * only 16 bits per element is usable.
         */
        struct
        {
            int     packetBuffer[3];            /* payload               */
        } rawPacket;

        /* Our keeping the first int identical between controlPacket
         * and dataPacket means we can use either union structure to refer
         * to the control flags "feedbackDirection," "feedbackStatus," and
         * "packetNo." As a consistency thing, we do refer to these
         * elements exclusively as being a member of controlPacket, and the
         * payload, as being a member of dataPacket.
         */
        struct
        {
            int     feedbackDirection:3;        /* Direction of message     */
            int     feedbackStatus:2;           /* ACK|NACK reply code      */
            int     packetNo:27;                /* packet within overall msg*/
            int     numBytes;                   /* # Bytes in total message */
            int     filler;                     /* Unused filler            */
        } controlPacket;

        struct
        {
            int     feedbackDirection:3;        /* Direction of message     */
            int     feedbackStatus:2;           /* Ack|NACK reply code      */
            int     packetNo:27;                /* packet within overall msg*/
            int     packetBuffer[2];            /* Two ints of whom we can
                                                 * use 16 bits each for
                                                 * our packet(s).
                                                 */
        } dataPacket;

    } manifest;

} AiptekFeedbackRec, *AiptekFeedbackPtr;

/******************************************************************************
 * Internal buffer used to construct/deconstruct incoming and outgoing
 * messages from/to the client and this driver.
 */
typedef struct
{
#define MESSAGE_STATUS_OFF              0   /* Not send/rec messages         */
#define MESSAGE_STATUS_COMMAND          1   /* receiving a command message   */
#define MESSAGE_STATUS_REPLY            2   /* sending a reply message       */
#define MESSAGE_STATUS_AWAIT_REPLY      3   /* transition from status 1 to 2 */
    int messageStatus;                      /* Sending/receiving mesg?       */
    int packetNo;                           /* Current packet number         */
    int messageLength;                      /* Size of the overall message   */
    int messageOffset;                      /* Current offset within message */
    char  marshallBuf[6];                   /* temp buffer for marshalling   */
    char* messageBuffer;                    /* The actual message            */
} AiptekMessageRec, *AiptekMessagePtr;

/*****************************************************************************
 * AiptekDeviceState is used to contain the 'record' of events read
 * from the tablet. In theory, all of these events are collected
 * prior to a report to XInput. In practice, some of these won't be
 * known.
 */
typedef struct
{
    int     eventType;      /* STYLUS_ID, CURSOR_ID, ERASER_ID */
    int     x;              /* X value read */
    int     y;              /* Y value read */
    int     z;              /* Z value read */
    int     xTilt;          /* Angle at which stylus is held, X coord */
    int     yTilt;          /* Angle at which stylus is held, Y coord */
    int     proximity;      /* Stylus proximity bit. */
    int     macroKey;       /* Macrokey read from tablet */
    int     button;         /* Button bitmask */
    int     wheel;          /* Wheel */
    int     distance;       /* Future capacity */

    char    ptrFeedback[6]; /* Just for now... */

} AiptekStateRec, *AiptekStatePtr;

/*****************************************************************************
 * AiptekCommonRec is used to contain information that's common
 * to pseudo-devices associated with a given tablet. By definition:
 * each tablet may support a Stylus, Cursor and Eraser X Devices
 * at the same time. We're able to divide the incoming events to their
 * respective X Device by inspecting the event type.
 *
 * From a UNIX perspective, there is a single file descriptor
 * associated with a single device, or tablet. That, we call a "device".
 *
 * The X Device perspective, that of allocating a Stylus, Cursor and
 * Eraser we're calling a "pseudo-device" in the comments of this source.
 * Why is that? Well, you'll note that the file descriptor for pseudo-devices
 * are SHARED. E.g., there is only one file descriptor open.
 *
 * If you have more than one tablet, there'd be two "devices" open, one for
 * each tablet, the pseudo-devices associated with each tablet would share
 * their respective file descriptor.
 *
 * Anyway, this structure keeps data that's common to pseudo devices
 * associated with a given tablet. Generally, that means keeping info
 * on the tablet's capacities.
 */

typedef struct
{
    char*           devicePath;  /* device file name             */
    int             fd;          /* file descriptor associated   */
    unsigned char   flags;       /* various flags (handle tilt)  */

    /* Data read from the 'evdev' device has to be collected into a single
     * record before we can present it to the XInput layer. Because we need
     * to do threshold comparisons, we need the current record and the
     * previous record submitted.
     *
     * This information is common insofar as we have up to three Aiptek
     * devices (cursor, stylus, eraser), and we decide on receipt of the
     * data which device to submit it to. E.g., the commonality is the
     * single tablet.
     */
    AiptekStateRec  currentValues;
    AiptekStateRec  previousValues;

    /* The physical capacity of the tablet in X, Y, and Z
     * coordinates. Comes directly from the tablet; cannot
     * be overwritten via XF86Config-4 parameters.
     */
    int             xMinCapacity;      /* reported from tablet: min X coord */
    int             xMaxCapacity;      /* reported from tablet: max X coord */
    int             yMinCapacity;      /* reported from tablet: min Y coord */
    int             yMaxCapacity;      /* reported from tablet: max Y coord */
    int             zMinCapacity;      /* reported from tablet: min Z */
    int             zMaxCapacity;      /* reported from tablet: max Z  */
    int             xtMinCapacity;     /* reported from tablet: min xtilt */
    int             xtMaxCapacity;     /* reported from tablet: max xtilt */
    int             ytMinCapacity;     /* reported from tablet: min ytilt */
    int             ytMaxCapacity;     /* reported from tablet: max ytilt */
    int             wMinCapacity;      /* reported from tablet: min wheel */
    int             wMaxCapacity;      /* reported from tablet: max wheel */

    unsigned char   data[9];           /* data read on the device */

#define AIPTEK_MAX_DEVICES 3
    LocalDevicePtr  deviceArray[AIPTEK_MAX_DEVICES];
                                        /* array of pseudo-devices */
} AiptekCommonRec, *AiptekCommonPtr;


/*****************************************************************************
 * AiptekDeviceRec contains information on how a physical device 
 * or pseudo-device is configured. Latter needs explanation: a tablet can
 * act as an input device with, say, a stylus, a cursor/puck, and
 * an eraser. The physical device is the Aiptek; but for purposes of inane
 * fun, the stylus, cursor/puck, and the eraser are considered to
 * be separate 'pseudo' devices. What this means is that you might set
 * the stylus to only work in a subset of the active area; the cursor/puck
 * may have access to the entire region, and the eraser has it's X & Y
 * coordinates inverted.
 *
 * Second point: the cursor device is also known as a 'puck'. Aiptek though
 * gives us a mouse to serve as our puck/cursor. But to keep our sanity,
 * we refer throughout this driver as either stylus or puck, as those are
 * Input Tablet terms. Hmmph. :-)
 */

typedef struct
{
    char *          devicePath;     /* pathname of kernel device driver */

#define DEVICE_ID(flags) ((flags) & 0x07)

#define STYLUS_ID               0x01
#define CURSOR_ID               0x02
#define ERASER_ID               0x04

    unsigned int    deviceId;       /* Use this to id the tablet & xwindow 
                                       device type   */

#define INVX_FLAG               0x01
#define INVY_FLAG               0x02
#define CURSOR_STYLUS_FLAG      0x04
#define ABSOLUTE_FLAG           0x08
#define KEEP_SHAPE_FLAG         0x10
#define ALWAYS_CORE_FLAG        0x20

    unsigned char   flags;          /* We keep inverse, absolute, keepShape
                                     * 
                                     * absolute | relative coordinates,
                                     * keepshape bits inside. */
    int             alwaysCore;     /* True|False */
    int             debugLevel;     /* Used for debug messages */

    /* By default, the tablet runs in window :0.0. But that's changeable.
     */
    int             screenNo;       /* Screen mapping */

    /* We provide the Z coordinate either in linear or log mode.
     * The nomenclature "log" is fairly generic, except that
     * our smoothing algorithms are based on sqrt(). We support,
     *
     * "Soft"
     *          z = z * z / maxPressure (512)
     * "Hard"
     *          z = maxPressure * sqrt( z / maxPressure )
     */    
    int             zMode;          /* Z reported linearly or 'smoothed' */

    /* If unspecified, we will set xSize and ySize to match the
     * active area of the tablet, and also set xOffset and yOffset to 0.
     *
     * Otherwise: this documents an active area of the tablet, outside
     * of whose coordinates input is ignored. Size is computed relative
     * to upper-left corner, known as 0,0.
     *
     * Now, to that upper-left corner, xOffset and yOffset may be
     * applied, which moves the origin coordinate right and down,
     * respectively.
     */
    int             xSize;          /* Active area X */
    int             ySize;          /* Active area Y */
    int             xOffset;        /* Active area offset X */
    int             yOffset;        /* Active area offset Y */

    /* Maximum size of the tablet. Presumed to be the size of
     * the tablet drawing area if unspecified; can be used
     * to define a cut-off point from where on the tablet 
     * input will be ignored. Yes, very synonymous with 'active area',
     * above. Just a different way of expressing the same concept,
     * except there is no xOffset/yOffset parameters.
     */

    int             xMax;           /* Maximum X value */
    int             yMax;           /* Maximum Y value */

    /* Limit the range of pressure values we will accept
     * as input. Note that throughout, coordinate 'Z' refers
     * to stylus pen pressure.
     *
     * Also, note that zMin/zMax is different than zThreshold.
     */
    int             zMin;           /* Minimum Z value */
    int             zMax;           /* Maximum Z value */

    /* Changing xTop/xBottom/yTop/yBottom affects the resolution 
     * in points/inch for that given axis.
     */
    int             xTop;
    int             yTop;
    int             xBottom;
    int             yBottom;

    /* Threshold allows the user to specify a minimal amount the device
     * has to move in a given direction before the input is accepted.
     * The benefits here are to help eliminate garbage input from unsteady
     * hands/tool.
     */
    int             xThreshold;     /* accept X report if delta > threshold */ 
    int             yThreshold;     /* accept Y report if delta > threshold */
    int             zThreshold;     /* accept pressure if delta > threshold */


    /* This struct is used to keep parameters, etc., that
     * are common between ALL pseudo-devices of the aiptek
     * driver. E.g., the physical size of the drawing
     * area is kept here.
     */
    AiptekCommonPtr     common;

    /* This structure keeps control information regarding incoming/
     * outgoing messages from the client application
     */
    AiptekMessageRec    message;

    /*PressCurve*/
    int* pPressCurve;               /* pressure curve */
    int nPressCtrl[4];              /* control points for curve */

} AiptekDeviceRec, *AiptekDevicePtr;

static Bool             AiptekIsValidFileDescriptor(int);
static Bool             AiptekDispatcher(DeviceIntPtr, int);
static void             AiptekUninit(InputDriverPtr, LocalDevicePtr, int);
static InputInfoPtr     AiptekInit(InputDriverPtr, IDevPtr, int);
static LocalDevicePtr   AiptekAllocateLocal(InputDriverPtr, char *,char *, int, int);
static AiptekCommonPtr  AiptekAssignCommon(LocalDevicePtr, char *);
static AiptekCommonPtr  AiptekAllocateCommon(LocalDevicePtr, char *);
static Bool             AiptekDetachCommon(LocalDevicePtr);
static Bool             AiptekOpenDriver(DeviceIntPtr);
static Bool             AiptekOpenFileDescriptor(LocalDevicePtr);
static Bool             AiptekGetTabletCapacity(LocalDevicePtr);
static Bool             AiptekLowLevelOpen(LocalDevicePtr);
static Bool             AiptekNormalizeSizeParameters(LocalDevicePtr);
static Bool             AiptekAllocateDriverClasses(DeviceIntPtr);
static Bool             AiptekAllocateDriverAxes(DeviceIntPtr);
static Bool             AiptekDeleteDriverClasses(DeviceIntPtr);
static void             AiptekCloseDriver(LocalDevicePtr);
static Bool             AiptekChangeControl(LocalDevicePtr, xDeviceCtl*);
static Bool             AiptekSwitchMode(ClientPtr, DeviceIntPtr, int);
static Bool             AiptekConvert(LocalDevicePtr, int, int, int, int, int, int, int, int, int*, int*);
static Bool             AiptekReverseConvert(LocalDevicePtr, int, int, int*);
static void             AiptekReadInput(LocalDevicePtr);
static Bool             AiptekIsEventValid(AiptekCommonPtr, int);
static void             AiptekSendEvents(LocalDevicePtr);
static void             AiptekPtrFeedbackHandler(DeviceIntPtr, PtrCtrl*);
static Bool             AiptekFeedbackDispatcher(DeviceIntPtr);
static Bool             AiptekFeedbackQueryHandler(DeviceIntPtr, AiptekDevicePtr, LocalDevicePtr);
static Bool             AiptekFeedbackIdentifyHandler(DeviceIntPtr, AiptekDevicePtr, LocalDevicePtr);
static Bool             AiptekFeedbackCommandHandler(DeviceIntPtr, AiptekDevicePtr, LocalDevicePtr, char*);
static Bool             AiptekFeedbackCommandParser(DeviceIntPtr, LocalDevicePtr, char*, char*);
static Bool             AiptekFeedbackCloseHandler(DeviceIntPtr, AiptekDevicePtr, LocalDevicePtr);
static Bool             AiptekFeedbackOpenHandler(DeviceIntPtr, AiptekDevicePtr, LocalDevicePtr, char*);
static void             AiptekMarshallRawPacket(AiptekFeedbackPtr, AiptekMessagePtr);
static void             AiptekDemarshallRawPacket(AiptekFeedbackPtr, AiptekMessagePtr);
static void             AiptekMarshallDataPacket(AiptekFeedbackPtr, AiptekMessagePtr);
static void             AiptekDemarshallDataPacket(AiptekFeedbackPtr, AiptekMessagePtr);
static void             AiptekInitializeDevicePtr(AiptekDevicePtr, char*, int, int, AiptekCommonPtr);
static void             AiptekParsePressureCurve(AiptekDevicePtr, char*);
static void             AiptekSetPressureCurvePoint(AiptekDevicePtr, int, char*);
static void             AiptekProjectCurvePoints(AiptekDevicePtr);
static char*            AiptekPrintPressureCurve(AiptekDevicePtr);
static void             AiptekLookupPressureValue(LocalDevicePtr, AiptekStatePtr);
static void             AiptekHandleMessageBegin(AiptekFeedbackPtr, AiptekMessagePtr);
static void             AiptekHandleAwaitReplyStatus(AiptekFeedbackPtr,AiptekMessagePtr);
static void             AiptekHandleDirectionCommandPacket(DeviceIntPtr, AiptekFeedbackPtr, AiptekMessagePtr);
static void             AiptekHandleDirectionReplyPacket(DeviceIntPtr, AiptekFeedbackPtr, AiptekMessagePtr);
static void             AiptekInitMessageStruct(AiptekMessagePtr,int);
static int              AiptekHandleAbsEvents(int, struct input_event*, AiptekCommonPtr);
static int              AiptekHandleRelEvents(int, struct input_event*, AiptekCommonPtr);
static int              AiptekHandleKeyEvents(int, struct input_event*, AiptekCommonPtr);

#define SYSCALL(err, call)  \
    errno = 0; \
    do \
    { \
        err = (call); \
    } while (err == -1 && errno == EINTR)

#define ABS(x) ((x) > 0 ? (x) : -(x))

/* uncomment the following line if you want to check several casts 
   and know what you are doing. */
/* #define CHECK_POINTER_CASTS */
#if defined(CHECK_POINTER_CASTS)

#define CHECK_DEVICE_INT_PTR(ptr, line) \
  if (strcmp(ptr->name, "pen") && \
      strcmp(ptr->name, "cursor") && \
      strcmp(ptr->name, "eraser")) \
      xf86Msg(X_INFO, "line %i unknown DeviceIntPtr (%s)\n", __LINE__, ptr->name);

#define CHECK_LOCAL_DEVICE_PTR(ptr, line) \
  if (strcmp(ptr->name, "pen") && \
      strcmp(ptr->name, "cursor") && \
      strcmp(ptr->name, "eraser")) \
      xf86Msg(X_INFO, "line %i unknown LocalDevicePtr (%s)\n", __LINE__, ptr->name);

#define CHECK_AIPTEK_DEVICE_PTR(ptr, line) \
  if (strncmp(ptr->devicePath, "/dev/input/aiptek", 17)) \
      xf86Msg(X_INFO, "line %i unknown AiptekDevicePtr (%s)\n", __LINE__, ptr->devicePath);

#define CHECK_INPUT_DRIVER_PTR(ptr, line) \
  if (strncmp(ptr->driverName, "aiptek", 6)) \
      xf86Msg(X_INFO, "line %i unknown InputDriverPtr (%s)\n", __LINE__, ptr->driverName);

#define CHECK_I_DEV_PTR(ptr, line) \
  if (strncmp(ptr->driver, "aiptek", 6)) \
      xf86Msg(X_INFO, "line %i unknown IDevPtr (%s)\n", __LINE__, ptr->driver);

#define CHECK_AIPTEK_COMMON_PTR(ptr, line) \
  if (strncmp(ptr->devicePath, "/dev/input/aiptek", 17)) \
      xf86Msg(X_INFO, "line %i unknown AiptekCommonPtr (%s)\n", __LINE__, ptr->devicePath);

#else

#define CHECK_DEVICE_INT_PTR(ptr, line)
#define CHECK_LOCAL_DEVICE_PTR(ptr, line)
#define CHECK_AIPTEK_DEVICE_PTR(ptr, line)
#define CHECK_INPUT_DRIVER_PTR(ptr, line)
#define CHECK_I_DEV_PTR(ptr, line)
#define CHECK_AIPTEK_COMMON_PTR(ptr, line)
#endif

#endif
