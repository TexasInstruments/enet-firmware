/*
 *
 * Copyright (c) 2020 Texas Instruments Incorporated
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

/**
 *  \file webpage.c
 *
 *  \brief Webpage source file.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* OSAL */
#include <ti/osal/osal.h>
#include <ti/osal/TaskP.h>

/* BSD support */
#include <ti/net/bsd/netinet/in.h>
#include <ti/net/bsd/arpa/inet.h>
#include <ti/net/bsd/sys/socket.h>

#include <ti/net/http/httpserver.h>
#include <ti/net/http/urlhandler.h>

#include "urlmemzip.h"
#include "urlsimpleput.h"
#include "urlsimple.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define NUM_URLHANDLERS               (3U)
#define SERVER_BACKLOG_COUNT          (20U)
#define HTTP_SERVER_TASK_PRI          (2U)
#define HTTP_SERVER_TASK_STACK_SIZE   (8 * 1024U)
#define HTTP_SERVER_PORT              (80U)

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Extern variables                                  */
/* ========================================================================== */

extern void fdOpenSession();
extern void fdCloseSession();
extern void *TaskSelf();

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/*
 * Structure that is passed to HTTPServer_create. It defines the callbacks
 * used by the server as it parses requests.
 */
URLHandler_Setup handlerTable[] =
{
    {
        NULL,
        NULL,
        NULL,
        URLSimple_process,
        NULL,
        NULL
    },
    {
        NULL,
        NULL,
        NULL,
        URLSimplePut_process,
        NULL,
        NULL
    },
    {
        NULL,
        NULL,
        NULL,
        URLMemzip_process,
        NULL,
        NULL
    }
};

static uint8_t gHttpServerStackBuf[HTTP_SERVER_TASK_STACK_SIZE] __attribute__ ((aligned(32)));

/* HTTP server handle */
HTTPServer_Handle srv;

 /* ========================================================================== */
 /*                          Function Definitions                              */
 /* ========================================================================== */

/*
 * Thread to be started once the NDK has acquired an IP address.
 * This thread initializes and starts a simple HTTP server. */
static void serverFxn(void* a0, void* a1)
{
    int32_t status;
    struct sockaddr_in addr;

    fdOpenSession(TaskSelf());
    HTTPServer_init();

    /* Create the HTTPServer and pass in your array of handlers */
    if ((srv = HTTPServer_create(handlerTable, NUM_URLHANDLERS, NULL)) == NULL)
    {
        exit(1);
    }

    /*
     * Create a connection point for the server.
     */
    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(HTTP_SERVER_PORT);

    status = HTTPServer_serveSelect(srv,
                                    (struct sockaddr *)&addr,
                                    sizeof(addr),
                                    SERVER_BACKLOG_COUNT);
    if (status != 0)
    {
        exit(1);
    }

    HTTPServer_delete(&srv);
    fdCloseSession(TaskSelf());
}

void AddWebFiles(void)
{
    TaskP_Handle task;
    TaskP_Params taskParams;

    TaskP_Params_init(&taskParams);
    taskParams.priority = HTTP_SERVER_TASK_PRI;
    taskParams.stack = &gHttpServerStackBuf[0];
    taskParams.stacksize = sizeof(gHttpServerStackBuf);
    taskParams.name = "EthFw HTTP server task";

    task = TaskP_create(serverFxn, &taskParams);
    if (NULL == task)
    {
        OS_stop();
    }

}

void RemoveWebFiles(void)
{
    HTTPServer_delete(&srv);
}
