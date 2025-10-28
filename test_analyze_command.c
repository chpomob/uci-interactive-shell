/**
 * Test program for the enhanced analyze command
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci_cmd_analysis.h"

// Mock ui_color_enabled for testing
int ui_color_enabled = 0;

int main() {
    printf("Testing enhanced analyze command functionality...\n");
    
    // Test help command
    char* help_args[] = {"-h"};
    printf("\n1. Testing help command:\n");
    handle_analyze_command(1, help_args);
    
    // Test examples command
    char* examples_args[] = {"-e"};
    printf("\n2. Testing examples command:\n");
    handle_analyze_command(1, examples_args);
    
    // Test basic packet analysis
    char* basic_args[] = {"20", "08", "00", "00"};
    printf("\n3. Testing basic packet analysis:\n");
    handle_analyze_command(4, basic_args);
    
    // Test verbose packet analysis
    char* verbose_args[] = {"-v", "21", "00", "00", "05", "00", "00", "00", "01", "00"};
    printf("\n4. Testing verbose packet analysis:\n");
    handle_analyze_command(10, verbose_args);
    
    printf("\nAll tests completed successfully!\n");
    return 0;
}