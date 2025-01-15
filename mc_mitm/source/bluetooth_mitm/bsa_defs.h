/*
 * Copyright (c) 2020-2025 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <switch.h>

#define BSA_DM_CONFIG_NONE                    0x0000
#define BSA_DM_CONFIG_VISIBILITY_MASK         0x0001
#define BSA_DM_CONFIG_BDADDR_MASK             0x0002
#define BSA_DM_CONFIG_NAME_MASK               0x0004
#define BSA_DM_CONFIG_DEV_CLASS_MASK          0x0008
#define BSA_DM_CONFIG_CHANNEL_MASK            0x0010
#define BSA_DM_CONFIG_BRCM_MASK               0x0020
#define BSA_DM_CONFIG_PAGE_SCAN_PARAM_MASK    0x0040
#define BSA_DM_CONFIG_INQ_SCAN_PARAM_MASK     0x0080
#define BSA_DM_CONFIG_TX_POWER_MASK           0x0100
#define BSA_DM_CONFIG_EIR_MASK                0x0200
#define BSA_DM_CONFIG_DUAL_STACK_MODE_MASK    0x0400
#define BSA_DM_CONFIG_BLE_BGCONN_TYPE_MASK    0x0800
#define BSA_DM_CONFIG_BLE_SCAN_PARAM_MASK     0x1000
#define BSA_DM_CONFIG_BLE_VISIBILITY_MASK     0x2000
#define BSA_DM_CONFIG_BLE_CONN_PARAM_MASK     0x4000
#define BSA_DM_CONFIG_BLE_ADV_CONFIG_MASK     0x8000
#define BSA_DM_CONFIG_BLE_ADV_PARAM_MASK      0x10000
#define BSA_DM_CONFIG_BLE_PRIVACY_MASK        0x20000
#define BSA_DM_CONFIG_LINK_POLICY_MASK        0x40000
#define BSA_DM_CONFIG_MONITOR_RSSI            0x80000

typedef void tBSA_DM_CBACK(u32, void *);

typedef struct {
    u16 len;
    u8 uu[16];
} tBT_UUID;

typedef struct {
    u16 low;
    u16 hi;
} tBSA_DM_BLE_INT_RANGE;

typedef struct {
    u16 left_open_offset;
    u16 left_close_offset;
    u16 right_open_offset;
    u16 right_close_offset;
    u16 delay;
    u8 dual_view;
} tBSA_DM_3D_TX_DATA;

typedef struct {
    u8 adv_type;
    u8 len;
    u8 *p_val;
} tBTA_BLE_PROP_ELEM;

typedef struct {
    bool list_cmpl;
    u8 uuid128[16];
} tBSA_DM_BLE_128SERVICE;

typedef struct {
    u8 num_elem;
    tBTA_BLE_PROP_ELEM *p_elem;
} tBSA_DM_BLE_PROPRIETARY;

typedef struct {
    u8 type;
    BtdrvAddress bd_addr;
} tBSA_DM_BLE_BD_ADDR;

typedef struct {
    BtdrvAddress bd_addr;
    u16 min_conn_int;
    u16 max_conn_int;
    u16 slave_latency;
    u16 supervision_tout;
    bool preference;
} tBSA_DM_BLE_CONN_PARAM;

typedef struct {
    u16 interval;
    u16 window;
} tBSA_DM_BLE_CONN_SCAN_PARAM;

typedef struct {
    u8 len;
    u8 flag;
    u8 p_val[31];
    u32 adv_data_mask;
    u16 appearance_data;
    u8 num_service;
    u16 uuid_val[6];
    u8 service_data_len;
    tBT_UUID service_data_uuid;
    u8 service_data_val[31];
    bool is_scan_rsp;
    u8 tx_power;
    tBSA_DM_BLE_INT_RANGE int_range;
    tBSA_DM_BLE_128SERVICE services_128b;
    tBSA_DM_BLE_128SERVICE sol_service_128b;
    tBSA_DM_BLE_PROPRIETARY proprietary;
    u8 inst_id;
} tBSA_DM_BLE_ADV_CONFIG;

typedef struct {
    u16 adv_int_min;
    u16 adv_int_max;
    tBSA_DM_BLE_BD_ADDR dir_bda;
    u8 adv_type;
    u8 channel_map;
    u8 adv_filter_policy;
    u8 tx_power;
    u8 inst_id;
} tBSA_DM_BLE_ADV_PARAM;

typedef struct {
    BtdrvAddress link_bd_addr;
    u16 policy_mask;
    bool set;
} tBSA_DM_LINK_POLICY_PARAM;

typedef struct {
    bool enable;
    u16 period;
} tBSA_DM_MONITOR_RSSI_PARAM;

typedef struct {
    u32 config_mask;
    bool enable;
    bool discoverable;
    bool connectable;
    BtdrvAddress bd_addr;
    char name[249];
    BtdrvClassOfDevice class_of_device;
    u8 first_disabled_channel;
    u8 last_disabled_channel;
    BtdrvAddress master_3d_bd_addr;
    u8 path_loss_threshold;
    u16 page_scan_interval;
    u16 page_scan_window;
    u16 inquiry_scan_interval;
    u16 inquiry_scan_window;
    tBSA_DM_CBACK *callback;
    u8 tx_power;
    u16 brcm_mask;
    u8 eir_length;
    u8 eir_data[200];
    u8 dual_stack_mode;
    tBSA_DM_3D_TX_DATA tx_data_3d;
    u8 ble_bg_conn_type;
    u16 ble_scan_interval;
    u16 ble_scan_window;
    u8 ble_scan_mode;
    bool ble_discoverable;
    bool ble_connectable;
    bool privacy_enable;
    tBSA_DM_BLE_CONN_PARAM ble_conn_param;
    tBSA_DM_BLE_CONN_SCAN_PARAM ble_conn_scan_param;
    tBSA_DM_BLE_ADV_CONFIG adv_config;
    tBSA_DM_BLE_ADV_PARAM ble_adv_param;
    u8 chip_id;
    tBSA_DM_LINK_POLICY_PARAM policy_param;
    tBSA_DM_MONITOR_RSSI_PARAM monitor_rssi_param;
} tBSA_DM_SET_CONFIG;
