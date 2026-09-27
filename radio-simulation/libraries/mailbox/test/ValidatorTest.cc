#include "gtest/gtest.h"

#include "BaseValidator.h"
#include "ErrorMessages.h"
#include "mailbox.pb.h"

namespace {
mailbox::MailboxRequest makeValidRequest() {
    mailbox::MailboxRequest request;
    request.set_request_id("req-1");
    request.set_source("NodeA");
    request.set_destination("NodeB");

    mailbox::NodeBRequest nodeBReq;
    nodeBReq.set_command("TEST_COMMAND");
    nodeBReq.set_transaction_id(1);
    nodeBReq.set_data("test data");
    request.mutable_payload()->PackFrom(nodeBReq);

    return request;
}
} // namespace

TEST(ValidatorTest, ValidRequestPasses) {
    BaseValidator validator;
    const auto request = makeValidRequest();
    EXPECT_TRUE(validator.validate(request).isSuccess());
}

TEST(ValidatorTest, EmptyRequestIdRejected) {
    BaseValidator validator;
    auto request = makeValidRequest();
    request.set_request_id("");
    const auto result = validator.validate(request);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.getErrorCode(), mailbox::MISSING_REQUIRED_FIELD);
    EXPECT_EQ(result.getErrorMessage(), ErrorMessages::REQUEST_ID_EMPTY);
}

TEST(ValidatorTest, EmptySourceRejected) {
    BaseValidator validator;
    auto request = makeValidRequest();
    request.set_source("");
    const auto result = validator.validate(request);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.getErrorCode(), mailbox::MISSING_REQUIRED_FIELD);
    EXPECT_EQ(result.getErrorMessage(), ErrorMessages::SENDER_EMPTY);
}

TEST(ValidatorTest, EmptyDestinationRejected) {
    BaseValidator validator;
    auto request = makeValidRequest();
    request.set_destination("");
    const auto result = validator.validate(request);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.getErrorCode(), mailbox::MISSING_REQUIRED_FIELD);
    EXPECT_EQ(result.getErrorMessage(), ErrorMessages::UNSUPPORTED_DESTINATION);
}

TEST(ValidatorTest, MissingPayloadRejected) {
    BaseValidator validator;
    auto request = makeValidRequest();
    request.clear_payload();
    const auto result = validator.validate(request);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_PAYLOAD);
    EXPECT_EQ(result.getErrorMessage(), ErrorMessages::PAYLOAD_EMPTY);
}
