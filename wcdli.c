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

#include "wcdli.h"

#include <stdlib.h>

#if !defined (LIBOHIBOARD_UART)
#error "WCDLI: You must enable UART peripheral."
#endif

//#if ((!defined (WCDLI_PORT))   || \
//     (!defined (WCDLI_PIN_RX)) || \
//     (!defined (WCDLI_PIN_TX)) || \
//     (!defined (WCDLI_BAUDRATE)))
//#error "WCDLI: You must define UART device and other informations."
//#endif

#if !defined (WCDLI_PORT_CLOCK_SOURCE)
#if defined (LIBOHIBOARD_ST_STM32)
#define WCDLI_PORT_CLOCK_SOURCE                  UART_CLOCKSOURCE_PCLK
#endif
#endif

#if !defined (WCDLI_PORT_OVERSAMPLING)
#if defined (LIBOHIBOARD_ST_STM32)
#define WCDLI_PORT_OVERSAMPLING                  8
#endif
#endif

#if !defined (WCDLI_MAX_CHARS_PER_LINE)
#define WCDLI_MAX_CHARS_PER_LINE                 80
#endif

#if !defined (WCDLI_MAX_CHARS_COMMAND_LINE)
#define WCDLI_MAX_CHARS_COMMAND_LINE             30
#endif

#if !defined (WCDLI_MAX_INDENTATION_CHAR)
#define WCDLI_MAX_INDENTATION_CHAR               4
#endif

#if !defined (WCDLI_MAX_PARAMS)
#define WCDLI_MAX_PARAMS                         10
#endif

#if !defined (WCDLI_MAX_EXTERNAL_COMMAND)
#define WCDLI_MAX_EXTERNAL_COMMAND               20
#endif

#if !defined (WCDLI_MAX_EXTERNAL_APP)
#define WCDLI_MAX_EXTERNAL_APP                   10
#endif

#if !defined (WCDLI_DIVIDING_CHAR)
#define WCDLI_DIVIDING_CHAR                      '*'
#endif

#if !defined (WCDLI_PROMPT_CHAR)
#define WCDLI_PROMPT_CHAR                        '%'
#endif

#if !defined (WCDLI_DIVIDING_DESCRIPTION_CHAR)
#define WCDLI_DIVIDING_DESCRIPTION_CHAR          ':'
#endif

#if !defined (WCDLI_BUFFER_DIMENSION)
#define WCDLI_BUFFER_DIMENSION                   0x00FFu
#endif

#define WCDLI_NEW_LINE                           "\r\n"

static void resetBuffer (void);
static void prompt (void);
static void sayHello (void);
static void reboot (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);
static void help (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);
static void save (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);

#if defined (LIBOHIBOARD_RTC)
static void setTime (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);
static void getTime (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);
#endif

static const WCDLI_Command_t mCommands[] =
{
    {"help"    , "Commands list"          , 0, help},
    {"version" , "Project version"        , 0, WCDLI_printProjectVersion},
    {"status"  , "Microcontroller status" , 0, WCDLI_printStatus},
#if defined (LIBOHIBOARD_RTC)
    {"settime" , "Set the current time"   , 0, setTime},
    {"gettime" , "Return the current time", 0, getTime},
#endif
    {"save"    , "Save parameters"        , 0, save},
    {"reboot"  , "Reboot..."              , 0, reboot},
};

#define WCDLI_COMMANDS_SIZE                      (sizeof(mCommands) / sizeof(mCommands[0]))

static WCDLI_Command_t mExternalCommands[WCDLI_MAX_EXTERNAL_COMMAND];
static uint8_t mExternalCommandsIndex = 0;

static WCDLI_Command_t mExternalApps[WCDLI_MAX_EXTERNAL_APP];
static uint8_t mExternalAppsIndex = 0;

static char mPromptString[6] = {0};

/*!
 * The buffer for the incoming command.
 */
static char mBuffer[WCDLI_BUFFER_DIMENSION+1] = {0};

static uint32_t mCurrentCommandIndex = 0;
static char mCurrentCommand[WCDLI_MAX_CHARS_PER_LINE] = {0};

static char mParams[WCDLI_MAX_PARAMS][WCDLI_BUFFER_SIZE];
static uint8_t mNumberOfParams = 0;

/*!
 *
 */
static UtilityBuffer_Descriptor mBufferDescriptor;

