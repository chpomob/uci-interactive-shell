#include "test_runner.h"

#include "../include/uci_pdl.h"
#include "../include/uci_opcode_constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHERRY_FIRA_HEADER "uci_analysis/uwb/Samples/Cherry/uci/uci_core/include/uci/uci_spec_fira.h"
#define CHERRY_QORVO_HEADER "uci_analysis/uwb/Samples/Cherry/uci/uci_core/include/uci/uci_spec_qorvo.h"
#define CHERRY_MCPS_HEADER "uci_analysis/uwb/Samples/Cherry/uci/uci_core/include/uci/uci_spec_mcps.h"
#define CHERRY_SESSION_CLIENT "uci_analysis/uwb/Samples/Cherry/cherry/src/uci_client/cherry_session_client.c"
#define PYTHON_QORVO_MSG "uci_analysis/uwb/Samples/Python/UWB-Qorvo-Tools/lib/uwb-uci/uci/qorvo_msg.py"
#define REPO_PDL_HEADER "include/uci_pdl.h"

static char *read_text_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    long size;
    char *buffer;

    if (!fp) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    rewind(fp);
    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    if (size > 0 && fread(buffer, 1, (size_t)size, fp) != (size_t)size) {
        free(buffer);
        fclose(fp);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(fp);
    return buffer;
}

static int contains_text(const char *haystack, const char *needle)
{
    return haystack && strstr(haystack, needle) != NULL;
}

