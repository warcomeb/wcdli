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

#if !defined (LIBOHIBOARD_UART)
#error "WCDLI: You must enable UART peripheral."
#endif

#if ((!defined (WCDLI_PORT))   || \
     (!defined (WCDLI_PIN_RX)) || \
     (!defined (WCDLI_PIN_TX)) || \
     (!defined (WCDLI_BAUDRATE)))
#error "WCDLI: You must define UART device and other informations."
#endif

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

#if !defined (WCDLI_MAX_PARAMS)
#define WCDLI_MAX_PARAMS                         10
#endif

#if !defined (WCDLI_DIVIDING_CHAR)
#define WCDLI_DIVIDING_CHAR                      '*'
#endif

#if !defined (WCDLI_PROMPT_CHAR)
#define WCDLI_PROMPT_CHAR                        '%'
#endif

#if !defined (WCDLI_BUFFER_DIMENSION)
#define WCDLI_BUFFER_DIMENSION                   0x00FFu
#endif

#define WCDLI_NEW_LINE                           "\r\n"

typedef struct _WCDLI_Command_t
{
    char *name;
    char *description;
    void *device;
    WCDLI_CommandCallback_t callback;
} WCDLI_Command_t;

static void resetBuffer (void);
static void prompt (void);
static void sayHello (void);
static void reboot (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);
static void help (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);
static void save (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE]);

static const WCDLI_Command_t mCommands[] =
{
    {"help"   , "Commands list"         , 0, help},
    {"version", "Project version"       , 0, WCDLI_printProjectVersion},
    {"status" , "Microcontroller status", 0, WCDLI_printStatus},
    {"save"   , "Save parameters"       , 0, save},
    {"reboot" , "Reboot..."             , 0, reboot},
};

#define WCDLI_COMMANDS_SIZE                      (sizeof(mCommands) / sizeof(mCommands[0]))

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

    Uart_sendString(WCDLI_PORT, "\r\n");
    WCDLI_PRINT_DIVIDING_LINE();
    Uart_sendString(WCDLI_PORT, "\r\n");

#if (defined (PROJECT_NAME) || defined (PROJECT_COPYRIGTH))
#if defined (PROJECT_NAME)
    Uart_sendStringln(WCDLI_PORT, PROJECT_NAME);
#endif
#if defined (PROJECT_COPYRIGTH)
    Uart_sendStringln(WCDLI_PORT, PROJECT_COPYRIGTH);
#endif
    WCDLI_PRINT_DIVIDING_LINE();
    Uart_sendString(WCDLI_PORT, "\r\n");
#endif

    WCDLI_printProjectVersion(0,0,0);
    WCDLI_PRINT_DIVIDING_LINE();
    Uart_sendString(WCDLI_PORT, "\r\n");
}

static void reboot (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    NVIC_SystemReset();
}

static void help (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    uint8_t noBlank = 0;
    for (uint8_t i = 0; i < WCDLI_COMMANDS_SIZE; ++i)
    {
        noBlank = WCDLI_MAX_CHARS_COMMAND_LINE - strlen(mCommands[i].name);
        Uart_sendString(WCDLI_PORT,mCommands[i].name);
        for (uint8_t j=0; j < noBlank; ++j)
        {
            Uart_putChar(WCDLI_PORT,' ');
        }
        Uart_putChar(WCDLI_PORT,':');
        Uart_sendStringln(WCDLI_PORT,mCommands[i].description);
    }
}

static void save (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    // TODO
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
            command->device      = mCommands[i].device;
            return;
        }
    }

    command->name = NULL;
}

static void parseParams (void)
{

}

_weak void WCDLI_printProjectVersion (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{

}

_weak void WCDLI_printStatus (void* app, int argc, char argv[][WCDLI_BUFFER_SIZE])
{
    // Intentionally empty
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

        mCurrentCommand[mCurrentCommandIndex] = c;

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

void WCDLI_init (void)
{
    Uart_Config config =
    {
        .rxPin        = WCDLI_PIN_RX,
        .txPin        = WCDLI_PIN_TX,

        .baudrate     = WCDLI_BAUDRATE,

        .dataBits     = UART_DATABITS_EIGHT,
        .parity       = UART_PARITY_NONE,

        .clockSource  = WCDLI_PORT_CLOCK_SOURCE,
        .oversampling = WCDLI_PORT_OVERSAMPLING,

        .mode         = UART_MODE_BOTH,

        .callbackRx   = callbackRx,
    };

    // Initialize buffer descriptor
    UtilityBuffer_init(&mBufferDescriptor, mBuffer, WCDLI_BUFFER_DIMENSION+1);

    Uart_init(WCDLI_PORT, &config);

    strcat(mPromptString,WCDLI_NEW_LINE);
    mPromptString[strlen(mPromptString)] = WCDLI_PROMPT_CHAR;
    strcat(mPromptString,"> ");

    // Send Hello World!
    sayHello();
    prompt();

}


