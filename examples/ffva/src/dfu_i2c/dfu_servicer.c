// Copyright 2023-2024 XMOS LIMITED.
// This Software is subject to the terms of the XCORE VocalFusion Licence.
#define DEBUG_UNIT DFU_SERVICER
#ifndef DEBUG_PRINT_ENABLE_DFU_SERVICER
#define DEBUG_PRINT_ENABLE_DFU_SERVICER 0
#endif
#include "debug_print.h"

#include <stdio.h>
#include <string.h>
#include <platform.h>
#include <xassert.h>

#include "platform/platform_conf.h"
#include "servicer.h"
#include "dfu_servicer.h"

#include "dfu_cmds.h"
#include "device_control_i2c.h"

// TODO: Enable lines below when the rest of DFU code is included
//#include "dfu_common.h" // for reboot()
//#include "dfu_state_machine.h"

void dfu_servicer_init(servicer_t *servicer)
{
    #include "dfu_cmds_map.h" // Included instead of directly adding code since this file is autogenerated.
    // Servicer resource info
    static control_resource_info_t dfu_res_info[NUM_RESOURCES_DFU_SERVICER];

    memset(servicer, 0, sizeof(servicer_t));
    servicer->id = DFU_CONTROLLER_SERVICER_RESID;
    servicer->start_io = 0;
    servicer->num_resources = NUM_RESOURCES_DFU_SERVICER;

    servicer->res_info = &dfu_res_info[0];
    // Servicer resource
    servicer->res_info[0].resource = DFU_CONTROLLER_SERVICER_RESID;
    servicer->res_info[0].control_pkt_queue.depth = 0;
    servicer->res_info[0].control_pkt_queue.pkts = NULL;
    servicer->res_info[0].command_map.num_commands = NUM_DFU_CONTROLLER_SERVICER_RESID_CMDS;
    servicer->res_info[0].command_map.commands = dfu_controller_servicer_resid_cmd_map;
}

