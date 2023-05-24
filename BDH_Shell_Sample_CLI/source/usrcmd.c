/**
 * @file usrcmd.c
 * @author CuBeatSystems
 * @author Shinichiro Nakamura
 * @copyright
 * ===============================================================
 * Natural Tiny Shell (NT-Shell) Version 0.3.1
 * ===============================================================
 * Copyright (c) 2010-2016 Shinichiro Nakamura
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#include "ntopt.h"
#include "ntlibc.h"
#include "ntshell.h"
#include <stdio.h>
#include <stdlib.h>

#include "ntshell.h"
#include "ntlibc.h"
#include "psoc6_ntshell_port.h"
#include "cy_retarget_io.h"

#include "FreeRTOS.h"
#include "task.h"
#include "capsense_task.h"

//temp sensor
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "adc_task.h"


static ntshell_t ntshell;


// define variable
#define MAX_HISTORY_COUNT 10 // Set the maximum number of commands to store in history
#define MAX_LINE_LENGTH 128 // Define the maximum line length for command history

static char command_history[MAX_HISTORY_COUNT][MAX_LINE_LENGTH];

static int history_count = 0;

// Declare a global variable to hold the ADC task handle:
//TaskHandle_t adcTaskHandle = NULL;
// Define a global variable to hold the ADC task handle:
TaskHandle_t adcTaskHandle = NULL;


typedef int (*USRCMDFUNC)(int argc, char **argv);

// All command line function
static int usrcmd_ntopt_callback(int argc, char **argv, void *extobj);
static int usrcmd_help(int argc, char **argv);
static int usrcmd_info(int argc, char **argv);
static int usrcmd_clear(int argc, char **argv);
static int usrcmd_pargs(int argc, char **argv);
static int usrcmd_blink(int argc, char **argv);
static int usrcmd_capsense(int argc, char **argv);
static int usrcmd_kill(int argc, char **argv);   // function for cmd kill
static int usrcmd_reboot(int argc, char **argv); // function for cmd reboot board
static int usrcmd_history(int argc, char **argv); // function for cmd history
static int usrcmd_ADC(int argc, char **argv); // function for read ADC.
//static int usrcmd_temperature(int argc, char **argv); // read temperature


#ifdef configUSE_TRACE_FACILITY
#if configUSE_STATS_FORMATTING_FUNCTIONS ==1
static int usrcmd_list(int argc, char **argv);
#endif
#endif

typedef struct {
    char *cmd;
    char *desc;
    USRCMDFUNC func;
} cmd_table_t;

static const cmd_table_t cmdlist[] = {
    { "help", " Show all commands on Infineon's PSoC 6 Board", usrcmd_help },
    { "info", " Describe the developer team and released version", usrcmd_info },
    { "clear", " Clear the screen", usrcmd_clear },
	{ "pargs","print the list of arguments", usrcmd_pargs},
	{ "blink"," Enable start/stop USER LED blinking", usrcmd_blink},
	{ "touch"," Read the Capsense's values", usrcmd_capsense},
	//***************** My command line ***********************
	{ "ADC", "Enable start/stop", usrcmd_ADC },
	{ "history", " Show the history of executed commands", usrcmd_history },
	{ "kill", " Kill an RTOS task by its name", usrcmd_kill },
	{ "reboot", " Reboot the system of board", usrcmd_reboot },

//	{ "temp", " Read the temperature sensor's value", usrcmd_temperature },


#ifdef configUSE_TRACE_FACILITY 
#if configUSE_STATS_FORMATTING_FUNCTIONS ==1
    { "tasks"," print the list of RTOS Tasks", usrcmd_list},
#endif
#endif
};

/* Print welcome message and we don't lose callback function.
 The ntshell_callback function from the provided by the Natural Tiny Shell
(NT-Shell) library will provides some default actions and features like command history and basic line editing. */
static int print_welcome_message(ntshell_t *ntshell, void *extobj)
{
    static int first_time = 1;
    if (first_time)
    {
        printf("Started Command Line Interface via Bangsaen Design House (BDH) Shell\n");
        printf("Welcome to the Simple CLI! Type 'help' for a list of commands.\n");
        first_time = 0;
    }

    // Call the original ntshell_callback function to preserve default behavior
    return ntshell_callback(ntshell, extobj);
}



void usrcmd_task()
{

  setvbuf(stdin, NULL, _IONBF, 0);
  /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
  printf("\x1b[2J\x1b[;H");

//  printf("Started Command Line Interface via Bangsaen Design House (BDH) Shell\n");
  ntshell_init(
	       &ntshell,
	       ntshell_read,
	       ntshell_write,
//	       ntshell_callback,
		   print_welcome_message,
	       (void *)&ntshell);
  ntshell_set_prompt(&ntshell, "BDH_Shell:~$ ");
  vtsend_erase_display(&ntshell.vtsend);

//  printf("Started Command Line Interface via Bangsaen Design House (BDH) Shell\n");
//  printf("Welcome to the Simple CLI! Type 'help' for a list of commands.\n"); // Add welcome.

  ntshell_execute(&ntshell);
}


