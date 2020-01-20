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

static char mPromptString[6] = {0};

/*!
 * The buffer for the incoming command.
 */
static char mBuffer[WCDLI_BUFFER_DIMENSION+1] = {0};

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

void callbackRx (struct _Uart_Device* dev, void* obj)
{
    (void)obj;

    uint8_t c = 0;
    Uart_read(dev,&c,100);

    UtilityBuffer_push(&mBufferDescriptor,c);
}

static void resetBuffer (void)
{

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

    WCDLI_printProjectVersion();
    WCDLI_PRINT_DIVIDING_LINE();
    Uart_sendString(WCDLI_PORT, "\r\n");
}

static void reboot (void)
{
//    sendString("Reboot...\r\n");
    NVIC_SystemReset();
}

_weak void WCDLI_printProjectVersion (void)
{

}

void WCDLI_ckeck (void)
{

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

    sayHello();
    prompt();

}