#define WCDLI_PRINT_DIVIDING_LINE()              \
    do {                                         \
      uint8_t c = WCDLI_DIVIDING_CHAR;           \
      for (i=0; i<WCDLI_MAX_CHARS_PER_LINE; ++i) \
          Uart_write(WCDLI_PORT,&c,100);         \
    } while (0)

#define WCDLI_PRINT_NEW_LINE()                     \
    do {                                           \
        Uart_sendString(WCDLI_PORT,WCDLI_NEW_LINE);\
    } while (0)

void callbackRx (struct _Uart_Device* dev, void* obj)
{
    (void)obj;

    uint8_t c = 0;
    Uart_read(dev,&c,100);

    UtilityBuffer_push(&mBufferDescriptor,c);
}

static void resetBuffer (void)
{
    mCurrentCommandIndex = 0;
}

static void prompt (void)
{
    resetBuffer();
    Uart_sendString(WCDLI_PORT, mPromptString);
}

static void sayHello (void)
{
    uint8_t i = 0;

    WCDLI_PRINT_NEW_LINE();
    WCDLI_PRINT_DIVIDING_LINE();
    WCDLI_PRINT_NEW_LINE();

#if (defined (PROJECT_NAME) || defined (PROJECT_COPYRIGTH))
#if defined (PROJECT_NAME)
    Uart_sendStringln(WCDLI_PORT, PROJECT_NAME);
#endif
#if defined (PROJECT_COPYRIGTH)
    Uart_sendStringln(WCDLI_PORT, PROJECT_COPYRIGTH);
#endif
    WCDLI_PRINT_DIVIDING_LINE();
    WCDLI_PRINT_NEW_LINE();
#endif

    WCDLI_printProjectVersion(0,0,0);
    WCDLI_PRINT_DIVIDING_LINE();
    WCDLI_PRINT_NEW_LINE();
}

static void reboot (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    Uart_sendStringln(WCDLI_PORT,"Reboot...");
    NVIC_SystemReset();
}

static void help (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    uint8_t noBlank = 0;
    uint8_t c = ' ';

    for (uint8_t i = 0; i < WCDLI_COMMANDS_SIZE; ++i)
    {
        noBlank = WCDLI_MAX_CHARS_COMMAND_LINE - strlen(mCommands[i].name);
        Uart_sendString(WCDLI_PORT,mCommands[i].name);
        c = ' ';
        for (uint8_t j=0; j < noBlank; ++j)
        {
            Uart_write(WCDLI_PORT,&c,100);
        }
        c = WCDLI_DIVIDING_DESCRIPTION_CHAR;
        Uart_write(WCDLI_PORT,&c,100);
        c = ' ';
        Uart_write(WCDLI_PORT,&c,100);
        Uart_sendStringln(WCDLI_PORT,mCommands[i].description);
    }

    for (uint8_t i = 0; i < mExternalCommandsIndex; ++i)
    {
        noBlank = WCDLI_MAX_CHARS_COMMAND_LINE - strlen(mExternalCommands[i].name);
        Uart_sendString(WCDLI_PORT,mExternalCommands[i].name);
        c = ' ';
        for (uint8_t j=0; j < noBlank; ++j)
        {
            Uart_write(WCDLI_PORT,&c,100);
        }
        c = WCDLI_DIVIDING_DESCRIPTION_CHAR;
        Uart_write(WCDLI_PORT,&c,100);
        c = ' ';
        Uart_write(WCDLI_PORT,&c,100);
        Uart_sendStringln(WCDLI_PORT,mExternalCommands[i].description);
    }

    for (uint8_t i = 0; i < mExternalAppsIndex; ++i)
    {
        noBlank = WCDLI_MAX_CHARS_COMMAND_LINE - strlen(mExternalApps[i].name);
        Uart_sendString(WCDLI_PORT,mExternalApps[i].name);
        c = ' ';
        for (uint8_t j=0; j < noBlank; ++j)
        {
            Uart_write(WCDLI_PORT,&c,100);
        }
        c = WCDLI_DIVIDING_DESCRIPTION_CHAR;
        Uart_write(WCDLI_PORT,&c,100);
        c = ' ';
        Uart_write(WCDLI_PORT,&c,100);
        Uart_sendStringln(WCDLI_PORT,mExternalApps[i].description);

        mExternalApps[i].callback(mExternalApps[i].device,1,0);
    }
}

