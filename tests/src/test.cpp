#include <gtest/gtest.h>
#include <xlog/xlog.h>

TEST(Logger, System)
{
    xlog::Logger log(u8"name");
    ASSERT_EQ(log.GetSystem()->GetLoggers().Size(), 1);
}