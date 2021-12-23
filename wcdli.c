/*
 * WC&DLI - Warcomeb Command & Debug Line Interface
 * Copyright (C) 2020-2021 Marco Giammarini <http://www.warcomeb.it>
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
#include "utility-buffer.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined (LIBOHIBOARD_VERSION)
#if !defined (LIBOHIBOARD_UART)
#error "WCDLI: You must enable UART peripheral."
#endif
#else
#if defined (__MCUXPRESSO)
#include "fsl_uart.h"
#include <stdbool.h>
#define TRUE                                     true
#define FALSE                                    false
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

#if !defined (WCDLI_DEFAULT_OPERATIVE_MODE)
#define WCDLI_DEFAULT_OPERATIVE_MODE             WCDLI_OPERATIVEMODE_COMMAND
#endif

#define WCDLI_NEW_LINE                           "\r\n"

#define WCDLI_BOARD_STRING                       "Board"

#define WCDLI_FIRMWARE_STRING                    "Firmware"

#define WCDLI_ENTER_COMMAND_MODE                 "+++"

#define WCDLI_ENTER_DEBUG_MODE                   "---"

static void resetBuffer (void);
static void prompt (void);
static void sayHello (void);
static void reboot (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);
static void help (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);
static void save (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);
static void manageDebugLevel (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);

#if defined (LIBOHIBOARD_VERSION)
static Uart_DeviceHandle mDevice = {0};
#else
#if defined (__MCUXPRESSO)
static UART_Type* mDevice = {0};
#endif
#endif

static WCDLI_MessageLevel_t mDebugLevel = WCDLI_DEBUG_MESSAGE_LEVEL;
static WCDLI_OperativeMode_t mOperativeMode = WCDLI_DEFAULT_OPERATIVE_MODE;

#if defined (LIBOHIBOARD_RTC)
static void setTime (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);
static void getTime (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);
#endif

static const WCDLI_Command_t mCommands[] =
{
    {"help"    , "Commands list"                    , 0, help},
    {"version" , "Project version"                  , 0, WCDLI_printProjectVersion},
    {"status"  , "Microcontroller status"           , 0, WCDLI_printStatus},
	{"debug"   , "Set/Get debug level with ?|[1-6]" , 0, manageDebugLevel},
#if defined (LIBOHIBOARD_RTC)
    {"settime" , "Set the current time"             , 0, setTime},
    {"gettime" , "Return the current time"          , 0, getTime},
#endif
    {"save"    , "Save parameters"                  , 0, WCDLI_save},
    {"reboot"  , "Reboot..."                        , 0, reboot},
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
          Uart_write(mDevice,&c,100);       \
    } while (0)

#define WCDLI_PRINT_NEW_LINE()                      \
    do {                                            \
        Uart_sendString(mDevice,WCDLI_NEW_LINE);\
    } while (0)

#if defined (LIBOHIBOARD_VERSION)
void callbackRx (struct _Uart_Device* dev, void* obj)
{
    (void)obj;

    uint8_t c = 0;
    Uart_read(dev,&c,100);

    UtilityBuffer_push(&mBufferDescriptor,c);
}
#else
#if defined (__MCUXPRESSO)
void WCDLI_callbackRx (UART_Type* base, void* obj)
{
    (void)obj;
    uint8_t c;
    // put the new received byte in the buffer
    uint32_t UartFlags = UART_GetStatusFlags(base);
    if ((kUART_RxDataRegFullFlag) & UartFlags)
    {
        c = UART_ReadByte(base);
        UtilityBuffer_push(&mBufferDescriptor,c);
    }
}
#else
#error "[ERROR] No interrupt implementation."
#endif
#endif

#if !defined (LIBOHIBOARD_VERSION)
#if defined (__MCUXPRESSO)
static void Uart_write (UART_Type* dev, const uint8_t* data, uint32_t timeout)
{
    UART_WriteBlocking(dev,(const uint8_t *)data,1);
}

static void Uart_sendString (UART_Type* dev, const char* text)
{
    uint16_t len = strlen(text);
    UART_WriteBlocking(dev,(const uint8_t *)text,len);
}

static void Uart_sendStringln (UART_Type* dev, const char* text)
{
    uint16_t len = strlen(text);
    UART_WriteBlocking(dev,(const uint8_t *)text,len);
    UART_WriteBlocking(dev,(const uint8_t *)"\r\n",2);
}
#else
#error "[ERROR] Implement UART wrapper functions."
#endif
#endif

static void resetBuffer (void)
{
    mCurrentCommandIndex = 0;
}

static void prompt (void)
{
    resetBuffer();
    Uart_sendString(mDevice, mPromptString);
}

static void printLibraryVersion (void)
{
    char versionString[64] = {0};
    char message[WCDLI_MAX_CHARS_PER_LINE] = {0};

#if defined (LIBOHIBOARD_VERSION)
    Utility_getVersionString(&WCDLI_FIRMWARE_VERSION,versionString);
#endif
    memset(message,0,sizeof(message));
    strcat(message,WCDLI_PROJECT_NAME);
    strcat(message," : ");
    strcat(message,versionString);
    Uart_sendStringln(mDevice, message);
}

static void sayHello (void)
{
    uint8_t i = 0;

    WCDLI_PRINT_NEW_LINE();
    WCDLI_PRINT_DIVIDING_LINE();
    WCDLI_PRINT_NEW_LINE();

    printLibraryVersion();

    WCDLI_PRINT_DIVIDING_LINE();
    WCDLI_PRINT_NEW_LINE();

#if (defined (PROJECT_NAME) || defined (PROJECT_COPYRIGTH))
#if defined (PROJECT_NAME)
    Uart_sendStringln(mDevice, PROJECT_NAME);
#endif
#if defined (PROJECT_COPYRIGTH)
    Uart_sendStringln(mDevice, PROJECT_COPYRIGTH);
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
    WCDLI_PRINT_CMD_MESSAGE("Reboot...");
    NVIC_SystemReset();
}

static void help (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    uint8_t noBlank = 0;
    uint8_t c = ' ';

    for (uint8_t i = 0; i < WCDLI_COMMANDS_SIZE; ++i)
    {
        noBlank = WCDLI_MAX_CHARS_COMMAND_LINE - strlen(mCommands[i].name);
        Uart_sendString(mDevice,mCommands[i].name);
        c = ' ';
        for (uint8_t j=0; j < noBlank; ++j)
        {
            Uart_write(mDevice,&c,100);
        }
        c = WCDLI_DIVIDING_DESCRIPTION_CHAR;
        Uart_write(mDevice,&c,100);
        c = ' ';
        Uart_write(mDevice,&c,100);
        Uart_sendStringln(mDevice,mCommands[i].description);
    }

    for (uint8_t i = 0; i < mExternalCommandsIndex; ++i)
    {
        noBlank = WCDLI_MAX_CHARS_COMMAND_LINE - strlen(mExternalCommands[i].name);
        Uart_sendString(mDevice,mExternalCommands[i].name);
        c = ' ';
        for (uint8_t j=0; j < noBlank; ++j)
        {
            Uart_write(mDevice,&c,100);
        }
        c = WCDLI_DIVIDING_DESCRIPTION_CHAR;
        Uart_write(mDevice,&c,100);
        c = ' ';
        Uart_write(mDevice,&c,100);
        Uart_sendStringln(mDevice,mExternalCommands[i].description);
    }

    for (uint8_t i = 0; i < mExternalAppsIndex; ++i)
    {
        noBlank = WCDLI_MAX_CHARS_COMMAND_LINE - strlen(mExternalApps[i].name);
        Uart_sendString(mDevice,mExternalApps[i].name);
        c = ' ';
        for (uint8_t j=0; j < noBlank; ++j)
        {
            Uart_write(mDevice,&c,100);
        }
        c = WCDLI_DIVIDING_DESCRIPTION_CHAR;
        Uart_write(mDevice,&c,100);
        c = ' ';
        Uart_write(mDevice,&c,100);
        Uart_sendStringln(mDevice,mExternalApps[i].description);

        mExternalApps[i].callback(mExternalApps[i].device,1,0);
    }

    WCDLI_PRINT_NEW_LINE();
}

static void manageDebugLevel (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    if ((argc == 2) && (strlen(&argv[1][0]) == 1))
    {
        if (argv[1][0] == '?')
        {
            WCDLI_debugByFormat(WCDLI_MESSAGELEVEL_NONE,"Current debug level is %d\r\n",mDebugLevel);
            return;
        }
    }

    // Send wrong command message!
    WCDLI_PRINT_WRONG_COMMAND();
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
    WCDLI_PRINT_COMMAND_NOT_IMPLEMENTED();
}
#endif

_weak void WCDLI_save (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    // TODO
    WCDLI_PRINT_COMMAND_NOT_IMPLEMENTED();
}

/*!
 * \param[out]    command:
 * \param[out] changeMode:
 */