#if defined (LIBOHIBOARD_RTC)
static void setTime (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    if (argc == 2)
    {
        uint32_t myTime = atoi (&argv[1][0]);
        Rtc_setTime(OB_RTC0, myTime);
        return;
    }

    // Send wrong command message!
    WCDLI_PRINT_WRONG_COMMAND();
}

static void getTime (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    // TODO
    Uart_sendStringln(WCDLI_PORT,"Command not implemented!");
}
#endif

static void save (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    // TODO
    Uart_sendStringln(WCDLI_PORT,"Command not implemented!");
}

static void parseCommand (WCDLI_Command_t* command)
{
    for (uint8_t i = 0; i < WCDLI_COMMANDS_SIZE; i++)
    {
        if (strncmp(mCurrentCommand, mCommands[i].name, strlen(mCommands[i].name)) == 0)
        {
            command->name        = mCommands[i].name;
            command->description = mCommands[i].description;
            command->callback    = mCommands[i].callback;
            command->device      = 0;
            return;
        }
    }

    for (uint8_t i = 0; i < WCDLI_MAX_EXTERNAL_COMMAND; i++)
    {
        if (strncmp(mCurrentCommand, mExternalCommands[i].name, strlen(mExternalCommands[i].name)) == 0)
        {
            command->name        = mExternalCommands[i].name;
            command->description = mExternalCommands[i].description;
            command->callback    = mExternalCommands[i].callback;
            command->device      = 0;
            return;
        }
    }

    command->name = NULL;
}

static void parseParams (void)
{
    // Buffer counter
    uint8_t i = 0;
    // Counter for each parameter
    uint8_t j = 0;
    // to know if double quote is for open or close
    bool isStringOpen = FALSE;

    mNumberOfParams = 0;

    for (i = 0; i < (mCurrentCommandIndex -2); ++i)
    {
        if ((mCurrentCommand[i] != ' ') && (mCurrentCommand[i] != '\"'))
        {
            mParams[mNumberOfParams][j++] = mCurrentCommand[i];
        }
        else if ((mCurrentCommand[i] == '\"') && (isStringOpen == FALSE))
        {
            isStringOpen = TRUE;
        }
        else if ((mCurrentCommand[i] == '\"') && (isStringOpen == TRUE))
        {
            isStringOpen = FALSE;
            mParams[mNumberOfParams][j] = '\0';
            // The string parameter is at the end, reset the counter
            j = 0;
            // Increase the parameter number
            mNumberOfParams++;
        }
        else if ((mCurrentCommand[i] == ' ') && (isStringOpen == TRUE))
        {
            mParams[mNumberOfParams][j++] = mCurrentCommand[i];
        }
        else if (((mCurrentCommand[i] == ' ') && (mCurrentCommand[i-1] == ' ')) ||
                 ((mCurrentCommand[i] == ' ') && (mCurrentCommand[i-1] == '\"')))
        {
            continue;
        }
        else
        {
            mParams[mNumberOfParams][j] = '\0';
            j = 0;
            mNumberOfParams++;
        }
    }
    mNumberOfParams++;
}

_weak void WCDLI_printProjectVersion (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    // Intentionally empty
    Uart_sendStringln(WCDLI_PORT,"Command not implemented!");
}

_weak void WCDLI_printStatus (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    // Intentionally empty
    Uart_sendStringln(WCDLI_PORT,"Command not implemented!");
}

void WCDLI_ckeck (void)
{
    char c = '\0';
    WCDLI_Command_t command = {0};

    while (!UtilityBuffer_isEmpty(&mBufferDescriptor))
    {
        UtilityBuffer_pull(&mBufferDescriptor,&c);

        // Use the back space for delete char
        if ((c == '\b') && (mCurrentCommandIndex > 0))
        {
            mCurrentCommandIndex--;
            continue;
        }

        if ((c == '\b') && (mCurrentCommandIndex == 0))
        {
            continue;
        }

        mCurrentCommand[mCurrentCommandIndex++] = c;

        if ((mCurrentCommandIndex != 0) &&
            (mCurrentCommand[mCurrentCommandIndex-2] == '\r') &&
            (mCurrentCommand[mCurrentCommandIndex-1] == '\n'))
        {
            // parse command

            // No message, only enter command!
            if (mCurrentCommandIndex == 2)
            {
                prompt();
                return;
            }

            WCDLI_PRINT_NEW_LINE();
            parseCommand(&command);

            if (command.name != NULL)
            {
                // Parse params
                parseParams();
                command.callback(command.device,mNumberOfParams,mParams);
            }
            else
            {
                // Command not found!
                WCDLI_PRINT_DANGER_MESSAGE("Command not found!");
            }

            prompt();
            break;
        }
        else
        {
            continue;
        }
    }
}

