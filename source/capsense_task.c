#include <stdio.h>

#include "cybsp.h"
#include "cyhal.h"
#include "cycfg.h"
#include "cycfg_capsense.h"
#include "cy_retarget_io.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "capsense_task.h"

static void capsense_init(void);
static void process_touch(void);
static void capsense_isr(void);
static void capsense_end_of_scan_callback(cy_stc_active_scan_sns_t* active_scan_sns_ptr);
static void capsense_register_deepsleep(void);

static QueueHandle_t capsense_done; // Semaphore set in Capsense Callback End of Scan

/*******************************************************************************
* Global Variables
*******************************************************************************/
uint16_t slider_pos = 0;
static uint8_t button0_value = 0;
static uint8_t button1_value = 0;

uint8_t slider_touched = 0;

//*******************************************************************************/


/*******************************************************************************
* Function Name: task_capsense
********************************************************************************
* Summary:
*  Task that initializes the CapSense block and processes the touch input.
*
* Parameters:
*  void *param : Task parameter defined during task creation (unused)
*
*******************************************************************************/
void capsense_task(void* param)
{
    (void)param;

    capsense_done = xQueueCreateCountingSemaphore(1,0);

    capsense_init();

    Cy_CapSense_ScanAllWidgets(&cy_capsense_context);

    for(;;)
    {
        xSemaphoreTake(capsense_done,portMAX_DELAY);

        Cy_CapSense_ProcessAllWidgets(&cy_capsense_context);
        process_touch();
        Cy_CapSense_ScanAllWidgets(&cy_capsense_context);
        vTaskDelay(50); // ~20hz update

    }
}

/*******************************************************************************
* Function Name: process_touch
*******************************************************************************/
static void process_touch(void)
{
    /* Variables used to store previous touch information */
    static uint32_t button0_status_prev = 0;
    static uint32_t button1_status_prev = 0;
    static uint16_t slider_pos_prev = 0;

    uint32_t button0_status = 0;
    uint32_t button1_status = 0;
//    uint16_t slider_pos = 0;
//    uint8_t slider_touched = 0;
    cy_stc_capsense_touch_t *slider_touch;

// Process the buttons

    button0_status = Cy_CapSense_IsWidgetActive(CY_CAPSENSE_BUTTON0_WDGT_ID,&cy_capsense_context);
    button1_status = Cy_CapSense_IsSensorActive( CY_CAPSENSE_BUTTON1_WDGT_ID, CY_CAPSENSE_BUTTON1_SNS0_ID, &cy_capsense_context);

    if((0u != button0_status) && (0u == button0_status_prev))
    {
      //  printf("Button 0 pressed\n");
    	button0_value = 1;
    }else
    {
    	button0_value = 0;
    }

    if((0u != button1_status) && (0u == button1_status_prev))
    {
      //  printf("Button 1 pressed\n");
    	button1_value = 1;
    }else
    {
    	button1_value = 0;
    }

// Process the slider
    slider_touch = Cy_CapSense_GetTouchInfo( CY_CAPSENSE_LINEARSLIDER0_WDGT_ID, &cy_capsense_context);
    slider_pos = (slider_touch->ptrPosition->x / 3); // Transform 0-300 into 0-100
    slider_touched = slider_touch->numPosition;

    if((0u != slider_touched) && (slider_pos_prev != slider_pos ))
    {
      //  printf("Slider position %d\n",slider_pos);
    }

    button0_status_prev = button0_status;
    button1_status_prev = button1_status;
    slider_pos_prev = slider_pos;
}

/*******************************************************************************
* Function Name: capsense_status
********************************************************************************
* Summary:
*  This function reads the slider, button0 and button1 value.
*******************************************************************************/
void print_capsense_status(void)
{
	printf("Please touch the CAPSENSE to be started ;) , [ENTER] to exit! \r\n");
	if((0u != slider_touched))
	{
		printf("Slider's position %3d\n",slider_pos);
//		if(button0_value == 1)
//			printf("Button0 pressed    \n");
//		else
//			printf("Button0            \n");
//		if(button1_value == 1)
//			printf("Button1 pressed    \n");
//		else
//			printf("Button1            \n");
	}
}


/*******************************************************************************
* Function Name: capsense_init
********************************************************************************
* Summary:
*  This function initializes the CSD HW block, and configures the CapSense
*  interrupt.
*******************************************************************************/
static void capsense_init(void)
{
    Cy_CapSense_Init(&cy_capsense_context);
    
    static const cy_stc_sysint_t capSense_intr_config =
    {
        .intrSrc = csd_interrupt_IRQn,
        .intrPriority = 7,
    };

    /* Initialize CapSense interrupt */
    Cy_SysInt_Init(&capSense_intr_config, &capsense_isr);
    NVIC_ClearPendingIRQ(capSense_intr_config.intrSrc);
    NVIC_EnableIRQ(capSense_intr_config.intrSrc);

    capsense_register_deepsleep();

    Cy_CapSense_RegisterCallback(CY_CAPSENSE_END_OF_SCAN_E,
                                              capsense_end_of_scan_callback, &cy_capsense_context);
    
    Cy_CapSense_Enable(&cy_capsense_context);
}

/*******************************************************************************
* Function Name: capsense_end_of_scan_callback
********************************************************************************
* Summary:
*  CapSense end of scan callback function. This function sends a command to
*  CapSense task to process scan.
*
* Parameters:
*  cy_stc_active_scan_sns_t * active_scan_sns_ptr (unused)
*
*******************************************************************************/
static void capsense_end_of_scan_callback(cy_stc_active_scan_sns_t* active_scan_sns_ptr)
{
    BaseType_t xYieldRequired;

    (void)active_scan_sns_ptr;
    xYieldRequired = xSemaphoreGiveFromISR(capsense_done,&xYieldRequired);

    portYIELD_FROM_ISR(xYieldRequired);
}

/*******************************************************************************
* Function Name: capsense_isr
********************************************************************************
* Summary:
*  Wrapper function for handling interrupts from CSD block.
*
*******************************************************************************/
static void capsense_isr(void)
{
    Cy_CapSense_InterruptHandler(CYBSP_CSD_HW, &cy_capsense_context);
}

/*******************************************************************************
* Function Name: capsense_register_deepsleep
********************************************************************************
* Summary:
*  Wrapper function for setting up the Deep Sleep callback for CapSense.
*  This is necessary so that a transition from active to Deep Sleep does not
*  occur during a CapSense scan.
*
*  See the "MTB CAT1 Peripheral driver library documentation > PDL API Reference > SysPM"
*  link in the Quick Panel Documentation for information on setting up the SysPm callbacks
*******************************************************************************/
static void capsense_register_deepsleep(void)
{
    static cy_stc_syspm_callback_params_t callback_params =
    {
        .base       = CYBSP_CSD_HW,
        .context    = &cy_capsense_context
    };

    static cy_stc_syspm_callback_t capsense_deep_sleep_cb =
    {
        Cy_CapSense_DeepSleepCallback,
        CY_SYSPM_DEEPSLEEP,
        0,
        &callback_params,
        NULL,
        NULL
    };

    Cy_SysPm_RegisterCallback(&capsense_deep_sleep_cb);
}