int usrcmd_execute(const char *text)
{
	// Store the command in the history
	    if (history_count < MAX_HISTORY_COUNT) {
	        strncpy(command_history[history_count], text, MAX_LINE_LENGTH);
	        command_history[history_count][MAX_LINE_LENGTH  - 1] = '\0';
	        history_count++;
	    } else {
	        // Shift the commands up in the history when the maximum count is reached
	        memmove(&command_history[0], &command_history[1], (MAX_HISTORY_COUNT - 1) * MAX_LINE_LENGTH );
	        strncpy(command_history[MAX_HISTORY_COUNT - 1], text, MAX_LINE_LENGTH );
	        command_history[MAX_HISTORY_COUNT - 1][MAX_LINE_LENGTH  - 1] = '\0';
	    }
    return ntopt_parse(text, usrcmd_ntopt_callback, 0);
}



static int usrcmd_ntopt_callback(int argc, char **argv, void *extobj)
{
    if (argc == 0) {
        return 0;
    }

    const cmd_table_t *p = &cmdlist[0];
    for (unsigned int i = 0; i < sizeof(cmdlist) / sizeof(cmdlist[0]); i++) {
        if (ntlibc_strcmp((const char *)argv[0], p->cmd) == 0) {
            return p->func(argc, argv);
        }
        p++;
    }
    printf("%s","Unknown command found.\n");
    return 0;
}

static int usrcmd_help(int argc, char **argv)
{
    const cmd_table_t *p = &cmdlist[0];
    for (unsigned int i = 0; i < sizeof(cmdlist) / sizeof(cmdlist[0]); i++) {
        printf("%s",p->cmd);
        printf("%s","\t:");
        printf("%s",p->desc);
        printf("%s","\n");
        p++;
    }
    return 0;
}

// ************************************
static int usrcmd_history(int argc, char **argv)
{
    printf("Command history:\n");
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, command_history[i]);
    }
    return 0;
}

static int usrcmd_reboot(int argc, char **argv)
{
    printf("Rebooting the system...\n");
    NVIC_SystemReset(); // This function is available in CMSIS (Cortex Microcontroller Software Interface Standard) library.
    					// Function performs a system reset by resetting the processor and all peripheral devices to their default state.
    return 0;
}

static int usrcmd_kill(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: kill <TaskName>\n");
        return 0;
    }

    TaskHandle_t task_to_delete = xTaskGetHandle(argv[1]);
    if (task_to_delete == NULL) {
        printf("Task not found: %s\n", argv[1]);
        return -1;
    }

    if (task_to_delete == xTaskGetCurrentTaskHandle()) {
        printf("Cannot delete the current task.\n");
        return -1;
    }

    vTaskDelete(task_to_delete);
    printf("Task '%s' deleted.\n", argv[1]);
    return 0;
}



static int usrcmd_ADC(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: ADC start|stop\n");
        return 0;
    }

    if (ntlibc_strcmp(argv[1], "start") == 0) {
        if (adcTaskHandle == NULL) {
            // Create the ADC task
            BaseType_t taskCreated = xTaskCreate(adc_task, "ADC_Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &adcTaskHandle);
            if (taskCreated != pdPASS) {
                printf("Failed to create the ADC task.\n");
                return -1;
            }
            printf("ADC task started.\n");
        } else {
            printf("ADC task is already running.\n");
        }
    } else if (ntlibc_strcmp(argv[1], "stop") == 0) {
        if (adcTaskHandle != NULL) {
            // Delete the ADC task
            vTaskDelete(adcTaskHandle);
            adcTaskHandle = NULL;
            printf("ADC task stopped.\n");
        } else {
            printf("ADC task is not running.\n");
        }
    } else {
        printf("Invalid command argument. Usage: ADC start|stop\n");
    }

    return 0;
}


//static int usrcmd_temperature(int argc, char **argv)
//{
//    // Replace the following lines with the actual code to read the temperature sensor
//    int temperature = rand() % 100; // Generate a random temperature value between 0 and 99
//    printf("Temperature: %d°C\n", temperature);
//
//    return 0;
//}
//static cy_rslt_t temperature_init(void)
//{
//    cy_rslt_t result;
//
//    // Initialize the ADC
//    result = cyhal_adc_init(&adc, CYBSP_TEMP_SENSOR_P10_0, NULL);
//    if (result != CY_RSLT_SUCCESS)
//    {
//        printf("ADC initialization failed with error: 0x%08lX\n", (unsigned long)result);
//        return result;
//    }
//
//    // Configure the ADC channel for the temperature sensor
//    cyhal_adc_channel_config_t channel_config = {
//        .enable_averaging = false,
//        .min_acquisition_time = CYHAL_ADC_ACQ_TIME_AUTO,
//        .resolution = 12
//    };
//
//    result = cyhal_adc_channel_init_diff(&adc_channel, &adc, CYBSP_TEMP_SENSOR_P10_0, CYHAL_ADC_TEMP, &channel_config);
//    if (result != CY_RSLT_SUCCESS)
//    {
//        printf("ADC channel initialization failed with error: 0x%08lX\n", (unsigned long)result);
//        return result;
//    }
//
//    return CY_RSLT_SUCCESS;
//}

