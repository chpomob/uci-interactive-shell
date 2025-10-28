#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci_packet_structures.h"
#include "../include/uci_packet_mutualization_utils.h"
#include "../include/uci.h"
#include "../include/uci_packet_utils.h"

int main() {
    printf("Testing Mutualized UCI Packet Structures\n");
    printf("========================================\n\n");

    // Test 1: Create a session init structure
    printf("Test 1: Creating and encoding/decoding SESSION_INIT packet\n");
    uci_session_init_payload_t session_init;
    uci_error_t result = uci_create_session_init_struct(&session_init, 1, FIRA_RANGING_SESSION);
    if (result != UCI_SUCCESS) {
        printf("  Error creating session init struct: %d\n", result);
        return 1;
    }
    
    printf("  Created struct - Session ID: %u, Type: %d\n", 
           session_init.session_id, session_init.session_type);
    
    // Encode the structure to a packet
    size_t packet_len;
    unsigned char* encoded_packet = uci_create_packet_from_struct(
        COMMAND, COMPLETE, SESSION_CONFIG, SESSION_INIT, &session_init, &packet_len);
    
    if (!encoded_packet) {
        printf("  Error encoding packet\n");
        return 1;
    }
    
    printf("  Encoded packet of size: %zu bytes\n", packet_len);
    printf("  Encoded header: %02X %02X %02X %02X\n", 
           encoded_packet[0], encoded_packet[1], encoded_packet[2], encoded_packet[3]);
    printf("  Encoded payload: ");
    for (size_t i = 4; i < packet_len; i++) {
        printf("%02X ", encoded_packet[i]);
    }
    printf("\n");
    
    // Decode the packet back to a structure
    uci_session_init_payload_t decoded_session_init;
    result = uci_decode_packet_to_struct(encoded_packet, packet_len, &decoded_session_init, 
                                         SESSION_CONFIG, SESSION_INIT);
    if (result != UCI_SUCCESS) {
        printf("  Error decoding packet: %d\n", result);
        free(encoded_packet);
        return 1;
    }
    
    printf("  Decoded struct - Session ID: %u, Type: %d\n", 
           decoded_session_init.session_id, decoded_session_init.session_type);
    
    // Verify that the values match
    if (session_init.session_id == decoded_session_init.session_id && 
        session_init.session_type == decoded_session_init.session_type) {
        printf("  ✓ Encode/decode round-trip successful!\n\n");
    } else {
        printf("  ✗ Encode/decode round-trip failed!\n\n");
        free(encoded_packet);
        return 1;
    }
    
    free(encoded_packet);

    // Test 2: Create a session deinit structure
    printf("Test 2: Creating and encoding/decoding SESSION_DEINIT packet\n");
    uci_session_deinit_payload_t session_deinit;
    result = uci_create_session_deinit_struct(&session_deinit, 2);
    if (result != UCI_SUCCESS) {
        printf("  Error creating session deinit struct: %d\n", result);
        return 1;
    }
    
    printf("  Created struct - Session ID: %u\n", session_deinit.session_id);
    
    // Encode the structure
    encoded_packet = uci_create_packet_from_struct(
        COMMAND, COMPLETE, SESSION_CONFIG, SESSION_DEINIT, &session_deinit, &packet_len);
    
    if (!encoded_packet) {
        printf("  Error encoding packet\n");
        return 1;
    }
    
    printf("  Encoded packet of size: %zu bytes\n", packet_len);
    
    // Decode the packet back
    uci_session_deinit_payload_t decoded_session_deinit;
    result = uci_decode_packet_to_struct(encoded_packet, packet_len, &decoded_session_deinit, 
                                         SESSION_CONFIG, SESSION_DEINIT);
    if (result != UCI_SUCCESS) {
        printf("  Error decoding packet: %d\n", result);
        free(encoded_packet);
        return 1;
    }
    
    printf("  Decoded struct - Session ID: %u\n", decoded_session_deinit.session_id);
    
    if (session_deinit.session_id == decoded_session_deinit.session_id) {
        printf("  ✓ Encode/decode round-trip successful!\n\n");
    } else {
        printf("  ✗ Encode/decode round-trip failed!\n\n");
        free(encoded_packet);
        return 1;
    }
    
    free(encoded_packet);

    // Test 3: Get format definition for a known command
    printf("Test 3: Getting payload format definition\n");
    const uci_payload_format_def_t* format = uci_get_payload_format(SESSION_CONFIG, SESSION_INIT);
    if (format) {
        printf("  Found format for SESSION_INIT with %zu fields\n", format->num_fields);
        for (size_t i = 0; i < format->num_fields; i++) {
            printf("    Field %zu: %s (offset=%zu, size=%zu)\n", 
                   i, format->fields[i].name, format->fields[i].offset, format->fields[i].size);
        }
    } else {
        printf("  Could not find format for SESSION_INIT\n");
    }
    
    printf("\nAll tests completed successfully!\n");
    printf("Mutualized packet structure system is working correctly.\n");
    
    return 0;
}