/**
 * @file mqtt_at_mgr.h
 * @author Mahesh Murty (mahesh@actchip.com)
 * @brief MQTT protocol management using modem AT commands.
 * 
 * @copyright Copyright Actchip Pvt. Ltd. (c) 2024
 * 
 */
#ifndef __MQTT_AT_MGR_H__
#define __MQTT_AT_MGR_H__               1

#include <stdint.h>

#include "mqtt_common.h"

int8_t MqttATMgrInit(mqttAgentConfig_t *config);
uint8_t WaitMQTT_ATConnect(int32_t timeoutMs);

#endif /* __MQTT_AT_MGR_H__ */