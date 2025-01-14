#include "unit_test.h"
#include "../src/luos_phy.c"
#include "../src/msg_alloc.c"
#include "../src/luos_io.c"
#include "../../core/src/node.c"
#include <default_scenario.h>

luos_phy_t *robus_phy;
uint8_t buffer[512];
phy_job_t *Luos_handled_job  = NULL;
phy_job_t *Robus_handled_job = NULL;
bool Luos_get_deadTarget     = false;
bool Robus_get_deadTarget    = false;

static void phy_luos_MsgHandler(luos_phy_t *phy_ptr, phy_job_t *job)
{
    // Test dispatch re-entry protection
    volatile uint16_t initial_job_nb = phy_ctx.io_job_nb;
    Phy_Dispatch();
    TEST_ASSERT_EQUAL(initial_job_nb, phy_ctx.io_job_nb);
    Luos_handled_job = job;
    if (job->msg_pt->header.cmd == DEADTARGET)
    {
        Luos_get_deadTarget = true;
    }
}

static void phy_robus_MsgHandler(luos_phy_t *phy_ptr, phy_job_t *job)
{
    // Test dispatch re-entry protection
    volatile uint16_t initial_job_nb = phy_ctx.io_job_nb;
    Phy_Dispatch();
    TEST_ASSERT_EQUAL(initial_job_nb, phy_ctx.io_job_nb);
    Robus_handled_job = job;
    if (job->msg_pt->header.cmd == DEADTARGET)
    {
        Robus_get_deadTarget = true;
    }
}

error_return_t topo_cb(luos_phy_t *phy_ptr, uint8_t *portId)
{
    return SUCCEED;
}

void reset_cb(luos_phy_t *phy_ptr)
{
}

static void phy_test_reset(void)
{
    Phy_Init();
    //  Init default scenario context
    luos_phy = Phy_Get(0, phy_luos_MsgHandler, topo_cb, reset_cb);
    TEST_ASSERT_EQUAL(&phy_ctx.phy[0], luos_phy);
    TEST_ASSERT_EQUAL(1, phy_ctx.phy_nb);
    robus_phy = Phy_Create(phy_robus_MsgHandler, topo_cb, reset_cb);
    TEST_ASSERT_EQUAL(&phy_ctx.phy[1], robus_phy);
    TEST_ASSERT_EQUAL(2, phy_ctx.phy_nb);
}

/*******************************************************************************
 * File function
 ******************************************************************************/

