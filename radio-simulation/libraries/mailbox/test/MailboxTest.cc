#include "gtest/gtest.h"

#include "Logger.h"
#include "Mailbox.h"

namespace {
struct MailboxTestLogger {
  MailboxTestLogger() { Logger::initialize("mailbox-test"); }
};
MailboxTestLogger g_mailboxTestLogger;
} // namespace

TEST(MailboxTest, DequeueReturnsNoRequestWhenShutdownWithPendingRequest) {
  // Arrange
  Mailbox mailbox;
  mailbox::MailboxRequest request;
  request.set_request_id("pending-request");
  mailbox.enqueue(request);

  // Act
  mailbox.shutdown();
  const auto result = mailbox.dequeue();

  // Assert
  EXPECT_FALSE(result.has_value());
}

TEST(MailboxTest, EnqueueIsIgnoredAfterShutdown) {
  // Arrange
  Mailbox mailbox;
  mailbox::MailboxRequest request;
  request.set_request_id("request-after-shutdown");
  mailbox.shutdown();

  // Act
  mailbox.enqueue(request);
  const auto result = mailbox.dequeue();

  // Assert
  EXPECT_FALSE(result.has_value());
}

TEST(MailboxTest, RepeatedShutdownLeavesMailboxShutdown) {
  // Arrange
  Mailbox mailbox;

  // Act
  mailbox.shutdown();
  mailbox.shutdown();
  const auto result = mailbox.dequeue();

  // Assert
  EXPECT_FALSE(result.has_value());
}