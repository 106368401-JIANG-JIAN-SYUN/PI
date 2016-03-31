/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PI/pi_p4info.h"
#include "p4info/p4info_struct.h"
#include "p4info/fields_int.h"
#include "p4info/actions_int.h"
#include "p4info/tables_int.h"
#include "PI/frontends/generic/pi.h"
#include "pi_int.h"

#include "unity/unity_fixture.h"

static pi_p4info_t *p4info;
static size_t num_fields;
static size_t num_actions;
static size_t num_tables;
static pi_match_key_t *mkey;
static pi_action_data_t *adata;
static pi_p4_id_t fid, aid, tid, pid;

TEST_GROUP(FrontendGeneric_OneExact);

TEST_SETUP(FrontendGeneric_OneExact) {
  p4info = calloc(1, sizeof(pi_p4info_t));
  num_fields = 1; num_actions = 1; num_tables = 1;
}

TEST_TEAR_DOWN(FrontendGeneric_OneExact) {
  free(p4info);
}

static void p4info_init(size_t bitwidth) {
  pi_p4info_field_init(p4info, num_fields);
  pi_p4info_action_init(p4info, num_actions);
  pi_p4info_table_init(p4info, num_tables);

  fid = pi_make_field_id(0);
  pi_p4info_field_add(p4info, fid, "f0", bitwidth);
  aid = pi_make_action_id(0);
  pi_p4info_action_add(p4info, aid, "a0", 1);
  pid = pi_make_action_param_id(aid, 0);
  pi_p4info_action_add_param(p4info, aid, pid, "p0_0", bitwidth);
  tid = pi_make_table_id(0);
  pi_p4info_table_add(p4info, tid, "t0", 1, 1);
  pi_p4info_table_add_match_field(p4info, tid, fid, "f0",
                                  PI_P4INFO_MATCH_TYPE_EXACT, bitwidth);
  pi_p4info_table_add_action(p4info, tid, aid);

  pi_match_key_allocate(p4info, tid, &mkey);

  pi_action_data_allocate(p4info, aid, &adata);
}

static void p4info_destroy() {
  pi_match_key_destroy(mkey);

  pi_action_data_destroy(adata);

  pi_p4info_field_free(p4info);
  pi_p4info_action_free(p4info);
  pi_p4info_table_free(p4info);  
}

TEST(FrontendGeneric_OneExact, U8) {
  pi_status_t rc;
  for (size_t bitwidth = 1; bitwidth <= 8; bitwidth++) {
    p4info_init(bitwidth);
    for (uint32_t v = 0; v < (uint32_t) (1 << bitwidth); v++)  {
      uint8_t test_v = v;
      pi_netv_t fv;
      pi_match_key_init(p4info, mkey);
      rc = pi_getnetv_u8(p4info, fid, test_v, &fv);
      TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
      rc = pi_match_key_exact_set(p4info, mkey, &fv);
      TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
      TEST_ASSERT_EQUAL_UINT(tid, mkey->table_id);
      TEST_ASSERT_EQUAL_UINT(1, mkey->nset);
      _compact_v_t *cv = &mkey->data[0];
      char expected_data[1];
      expected_data[0] = test_v;
      TEST_ASSERT_EQUAL_MEMORY(&expected_data, cv->bytes,
                               sizeof(expected_data));
    }
    p4info_destroy();
  }
}

TEST(FrontendGeneric_OneExact, U128) {
  pi_status_t rc;
  char test_v[16];
  size_t bitwidth = 8 * sizeof(test_v);
  for (size_t i = 0; i < sizeof(test_v); i++)
    test_v[i] = rand() % 256;
  pi_netv_t fv;
  p4info_init(bitwidth);
  pi_match_key_init(p4info, mkey);
  rc = pi_getnetv_ptr(p4info, fid, test_v, sizeof(test_v), &fv);
  TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
  rc = pi_match_key_exact_set(p4info, mkey, &fv);
  TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
  TEST_ASSERT_EQUAL_UINT(tid, mkey->table_id);
  TEST_ASSERT_EQUAL_UINT(1, mkey->nset);
  _compact_v_t *cv = &mkey->data[0];
  TEST_ASSERT_EQUAL_MEMORY(test_v, cv->more_bytes, sizeof(test_v));
  p4info_destroy();
}

TEST_GROUP_RUNNER(FrontendGeneric_OneExact) {
  RUN_TEST_CASE(FrontendGeneric_OneExact, U8);
  RUN_TEST_CASE(FrontendGeneric_OneExact, U128);
}


TEST_GROUP(FrontendGeneric_OneLPM);

TEST_SETUP(FrontendGeneric_OneLPM) {
  p4info = calloc(1, sizeof(pi_p4info_t));
  num_fields = 1; num_actions = 1; num_tables = 1;
}

TEST_TEAR_DOWN(FrontendGeneric_OneLPM) {
  free(p4info);
}

