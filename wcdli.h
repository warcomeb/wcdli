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
 * \file  /wets.h
 * \brief
 */

#ifndef __WARCOMEB_WCDLI_H
#define __WARCOMEB_WCDLI_H

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 * \mainpage WC&DLI - Warcomeb Command & Debug Line Interface
 *
 * This library is developed in order to ...
 *
 * \section changelog ChangeLog
 *
 * \li v1.0.0 of 2020/08/xx - First release
 *
 * \section library External Library
 *
 * The library use the following external library:
 * \li libohiboard https://github.com/ohilab/libohiboard a multiplatform C
 * framework for multi microcontroller.
 *
 * \section example Example
 *
 * \section credits Credits
 * \li Marco Giammarini (warcomeb)
 */

/*!
 * \defgroup WCDLI WC&DLI Module APIs
 * \{
 */

#include "wcdli-types.h"

/*!
 *
 */
void WCDLI_init (void);

/*!
 *
 */
void WCDLI_printProjectVersion (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);

/*!
 *
 */
void WCDLI_printStatus (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);

/*!
 *
 */
void WCDLI_ckeck (void);

/*!
 *
 * \param[in]        name:
 * \param[in] description:
 * \param[in]    callback:
 * \return
 */
WCDLI_Error_t WCDLI_addCommandByParam (char* name,
                                       char* description,
                                       WCDLI_CommandCallback_t callback);

/*!
 *
 *
 * \param[in] command:
 * \return
 */
WCDLI_Error_t WCDLI_addCommand (WCDLI_Command_t* command);

/*!
 *
 * \param[in]        name:
 * \param[in] description:
 * \param[in]    callback:
 * \param[in]         dev:
 * \return
 */
WCDLI_Error_t WCDLI_addAppByParam (char* name,
                                   char* description,
                                   void* app,
                                   WCDLI_CommandCallback_t callback);

/*!
 * \param[in] app:
 * \return
 */
WCDLI_Error_t WCDLI_addApp (WCDLI_Command_t* app);

static inline void WCDLI_printString1 (const char* str1)
{
    Uart_sendStringln(WCDLI_PORT, str1);
}

static inline void WCDLI_printString2 (const char* str1, const char* str2)
{
    Uart_sendString(WCDLI_PORT, str1);
    Uart_sendStringln(WCDLI_PORT, str2);
}

#define WCDLI_PRINT_MESSAGE(LEVELSTRING,MESSAGE) \
    do {                                         \
        WCDLI_printString2(LEVELSTRING,MESSAGE); \
    } while (0)

#if (WCDLI_MESSAGE_LEVEL_INFO <= WCDLI_DEBUG_MESSAGE_LEVEL)
#define WCDLI_PRINT_INFO_MESSAGE(MESSAGE)        WCDLI_PRINT_MESSAGE("[INF]: ",MESSAGE)
#else
#define WCDLI_PRINT_INFO_MESSAGE(MESSAGE)        {asm("NOP");}
#endif

#if (WCDLI_MESSAGE_LEVEL_WARNING <= WCDLI_DEBUG_MESSAGE_LEVEL)
#define WCDLI_PRINT_WARNING_MESSAGE(MESSAGE)     WCDLI_PRINT_MESSAGE("[WAR]: ",MESSAGE)
#else
#define WCDLI_PRINT_WARNING_MESSAGE(MESSAGE)     {asm("NOP");}
#endif

#if (WCDLI_MESSAGE_LEVEL_DANGER <= WCDLI_DEBUG_MESSAGE_LEVEL)
#define WCDLI_PRINT_DANGER_MESSAGE(MESSAGE)      WCDLI_PRINT_MESSAGE("[ERR]: ",MESSAGE)
#else
#define WCDLI_PRINT_DANGER_MESSAGE(MESSAGE)      {asm("NOP");}
#endif

/*!
 *
 */
#define WCDLI_PRINT_WRONG_COMMAND()              WCDLI_PRINT_DANGER_MESSAGE("Wrong Command!")

/*!
 *
 */
#define WCDLI_PRINT_WRONG_PARAM()                WCDLI_PRINT_DANGER_MESSAGE("Wrong Params!")

/*!
 * \}
 */


#ifdef __cplusplus
}
#endif

#endif // __WARCOMEB_WCDLI_H