void WCDLI_init (Uart_DeviceHandle dev)
{
	Uart_addRxCallback(dev,callbackRx);

    // Initialize buffer descriptor
    UtilityBuffer_init(&mBufferDescriptor, mBuffer, WCDLI_BUFFER_DIMENSION+1);

    strcat(mPromptString,WCDLI_NEW_LINE);
    mPromptString[strlen(mPromptString)] = WCDLI_PROMPT_CHAR;
    strcat(mPromptString,"> ");

    // Send Hello World!
    sayHello();
    prompt();

}

WCDLI_Error_t WCDLI_addCommandByParam (char* name,
                                       char* description,
                                       WCDLI_CommandCallback_t callback)
{
    ohiassert(callback != NULL);

    if (callback == NULL)
    {
        return WCDLI_ERROR_EMPTY_CALLBACK;
    }

    if (mExternalCommandsIndex < WCDLI_MAX_EXTERNAL_COMMAND)
    {
        mExternalCommands[mExternalCommandsIndex].name        = name;
        mExternalCommands[mExternalCommandsIndex].description = description;

        mExternalCommands[mExternalCommandsIndex].device      = 0;
        mExternalCommands[mExternalCommandsIndex].callback    = callback;

        mExternalCommandsIndex++;

        return WCDLI_ERROR_SUCCESS;
    }
    else
    {
        return WCDLI_ERROR_ADD_COMMAND_FAIL;
    }
}

WCDLI_Error_t WCDLI_addCommand (WCDLI_Command_t* command)
{
    ohiassert(command != NULL);

    if (command != NULL)
    {
        return WCDLI_addCommandByParam(command->name, command->description, command->callback);
    }

    return WCDLI_ERROR_ADD_COMMAND_FAIL;
}

WCDLI_Error_t WCDLI_addAppByParam (char* name,
                                   char* description,
                                   void* app,
                                   WCDLI_CommandCallback_t callback)
{
    ohiassert(callback != NULL);

    if (callback == NULL)
    {
        return WCDLI_ERROR_EMPTY_CALLBACK;
    }

    if (mExternalAppsIndex < WCDLI_MAX_EXTERNAL_APP)
    {
        mExternalApps[mExternalAppsIndex].name        = name;
        mExternalApps[mExternalAppsIndex].description = description;

        mExternalApps[mExternalAppsIndex].device      = app;
        mExternalApps[mExternalAppsIndex].callback    = callback;

        mExternalAppsIndex++;

        return WCDLI_ERROR_SUCCESS;
    }
    else
    {
        return WCDLI_ERROR_ADD_APP_FAIL;
    }
}

WCDLI_Error_t WCDLI_addApp (WCDLI_Command_t* app)
{
    ohiassert(app != NULL);

    if (app != NULL)
    {
        return WCDLI_addAppByParam(app->name,
                                   app->description,
                                   app->device,
                                   app->callback);
    }

    return WCDLI_ERROR_ADD_APP_FAIL;
}


void WCDLI_helpLine (const char* name, const char* description)
{
    uint8_t noBlank = 0;
    uint8_t c = ' ';

    noBlank = WCDLI_MAX_CHARS_COMMAND_LINE - strlen(name) - WCDLI_MAX_INDENTATION_CHAR;
    for (uint8_t i=0; i < WCDLI_MAX_INDENTATION_CHAR; ++i)
    {
        Uart_write(WCDLI_PORT,&c,100);
    }
    Uart_sendString(WCDLI_PORT,name);
    for (uint8_t i=0; i < noBlank; ++i)
    {
        Uart_write(WCDLI_PORT,&c,100);
    }
    c = WCDLI_DIVIDING_DESCRIPTION_CHAR;
    Uart_write(WCDLI_PORT,&c,100);
    c = ' ';
    Uart_write(WCDLI_PORT,&c,100);
    Uart_sendStringln(WCDLI_PORT,description);
}