int main(void)
{
    TEST_SUITE(cherry_alignment);

#define test_case_end test_case_end_cherry_fira_groups
    TEST_CASE(cherry_fira_groups_match_repository_constants);
    {
        char *source = read_text_file(CHERRY_FIRA_HEADER);

        ASSERT_TRUE(source != NULL);
        ASSERT_TRUE(contains_text(source, "UCI_GID_CORE = 0x0,"));
        ASSERT_TRUE(contains_text(source, "UCI_GID_SESSION_CONFIG = 0x1,"));
        ASSERT_TRUE(contains_text(source, "UCI_GID_SESSION_CONTROL = 0x2,"));
        ASSERT_TRUE(contains_text(source, "UCI_GID_QORVO_EXT1 = 0x9,"));
        ASSERT_TRUE(contains_text(source, "UCI_GID_QORVO_EXT2 = 0xb,"));
        ASSERT_TRUE(contains_text(source, "UCI_GID_ANDROID = 0xc,"));
        ASSERT_TRUE(contains_text(source, "UCI_GID_TEST = 0xd,"));
        ASSERT_TRUE(contains_text(source, "UCI_GID_QORVO_MAC = 0xe,"));
        ASSERT_TRUE(contains_text(source, "UCI_GID_QORVO_CALIB = 0xf,"));
        ASSERT_EQUAL(0x00, CORE);
        ASSERT_EQUAL(0x01, SESSION_CONFIG);
        ASSERT_EQUAL(0x02, SESSION_CONTROL);
        ASSERT_EQUAL(0x09, QORVO_EXT1);
        ASSERT_EQUAL(0x0B, QORVO_EXT2);
        ASSERT_EQUAL(0x0C, ANDROID);
        ASSERT_EQUAL(0x0D, TEST);
        ASSERT_EQUAL(0x0E, QORVO_MAC);
        ASSERT_EQUAL(0x0F, QORVO_CALIB);

        free(source);
        TEST_PASS();
    }
    test_case_end_cherry_fira_groups:
#undef test_case_end

#define test_case_end test_case_end_cherry_standard_oids
    TEST_CASE(cherry_standard_oids_match_repository_constants);
    {
        char *source = read_text_file(CHERRY_FIRA_HEADER);

        ASSERT_TRUE(source != NULL);
        ASSERT_TRUE(contains_text(source, "UCI_OID_CORE_GET_DEVICE_INFO = 0x2,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_CORE_GET_CAPS_INFO = 0x3,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_CORE_SET_CONFIG = 0x4,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_CORE_GET_CONFIG = 0x5,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_SESSION_INIT = 0x0,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_SESSION_GET_COUNT = 0x5,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_SESSION_GET_STATE = 0x6,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_SESSION_START = 0x0,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_SESSION_STOP = 0x1,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_SESSION_INFO = UCI_OID_SESSION_START,"));
        ASSERT_EQUAL(0x02, CORE_DEVICE_INFO);
        ASSERT_EQUAL(0x03, CORE_GET_CAPS_INFO);
        ASSERT_EQUAL(0x04, CORE_SET_CONFIG);
        ASSERT_EQUAL(0x05, CORE_GET_CONFIG);
        ASSERT_EQUAL(0x00, SESSION_INIT);
        ASSERT_EQUAL(0x05, SESSION_GET_COUNT);
        ASSERT_EQUAL(0x06, SESSION_GET_STATE);
        ASSERT_EQUAL(0x00, SESSION_START);
        ASSERT_EQUAL(0x01, SESSION_STOP);
        ASSERT_EQUAL(SESSION_START, SESSION_INFO_NTF);

        free(source);
        TEST_PASS();
    }
    test_case_end_cherry_standard_oids:
#undef test_case_end

#define test_case_end test_case_end_cherry_qorvo_ext2
    TEST_CASE(cherry_qorvo_ext2_opcodes_match_repository_constants);
    {
        char *source = read_text_file(CHERRY_QORVO_HEADER);

        ASSERT_TRUE(source != NULL);
        ASSERT_TRUE(contains_text(source, "UCI_OID_QORVO_TEST_DEBUG = 0x00,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_QORVO_TEST_TX_CW = 0x01,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_QORVO_TEST_PLLRF = 0x02,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_QORVO_FIRA_RANGE_DIAGNOSTICS = 0x03,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_QORVO_SESSION_GET = 0x07,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_QORVO_CORE_PSDU_DUMP = 0x22,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_QORVO_CORE_GET_DEVICE_STATS = 0x27,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_QORVO_CORE_DEVICE_BOOT = 0x31,"));
        ASSERT_EQUAL(0x00, QORVO_TEST_DEBUG);
        ASSERT_EQUAL(0x01, QORVO_TEST_TX_CW);
        ASSERT_EQUAL(0x02, QORVO_TEST_PLLRF);
        ASSERT_EQUAL(0x03, QORVO_FIRA_RANGE_DIAGNOSTICS);
        ASSERT_EQUAL(0x07, QORVO_SESSION_GET);
        ASSERT_EQUAL(0x22, QORVO_CORE_PSDU_DUMP);
        ASSERT_EQUAL(0x27, QORVO_CORE_GET_DEVICE_STATS);
        ASSERT_EQUAL(0x31, QORVO_CORE_DEVICE_BOOT);

        free(source);
        TEST_PASS();
    }
    test_case_end_cherry_qorvo_ext2:
#undef test_case_end

#define test_case_end test_case_end_cherry_mcps_surface
    TEST_CASE(cherry_mcps_surface_is_explicit_in_repository_basis);
    {
        char *mcps = read_text_file(CHERRY_MCPS_HEADER);
        char *pdl = read_text_file(REPO_PDL_HEADER);

        ASSERT_TRUE(mcps != NULL);
        ASSERT_TRUE(pdl != NULL);
        ASSERT_TRUE(contains_text(mcps, "UCI_OID_QORVO_MAC_START = 0x00,"));
        ASSERT_TRUE(contains_text(mcps, "UCI_OID_QORVO_MAC_TESTMODE = 0x3f,"));
        ASSERT_TRUE(contains_text(pdl, "#define QORVO_MAC       0x0E"));
        ASSERT_TRUE(contains_text(pdl, "#define QORVO_CALIB     0x0F"));

        free(mcps);
        free(pdl);
        TEST_PASS();
    }
    test_case_end_cherry_mcps_surface:
#undef test_case_end

#define test_case_end test_case_end_gid_0x0e_basis
    TEST_CASE(gid_0x0e_basis_choice_is_explicit);
    {
        char *cherry = read_text_file(CHERRY_FIRA_HEADER);
        char *python = read_text_file(PYTHON_QORVO_MSG);
        char *pdl = read_text_file(REPO_PDL_HEADER);

        ASSERT_TRUE(cherry != NULL);
        ASSERT_TRUE(python != NULL);
        ASSERT_TRUE(pdl != NULL);
        ASSERT_TRUE(contains_text(cherry, "UCI_GID_QORVO_MAC = 0xe,"));
        ASSERT_TRUE(contains_text(python, "ConfigManager = 0x0E"));
        ASSERT_TRUE(contains_text(pdl, "GID 0x0E follows"));
        ASSERT_TRUE(contains_text(pdl, "#define QORVO_MAC       0x0E"));

        free(cherry);
        free(python);
        free(pdl);
        TEST_PASS();
    }
    test_case_end_gid_0x0e_basis:
#undef test_case_end

#define test_case_end test_case_end_session_info_mapping
    TEST_CASE(cherry_session_info_wire_mapping_matches_repository_constants);
    {
        char *source = read_text_file(CHERRY_SESSION_CLIENT);

        ASSERT_TRUE(source != NULL);
        ASSERT_TRUE(contains_text(source, "UCI_GID_SESSION_CONTROL,"));
        ASSERT_TRUE(contains_text(source, "UCI_OID_SESSION_INFO)"));
        ASSERT_TRUE(contains_text(source, "#define INFO_NTF_HEADER_SIZE 25"));
        ASSERT_TRUE(contains_text(source, ".handler = uci_rsp_range_data_ntf_handler,"));
        ASSERT_TRUE(contains_text(source, "data.session_handle = uci_message_get_32bit(&parser);"));
        ASSERT_TRUE(contains_text(source, "data.n_measurements = uci_message_get_8bit(&parser);"));
        ASSERT_EQUAL(0x02, SESSION_CONTROL);
        ASSERT_EQUAL(0x00, SESSION_INFO_NTF);
        ASSERT_EQUAL(SESSION_START, SESSION_INFO_NTF_OPCODE);

        free(source);
        TEST_PASS();
    }
    test_case_end_session_info_mapping:
#undef test_case_end

    TEST_SUITE_END();
}