static void parseCommand (WCDLI_Command_t* command, bool* changeMode)
{
    if (mOperativeMode == WCDLI_OPERATIVEMODE_COMMAND)
    {
        for (uint8_t i = 0; i < WCDLI_COMMANDS_SIZE; i++)
        {
            if (strncmp(mCurrentCommand, mCommands[i].name, strlen(mCommands[i].name)) == 0)
            {
                command->name        = mCommands[i].name;
                command->description = mCommands[i].description;
                command->callback    = mCommands[i].callback;
                command->device      = 0;

                *changeMode = FALSE;
                return;
            }
        }

        for (uint8_t i = 0; i < mExternalCommandsIndex; i++)
        {
            if (strncmp(mCurrentCommand, mExternalCommands[i].name, strlen(mExternalCommands[i].name)) == 0)
            {
                command->name        = mExternalCommands[i].name;
                command->description = mExternalCommands[i].description;
                command->callback    = mExternalCommands[i].callback;
                command->device      = 0;

                *changeMode = FALSE;
                return;
            }
        }

        for (uint8_t i = 0; i < mExternalAppsIndex; i++)
        {
            if (strncmp(mCurrentCommand, mExternalApps[i].name, strlen(mExternalApps[i].name)) == 0)
            {
                command->name        = mExternalApps[i].name;
                command->description = mExternalApps[i].description;
                command->callback    = mExternalApps[i].callback;
                command->device      = mExternalApps[i].device;

                *changeMode = FALSE;
                return;
            }
        }
    }

    if (((strncmp(mCurrentCommand, WCDLI_ENTER_COMMAND_MODE, strlen(WCDLI_ENTER_COMMAND_MODE)) == 0) &&
         (mOperativeMode == WCDLI_OPERATIVEMODE_DEBUG)) ||
        ((strncmp(mCurrentCommand, WCDLI_ENTER_DEBUG_MODE, strlen(WCDLI_ENTER_DEBUG_MODE)) == 0) &&
         (mOperativeMode == WCDLI_OPERATIVEMODE_COMMAND)))
    {
        command->name = mCurrentCommand;
        *changeMode = TRUE;
        return;
    }
    command->name = NULL;
    return;
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
    mParams[mNumberOfParams][j] = '\0';
    mNumberOfParams++;
}

