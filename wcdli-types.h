/*
 * WC&DLI - Warcomeb Command & Debug Line Interface
 * Copyright (C) 2020 Marco Giammarini <http://www.warcomeb.it>
 *
 * Authors:
 *  Marco Giammarini <m.giammarini@warcomeb.it>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*!
 * \file  /wcdli-types.h
 * \brief
 */

#ifndef __WARCOMEB_WCDLI_TYPES_H
#define __WARCOMEB_WCDLI_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

#define WARCOMEB_WCDLI_LIBRARY_VERSION_MAJOR     (0x1ul)
#define WARCOMEB_WCDLI_LIBRARY_VERSION_MINOR     (0x0ul)
#define WARCOMEB_WCDLI_LIBRARY_VERSION_BUG       (0x0ul)
#define WARCOMEB_WCDLI_LIBRARY_TIME              0

#ifndef __NO_PROFILES
#include "hardware.h"
#include "firmware.h"
#endif

/*!
 * \defgroup WCDLI_Types WCDLI Types
 * \ingroup  WCDLI
 * \{
 */

#define WCDLI_PROJECT_NAME                       "WC&DLI"

#if defined (LIBOHIBOARD_VERSION)
static const Utility_Version_t WCDLI_FIRMWARE_VERSION =
{
    {
        WARCOMEB_WCDLI_LIBRARY_VERSION_MAJOR,
        WARCOMEB_WCDLI_LIBRARY_VERSION_MINOR,
        WARCOMEB_WCDLI_LIBRARY_VERSION_BUG,
        WARCOMEB_WCDLI_LIBRARY_TIME
    }
};
#endif

#if !defined _weak
#define _weak __WEAK
#endif

/*!
 * List of all possible errors.
 */
typedef enum _WCDLI_Errors_t
{
    WCDLI_ERROR_SUCCESS            = 0x0000,

    WCDLI_ERROR_WRONG_PARAMS       = 0x0100,

    WCDLI_ERROR_ADD_COMMAND_FAIL   = 0x0200,
    WCDLI_ERROR_ADD_APP_FAIL       = 0x0201,
    WCDLI_ERROR_EMPTY_CALLBACK     = 0x0202,

} WCDLI_Error_t;

/*!
 * Debug message level list.
 */
typedef enum _WCDLI_MessageLevel_t
{
	WCDLI_MESSAGELEVEL_NONE    = 0,
	WCDLI_MESSAGELEVEL_FATAL   = 1,
	WCDLI_MESSAGELEVEL_DANGER  = 2,
	WCDLI_MESSAGELEVEL_WARNING = 3,
	WCDLI_MESSAGELEVEL_INFO    = 4,
	WCDLI_MESSAGELEVEL_DEBUG   = 5,
    WCDLI_MESSAGELEVEL_ALL     = 6,
} WCDLI_MessageLevel_t;

/*!
 * Device operative mode.
 */
typedef enum _WCDLI_OperativeMode_t
{
    WCDLI_OPERATIVEMODE_DEBUG   = 0,
    WCDLI_OPERATIVEMODE_COMMAND = 1,
} WCDLI_OperativeMode_t;

#if !defined (WCDLI_BUFFER_SIZE)
#define WCDLI_BUFFER_SIZE                        80
#endif

/*!
 *
 */
typedef void (*WCDLI_CommandCallback_t)(void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);

/*!
 *
 */
typedef struct _WCDLI_Command_t
{
    char *name;
    char *description;
    void *device;
    WCDLI_CommandCallback_t callback;
} WCDLI_Command_t;

#if !defined (WCDLI_DEBUG_MESSAGE_LEVEL)
#define WCDLI_DEBUG_MESSAGE_LEVEL                WCDLI_MESSAGELEVEL_ALL
#endif

/*!
 * \}
 */

#ifdef __cplusplus
}
#endif

#endif // __WARCOMEB_WCDLI_TYPES_H