//
//static int usrcmd_temperature(int argc, char **argv)
//{
//    cy_rslt_t result;
//    int16_t adc_result;
//    float temperature;
//
//    // Initialize the temperature sensor
//    result = temperature_init();
//    if (result != CY_RSLT_SUCCESS)
//    {
//        printf("Temperature sensor initialization failed with error: 0x%08lX\n", (unsigned long)result);
//        return -1;
//    }
//
//    // Start the ADC conversion
//    result = cyhal_adc_start_conversion(&adc);
//    if (result != CY_RSLT_SUCCESS)
//    {
//        printf("ADC conversion failed with error: 0x%08lX\n", (unsigned long)result);
//        return -1;
//    }
//
//    // Read the ADC result
//    adc_result = cyhal_adc_read_u16(&adc_channel);
//
//    // Convert the ADC result to temperature in degree Celsius
//    temperature = (float)Cy_SysAnalog_GetDieTemp(adc_result);
//
//    printf("Temperature: %.2f°C\n", temperature);
//
//    // Free the resources
//    cyhal_adc_channel_free(&adc_channel);
//    cyhal_adc_free(&adc);
//
//    return 0;
//}
//




// ************************************

static int usrcmd_info(int argc, char **argv)
{
    if (argc != 2) {
        printf("%s","info sys\n");
        printf("%s","info ver\n");
        printf("%s", "info BDH_Shell\n");
        return 0;
    }
    if (ntlibc_strcmp(argv[1], "sys") == 0) {
        printf("%s","Infineon's PSoC 6 CLI Console by Bangsaen Design House\n");
        return 0;
    }
    if (ntlibc_strcmp(argv[1], "ver") == 0) {
        printf("%s","Version 0.0.1, Dec 2022\n");
        return 0;
    }
    if (ntlibc_strcmp(argv[1], "BDH_Shell") == 0) {
            printf("%s", "BDH_Shell Version V.0.1, Apr 2023\n");
            return 0;
        }

    printf("%s","Unknown sub command found\n");
    return -1;
}


static int usrcmd_clear(int argc, char **argv)
{
    vtsend_erase_display_home(&ntshell.vtsend);
    return 0;
}

static int usrcmd_pargs(int argc, char **argv)
{
    printf("ARGC = %d\n",argc);

    for(int i =0;i<argc;i++)
    {
        printf("argv[%d] = %s\n",i,argv[i]);
    }
    return 0;

}

static int usrcmd_blink(int argc, char **argv)
{
    if (argc != 2) {
        printf("%s","blink start\n");
        printf("%s","blink stop\n");
        return 0;
    }
    if (ntlibc_strcmp(argv[1], "start") == 0) {
        printf("%s","Starting blinking...\n");
        LED_blink_control(1);
        return 0;
    }
    if (ntlibc_strcmp(argv[1], "stop") == 0) {
        printf("%s","Stopping blinking...\n");
        LED_blink_control(0);
        return 0;
    }
    printf("%s","Unknown sub command found\n");
    return -1;
}

// History function:




static int usrcmd_capsense(int argc, char **argv)
{
	vtsend_erase_display_home(&ntshell.vtsend);
	vtsend_set_cursor(&ntshell.vtsend, 0);
	live_print_capsense_status();
	vtsend_set_cursor(&ntshell.vtsend, 1);
    return 0;
}

void live_print_capsense_status(void)
{
	/* Variable for storing character read from terminal */
	uint8_t uart_read_value;
	for (;;)
	    {
			print_capsense_status();
			vtsend_cursor_up(&ntshell.vtsend, 3);
	        /* Check if 'Enter' key was pressed */
	        if (cyhal_uart_getc(&cy_retarget_io_uart_obj, &uart_read_value, 1)
	             == CY_RSLT_SUCCESS)
	        {
	            if (uart_read_value == '\r')
	            	break;
	        }
	}
	printf("\n\n\n");
}

#ifdef configUSE_TRACE_FACILITY
#if configUSE_STATS_FORMATTING_FUNCTIONS ==1
static int usrcmd_list(int argc,char **argv)
{
    // 40 bytes/task + some margin
    char buff[40*10 + 100];

    vTaskList( buff );
    printf("Name          State Priority   Stack  Num\n");
    printf("------------------------------------------\n");
    printf("%s",buff);

    printf("‘B’ – Blocked\n‘R’ – Ready\n‘D’ – Deleted (waiting clean up)\n‘S’ – Suspended, or Blocked without a timeout\n");
    printf("Stack = bytes free at highwater\n");
    return 0;
}

#endif
#endif
