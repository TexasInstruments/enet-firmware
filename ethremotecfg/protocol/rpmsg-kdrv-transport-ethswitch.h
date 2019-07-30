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


#ifndef __RPMSG_KDRV_TRANSPORT_ETHSWITCH_H__
#define __RPMSG_KDRV_TRANSPORT_ETHSWITCH_H__

#include <protocol/rpmsg-kdrv-transport-common.h>

enum rpmsg_kdrv_ethswitch_message_type {
    RPMSG_KDRV_TP_ETHSWITCH_ATTACH                  = 0x00,
    RPMSG_KDRV_TP_ETHSWITCH_ATTACH_EXT              = 0x01,
    RPMSG_KDRV_TP_ETHSWITCH_ALLOC_TX                = 0x02,
    RPMSG_KDRV_TP_ETHSWITCH_ALLOC_RX                = 0x03,
    RPMSG_KDRV_TP_ETHSWITCH_REGISTER_DEFAULTFLOW    = 0x04,
    RPMSG_KDRV_TP_ETHSWITCH_ALLOC_MAC               = 0x05,
    RPMSG_KDRV_TP_ETHSWITCH_REGISTER_MAC            = 0x06,
    RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_MAC          = 0x07,
    RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_DEFAULTFLOW  = 0x08,
    RPMSG_KDRV_TP_ETHSWITCH_FREE_MAC                = 0x09,
    RPMSG_KDRV_TP_ETHSWITCH_FREE_TX                 = 0x0A,
    RPMSG_KDRV_TP_ETHSWITCH_FREE_RX                 = 0x0B,
    RPMSG_KDRV_TP_ETHSWITCH_DETACH                  = 0x0C,
    RPMSG_KDRV_TP_ETHSWITCH_IOCTL                   = 0x0D,
    RPMSG_KDRV_TP_ETHSWITCH_REGWR                   = 0x0E,
    RPMSG_KDRV_TP_ETHSWITCH_REGRD                   = 0x0F,
    RPMSG_KDRV_TP_ETHSWITCH_IPV4_MAC_REGISTER       = 0x10,
    RPMSG_KDRV_TP_ETHSWITCH_IPV6_MAC_REGISTER       = 0x11,
    RPMSG_KDRV_TP_ETHSWITCH_IPV4_MAC_UNREGISTER     = 0x12,
    RPMSG_KDRV_TP_ETHSWITCH_IPV6_MAC_UNREGISTER     = 0x13,
    RPMSG_KDRV_TP_ETHSWITCH_PING_REQUEST            = 0x14,
    RPMSG_KDRV_TP_ETHSWITCH_S2C_NOTIFY              = 0x15,
    RPMSG_KDRV_TP_ETHSWITCH_C2S_NOTIFY              = 0x16,
    RPMSG_KDRV_TP_ETHSWITCH_MAX                     = 0x17,
};

enum rpmsg_kdrv_ethswitch_cpsw_type {
    RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_2G,
    RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_9G,
    RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_MAX,
};


enum rpmsg_kdrv_ethswitch_client_notify_type {
    RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_DUMPSTATS,
    RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_MAX,
};

/*
 * Response status codes
 */
#define RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK       (0)
#define RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EAGAIN   (-1)
#define RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL    (-2)
#define RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EACCESS  (-3)


/*
 * Maximum length of demo device data
 */
#define RPMSG_KDRV_TP_ETHSWITCH_DEVICE_DATA_LEN     (32)

/*
 * Maximum length of demo device message data
 */
#define RPMSG_KDRV_TP_ETHSWITCH_MESSAGE_DATA_LEN    (128)

/*
 * Maximum length of input arguments for IOCTL
 */
#define RPMSG_KDRV_TP_ETHSWITCH_IOCTL_INARGS_LEN    (128)

/*
 * Maximum length of output arguments for IOCTL
 */
#define RPMSG_KDRV_TP_ETHSWITCH_IOCTL_OUTARGS_LEN    (128)


/*
 * Number of priorities supported by CPSW
 */
#define RPMSG_KDRV_TP_ETHSWITCH_CPSW_PRIORITY_NUM   (8)

/*
 * MAC Address length in octets
 */
#define RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN          (6)

/*
 * IPv4 Address length in octets
 */
#define RPMSG_KDRV_TP_ETHSWITCH_IPV4ADDRLEN         (4)

/*
 * IPv6 Address length in octets
 */
#define RPMSG_KDRV_TP_ETHSWITCH_IPV6ADDRLEN         (16)

#define RPMSG_KDRV_TP_ETHSWITCH_FEATURE_TXCSUM      (1 << 0)

/*
 * message header for demo device
 */
struct rpmsg_kdrv_ethswitch_message_header {
    /* enum: rpmsg_kdrv_ethswitch_message_type */
    u8 message_type;
} __packed;

/*
 * Common structure used for all ETHSWITCH config command request msgs except attach
 */
