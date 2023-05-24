/******************************************************************************
* File Name:   main.c
*
* Created by Assoc. Prof. Wiroon Sriborrirux
*
*******************************************************************************/

/* Header file includes */
#include <blink_task.h>
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "capsense_task.h"
#include "adc_task.h"
#include "FreeRTOS.h"
#include "task.h"

#include "usrcmd.h"

int main()
{
    cy_rslt_t result;

    /* Initialize the board support package. */
    result = cybsp_init();
    CY_ASSERT(CY_RSLT_SUCCESS == result);

    /* Initialize the User LED */
       result = cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT,
           CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

       /* Initialize the GPIO */
       CY_ASSERT(CY_RSLT_SUCCESS == result);

    /* To avoid compiler warnings. */
       (void) result;

    /* Enable global interrupts. */
    	__enable_irq();

    /* Create the LED Blinking task. */
    xTaskCreate(task_blink, "blink task", configMINIMAL_STACK_SIZE,
        NULL, configMAX_PRIORITIES - 7, NULL);

    /* Initialize retarget-io to use the debug UART port. */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                        CY_RETARGET_IO_BAUDRATE);

#if defined(CY_DEVICE_PSOC6A512K)
    /* Initialize the QSPI serial NOR flash with clock frequency of 50 MHz. */
    const uint32_t bus_frequency = 50000000lu;
    cy_serial_flash_qspi_init(smifMemConfigs[0], CYBSP_QSPI_D0, CYBSP_QSPI_D1,
                                  CYBSP_QSPI_D2, CYBSP_QSPI_D3, NC, NC, NC, NC,
                                  CYBSP_QSPI_SCK, CYBSP_QSPI_SS, bus_frequency);

    /* Enable the XIP mode to get the Wi-Fi firmware from the external flash. */
    cy_serial_flash_qspi_enable_xip(true);
#endif


    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
    printf("\x1b[2J\x1b[;H");

    // Stack size in WORDs
    // Idle task = priority 0
    xTaskCreate(capsense_task, "CapSense"  ,configMINIMAL_STACK_SIZE*4,
    		NULL, 1, 0);

    // Start BDH Shell
    xTaskCreate(usrcmd_task, "usrcmd_task", configMINIMAL_STACK_SIZE*4,
    		0 /* args */ ,0 /* priority */, 0);

    // Start read ADC
    // Create and start the ADC task
    xTaskCreate(adc_task, "ADC Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();


    /* Should never get here. */
    CY_ASSERT(0);
}

/* [] END OF FILE */
