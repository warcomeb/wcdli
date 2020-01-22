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


#define WARCOMEB_WCDLI_LIBRARY_VERSION_MAJOR     (0x1ul)
#define WARCOMEB_WCDLI_LIBRARY_VERSION_MINOR     (0x0ul)
#define WARCOMEB_WCDLI_LIBRARY_VERSION_BUG       (0x0ul)
#define WARCOMEB_WCDLI_LIBRARY_VERSION           ((WARCOMEB_WCDLI_LIBRARY_VERSION_MAJOR << 16)\
                                                 |(WARCOMEB_WCDLI_LIBRARY_VERSION_MINOR << 8 )\
                                                 |(WARCOMEB_WCDLI_LIBRARY_VERSION_BUG        ))
#define WARCOMEB_WCDLI_LIBRARY_TIME              0

#ifndef __NO_PROFILES
#include "board.h"
#include "firmware.h"
#endif

/*!
 * \defgroup WCDLI_Types WCDLI Types
 * \ingroup  WCDLI
 * \{
 */

/*!
 * List of all possible errors.
 */
typedef enum _WCDLI_Errors
{
    WCDLI_ERROR_SUCCESS            = 0x0000,

    WCDLI_ERROR_WRONG_PARAMS       = 0x0100,

} WCDLI_Error_t;

/*!
 * \defgroup WCDLI_Message_Level WCDLI Debug Message Level
 * \{
 */
#define WCDLI_MESSAGE_LEVEL_NONE                 0
#define WCDLI_MESSAGE_LEVEL_DANGER               1
#define WCDLI_MESSAGE_LEVEL_WARNING              2
#define WCDLI_MESSAGE_LEVEL_INFO                 3
#define WCDLI_MESSAGE_LEVEL_ALL                  4
/*!
 * \}
 */

#if !defined (WCDLI_BUFFER_SIZE)
#define WCDLI_BUFFER_SIZE                        80
#endif

/*!
 *
 */
typedef void (*WCDLI_CommandCallback_t)(void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);

#if !defined (WCDLI_DEBUG_MESSAGE_LEVEL)
#define WCDLI_DEBUG_MESSAGE_LEVEL                WCDLI_MESSAGE_LEVEL_ALL
#endif

/*!
 * \}
 */

#endif // __WARCOMEB_WETS_TYPES_H