struct rpmsg_kdrv_ethswitch_common_request_info {
    /* unique handle returned by ATTACH  */
    u64 id;
    /* Core specific key returned by attach */
    u32 core_key;
} __packed;

/*
 * Common header used for all ETHSWITCH config commands response msgs
 */
struct rpmsg_kdrv_ethswitch_common_response_info {
    /* Status of request */
    s32 status;
} __packed;

/*
 * RPMSG_KDRV_TP_ETHSWITCH_ATTACH cmd client request
 */
struct rpmsg_kdrv_ethswitch_attach_request {
    /* message header */
    struct rpmsg_kdrv_ethswitch_message_header header;
    /* enum: rpmsg_kdrv_ethswitch_cpsw_type  */
    u8 cpsw_type;
} __packed;

/*
 * RPMSG_KDRV_TP_ETHSWITCH_ATTACH cmd server response
 */
struct rpmsg_kdrv_ethswitch_attach_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
    /* unique handle used by all further CMDs  */
    u64 id;
    /* Core specific key to indicate attached core */
    u32 core_key;
    /* MTU of rx packet */
    u32 rx_mtu;
    /* MTU of tx packet per priority */
    u32 tx_mtu[RPMSG_KDRV_TP_ETHSWITCH_CPSW_PRIORITY_NUM];
    /* Feature bitmask based on defines RPMSG_KDRV_TP_ETHSWITCH_FEATURE_xxx */
    u32 features;
} __packed;

/*
 * RPMSG_KDRV_TP_ETHSWITCH_ATTACH_EXT cmd client request
 */
struct rpmsg_kdrv_ethswitch_attach_extended_request {
    /* message header */
    struct rpmsg_kdrv_ethswitch_message_header header;
    /* enum: rpmsg_kdrv_ethswitch_cpsw_type  */
    u8 cpsw_type;
} __packed;

/*
 * RPMSG_KDRV_TP_ETHSWITCH_ATTACH_EXT cmd server response
 */
struct rpmsg_kdrv_ethswitch_attach_extended_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
    /* unique handle used by all further CMDs  */
    u64 id;
    /* Core specific key to indicate attached core */
    u32 core_key;
    /* MTU of rx packet */
    u32 rx_mtu;
    /* MTU of tx packet per priority */
    u32 tx_mtu[RPMSG_KDRV_TP_ETHSWITCH_CPSW_PRIORITY_NUM];
    /* Feature bitmask based on defines RPMSG_KDRV_TP_ETHSWITCH_FEATURE_xxx */
    u32 features;
    /*! Allocated flow's index */
    u32 alloc_flow_idx;
    /*! Tx PSIL Peer destination thread id which should be paired with the
      * Tx UDMA channel
      */
    u32 tx_cpsw_psil_dst_id;
    /*! Mac address allocated */
    u8 mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];


} __packed;


/*
 * RPMSG_KDRV_TP_ETHSWITCH_ALLOC_RX,
 * RPMSG_KDRV_TP_ETHSWITCH_ALLOC_TX,
 * RPMSG_KDRV_TP_ETHSWITCH_ALLOC_MAC,
 * RPMSG_KDRV_TP_ETHSWITCH_ALLOC_RX_DEFAULT
 * cmd 
 */
struct rpmsg_kdrv_ethswitch_alloc_request {
    /* message header */
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
} __packed;

struct rpmsg_kdrv_ethswitch_alloc_rx_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
    /*! Allocated flow's index */
    u32 alloc_flow_idx;
} __packed;

struct rpmsg_kdrv_ethswitch_alloc_tx_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
    /*! Tx PSIL Peer destination thread id which should be paired with the
      * Tx UDMA channel
      */
    u32 tx_cpsw_psil_dst_id;
} __packed;

struct rpmsg_kdrv_ethswitch_alloc_mac_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
    /*! Mac address allocated */
    u8 mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
} __packed;

struct rpmsg_kdrv_ethswitch_register_rx_default_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    u32    default_flow_idx;
} __packed;

struct rpmsg_kdrv_ethswitch_register_rx_default_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;


struct rpmsg_kdrv_ethswitch_register_mac_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    u8 mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
    /*! Flow's index associated with the mac address to be registered in ALE */
    u32 flow_idx;
} __packed;

struct rpmsg_kdrv_ethswitch_register_mac_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;

struct rpmsg_kdrv_ethswitch_unregister_mac_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    u8 mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
    u32 flow_idx;
} __packed;

struct rpmsg_kdrv_ethswitch_unregister_mac_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;

struct rpmsg_kdrv_ethswitch_free_mac_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    u8 mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
}  __packed;

struct rpmsg_kdrv_ethswitch_free_mac_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;

struct rpmsg_kdrv_ethswitch_free_tx_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    u32 tx_cpsw_psil_dst_id;
}  __packed;

struct rpmsg_kdrv_ethswitch_free_tx_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;

