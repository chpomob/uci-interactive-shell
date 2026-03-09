#ifndef UCI_PLAIN_DECODERS_INTERNAL_H
#define UCI_PLAIN_DECODERS_INTERNAL_H

void print_short_address_measurement(const unsigned char* data);
void print_extended_address_measurement(const unsigned char* data);
void decode_range_vendor_data(const unsigned char* data, int length);

#endif
