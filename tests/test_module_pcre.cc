//////////////////////////////////////////////////////////////////////////////
// Licensed to Qualys, Inc. (QUALYS) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.
// QUALYS licenses this file to You under the Apache License, Version 2.0
// (the "License"); you may not use this file except in compliance with
// the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief IronBee - poc_sig module tests
/// 
/// @author Brian Rectanus <brectanus@qualys.com>
//////////////////////////////////////////////////////////////////////////////

#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"

#define TESTING

#include "base_fixture.h"
#include <ironbee/operator.h>
#include <ironbee/hash.h>
#include <ironbee/mpool.h>

class PcreModuleTest : public BaseModuleFixture {
public:
    PcreModuleTest() : BaseModuleFixture("ibmod_pcre.so") 
    {
    }

    virtual void SetUp() {
        BaseModuleFixture::SetUp();
        configureIronBee();
    }
};

TEST_F(PcreModuleTest, test_load_module)
{
    ib_operator_t op;
    ib_operator_inst_t *op_inst=NULL;
    ib_num_t result;

    ib_field_t* field1;
    ib_field_t* field2;

    char* str1 = (char*) ib_mpool_alloc(ib_engine->mp, (strlen("string 1")+1));
    char* str2 = (char*) ib_mpool_alloc(ib_engine->mp, (strlen("string 2")+1));
    strcpy(str1, "string 1");
    strcpy(str2, "string 2");
    
    // Create field 1.
    ASSERT_EQ(IB_OK,
        ib_field_create(
            &field1, ib_engine->mp, "field1", IB_FTYPE_NULSTR, &str1));

    // Create field 2.
    ASSERT_EQ(IB_OK,
        ib_field_create(
            &field2, ib_engine->mp, "field2", IB_FTYPE_NULSTR, &str2));

    // Ensure that the operator exists.
    ASSERT_EQ(IB_OK, ib_hash_get(ib_engine->operators, "@pcre", &op));

    // Get the operator.
    ASSERT_EQ(IB_OK,
              ib_operator_inst_create(ib_engine,
                                      "@pcre",
                                      "string\\s2",
                                      0,
                                      &op_inst));

    // Attempt to match.
    ASSERT_EQ(IB_OK, op_inst->op->fn_execute(
        ib_engine, NULL, op_inst->data, field1, &result));

    // We should fail.
    ASSERT_FALSE(result);

    // Attempt to match again.
    ASSERT_EQ(IB_OK, op_inst->op->fn_execute(
        ib_engine, NULL, op_inst->data, field2, &result));

    // This time we should succeed.
    ASSERT_TRUE(result);
}