struct rpmsg_kdrv_ethswitch_unregister_rx_default_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    u32 default_flow_idx;
}  __packed;

struct rpmsg_kdrv_ethswitch_unregister_rx_default_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;

struct rpmsg_kdrv_ethswitch_free_rx_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    u32 alloc_flow_idx;
}  __packed;

struct rpmsg_kdrv_ethswitch_free_rx_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;

struct rpmsg_kdrv_ethswitch_detach_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
}  __packed;

struct rpmsg_kdrv_ethswitch_detach_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;

struct rpmsg_kdrv_ethswitch_ioctl_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    u32    cmd;
    u32    inargs_len;
    u8     inargs[RPMSG_KDRV_TP_ETHSWITCH_IOCTL_INARGS_LEN];
    u32    outargs_len;
}  __packed;

struct rpmsg_kdrv_ethswitch_ioctl_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
    u8     outargs[RPMSG_KDRV_TP_ETHSWITCH_IOCTL_OUTARGS_LEN];
}  __packed;

struct rpmsg_kdrv_ethswitch_regwr_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    u32    regaddr;
    u32    regval;
}  __packed;

struct rpmsg_kdrv_ethswitch_regwr_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
    /*! Updated register value */
    u32    regval;
}  __packed;

struct rpmsg_kdrv_ethswitch_regrd_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    u32    regaddr;
}  __packed;

struct rpmsg_kdrv_ethswitch_regrd_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
    u32    regval;
}  __packed;

struct rpmsg_kdrv_ethswitch_ipv4_register_mac_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    /*! Mac address associated with the IP address which should be added to 
     *  the ARP table
     */
    uint8_t mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
    /*! IPv4 address  */
    uint8_t ipv4_addr[RPMSG_KDRV_TP_ETHSWITCH_IPV4ADDRLEN];
}  __packed;

struct rpmsg_kdrv_ethswitch_ipv6_register_mac_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    /*! Mac address associated with the IP address which should be added to 
     *  the ARP table
     */
    uint8_t mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
    /*! IPv6 address */
    uint8_t ipv6_addr[RPMSG_KDRV_TP_ETHSWITCH_IPV6ADDRLEN];
}  __packed;


struct rpmsg_kdrv_ethswitch_ipv4_register_mac_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;

struct rpmsg_kdrv_ethswitch_ipv6_register_mac_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;

struct rpmsg_kdrv_ethswitch_ipv4_unregister_mac_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    /*! IPv4 address  */
    uint8_t ipv4_addr[RPMSG_KDRV_TP_ETHSWITCH_IPV4ADDRLEN];
}  __packed;

struct rpmsg_kdrv_ethswitch_ipv6_unregister_mac_request {
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    /*! IPv6 address */
    uint8_t ipv6_addr[RPMSG_KDRV_TP_ETHSWITCH_IPV6ADDRLEN];
}  __packed;

struct rpmsg_kdrv_ethswitch_ipv4_unregister_mac_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;

struct rpmsg_kdrv_ethswitch_ipv6_unregister_mac_response {
    struct rpmsg_kdrv_ethswitch_common_response_info info;
}  __packed;


/*
 * per-device data for ethswitch device
 */
struct rpmsg_kdrv_ethswitch_device_data {
    /* Does the device send all vsyncs? */
    u8 char_string[RPMSG_KDRV_TP_ETHSWITCH_DEVICE_DATA_LEN];
} __packed;


/* demo device ping request - always client to server */
struct rpmsg_kdrv_ethswitch_ping_request {
    /* message header */
    struct rpmsg_kdrv_ethswitch_message_header header;
    /* ping data */
    u8 data[RPMSG_KDRV_TP_ETHSWITCH_MESSAGE_DATA_LEN];
} __packed;

/* demo device ping response - always server to client */
struct rpmsg_kdrv_ethswitch_ping_response {
    /* ping data */
    u8 data[RPMSG_KDRV_TP_ETHSWITCH_MESSAGE_DATA_LEN];
} __packed;

/* demo device server to client one-way message */
struct rpmsg_kdrv_ethswitch_s2c_notify {
    /* message header */
    struct rpmsg_kdrv_ethswitch_message_header header;
    /* message data */
    u8 data[RPMSG_KDRV_TP_ETHSWITCH_MESSAGE_DATA_LEN];
} __packed;

/* demo device client to server one-way message */
struct rpmsg_kdrv_ethswitch_c2s_notify {
    /* message header */
    struct rpmsg_kdrv_ethswitch_message_header header;
    struct rpmsg_kdrv_ethswitch_common_request_info info;
    /* enum: enum rpmsg_kdrv_ethswitch_client_notify_type */
    u8 notifyid;
    /* filled length of notify info */
    u32 notify_info_len;
    /* message data */
    u8 notify_info[RPMSG_KDRV_TP_ETHSWITCH_MESSAGE_DATA_LEN];
} __packed;

#endif