TEST(FrontendGeneric_OneLPM, U8) {
  pi_status_t rc;
  size_t bitwidth = 7;
  pi_prefix_length_t prefix_length = 5;
  p4info_init(bitwidth);
  uint8_t test_v = 0x5b;
  pi_netv_t fv;
  pi_match_key_init(p4info, mkey);
  rc = pi_getnetv_u8(p4info, fid, test_v, &fv);
  TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
  rc = pi_match_key_lpm_set(p4info, mkey, &fv, prefix_length);
  TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
  TEST_ASSERT_EQUAL_UINT(tid, mkey->table_id);
  TEST_ASSERT_EQUAL_UINT(1, mkey->nset);
  _compact_v_t *cv = &mkey->data[0];
  char expected_data[1];
  expected_data[0] = test_v;
  TEST_ASSERT_EQUAL_MEMORY(&expected_data, cv->bytes,
                           sizeof(expected_data));
  cv = &mkey->data[1];
  TEST_ASSERT_EQUAL_UINT64(prefix_length, cv->v);
  p4info_destroy();
}

TEST_GROUP_RUNNER(FrontendGeneric_OneLPM) {
  RUN_TEST_CASE(FrontendGeneric_OneLPM, U8);
}


TEST_GROUP(FrontendGeneric_OneTernary);

TEST_SETUP(FrontendGeneric_OneTernary) {
  p4info = calloc(1, sizeof(pi_p4info_t));
  num_fields = 1; num_actions = 1; num_tables = 1;
}

TEST_TEAR_DOWN(FrontendGeneric_OneTernary) {
  free(p4info);
}

TEST(FrontendGeneric_OneTernary, U8) {
  pi_status_t rc;
  size_t bitwidth = 7;
  p4info_init(bitwidth);
  uint8_t test_v = 0x5b;
  uint8_t test_mask = 0x72;
  pi_netv_t fv, mask;
  pi_match_key_init(p4info, mkey);
  rc = pi_getnetv_u8(p4info, fid, test_v, &fv);
  TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
  rc = pi_getnetv_u8(p4info, fid, test_mask, &mask);
  TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
  rc = pi_match_key_ternary_set(p4info, mkey, &fv, &mask);
  TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
  TEST_ASSERT_EQUAL_UINT(tid, mkey->table_id);
  TEST_ASSERT_EQUAL_UINT(1, mkey->nset);
  _compact_v_t *cv = &mkey->data[0];
  char expected_data[1];
  expected_data[0] = test_v;
  TEST_ASSERT_EQUAL_MEMORY(&expected_data, cv->bytes,
                           sizeof(expected_data));
  cv = &mkey->data[1];
  expected_data[0] = test_mask;
  TEST_ASSERT_EQUAL_MEMORY(&expected_data, cv->bytes,
                           sizeof(expected_data));
  p4info_destroy();
}

TEST_GROUP_RUNNER(FrontendGeneric_OneTernary) {
  RUN_TEST_CASE(FrontendGeneric_OneTernary, U8);
}


TEST_GROUP(FrontendGeneric_Adata);

TEST_SETUP(FrontendGeneric_Adata) {
  p4info = calloc(1, sizeof(pi_p4info_t));
  num_fields = 1; num_actions = 1; num_tables = 1;
}

TEST_TEAR_DOWN(FrontendGeneric_Adata) {
  free(p4info);
}

TEST(FrontendGeneric_Adata, U8) {
  pi_status_t rc;
  for (size_t bitwidth = 1; bitwidth <= 8; bitwidth++) {
    p4info_init(bitwidth);
    for (uint32_t v = 0; v < (uint32_t) (1 << bitwidth); v++)  {
      uint8_t test_v = v;
      pi_netv_t argv;
      pi_action_data_init(p4info, adata);
      rc = pi_getnetv_u8(p4info, pid, test_v, &argv);
      TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
      rc = pi_action_data_arg_set(p4info, adata, &argv);
      TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
      TEST_ASSERT_EQUAL_UINT(aid, adata->action_id);
      TEST_ASSERT_EQUAL_UINT(1, adata->nset);
      _compact_v_t *cv = &adata->data[0];
      char expected_data[1];
      expected_data[0] = test_v;
      TEST_ASSERT_EQUAL_MEMORY(&expected_data, cv->bytes,
                               sizeof(expected_data));
    }
    p4info_destroy();
  }
}

TEST(FrontendGeneric_Adata, U128) {
  pi_status_t rc;
  char test_v[16];
  size_t bitwidth = 8 * sizeof(test_v);
  for (size_t i = 0; i < sizeof(test_v); i++)
    test_v[i] = rand() % 256;
  pi_netv_t argv;
  p4info_init(bitwidth);
  pi_action_data_init(p4info, adata);
  rc = pi_getnetv_ptr(p4info, pid, test_v, sizeof(test_v), &argv);
  TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
  rc = pi_action_data_arg_set(p4info, adata, &argv);
  TEST_ASSERT_EQUAL_INT(PI_STATUS_SUCCESS, rc);
  TEST_ASSERT_EQUAL_UINT(aid, adata->action_id);
  TEST_ASSERT_EQUAL_UINT(1, adata->nset);
  _compact_v_t *cv = &adata->data[0];
  TEST_ASSERT_EQUAL_MEMORY(test_v, cv->more_bytes, sizeof(test_v));
  p4info_destroy();
}

TEST_GROUP_RUNNER(FrontendGeneric_Adata) {
  RUN_TEST_CASE(FrontendGeneric_Adata, U8);
  RUN_TEST_CASE(FrontendGeneric_Adata, U128);
}


void test_frontends_generic() {
  RUN_TEST_GROUP(FrontendGeneric_OneExact);
  RUN_TEST_GROUP(FrontendGeneric_OneLPM);
  RUN_TEST_GROUP(FrontendGeneric_OneTernary);
  RUN_TEST_GROUP(FrontendGeneric_Adata);
}