_weak void WCDLI_printProjectVersion (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    char message[WCDLI_MAX_CHARS_PER_LINE] = {0};
    bool isHello = false;

    if ((app == 0) && (argc == 0) && (argv == 0))
    {
        isHello = true;
    }

    /* Board version */
#if defined (BOARD_VERSION_STRING)
    memset(message,0,sizeof(message));
    strcat(message,WCDLI_BOARD_STRING);
    strcat(message," : ");
    strcat(message,BOARD_VERSION_STRING);
    if (isHello)
    {
        Uart_sendStringln(mDevice, message);
    }
    else
    {
        WCDLI_debug(WCDLI_MESSAGELEVEL_NONE, message);
    }
#endif

#if defined (FIRMWARE_VERSION_STRING) || (defined (FIRMWARE_VERSION_MAJOR) && defined (FIRMWARE_VERSION_TIME))
#if defined (FIRMWARE_VERSION_STRING)
    memset(message,0,sizeof(message));
    strcat(message,WCDLI_FIRMWARE_STRING);
    strcat(message," : ");
    strcat(message,FIRMWARE_VERSION_STRING);
    if (isHello)
    {
        Uart_sendStringln(mDevice, message);
    }
    else
    {
        WCDLI_debug(WCDLI_MESSAGELEVEL_NONE, message);
    }
#else
    char versionString[64] = {0};
    Utility_Version_t v =
    {
        .f.major    = FIRMWARE_VERSION_MAJOR,
        .f.minor    = FIRMWARE_VERSION_MINOR,
        .f.subminor = FIRMWARE_VERSION_BUG,
        .f.time     = FIRMWARE_VERSION_TIME,
    };
    Utility_getVersionString(&v,versionString);
    memset(message,0,sizeof(message));
    strcat(message,WCDLI_FIRMWARE_STRING);
    strcat(message," : ");
    strcat(message,versionString);
    if (isHello)
    {
        Uart_sendStringln(mDevice, message);
    }
    else
    {
        WCDLI_debug(WCDLI_MESSAGELEVEL_NONE, message);
    }
#endif
#else
    WCDLI_PRINT_COMMAND_NOT_IMPLEMENTED();
#endif
}

_weak void WCDLI_printStatus (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    // Intentionally empty
    WCDLI_PRINT_COMMAND_NOT_IMPLEMENTED();
}

void WCDLI_ckeck (void)
{
    char c = '\0';
    WCDLI_Command_t command = {0};
    bool changeMode = FALSE;

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

            //WCDLI_PRINT_NEW_LINE();
            parseCommand(&command, &changeMode);

            if (command.name != NULL)
            {
                if (changeMode == FALSE)
                {
                    // Parse params
                    parseParams();
                    command.callback(command.device, mNumberOfParams, mParams);
                }
                else
                {
                    // TODO
                }
            }
            else
            {
                if (mOperativeMode == WCDLI_OPERATIVEMODE_COMMAND)
                {
                    // Command not found!
                    WCDLI_PRINT_NO_COMMAND();
                }
            }

            if (mOperativeMode == WCDLI_OPERATIVEMODE_COMMAND)
            {
                prompt();
            }
            break;
        }
        else
        {
            continue;
        }
    }
}

