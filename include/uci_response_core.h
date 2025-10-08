#ifndef UCI_RESPONSE_CORE_H
#define UCI_RESPONSE_CORE_H

#include <stddef.h>

// CORE group response builders
int build_core_device_info_response(unsigned char* response_payload, size_t max_len);
int build_core_get_caps_info_response(unsigned char* response_payload, size_t max_len);
int build_core_query_timestamp_response(unsigned char* response_payload, size_t max_len);
int build_core_get_config_response(unsigned char* response_payload, size_t max_len,
                                   const unsigned char* payload, int payload_len);
int build_core_set_config_response(unsigned char* response_payload, size_t max_len,
                                   const unsigned char* payload, int payload_len);
int build_core_device_reset_response(unsigned char* response_payload, size_t max_len);
int build_core_device_suspend_response(unsigned char* response_payload, size_t max_len);

#endif // UCI_RESPONSE_CORE_H
