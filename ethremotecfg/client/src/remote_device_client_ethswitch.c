/*
 *
 * Copyright (c) 2017 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 *
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
 * license under copyrights and patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this software subject to the
 * terms herein.  With respect to the foregoing patent license, such license is granted
 * solely to the extent that any such patent is necessary to Utilize the software alone.
 * The patent license shall not apply to any combinations which include this software,
 * other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source
 * code license limitations below) in the documentation and/or other materials provided
 * with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided
 * that the following conditions are met:
 *
 * *       No reverse engineering, decompilation, or disassembly of this software is
 * permitted with respect to any software provided in binary form.
 *
 * *       any redistribution and use are licensed by TI for use only with TI Devices.
 *
 * *       Nothing shall obligate TI to provide you with source code for the software
 * licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the
 * source code are permitted provided that the following conditions are met:
 *
 * *       any redistribution and use of the source code, including any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * *       any redistribution and use of any object code compiled from the source code
 * and any resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers
 *
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <stdio.h>
#include <stdint.h>

#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/osal/osal.h>
#include <ethremotecfg/protocol/rpmsg-kdrv-transport-ethswitch.h>
#include <client-rtos/remote-device.h>
#include <ethremotecfg/client/include/ethremotecfg_client.h>


uint32_t rdevEthSwitchClient_printText(void *priv, void *data)
{
    struct rpmsg_kdrv_device_header *hdr = (struct rpmsg_kdrv_device_header *)data;
    struct rpmsg_kdrv_ethswitch_s2c_notify *msg = (struct rpmsg_kdrv_ethswitch_s2c_notify *)(&hdr[1]);
    System_printf("%s: message (hdr = %u) %s\n", __func__, msg->header.message_type, msg->data);
    return 0;
}

static int32_t rdevEthSwitchClientFreeMsg(void *priv, void *data, uint32_t len)
{
    return 0;
}

int32_t rdevEthSwitchClient_sendText(uint32_t device_id, char *text)
{
    uint8_t data[512];
    struct rpmsg_kdrv_device_header *hdr = (struct rpmsg_kdrv_device_header *)data;
    struct rpmsg_kdrv_ethswitch_c2s_notify *msg = (struct rpmsg_kdrv_ethswitch_c2s_notify *)(&hdr[1]);
    int32_t ret;

    memset(&data[0], 0, 512);
    msg->header.message_type = RPMSG_KDRV_TP_ETHSWITCH_C2S_NOTIFY;
    snprintf((char *)&msg->data[0], RPMSG_KDRV_TP_ETHSWITCH_MESSAGE_DATA_LEN, "%s", text);
    ret = appRemoteDeviceSendMessage(device_id, data, sizeof(*hdr) + sizeof(*msg), NULL, rdevEthSwitchClientFreeMsg);
    return ret;
}

int32_t rdevEthSwitchClient_sendPing(uint32_t device_id, char *text, uint8_t *respMsg, uint32_t respMaxLen, uint32_t *respMsgSize)
{
    uint8_t data[512];
    struct rpmsg_kdrv_device_header *hdr = (struct rpmsg_kdrv_device_header *)data;
    struct rpmsg_kdrv_ethswitch_ping_request *msg = (struct rpmsg_kdrv_ethswitch_ping_request *)(&hdr[1]);
    int32_t ret;

    memset(&data[0], 0, 512);
    msg->header.message_type = RPMSG_KDRV_TP_ETHSWITCH_PING_REQUEST;
    snprintf((char *)&msg->data[0], RPMSG_KDRV_TP_ETHSWITCH_MESSAGE_DATA_LEN, "%s", text);
    ret = appRemoteDeviceServiceRequest(device_id, data, sizeof(*hdr) + sizeof(*msg), respMsg, respMaxLen, respMsgSize);
    return ret;
}


int32_t rdevEthSwitchClient_connect(rdevEthSwitchClientInitPrms_t *initPrms)
{
    int32_t ret = 0;
    app_remote_device_device_connect_prm_t prm;

    appRemoteDeviceDeviceConnectParamsInit(&prm);

    sprintf(prm.device_name, "%s", initPrms->device_name);
    prm.message_cb = initPrms->cbHandler;
    prm.message_cb_priv = NULL;
    ret = appRemoteDeviceConnect(&prm, &initPrms->device_id);

    if ((ret == 0) && (initPrms->device_id != APP_REMOTE_DEVICE_DEVICE_ID_EAGAIN))
    {
        ret = appRemoteDeviceGetType(initPrms->device_id, &initPrms->device_type);
    }

    if ((ret == 0) && (initPrms->device_id != APP_REMOTE_DEVICE_DEVICE_ID_EAGAIN)) 
    {
        ret = appRemoteDeviceGetData(initPrms->device_id, 
                                     initPrms->data, 
                                     sizeof(initPrms->data), 
                                     &initPrms->dataFilledLen);
    }
    return ret;

}