void unittest_phy_alloc()
{
    NEW_TEST_CASE("Check assertion condition");
    {
        TRY
        {
            phy_test_reset();
            Phy_alloc(NULL);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            phy_test_reset();
            luos_phy->rx_alloc_job   = true;
            luos_phy->received_data  = sizeof(header_t) - 1;
            luos_phy->rx_buffer_base = buffer;
            luos_phy->rx_data        = buffer;
            Phy_alloc(luos_phy);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            phy_test_reset();
            luos_phy->rx_alloc_job   = true;
            luos_phy->received_data  = sizeof(header_t);
            luos_phy->rx_buffer_base = buffer;
            luos_phy->rx_data        = buffer;
            Phy_alloc(luos_phy);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            phy_test_reset();
            luos_phy->rx_alloc_job   = true;
            luos_phy->received_data  = MAX_DATA_MSG_SIZE + sizeof(header_t) + 1;
            luos_phy->rx_buffer_base = buffer;
            luos_phy->rx_data        = buffer;
            Phy_alloc(luos_phy);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            phy_test_reset();
            luos_phy->rx_alloc_job   = true;
            luos_phy->received_data  = MAX_DATA_MSG_SIZE + sizeof(header_t);
            luos_phy->rx_buffer_base = buffer;
            luos_phy->rx_data        = buffer;
            Phy_alloc(luos_phy);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            phy_test_reset();
            luos_phy->rx_alloc_job   = true;
            luos_phy->received_data  = sizeof(header_t);
            luos_phy->rx_buffer_base = buffer;
            luos_phy->rx_data        = 0;
            Phy_alloc(luos_phy);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            phy_test_reset();
            luos_phy->rx_alloc_job   = true;
            luos_phy->received_data  = sizeof(header_t);
            luos_phy->rx_buffer_base = 0;
            luos_phy->rx_data        = buffer;
            Phy_alloc(luos_phy);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            phy_test_reset();
            luos_phy->rx_alloc_job   = false;
            luos_phy->received_data  = sizeof(header_t) - 1;
            luos_phy->rx_buffer_base = 0;
            luos_phy->rx_data        = buffer;
            Phy_alloc(luos_phy);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check that we don't allocate anything if we don't need to keep the data");
    {
        TRY
        {
            phy_test_reset();
            luos_phy->rx_alloc_job   = true;
            luos_phy->received_data  = sizeof(header_t);
            luos_phy->rx_buffer_base = buffer;
            luos_phy->rx_data        = buffer;
            luos_phy->rx_keep        = false;
            Phy_alloc(luos_phy);
            TEST_ASSERT_EQUAL(luos_phy->rx_buffer_base, luos_phy->rx_data);
            TEST_ASSERT_EQUAL(false, luos_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check normal allocation condition");
    {
        memory_stats_t memory_stats;
        MsgAlloc_Init(&memory_stats);
        TRY
        {
            phy_test_reset();
            luos_phy->rx_alloc_job   = true;
            luos_phy->received_data  = sizeof(header_t);
            luos_phy->rx_buffer_base = buffer;
            luos_phy->rx_data        = buffer;
            luos_phy->rx_keep        = true;
            // Message computed details
            memcpy(buffer, "123test", sizeof("123test"));
            luos_phy->rx_size       = 10;
            luos_phy->rx_phy_filter = 1;
            Phy_alloc(luos_phy);
            TEST_ASSERT_EQUAL(false, luos_phy->rx_alloc_job);
            TEST_ASSERT_NOT_EQUAL(luos_phy->rx_buffer_base, luos_phy->rx_data);
            TEST_ASSERT_EQUAL_STRING("123test", luos_phy->rx_data);
            TEST_ASSERT_EQUAL_STRING("123test", msg_buffer);
            // write the data we receive next
            luos_phy->rx_data[sizeof(header_t)]     = '3';
            luos_phy->rx_data[sizeof(header_t) + 1] = '2';
            luos_phy->rx_data[sizeof(header_t) + 2] = '1';
            TEST_ASSERT_EQUAL_STRING("123test321", msg_buffer);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            // Allocate a second message
            luos_phy->rx_alloc_job   = true;
            luos_phy->received_data  = sizeof(header_t);
            luos_phy->rx_buffer_base = buffer;
            luos_phy->rx_data        = buffer;
            luos_phy->rx_keep        = true;
            // Message computed details
            memcpy(buffer, "123test", sizeof("123test"));
            luos_phy->rx_size       = 10;
            luos_phy->rx_phy_filter = 1;
            Phy_alloc(luos_phy);
            TEST_ASSERT_EQUAL(false, luos_phy->rx_alloc_job);
            TEST_ASSERT_NOT_EQUAL(luos_phy->rx_buffer_base, luos_phy->rx_data);
            TEST_ASSERT_EQUAL_STRING("123test", luos_phy->rx_data);
            TEST_ASSERT_EQUAL_STRING("123test321123test", msg_buffer);
            // write the data we receive next
            luos_phy->rx_data[sizeof(header_t)]     = '3';
            luos_phy->rx_data[sizeof(header_t) + 1] = '2';
            luos_phy->rx_data[sizeof(header_t) + 2] = '1';
            TEST_ASSERT_EQUAL_STRING("123test321123test321", msg_buffer);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check allocation overflow");
    {
        memory_stats_t memory_stats;
        MsgAlloc_Init(&memory_stats);
        // Put a fake message in allocation
        oldest_alloc_slot    = 0;
        available_alloc_slot = 1;
        alloc_slots[0].data  = (uint8_t *)&msg_buffer[7];
        TRY
        {
            phy_test_reset();
            luos_phy->rx_alloc_job   = true;
            luos_phy->received_data  = sizeof(header_t);
            luos_phy->rx_buffer_base = buffer;
            luos_phy->rx_data        = buffer;
            luos_phy->rx_keep        = true;
            // Message computed details
            memcpy(buffer, "123test", sizeof("123test"));
            luos_phy->rx_size       = 10;
            luos_phy->rx_phy_filter = 1;
            Phy_alloc(luos_phy);

            TEST_ASSERT_EQUAL(false, luos_phy->rx_alloc_job);
            TEST_ASSERT_EQUAL(false, luos_phy->rx_keep);
            TEST_ASSERT_EQUAL_STRING(NULL, luos_phy->rx_data);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            // In the same condition than the previous one we should assert in any other phy than the Luos one.
            phy_test_reset();
            robus_phy->rx_alloc_job   = true;
            robus_phy->received_data  = sizeof(header_t);
            robus_phy->rx_buffer_base = buffer;
            robus_phy->rx_data        = buffer;
            robus_phy->rx_keep        = true;
            // Message computed details
            robus_phy->rx_size       = 10;
            robus_phy->rx_phy_filter = 1;
            Phy_alloc(robus_phy);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }
}

void unittest_phy_dispatch()
{
    NEW_TEST_CASE("Check re-entry protection and single dispatching with all values");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake job
            phy_ctx.io_job_nb = 1;
            /// Create msg data
            phy_ctx.io_job[0].alloc_msg                     = (msg_t *)&msg_buffer[0];
            phy_ctx.io_job[0].alloc_msg->header.config      = TIMESTAMP_PROTOCOL;
            phy_ctx.io_job[0].alloc_msg->header.size        = 3;
            phy_ctx.io_job[0].alloc_msg->header.target_mode = NODEIDACK;
            phy_ctx.io_job[0].size                          = 10;
            phy_ctx.io_job[0].phy_filter                    = 0x01; // Target Luos phy only
            phy_ctx.io_job[0].timestamp                     = 10;   // This represent the reception date
            time_luos_t timestamp_latency                   = TimeOD_TimeFrom_ns(10);
            memcpy(&phy_ctx.io_job[0].alloc_msg->data[phy_ctx.io_job[0].alloc_msg->header.size], &timestamp_latency, sizeof(time_luos_t));

            Phy_Dispatch();

            TEST_ASSERT_EQUAL(0, phy_ctx.io_job_nb);
            TEST_ASSERT_EQUAL(1, luos_phy->job_nb);
            TEST_ASSERT_EQUAL(&luos_phy->job[0], Luos_handled_job);
            TEST_ASSERT_EQUAL(&msg_buffer[0], luos_phy->job[0].data_pt);
            TEST_ASSERT_EQUAL(10, luos_phy->job[0].size);
            TEST_ASSERT_EQUAL(true, luos_phy->job[0].ack);
            TEST_ASSERT_EQUAL(true, luos_phy->job[0].timestamp);
            time_luos_t timestamp_date;
            memcpy(&timestamp_date, &phy_ctx.io_job[0].alloc_msg->data[phy_ctx.io_job[0].alloc_msg->header.size], sizeof(time_luos_t));
            TEST_ASSERT_FLOAT_WITHIN(1.0, 20.0, TimeOD_TimeTo_ns(timestamp_date));
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check re-entry protection and multiple dispatching with all values");
    {
        TRY
        {
            phy_test_reset();
            memset(&luos_phy->job[0], 0, sizeof(phy_job_t));
            luos_phy->job_nb              = 0;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 0;

            // Create a fake job
            phy_ctx.io_job_nb = 1;
            /// Create msg data
            phy_ctx.io_job[0].alloc_msg                     = (msg_t *)&msg_buffer[0];
            phy_ctx.io_job[0].alloc_msg->header.config      = TIMESTAMP_PROTOCOL;
            phy_ctx.io_job[0].alloc_msg->header.size        = 3;
            phy_ctx.io_job[0].alloc_msg->header.target_mode = NODEIDACK;
            phy_ctx.io_job[0].size                          = 10;
            phy_ctx.io_job[0].phy_filter                    = 0x03; // Target Luos and Robus phy
            phy_ctx.io_job[0].timestamp                     = 10;   // This represent the reception date
            time_luos_t timestamp_latency                   = TimeOD_TimeFrom_ns(10);
            memcpy(&phy_ctx.io_job[0].alloc_msg->data[phy_ctx.io_job[0].alloc_msg->header.size], &timestamp_latency, sizeof(time_luos_t));

            Phy_Dispatch();

            time_luos_t timestamp_date;
            TEST_ASSERT_EQUAL(0, phy_ctx.io_job_nb);

            TEST_ASSERT_EQUAL(1, luos_phy->job_nb);
            TEST_ASSERT_EQUAL(0, luos_phy->oldest_job_index);
            TEST_ASSERT_EQUAL(1, luos_phy->available_job_index);
            TEST_ASSERT_EQUAL(&luos_phy->job[0], Luos_handled_job);
            TEST_ASSERT_EQUAL(&msg_buffer[0], luos_phy->job[0].data_pt);
            TEST_ASSERT_EQUAL(10, luos_phy->job[0].size);
            TEST_ASSERT_EQUAL(true, luos_phy->job[0].ack);
            TEST_ASSERT_EQUAL(true, luos_phy->job[0].timestamp);
            memcpy(&timestamp_date, &luos_phy->job[0].msg_pt->data[luos_phy->job[0].msg_pt->header.size], sizeof(time_luos_t));
            TEST_ASSERT_FLOAT_WITHIN(1.0, 20.0, TimeOD_TimeTo_ns(timestamp_date));

            TEST_ASSERT_EQUAL(1, robus_phy->job_nb);
            TEST_ASSERT_EQUAL(&robus_phy->job[0], Robus_handled_job);
            TEST_ASSERT_EQUAL(&msg_buffer[0], robus_phy->job[0].data_pt);
            TEST_ASSERT_EQUAL(10, robus_phy->job[0].size);
            TEST_ASSERT_EQUAL(true, robus_phy->job[0].ack);
            TEST_ASSERT_EQUAL(true, robus_phy->job[0].timestamp);
            memcpy(&timestamp_date, &robus_phy->job[0].msg_pt->data[robus_phy->job[0].msg_pt->header.size], sizeof(time_luos_t));
            TEST_ASSERT_FLOAT_WITHIN(1.0, 20.0, TimeOD_TimeTo_ns(timestamp_date));
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Try to dispatch corrupted jobs");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake job
            phy_ctx.io_job_nb = 1;
            /// Create msg data
            phy_ctx.io_job[0].alloc_msg  = 0;
            phy_ctx.io_job[0].size       = 10;
            phy_ctx.io_job[0].phy_filter = 0x01; // Target Luos phy only
            phy_ctx.io_job[0].timestamp  = 10;   // This represent the reception date

            Phy_Dispatch();
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            phy_test_reset();
            // Create a fake job
            phy_ctx.io_job_nb = 1;
            /// Create msg data
            phy_ctx.io_job[0].alloc_msg                     = (msg_t *)&msg_buffer[0];
            phy_ctx.io_job[0].alloc_msg->header.config      = TIMESTAMP_PROTOCOL;
            phy_ctx.io_job[0].alloc_msg->header.size        = 3;
            phy_ctx.io_job[0].alloc_msg->header.target_mode = NODEIDACK;
            phy_ctx.io_job[0].size                          = 0;
            phy_ctx.io_job[0].phy_filter                    = 0x01; // Target Luos phy only
            phy_ctx.io_job[0].timestamp                     = 10;   // This represent the reception date
            time_luos_t timestamp_latency                   = TimeOD_TimeFrom_ns(10);
            memcpy(&phy_ctx.io_job[0].alloc_msg->data[phy_ctx.io_job[0].alloc_msg->header.size], &timestamp_latency, sizeof(time_luos_t));

            Phy_Dispatch();
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }
}

void unittest_phy_deadTarget()
{
    NEW_TEST_CASE("Check DeadTargetSpotted assertion conditions");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake phy job
            luos_phy->job_nb         = 1;
            luos_phy->job[0].data_pt = (uint8_t *)msg_buffer;
            Phy_FailedJob(luos_phy, 0);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            phy_test_reset();
            // Create a fake phy job
            luos_phy->job_nb         = 1;
            luos_phy->job[0].data_pt = (uint8_t *)msg_buffer;
            Phy_FailedJob(0, &luos_phy->job[0]);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            phy_test_reset();
            // Create a fake phy job
            luos_phy->job_nb         = 1;
            luos_phy->job[0].data_pt = 0;
            Phy_FailedJob(luos_phy, &luos_phy->job[0]);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            phy_test_reset();
            // Create a fake phy job
            luos_phy->job_nb         = 1;
            luos_phy->job[0].data_pt = (uint8_t *)msg_buffer;
            msg_t *msg               = (msg_t *)msg_buffer;
            msg->header.target_mode  = 0x0f;
            Phy_FailedJob(luos_phy, &luos_phy->job[0]);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }

    NEW_TEST_CASE("Check DeadTargetSpotted failed job creation");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake phy job
            luos_phy->job_nb              = 1;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 1;
            luos_phy->job[0].data_pt      = (uint8_t *)msg_buffer;
            msg_t *msg                    = (msg_t *)msg_buffer;
            msg->header.target_mode       = NODEIDACK;
            msg->header.target            = 1;

            TEST_ASSERT_EQUAL(0, phy_ctx.failed_job_nb);
            Phy_FailedJob(luos_phy, &luos_phy->job[0]);
            TEST_ASSERT_EQUAL(1, phy_ctx.failed_job_nb);
            TEST_ASSERT_EQUAL(msg_buffer, phy_ctx.failed_job[0].data_pt);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check DeadTargetSpotted phy job removal");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake phy job
            luos_phy->job_nb              = 3;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 3;
            luos_phy->job[0].data_pt      = (uint8_t *)msg_buffer;
            luos_phy->job[1].data_pt      = (uint8_t *)&msg_buffer[20]; // This is a different target, it should not be removed
            luos_phy->job[2].data_pt      = (uint8_t *)msg_buffer;
            msg_t *msg                    = (msg_t *)msg_buffer;
            msg->header.target_mode       = NODEIDACK;
            msg->header.target            = 1;

            TEST_ASSERT_EQUAL(0, phy_ctx.failed_job_nb);
            Phy_FailedJob(luos_phy, &luos_phy->job[0]);
            TEST_ASSERT_EQUAL(1, phy_ctx.failed_job_nb);
            TEST_ASSERT_EQUAL(msg_buffer, phy_ctx.failed_job[0].data_pt);
            TEST_ASSERT_EQUAL(1, luos_phy->job_nb);
            TEST_ASSERT_EQUAL(NULL, luos_phy->job[0].data_pt);
            TEST_ASSERT_EQUAL(&msg_buffer[20], luos_phy->job[1].data_pt);
            TEST_ASSERT_EQUAL(NULL, luos_phy->job[2].data_pt);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            phy_test_reset();
            // Create a fake phy job
            luos_phy->job_nb              = 3;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 3;
            luos_phy->job[0].data_pt      = (uint8_t *)msg_buffer;
            luos_phy->job[1].data_pt      = (uint8_t *)msg_buffer; // All jobs should be removed
            luos_phy->job[2].data_pt      = (uint8_t *)msg_buffer;
            msg_t *msg                    = (msg_t *)msg_buffer;
            msg->header.target_mode       = NODEIDACK;
            msg->header.target            = 1;

            TEST_ASSERT_EQUAL(0, phy_ctx.failed_job_nb);
            Phy_FailedJob(luos_phy, &luos_phy->job[0]);
            TEST_ASSERT_EQUAL(1, phy_ctx.failed_job_nb);
            TEST_ASSERT_EQUAL(msg_buffer, phy_ctx.failed_job[0].data_pt);
            TEST_ASSERT_EQUAL(0, luos_phy->job_nb);
            TEST_ASSERT_EQUAL(3, luos_phy->oldest_job_index);
            TEST_ASSERT_EQUAL(3, luos_phy->available_job_index);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check Failed job management and user notification");
    {
        TRY
        {
            phy_test_reset();
            memory_stats_t memory_stats;
            MsgAlloc_Init(&memory_stats);
            // Create a fake phy job
            luos_phy->job_nb              = 3;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 3;
            luos_phy->rx_phy_filter       = 0;
            luos_phy->job[0].data_pt      = (uint8_t *)msg_buffer;
            luos_phy->job[1].data_pt      = (uint8_t *)&msg_buffer[20]; // This is a different target, it should not be removed
            luos_phy->job[2].data_pt      = (uint8_t *)msg_buffer;
            msg_t *msg                    = (msg_t *)msg_buffer;
            msg->header.target_mode       = NODEIDACK;
            msg->header.target            = 1;

            TEST_ASSERT_EQUAL(0, phy_ctx.failed_job_nb);
            Phy_FailedJob(luos_phy, &luos_phy->job[0]);
            TEST_ASSERT_EQUAL(msg_buffer, phy_ctx.failed_job[0].data_pt);
            TEST_ASSERT_EQUAL(1, luos_phy->job_nb);
            TEST_ASSERT_EQUAL(NULL, luos_phy->job[0].data_pt);
            TEST_ASSERT_EQUAL(&msg_buffer[20], luos_phy->job[1].data_pt);
            TEST_ASSERT_EQUAL(NULL, luos_phy->job[2].data_pt);

            TEST_ASSERT_EQUAL(1, phy_ctx.failed_job_nb);
            TEST_ASSERT_EQUAL(false, Luos_get_deadTarget);
            TEST_ASSERT_EQUAL(false, Robus_get_deadTarget);
            Phy_ManageFailedJob();
            TEST_ASSERT_EQUAL(0, phy_ctx.failed_job_nb);
            Phy_Dispatch(); // Deal with messages
            TEST_ASSERT_EQUAL(true, Luos_get_deadTarget);
            TEST_ASSERT_EQUAL(true, Robus_get_deadTarget);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_phy_loop()
{

    NEW_TEST_CASE("Check allocation");
    {
        memory_stats_t memory_stats;
        MsgAlloc_Init(&memory_stats);
        TRY
        {
            phy_test_reset();
            robus_phy->rx_alloc_job   = true;
            robus_phy->received_data  = sizeof(header_t);
            robus_phy->rx_buffer_base = buffer;
            robus_phy->rx_data        = buffer;
            robus_phy->rx_keep        = true;
            // Message computed details
            robus_phy->rx_size       = 10;
            robus_phy->rx_phy_filter = 1;
            Phy_Loop();
            TEST_ASSERT_EQUAL(false, robus_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check dispatching and Failed job management");
    {
        TRY
        {
            phy_test_reset();
            memory_stats_t memory_stats;
            MsgAlloc_Init(&memory_stats);
            // Create a fake phy job
            luos_phy->job_nb         = 3;
            luos_phy->job[0].data_pt = (uint8_t *)msg_buffer;
            luos_phy->job[1].data_pt = (uint8_t *)&msg_buffer[20]; // This is a different target, it should not be removed
            luos_phy->job[2].data_pt = (uint8_t *)msg_buffer;
            msg_t *msg               = (msg_t *)msg_buffer;
            msg->header.target_mode  = NODEIDACK;
            msg->header.target       = 1;
            Luos_get_deadTarget      = false;
            Robus_get_deadTarget     = false;

            Phy_FailedJob(luos_phy, &luos_phy->job[0]);

            TEST_ASSERT_EQUAL(1, phy_ctx.failed_job_nb);
            Phy_Loop();
            TEST_ASSERT_EQUAL(0, phy_ctx.failed_job_nb);
            TEST_ASSERT_EQUAL(true, Luos_get_deadTarget);
            TEST_ASSERT_EQUAL(true, Robus_get_deadTarget);
            Luos_get_deadTarget  = false;
            Robus_get_deadTarget = false;
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_phy_ComputeHeader()
{
    NEW_TEST_CASE("Check ComputeHeader assertion conditions");
    {
        TRY
        {
            Phy_ComputeHeader(NULL);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader for a non timestamped message from Luos");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = BASE_PROTOCOL;
            msg.header.target_mode = SERVICEIDACK;
            msg.header.target      = 1;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 1;
            msg.data[0]            = 0xAE;

            // Save message information in the Luos phy struct
            // Luos can target himself.
            luos_phy->rx_buffer_base = (uint8_t *)&msg;
            luos_phy->rx_data        = luos_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            luos_phy->received_data = sizeof(header_t);
            luos_phy->rx_keep       = true; // Tell phy that we want to keep this message
            luos_phy->rx_ack        = false;
            luos_phy->rx_alloc_job  = false;
            luos_phy->rx_size       = 0;
            luos_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(luos_phy); // Here, because we use luos_phy, the message is accepted no matter what.
            TEST_ASSERT_EQUAL(msg.header.size + sizeof(header_t), luos_phy->rx_size);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_keep);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_ack);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader for a non timestamped message needed by Luos");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = BASE_PROTOCOL;
            msg.header.target_mode = SERVICEIDACK;
            msg.header.target      = 1;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 1;
            msg.header.source      = 2;
            msg.data[0]            = 0xAE;

            // Configure the source in indexes
            Phy_IndexSet(robus_phy->services, 2);
            // Save message information in the Luos phy struct
            robus_phy->rx_buffer_base = (uint8_t *)&msg;
            robus_phy->rx_data        = luos_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            robus_phy->received_data = sizeof(header_t);
            robus_phy->rx_keep       = true; // Tell phy that we want to keep this message
            robus_phy->rx_ack        = false;
            robus_phy->rx_alloc_job  = false;
            robus_phy->rx_size       = 0;
            robus_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(robus_phy); // Here, because we use luos_phy, the message is accepted no matter what.
            TEST_ASSERT_EQUAL(msg.header.size + sizeof(header_t), robus_phy->rx_size);
            TEST_ASSERT_EQUAL(true, robus_phy->rx_keep);
            TEST_ASSERT_EQUAL(true, robus_phy->rx_ack);
            TEST_ASSERT_EQUAL(true, robus_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader failure for a non referenced service source message needed by Luos");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = BASE_PROTOCOL;
            msg.header.target_mode = SERVICEIDACK;
            msg.header.target      = 1;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 1;
            msg.header.source      = 3;
            msg.data[0]            = 0xAE;

            // Configure the source in indexes
            Phy_IndexSet(robus_phy->services, 2);
            // Save message information in the Luos phy struct
            robus_phy->rx_buffer_base = (uint8_t *)&msg;
            robus_phy->rx_data        = luos_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            robus_phy->received_data = sizeof(header_t);
            robus_phy->rx_keep       = true; // Tell phy that we want to keep this message
            robus_phy->rx_ack        = false;
            robus_phy->rx_alloc_job  = false;
            robus_phy->rx_size       = 0;
            robus_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(robus_phy);
            TEST_ASSERT_EQUAL(msg.header.size + sizeof(header_t), robus_phy->rx_size);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_keep);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_ack);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader failure for a non referenced service source node message needed by Luos during the detection");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = BASE_PROTOCOL;
            msg.header.target_mode = NODEIDACK;
            msg.header.target      = 2;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 1;
            msg.header.source      = 2;
            msg.data[0]            = 0xAE;
            Node_Get()->node_id    = 0;

            // Configure the source in indexes
            Phy_IndexSet(robus_phy->services, 2);
            // Save message information in the Luos phy struct
            robus_phy->rx_buffer_base = (uint8_t *)&msg;
            robus_phy->rx_data        = luos_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            robus_phy->received_data = sizeof(header_t);
            robus_phy->rx_keep       = true; // Tell phy that we want to keep this message
            robus_phy->rx_ack        = false;
            robus_phy->rx_alloc_job  = false;
            robus_phy->rx_size       = 0;
            robus_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(robus_phy); // Here, because we don't have node_id yet, the message is rejected no matter what.
            TEST_ASSERT_EQUAL(msg.header.size + sizeof(header_t), robus_phy->rx_size);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_keep);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_ack);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader failure for a non referenced service source node message needed by Luos before detection");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = BASE_PROTOCOL;
            msg.header.target_mode = NODEIDACK;
            msg.header.target      = 2;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 1;
            msg.header.source      = 3;
            msg.data[0]            = 0xAE;
            Node_Get()->node_id    = 2;

            // Configure the source in indexes
            Phy_IndexSet(robus_phy->services, 2);
            // Save message information in the Luos phy struct
            robus_phy->rx_buffer_base = (uint8_t *)&msg;
            robus_phy->rx_data        = luos_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            robus_phy->received_data = sizeof(header_t);
            robus_phy->rx_keep       = true; // Tell phy that we want to keep this message
            robus_phy->rx_ack        = false;
            robus_phy->rx_alloc_job  = false;
            robus_phy->rx_size       = 0;
            robus_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(robus_phy); // Here, the source is not a referenced service, we have a node ID, but detection is not done, so the message is accepted.
            TEST_ASSERT_EQUAL(msg.header.size + sizeof(header_t), robus_phy->rx_size);
            TEST_ASSERT_EQUAL(true, robus_phy->rx_keep);
            TEST_ASSERT_EQUAL(true, robus_phy->rx_ack);
            TEST_ASSERT_EQUAL(true, robus_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader failure for a non referenced service source node message needed by Luos");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = BASE_PROTOCOL;
            msg.header.target_mode = NODEIDACK;
            msg.header.target      = 2;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 1;
            msg.header.source      = 3;
            msg.data[0]            = 0xAE;
            Node_Get()->node_id    = 2;
            node_ctx.state         = DETECTION_OK;

            // Configure the source in indexes
            Phy_IndexSet(robus_phy->services, 2);
            // Save message information in the Luos phy struct
            robus_phy->rx_buffer_base = (uint8_t *)&msg;
            robus_phy->rx_data        = luos_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            robus_phy->received_data = sizeof(header_t);
            robus_phy->rx_keep       = true; // Tell phy that we want to keep this message
            robus_phy->rx_ack        = false;
            robus_phy->rx_alloc_job  = false;
            robus_phy->rx_size       = 0;
            robus_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(robus_phy); // Here, the source is not a referenced service, we have a node ID, and detection is  done, so the message is rejected.
            TEST_ASSERT_EQUAL(msg.header.size + sizeof(header_t), robus_phy->rx_size);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_keep);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_ack);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader failure for a referenced service source node message needed by Luos");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = BASE_PROTOCOL;
            msg.header.target_mode = NODEIDACK;
            msg.header.target      = 2;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 1;
            msg.header.source      = 2;
            msg.data[0]            = 0xAE;
            Node_Get()->node_id    = 2;
            node_ctx.state         = DETECTION_OK;

            // Configure the source in indexes
            Phy_IndexSet(robus_phy->services, 2);
            // Save message information in the Luos phy struct
            robus_phy->rx_buffer_base = (uint8_t *)&msg;
            robus_phy->rx_data        = luos_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            robus_phy->received_data = sizeof(header_t);
            robus_phy->rx_keep       = true; // Tell phy that we want to keep this message
            robus_phy->rx_ack        = false;
            robus_phy->rx_alloc_job  = false;
            robus_phy->rx_size       = 0;
            robus_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(robus_phy); // Here, the source service is referenced, we have a node Id, and detection is done. The message is accepted.
            TEST_ASSERT_EQUAL(msg.header.size + sizeof(header_t), robus_phy->rx_size);
            TEST_ASSERT_EQUAL(true, robus_phy->rx_keep);
            TEST_ASSERT_EQUAL(true, robus_phy->rx_ack);
            TEST_ASSERT_EQUAL(true, robus_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader for a timestamped message needed by Luos");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = TIMESTAMP_PROTOCOL;
            msg.header.target_mode = SERVICEIDACK;
            msg.header.target      = 1;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 1;
            msg.header.source      = 2;
            msg.data[0]            = 0xAE;

            // Save message information in the Luos phy struct
            // Luos can target himself.
            luos_phy->rx_buffer_base = (uint8_t *)&msg;
            luos_phy->rx_data        = luos_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            luos_phy->received_data = sizeof(header_t);
            luos_phy->rx_keep       = true; // Tell phy that we want to keep this message
            luos_phy->rx_ack        = false;
            luos_phy->rx_alloc_job  = false;
            luos_phy->rx_size       = 0;
            luos_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(luos_phy);
            TEST_ASSERT_EQUAL(msg.header.size + sizeof(header_t) + sizeof(time_luos_t), luos_phy->rx_size);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_keep);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_ack);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader for a non timestamped large message needed by Luos");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = BASE_PROTOCOL;
            msg.header.target_mode = SERVICEID;
            msg.header.target      = 1;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 600;
            msg.data[0]            = 0xAE;

            // Save message information in the Luos phy struct
            // Luos can target himself.
            luos_phy->rx_buffer_base = (uint8_t *)&msg;
            luos_phy->rx_data        = luos_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            luos_phy->received_data = sizeof(header_t);
            luos_phy->rx_keep       = true; // Tell phy that we want to keep this message
            luos_phy->rx_ack        = false;
            luos_phy->rx_alloc_job  = false;
            luos_phy->rx_size       = 0;
            luos_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(luos_phy);
            TEST_ASSERT_EQUAL(MAX_DATA_MSG_SIZE + sizeof(header_t), luos_phy->rx_size);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_keep);
            TEST_ASSERT_EQUAL(false, luos_phy->rx_ack);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader for a timestamped large message needed by Luos");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = TIMESTAMP_PROTOCOL;
            msg.header.target_mode = SERVICEIDACK;
            msg.header.target      = 1;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 600;
            msg.data[0]            = 0xAE;

            // Save message information in the Luos phy struct
            // Luos can target himself.
            luos_phy->rx_buffer_base = (uint8_t *)&msg;
            luos_phy->rx_data        = luos_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            luos_phy->received_data = sizeof(header_t);
            luos_phy->rx_keep       = true; // Tell phy that we want to keep this message
            luos_phy->rx_ack        = false;
            luos_phy->rx_alloc_job  = false;
            luos_phy->rx_size       = 0;
            luos_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(luos_phy);
            TEST_ASSERT_EQUAL(MAX_DATA_MSG_SIZE + sizeof(header_t) + sizeof(time_luos_t), luos_phy->rx_size);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_keep);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_ack);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader for a timestamped large message needed by Robus");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = TIMESTAMP_PROTOCOL;
            msg.header.target_mode = NODEIDACK;
            msg.header.target      = 2;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 600;
            msg.data[0]            = 0xAE;

            // Save message information in the Luos phy struct
            luos_phy->rx_buffer_base = (uint8_t *)&msg;
            luos_phy->rx_data        = luos_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            luos_phy->received_data = sizeof(header_t);
            luos_phy->rx_keep       = true; // Tell phy that we want to keep this message
            luos_phy->rx_ack        = false;
            luos_phy->rx_alloc_job  = false;
            luos_phy->rx_size       = 0;
            luos_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(luos_phy);
            TEST_ASSERT_EQUAL(MAX_DATA_MSG_SIZE + sizeof(header_t) + sizeof(time_luos_t), luos_phy->rx_size);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_keep);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_ack);
            TEST_ASSERT_EQUAL(true, luos_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeHeader for a timestamped large message not needed by any phy");
    {
        TRY
        {
            phy_test_reset();
            // Create a fake service with id 1
            Phy_AddLocalServices(1, 1);

            msg_t msg;
            msg.header.config      = TIMESTAMP_PROTOCOL;
            msg.header.target_mode = NODEIDACK;
            msg.header.target      = 2;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 600;
            msg.data[0]            = 0xAE;

            // Save message information in the Luos phy struct
            // Robus cannot target himself.
            robus_phy->rx_buffer_base = (uint8_t *)&msg;
            robus_phy->rx_data        = robus_phy->rx_buffer_base;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            robus_phy->received_data = sizeof(header_t);
            robus_phy->rx_keep       = true; // Tell phy that we want to keep this message
            robus_phy->rx_ack        = false;
            robus_phy->rx_alloc_job  = false;
            robus_phy->rx_size       = 0;
            robus_phy->rx_phy_filter = 0x00;
            Phy_ComputeHeader(robus_phy);
            TEST_ASSERT_EQUAL(MAX_DATA_MSG_SIZE + sizeof(header_t) + sizeof(time_luos_t), luos_phy->rx_size);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_keep);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_ack);
            TEST_ASSERT_EQUAL(false, robus_phy->rx_alloc_job);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_phy_ValidMsg()
{
    NEW_TEST_CASE("Check ValidMsg assertion conditions");
    {
        TRY
        {
            Phy_ValidMsg(NULL);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }

    NEW_TEST_CASE("Check ValidMsg job creation");
    {
        TRY
        {
            phy_test_reset();

            msg_t msg;
            // Save message information in the Luos phy struct
            // Robus cannot target himself.
            luos_phy->rx_buffer_base = (uint8_t *)&msg;
            luos_phy->rx_data        = msg_buffer;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            luos_phy->received_data = sizeof(header_t);
            luos_phy->rx_keep       = true; // Tell phy that we want to keep this message
            luos_phy->rx_ack        = true;
            luos_phy->rx_alloc_job  = false;
            luos_phy->rx_size       = MAX_DATA_MSG_SIZE + sizeof(header_t) + sizeof(time_luos_t);
            luos_phy->rx_phy_filter = 0x02; // A Robus node is targeted
            luos_phy->rx_timestamp  = 10;

            TEST_ASSERT_EQUAL(0, phy_ctx.io_job_nb);
            Phy_ValidMsg(luos_phy);
            TEST_ASSERT_EQUAL(1, phy_ctx.io_job_nb);
            TEST_ASSERT_EQUAL(10, phy_ctx.io_job[0].timestamp);
            TEST_ASSERT_EQUAL(msg_buffer, phy_ctx.io_job[0].alloc_msg);
            TEST_ASSERT_EQUAL(0x02, phy_ctx.io_job[0].phy_filter);
            TEST_ASSERT_EQUAL(MAX_DATA_MSG_SIZE + sizeof(header_t) + sizeof(time_luos_t), phy_ctx.io_job[0].size);
            TEST_ASSERT_EQUAL(0, luos_phy->received_data);
            TEST_ASSERT_EQUAL(luos_phy->rx_buffer_base, luos_phy->rx_data);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check that ValidMsg don't create job if we don't need it");
    {
        TRY
        {
            phy_test_reset();

            msg_t msg;
            // Save message information in the Luos phy struct
            // Robus cannot target himself.
            luos_phy->rx_buffer_base = (uint8_t *)&msg;
            luos_phy->rx_data        = msg_buffer;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            luos_phy->received_data = sizeof(header_t);
            luos_phy->rx_keep       = false;
            luos_phy->rx_ack        = true;
            luos_phy->rx_alloc_job  = false;
            luos_phy->rx_size       = MAX_DATA_MSG_SIZE + sizeof(header_t) + sizeof(time_luos_t);
            luos_phy->rx_phy_filter = 0x02; // A Robus node is targeted
            luos_phy->rx_timestamp  = 10;

            TEST_ASSERT_EQUAL(0, phy_ctx.io_job_nb);
            Phy_ValidMsg(luos_phy);
            TEST_ASSERT_EQUAL(0, phy_ctx.io_job_nb);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }

    NEW_TEST_CASE("Check ValidMsg allocation");
    {
        TRY
        {
            phy_test_reset();
            memory_stats_t memory_stats;
            MsgAlloc_Init(&memory_stats);

            msg_t msg;
            msg.header.config      = TIMESTAMP_PROTOCOL;
            msg.header.target_mode = SERVICEIDACK;
            msg.header.target      = 2;
            msg.header.source      = 1;
            msg.header.cmd         = IO_STATE;
            msg.header.size        = 600;
            msg.data[0]            = 0xAE;
            // Save message information in the Luos phy struct
            // Robus cannot target himself.
            luos_phy->rx_buffer_base = (uint8_t *)&msg;
            luos_phy->rx_data        = (uint8_t *)&msg;
            // For now let just consider that we received the header allowing us to compute the message things based on it.
            luos_phy->received_data = sizeof(header_t);
            luos_phy->rx_keep       = true; // Tell phy that we want to keep this message
            luos_phy->rx_ack        = true;
            luos_phy->rx_alloc_job  = true;
            luos_phy->rx_size       = MAX_DATA_MSG_SIZE + sizeof(header_t) + sizeof(time_luos_t);
            luos_phy->rx_phy_filter = 0x00;
            luos_phy->rx_timestamp  = 10;
            luos_phy->services[0]   = 0x01; // Configure service 1 as accessible from Luos
            robus_phy->services[0]  = 0x02; // Configure this service as accessible from Robus

            TEST_ASSERT_EQUAL(0, phy_ctx.io_job_nb);
            Phy_ValidMsg(luos_phy);
            TEST_ASSERT_EQUAL(1, phy_ctx.io_job_nb);
            TEST_ASSERT_EQUAL(10, phy_ctx.io_job[0].timestamp);
            TEST_ASSERT_EQUAL(false, luos_phy->rx_alloc_job);
            TEST_ASSERT_EQUAL(msg_buffer, phy_ctx.io_job[0].alloc_msg);
            TEST_ASSERT_EQUAL(0x02, phy_ctx.io_job[0].phy_filter);
            TEST_ASSERT_EQUAL(MAX_DATA_MSG_SIZE + sizeof(header_t) + sizeof(time_luos_t), phy_ctx.io_job[0].size);
            TEST_ASSERT_EQUAL(0, luos_phy->received_data);
            TEST_ASSERT_EQUAL(luos_phy->rx_buffer_base, luos_phy->rx_data);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_phy_ComputeTimestamp()
{
    NEW_TEST_CASE("Check ComputeTimestamp assertion conditions");
    {
        phy_job_t job;
        TRY
        {
            Phy_ComputeMsgTimestamp(luos_phy, NULL);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            job.timestamp = true;
            job.data_pt   = (uint8_t *)msg_buffer;
            Phy_ComputeMsgTimestamp(NULL, &job);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            job.timestamp = false;
            job.data_pt   = (uint8_t *)msg_buffer;
            Phy_ComputeMsgTimestamp(luos_phy, &job);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            job.timestamp = true;
            job.data_pt   = NULL;
            Phy_ComputeMsgTimestamp(luos_phy, &job);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }

    NEW_TEST_CASE("Check ComputeTimestamp normal conditions");
    {
        TRY
        {
            phy_test_reset();
            memory_stats_t memory_stats;
            MsgAlloc_Init(&memory_stats);

            phy_job_t job;
            job.timestamp = true;
            job.data_pt   = (uint8_t *)msg_buffer;

            msg_t *msg              = (msg_t *)msg_buffer;
            msg->header.config      = TIMESTAMP_PROTOCOL;
            msg->header.target_mode = SERVICEIDACK;
            msg->header.target      = 1;
            msg->header.cmd         = IO_STATE;
            msg->header.size        = 1;
            msg->data[0]            = 0xAE;

            volatile time_luos_t timestamp = TimeOD_TimeFrom_ns(10);
            memcpy(&msg->data[msg->header.size], (void *)&timestamp, sizeof(time_luos_t));

            volatile time_luos_t resulting_latency = Phy_ComputeMsgTimestamp(luos_phy, &job);

#ifndef _WIN32
            TEST_ASSERT_NOT_EQUAL(TimeOD_TimeTo_ns(timestamp), TimeOD_TimeTo_ns(resulting_latency));
#endif
            Phy_DisableSynchro(luos_phy);
            resulting_latency = Phy_ComputeMsgTimestamp(luos_phy, &job);

#ifndef _WIN32
            TEST_ASSERT_EQUAL(TimeOD_TimeTo_ns(timestamp), TimeOD_TimeTo_ns(resulting_latency));
#endif
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_phy_GetNodeId()
{
    NEW_TEST_CASE("Check GetNodeId ");
    {
        TRY
        {
            uint16_t node_id = Phy_GetNodeId();
            TEST_ASSERT_EQUAL(Node_Get()->node_id, node_id);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_phy_AddJob()
{
    NEW_TEST_CASE("Check AddJob assertion conditions");
    {
        TRY
        {
            phy_job_t phy_job;
            luos_phy->job_nb         = 0;
            phy_job_t *resulting_job = Phy_AddJob(luos_phy, NULL);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            phy_job_t phy_job;
            luos_phy->job_nb         = 0;
            phy_job_t *resulting_job = Phy_AddJob(NULL, &phy_job);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            phy_job_t phy_job;
            luos_phy->job_nb         = MAX_MSG_NB;
            phy_job_t *resulting_job = Phy_AddJob(luos_phy, &phy_job);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }

    NEW_TEST_CASE("Check AddJob normal conditions");
    {
        TRY
        {
            phy_job_t phy_job;
            luos_phy->job_nb         = 0;
            phy_job_t *resulting_job = Phy_AddJob(luos_phy, &phy_job);
            TEST_ASSERT_EQUAL(&luos_phy->job[0], resulting_job);
            TEST_ASSERT_EQUAL(1, luos_phy->job_nb);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            phy_job_t phy_job;
            luos_phy->job_nb              = 3;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 3;
            phy_job_t *resulting_job      = Phy_AddJob(luos_phy, &phy_job);
            TEST_ASSERT_EQUAL(&luos_phy->job[3], resulting_job);
            TEST_ASSERT_EQUAL(4, luos_phy->job_nb);
            TEST_ASSERT_EQUAL(4, luos_phy->available_job_index);
            TEST_ASSERT_EQUAL(0, luos_phy->oldest_job_index);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_phy_GetJob()
{
    NEW_TEST_CASE("Check GetJob assertion conditions");
    {
        TRY
        {
            luos_phy->job_nb         = 0;
            phy_job_t *resulting_job = Phy_GetJob(NULL);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }

    NEW_TEST_CASE("Check GetJob normal conditions");
    {
        TRY
        {
            luos_phy->job_nb              = 0;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 0;
            phy_job_t *resulting_job      = Phy_GetJob(luos_phy);
            TEST_ASSERT_EQUAL(NULL, resulting_job);
            TEST_ASSERT_EQUAL(0, luos_phy->job_nb);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            luos_phy->job_nb              = 1;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 1;
            phy_job_t *resulting_job      = Phy_GetJob(luos_phy);
            TEST_ASSERT_EQUAL(&luos_phy->job[0], resulting_job);
            TEST_ASSERT_EQUAL(1, luos_phy->job_nb);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            luos_phy->job_nb              = 3;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 3;
            phy_job_t *resulting_job      = Phy_GetJob(luos_phy);
            TEST_ASSERT_EQUAL(&luos_phy->job[0], resulting_job);
            TEST_ASSERT_EQUAL(3, luos_phy->job_nb);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_phy_GetNextJob()
{
    NEW_TEST_CASE("Check GetNextJob assertion conditions");
    {
        TRY
        {
            luos_phy->job_nb              = 0;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 0;
            phy_job_t *job                = NULL;
            phy_job_t *resulting_job      = Phy_GetNextJob(NULL, job);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }

    NEW_TEST_CASE("Check GetNextJob normal conditions");
    {
        TRY
        {
            luos_phy->job_nb              = 0;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 0;
            phy_job_t *job                = NULL;
            job                           = Phy_GetNextJob(luos_phy, job);
            TEST_ASSERT_EQUAL(NULL, job);
            TEST_ASSERT_EQUAL(0, luos_phy->job_nb);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        phy_job_t *job = NULL;
        TRY
        {
            luos_phy->job_nb              = 1;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 1;
            luos_phy->job[0].data_pt      = (const uint8_t *)&msg_buffer[0];
            job                           = Phy_GetNextJob(luos_phy, job);
            TEST_ASSERT_EQUAL(&luos_phy->job[0], job);
            TEST_ASSERT_EQUAL(1, luos_phy->job_nb);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            luos_phy->job_nb              = 3;
            luos_phy->oldest_job_index    = 0;
            luos_phy->available_job_index = 3;
            luos_phy->job[0].data_pt      = (const uint8_t *)&msg_buffer[0];
            luos_phy->job[1].data_pt      = (const uint8_t *)&msg_buffer[1];
            luos_phy->job[2].data_pt      = (const uint8_t *)&msg_buffer[2];
            job                           = Phy_GetNextJob(luos_phy, job);
            TEST_ASSERT_EQUAL(&luos_phy->job[1], job);
            TEST_ASSERT_EQUAL(3, luos_phy->job_nb);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_phy_GetJobId()
{
    NEW_TEST_CASE("Check GetJobId assertion conditions");
    {
        TRY
        {
            int value = Phy_GetJobId(NULL, &luos_phy->job[0]);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            int value = Phy_GetJobId(luos_phy, NULL);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            int value = Phy_GetJobId(luos_phy, &luos_phy->job[MAX_MSG_NB]);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }

    NEW_TEST_CASE("Check GetJobId normal conditions");
    {
        TRY
        {
            for (int i = 0; i < MAX_MSG_NB; i++)
            {
                int value = Phy_GetJobId(luos_phy, &luos_phy->job[i]);
                TEST_ASSERT_EQUAL(i, value);
            }
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            luos_phy->job_nb         = 1;
            phy_job_t *resulting_job = Phy_GetJob(luos_phy);
            TEST_ASSERT_EQUAL(&luos_phy->job[0], resulting_job);
            TEST_ASSERT_EQUAL(1, luos_phy->job_nb);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            luos_phy->job_nb         = 3;
            phy_job_t *resulting_job = Phy_GetJob(luos_phy);
            TEST_ASSERT_EQUAL(&luos_phy->job[0], resulting_job);
            TEST_ASSERT_EQUAL(3, luos_phy->job_nb);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_phy_GetPhyId()
{
    NEW_TEST_CASE("Check GetPhyId assertion conditions");
    {
        TRY
        {
            int value = Phy_GetPhyId(NULL);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            int value = Phy_GetPhyId(&phy_ctx.phy[LOCAL_PHY_NB + 1]);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }

    NEW_TEST_CASE("Check GetPhyId normal conditions");
    {
        TRY
        {
            for (int i = 0; i <= LOCAL_PHY_NB; i++)
            {
                int value = Phy_GetPhyId(&phy_ctx.phy[i]);
                TEST_ASSERT_EQUAL(i, value);
            }
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_phy_RmJob()
{
    NEW_TEST_CASE("Check RmJob assertion conditions");
    {
        TRY
        {
            Phy_RmJob(NULL, &luos_phy->job[0]);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            Phy_RmJob(luos_phy, NULL);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            Phy_RmJob(luos_phy, &luos_phy->job[MAX_MSG_NB]);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;

        TRY
        {
            luos_phy->job_nb = 0;
            Phy_RmJob(luos_phy, &luos_phy->job[0]);
            TEST_ASSERT_EQUAL(0, luos_phy->job_nb);
        }
        TEST_ASSERT_TRUE(IS_ASSERT());
        END_TRY;
    }

    NEW_TEST_CASE("Check RmJob normal conditions");
    {

        phy_test_reset();
        memory_stats_t memory_stats;
        MsgAlloc_Init(&memory_stats);

        TRY
        {
            for (int i = 0; i < MAX_MSG_NB - 1; i++)
            {
                luos_phy->job_nb              = 1;
                luos_phy->oldest_job_index    = i;
                luos_phy->available_job_index = i + 1;
                luos_phy->job[i].data_pt      = (uint8_t *)&msg_buffer[i];
                Phy_RmJob(luos_phy, &luos_phy->job[i]);
                TEST_ASSERT_EQUAL(0, luos_phy->job_nb);
                TEST_ASSERT_EQUAL(NULL, luos_phy->job[i].data_pt);
            }
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

void unittest_add_and_remove_jobs(void)
{
    NEW_TEST_CASE("Try to add and remove jobs massively");
    {
        Phy_Init();
        for (int i = 0; i < MAX_MSG_NB - 1; i++)
        {
            for (int y = 0; y <= i; y++)
            {
                phy_job_t job;
                job.data_pt = (uint8_t *)&msg_buffer[0];
                Phy_AddJob(luos_phy, &job);
                TEST_ASSERT_EQUAL(y + 1, luos_phy->job_nb);
            }
            for (int y = 0; y <= i; y++)
            {
                phy_job_t *job_get = Phy_GetJob(luos_phy);
                Phy_RmJob(luos_phy, job_get);
                TEST_ASSERT_EQUAL(i - y, luos_phy->job_nb);
            }
            for (int y = 0; y <= i; y++)
            {
                phy_job_t job;
                job.data_pt = (uint8_t *)&msg_buffer[0];
                Phy_AddJob(luos_phy, &job);
                TEST_ASSERT_EQUAL(y + 1, luos_phy->job_nb);
            }
            for (int y = i; y >= 0; y--)
            {
                uint16_t get_id = luos_phy->oldest_job_index + y;
                if (get_id >= MAX_MSG_NB)
                {
                    get_id -= MAX_MSG_NB;
                }
                phy_job_t *job_get = &luos_phy->job[get_id];
                Phy_RmJob(luos_phy, job_get);
                TEST_ASSERT_EQUAL(y, luos_phy->job_nb);
            }
        }
    }
}

void unittest_phy_TxAllComplete()
{
    NEW_TEST_CASE("Check TxAllComplete normal conditions");
    {
        TRY
        {
            luos_phy->job_nb               = 1;
            luos_phy->oldest_job_index     = 0;
            luos_phy->available_job_index  = 1;
            robus_phy->job_nb              = 1;
            robus_phy->oldest_job_index    = 0;
            robus_phy->available_job_index = 1;
            phy_ctx.phy_nb                 = 2;
            error_return_t result          = Phy_TxAllComplete();
            TEST_ASSERT_EQUAL(FAILED, result);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            luos_phy->job_nb               = 0;
            luos_phy->oldest_job_index     = 0;
            luos_phy->available_job_index  = 0;
            robus_phy->job_nb              = 1;
            robus_phy->oldest_job_index    = 0;
            robus_phy->available_job_index = 1;
            error_return_t result          = Phy_TxAllComplete();
            TEST_ASSERT_EQUAL(FAILED, result);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;

        TRY
        {
            luos_phy->job_nb      = 1;
            robus_phy->job_nb     = 0;
            error_return_t result = Phy_TxAllComplete();
            TEST_ASSERT_EQUAL(SUCCEED, result);
        }
        CATCH
        {
            TEST_ASSERT_TRUE(false);
        }
        END_TRY;
    }
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    UNIT_TEST_RUN(unittest_phy_alloc);
    UNIT_TEST_RUN(unittest_phy_dispatch);
    UNIT_TEST_RUN(unittest_phy_deadTarget);
    UNIT_TEST_RUN(unittest_phy_loop);
    UNIT_TEST_RUN(unittest_phy_ComputeHeader);
    UNIT_TEST_RUN(unittest_phy_ValidMsg);
    UNIT_TEST_RUN(unittest_phy_ComputeTimestamp);
    UNIT_TEST_RUN(unittest_phy_GetNodeId);
    UNIT_TEST_RUN(unittest_phy_AddJob);
    UNIT_TEST_RUN(unittest_phy_GetJob);
    UNIT_TEST_RUN(unittest_phy_GetNextJob);
    UNIT_TEST_RUN(unittest_phy_GetJobId);
    UNIT_TEST_RUN(unittest_phy_GetPhyId);
    UNIT_TEST_RUN(unittest_phy_RmJob);
    UNIT_TEST_RUN(unittest_add_and_remove_jobs);
    UNIT_TEST_RUN(unittest_phy_TxAllComplete);

    UNITY_END();
}