void dfu_servicer(void *args) {
    device_control_servicer_t servicer_ctx;

    servicer_t *servicer = (servicer_t*)args;
    xassert(servicer != NULL);

    for(int i=0; i<servicer->num_resources; i++) {
        servicer->res_info[i].control_pkt_queue.queue_wr_index = 0;
    }
    control_resid_t *resources = (control_resid_t*)pvPortMalloc(servicer->num_resources * sizeof(control_resid_t));
    for(int i=0; i<servicer->num_resources; i++)
    {
        resources[i] = servicer->res_info[i].resource;
    }

    if(appconfI2C_CTRL_ENABLED > 0)
    {
        control_ret_t dc_ret;
        debug_printf("Calling device_control_servicer_register(), servicer ID %d, on tile %d, core %d.\n", servicer->id, THIS_XCORE_TILE, rtos_core_id_get());

        dc_ret = device_control_servicer_register(&servicer_ctx,
                                            device_control_ctxs,
                                            1,
                                            resources, servicer->num_resources);
        debug_printf("Out of device_control_servicer_register(), servicer ID %d, on tile %d. servicer_ctx address = 0x%x\n", servicer->id, THIS_XCORE_TILE, &servicer_ctx);
    }

 #if appconfI2C_CTRL_ENABLED && ON_TILE(I2C_CTRL_TILE_NO)
    // Start I2C slave.
    // This ends up calling device_control_resources_register() which has a strange non-yielding implementation,
    // where it waits for servicers to register with the device control context. That's why, control_start_io_tasks()
    // is not called from platform_start(). Additionally, if there is only one xcore core dedicated for all RTOS tasks,
    // this design will not work, since device_control_resources_register() will not yield and the servicers wouldn't get
    // scheduled so they could register with the device control leading to an eventual timeout error from device_control_resources_register().

    rtos_i2c_slave_start(i2c_slave_ctx,
                         device_control_i2c_ctx,
                         (rtos_i2c_slave_start_cb_t) device_control_i2c_start_cb,
                         (rtos_i2c_slave_rx_cb_t) device_control_i2c_rx_cb,
                         (rtos_i2c_slave_tx_start_cb_t) device_control_i2c_tx_start_cb,
                         (rtos_i2c_slave_tx_done_cb_t) NULL,
                         NULL,
                         NULL,
                         appconfI2C_INTERRUPT_CORE,
                         appconfI2C_TASK_PRIORITY);
#endif
    vPortFree(resources);

    // The first call to device_control_servicer_cmd_recv triggers the device_control_i2c_start_cb() through I2S slave ISR call somehow. device_control_i2c_start_cb() and device_control_servicer_cmd_recv() have to be in separate logical cores
    // and be scheduled simultaneously for this to work.
    if(APP_CONTROL_TRANSPORT_COUNT > 0)
    {
        for(;;){
            device_control_servicer_cmd_recv(&servicer_ctx, read_cmd, write_cmd, servicer, RTOS_OSAL_WAIT_FOREVER);
        }
    }
    else
    {
        for(;;){
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    // TODO: Enable lines below when the rest of DFU code is included
    /*xTaskCreate(
        dfu_int_state_machine,
        "DFU state machine task",
        RTOS_THREAD_STACK_SIZE(dfu_int_state_machine),
        NULL,
        uxTaskPriorityGet(NULL), // Same priority so should run after this task
        NULL
    );*/
    
    if(appconfI2C_CTRL_ENABLED > 0)
    {
        for(;;){
            device_control_servicer_cmd_recv(&servicer_ctx, read_cmd, write_cmd, servicer, RTOS_OSAL_WAIT_FOREVER);
        }
    }
    else
    {
        for(;;){
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

control_ret_t dfu_servicer_read_cmd(control_resource_info_t *res_info, control_cmd_t cmd, uint8_t *payload, size_t payload_len)
{
    control_ret_t ret = CONTROL_SUCCESS;
    uint8_t cmd_id = CONTROL_CMD_CLEAR_READ(cmd);

    memset(payload, 0, payload_len);

    debug_printf("dfu_servicer_read_cmd, cmd_id: %d.\n", cmd_id);

    switch (cmd_id)
    {
    case DFU_CONTROLLER_SERVICER_RESID_DFU_UPLOAD:
    {
        debug_printf("DFU_CONTROLLER_SERVICER_RESID_DFU_UPLOAD\n");
        // TODO: Undo the change below when the rest of DFU code is included
        size_t upload_len = 0; //dfu_int_upload(&payload[2], DFU_DATA_XFER_SIZE);

        payload[0] = upload_len & 0xFF;
        payload[1] = (upload_len >> 8) & 0xFF;
        // Rest of payload is filled by dfu_int_upload function
        break;
    }

    case DFU_CONTROLLER_SERVICER_RESID_DFU_GETSTATUS:
    {
        debug_printf("DFU_CONTROLLER_SERVICER_RESID_DFU_GETSTATUS\n");
        // TODO: Enable lines below when the rest of DFU code is included
        // dfu_int_get_status_packet_t retval;
        // dfu_int_get_status(&retval);

        // uint8_t time_high, time_mid, time_low;
        // time_low = (uint8_t)(retval.timeout_ms & 0xFF);
        // time_mid = (uint8_t)((retval.timeout_ms >> 8) & 0xFF);
        // time_high = (uint8_t)((retval.timeout_ms >> 16) & 0xFF);

        // payload[0] = retval.current_status;
        // payload[1] = time_low;
        // payload[2] = time_mid;
        // payload[3] = time_high;
        // payload[4] = retval.next_state;
        
        break;
    }

    case DFU_CONTROLLER_SERVICER_RESID_DFU_GETSTATE:
    {
        debug_printf("DFU_CONTROLLER_SERVICER_RESID_DFU_GETSTATE\n");
        // TODO: Undo the change below when the rest of DFU code is included
        uint8_t state = 0; // dfu_int_get_state();
        payload[0] = state;
        break;
    }

    case DFU_CONTROLLER_SERVICER_RESID_DFU_TRANSFERBLOCK:
    {
        debug_printf("DFU_CONTROLLER_SERVICER_RESID_DFU_TRANSFERBLOCK\n");
        // TODO: Undo the change below when the rest of DFU code is included
        uint16_t transferblock =  0; //dfu_int_get_transfer_block();
        
        uint8_t tb_high, tb_low;
        tb_low = (uint8_t)(transferblock & 0xFF);
        tb_high = (uint8_t)((transferblock >> 8) & 0xFF);

        payload[0] = tb_low;
        payload[1] = tb_high;

        break;
    }

    default:
    {
        debug_printf("DFU_CONTROLLER_SERVICER UNHANDLED COMMAND!!!\n");
        ret = CONTROL_BAD_COMMAND;
        break;
    }
    }

    return ret;
}

control_ret_t dfu_servicer_write_cmd(control_resource_info_t *res_info, control_cmd_t cmd, const uint8_t *payload, size_t payload_len)
{
    control_ret_t ret = CONTROL_SUCCESS;

    uint8_t cmd_id = CONTROL_CMD_CLEAR_READ(cmd);
    debug_printf("dfu_servicer_write_cmd cmd_id %d.\n", cmd_id);

    switch (cmd_id)
    {
    case DFU_CONTROLLER_SERVICER_RESID_DFU_DETACH:
        debug_printf("DFU_CONTROLLER_SERVICER_RESID_DFU_DETACH\n");
        // TODO: Enable the line below when the rest of DFU code is included
        // dfu_int_detach();
        break;

    case DFU_CONTROLLER_SERVICER_RESID_DFU_DNLOAD:
        debug_printf("DFU_CONTROLLER_SERVICER_RESID_DFU_DNLOAD\n");
        // TODO: Enable the line below when the rest of DFU code is included
        // uint16_t dnload_length = payload[0] + (payload[1] << 8);
        // const uint8_t * dnload_data = &payload[2];
        // dfu_int_download(dnload_length, dnload_data);
        break;

    case DFU_CONTROLLER_SERVICER_RESID_DFU_CLRSTATUS:
        debug_printf("DFU_CONTROLLER_SERVICER_RESID_DFU_CLRSTATUS\n");
        // TODO: Enable the line below when the rest of DFU code is included
        // dfu_int_clear_status();
        break;

    case DFU_CONTROLLER_SERVICER_RESID_DFU_ABORT:
        debug_printf("DFU_CONTROLLER_SERVICER_RESID_DFU_ABORT\n");
        // TODO: Enable the line below when the rest of DFU code is included
        // dfu_int_abort();
        break;

    case DFU_CONTROLLER_SERVICER_RESID_DFU_SETALTERNATE:
        debug_printf("DFU_CONTROLLER_SERVICER_RESID_DFU_SETALTERNATE\n");
        // TODO: Enable the line below when the rest of DFU code is included
        // dfu_int_set_alternate(payload[0]);
        break;

    case DFU_CONTROLLER_SERVICER_RESID_DFU_TRANSFERBLOCK:
        debug_printf("DFU_CONTROLLER_SERVICER_RESID_DFU_TRANSFERBLOCK\n");
        // TODO: Enable lines below when the rest of DFU code is included
        // uint16_t const transferblock = (payload[1] << 8) + payload[0];
        // dfu_int_set_transfer_block(transferblock);
        break;

    case DFU_CONTROLLER_SERVICER_RESID_DFU_REBOOT:
        debug_printf("DFU_CONTROLLER_SERVICER_RESID_DFU_REBOOT\n");
        // TODO: Enable the line below when the rest of DFU code is included
        // reboot();
        break;

    default:
        debug_printf("DFU_CONTROLLER_SERVICER UNHANDLED COMMAND!!!\n");
        ret = CONTROL_BAD_COMMAND;
        break;
    }

    return ret;
}
