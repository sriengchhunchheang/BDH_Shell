/**
 * @file adc_task.c
 * @brief ADC Task implementation
 */

#include "adc_task.h"
#include "cy_retarget_io.h"
#include "cyhal_adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include <stdio.h>

//extern const cyhal_adc_t cy_retarget_io_adc_obj;
cyhal_adc_t cy_retarget_io_adc_obj;


//void adc_task(void* arg)
//{
//    // Initialize the ADC
//    cy_rslt_t result = cyhal_adc_init(&cy_retarget_io_adc_obj, CYBSP_A0, NULL);
//    if (result != CY_RSLT_SUCCESS) {
//        printf("ADC initialization failed with error: %ld\n", (long)result);
//        vTaskDelete(NULL);  // Exit the task if initialization fails
//    }
//
//    // Configure the ADC channel
//    cyhal_adc_channel_t adc_channel;
//    result = cyhal_adc_channel_init(&adc_channel, &cy_retarget_io_adc_obj, CYBSP_A0, NULL);
//    if (result != CY_RSLT_SUCCESS) {
//        printf("ADC channel initialization failed with error: %ld\n", (long)result);
//        cyhal_adc_free(&cy_retarget_io_adc_obj);  // Clean up the ADC instance
//        vTaskDelete(NULL);  // Exit the task if initialization fails
//    }
//
//    for (;;) {
//        // Read ADC value
//        uint32_t adc_value;
//        result = cyhal_adc_read(&adc_channel, &adc_value);
//        if (result == CY_RSLT_SUCCESS) {
//            // Perform desired operations with the ADC value
//            // For example, print the ADC value
//            printf("ADC value: %lu\n", adc_value);
//        } else {
//            printf("ADC read failed with error: %ld\n", (long)result);
//        }
//
//        // Delay before reading the ADC again
//        vTaskDelay(pdMS_TO_TICKS(100));
//    }
//}

void adc_task(void* arg)
{
    // Initialize the ADC
    cy_rslt_t result = cyhal_adc_init(&cy_retarget_io_adc_obj, CYBSP_A0, NULL);
    if (result != CY_RSLT_SUCCESS) {
        printf("ADC initialization failed with error: %ld\n", (long)result);
        vTaskDelete(NULL);  // Exit the task if initialization fails
    }

    for (;;) {
        // Read ADC value
        int32_t adc_value = cyhal_adc_read(&cy_retarget_io_adc_obj);
        if (adc_value >= 0) {
            // Perform desired operations with the ADC value
            // For example, print the ADC value
            printf("ADC value: %ld\n", (long)adc_value);
        } else {
            printf("ADC read failed\n");
        }

        // Delay before reading the ADC again
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}





///**
// * @file adc_task.c
// * @brief ADC Task implementation
// */
//
//#include "adc_task.h"
//#include "cy_retarget_io.h"
//#include "cyhal_adc.h"
//#include "FreeRTOS.h"
//#include "task.h"
//#include "cy_pdl.h"
//#include "cyhal.h"
//#include "cybsp.h"
//#include <stdio.h>
//
//extern const cyhal_adc_t cy_retarget_io_adc_obj;
//
//void adc_task(void* arg)
//{
//    // Initialize the ADC
//    cy_rslt_t result = cyhal_adc_init(&cy_retarget_io_adc_obj, CYBSP_A0, NULL);
//    if (result != CY_RSLT_SUCCESS) {
//        printf("ADC initialization failed with error: %ld\n", (long)result);
//        vTaskDelete(NULL);  // Exit the task if initialization fails
//    }
//
//    // Configure the ADC channel
//    cyhal_adc_channel_t adc_channel;
//    result = cyhal_adc_channel_init(&adc_channel, &cy_retarget_io_adc_obj, CYBSP_A0, NULL);
//    if (result != CY_RSLT_SUCCESS) {
//        printf("ADC channel initialization failed with error: %ld\n", (long)result);
//        cyhal_adc_free(&cy_retarget_io_adc_obj);  // Clean up the ADC instance
//        vTaskDelete(NULL);  // Exit the task if initialization fails
//    }
//
//    for (;;) {
//        // Read ADC value
//        uint32_t adc_value;
//        result = cyhal_adc_read(&adc_channel, &adc_value, 1);
//        if (result == CY_RSLT_SUCCESS) {
//            // Perform desired operations with the ADC value
//            // For example, print the ADC value
//            printf("ADC value: %lu\n", adc_value);
//        } else {
//            printf("ADC read failed with error: %ld\n", (long)result);
//        }
//
//        // Delay before reading the ADC again
//        vTaskDelay(pdMS_TO_TICKS(100));
//    }
//}