#if defined (LIBOHIBOARD_VERSION)
void WCDLI_init (Uart_DeviceHandle dev)
#else
#if defined (__MCUXPRESSO)
void WCDLI_init (UART_Type* dev)
#endif
#endif
{
    if (dev == NULL)
    {
#if defined (LIBOHIBOARD_VERSION)
        ohiassert(0);
#endif
        return;
    }
    // Save device handle
    mDevice = dev;
#if defined (LIBOHIBOARD_VERSION)
    Uart_addRxCallback(mDevice,callbackRx);
#else
    // This association is external...
#endif

    // Initialize buffer descriptor
    UtilityBuffer_init(&mBufferDescriptor,(uint8_t*)mBuffer, WCDLI_BUFFER_DIMENSION+1);

//    strcat(mPromptString,WCDLI_NEW_LINE);
    mPromptString[strlen(mPromptString)] = WCDLI_PROMPT_CHAR;
    strcat(mPromptString,"> ");

    // Send Hello World!
    sayHello();
    prompt();

}

WCDLI_Error_t WCDLI_addCommandByParam (const char* name,
                                       const char* description,
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

WCDLI_Error_t WCDLI_addAppByParam (const char* name,
                                   const char* description,
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
        Uart_write(mDevice,&c,100);
    }
    Uart_sendString(mDevice,name);
    for (uint8_t i=0; i < noBlank; ++i)
    {
        Uart_write(mDevice,&c,100);
    }
    c = WCDLI_DIVIDING_DESCRIPTION_CHAR;
    Uart_write(mDevice,&c,100);
    c = ' ';
    Uart_write(mDevice,&c,100);
    Uart_sendStringln(mDevice,description);
}

static inline void getDebugLevelString (WCDLI_MessageLevel_t level, char* ascii)
{
    switch (level)
    {
    case WCDLI_MESSAGELEVEL_FATAL:
        strcpy(ascii,"[FAT]: ");
        break;
    case WCDLI_MESSAGELEVEL_DANGER:
        strcpy(ascii,"[ERR]: ");
        break;
    case WCDLI_MESSAGELEVEL_WARNING:
        strcpy(ascii,"[WAR]: ");
        break;
    case WCDLI_MESSAGELEVEL_INFO:
        strcpy(ascii,"[INF]: ");
        break;
    case WCDLI_MESSAGELEVEL_DEBUG:
        strcpy(ascii,"[DBG]: ");
        break;
    case WCDLI_MESSAGELEVEL_NONE:
        // No string!
        break;
    default:
#if defined (LIBOHIBOARD_VERSION)
        ohiassert(0);
#endif
        break;
    }
}

void WCDLI_debug (WCDLI_MessageLevel_t level, const char* str)
{
    char buffer[WCDLI_MAX_CHARS_PER_LINE] = {0};

    if (level <= mDebugLevel)
    {
        strcat(buffer,mPromptString);

        if ((level != WCDLI_MESSAGELEVEL_NONE) && (mOperativeMode == WCDLI_OPERATIVEMODE_DEBUG))
        {
            getDebugLevelString(level,&buffer[strlen(mPromptString)]);
        }
        else if ((level == WCDLI_MESSAGELEVEL_NONE) && (mOperativeMode == WCDLI_OPERATIVEMODE_COMMAND))
        {
            strcat(buffer,"  ");
        }
        else
        {
            // Exit without print anything!
            return;
        }

        strcat(buffer,str);
        // Print string...
        Uart_sendStringln(mDevice,buffer);
    }
}

void WCDLI_debugByFormat (WCDLI_MessageLevel_t level, const char* format, ...)
{
	// Delete space for message level...
    char buffer[WCDLI_MAX_CHARS_PER_LINE] = {0};
    char msgLevel[20] = {0};

    if (level <= mDebugLevel)
    {
        va_list argptr;
        va_start(argptr,format);
        vsnprintf(buffer,WCDLI_MAX_CHARS_PER_LINE,format,argptr);
        va_end(argptr);

        // Print prompt chars
        strcat(msgLevel,mPromptString);

        // Print string...
        if ((level != WCDLI_MESSAGELEVEL_NONE) && (mOperativeMode == WCDLI_OPERATIVEMODE_DEBUG))
        {
            // Get message level string
            getDebugLevelString(level,&msgLevel[strlen(mPromptString)]);
        }
        else if ((level == WCDLI_MESSAGELEVEL_NONE) && (mOperativeMode == WCDLI_OPERATIVEMODE_COMMAND))
        {
            strcat(msgLevel,"  ");
        }
        else
        {
            // Exit without print anything!
            return;
        }

        Uart_sendString(mDevice,msgLevel);
        Uart_sendString(mDevice,buffer);
    }
}

#ifdef __cplusplus
}
#endif
