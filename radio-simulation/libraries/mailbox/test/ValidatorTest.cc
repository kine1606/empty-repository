#include "gtest/gtest.h"

#include "BaseValidator.h"

#include <string>

namespace {
mailbox::MailboxRequest makeValidRequest() {
  mailbox::MailboxRequest request;
  request.set_request_id("req-1");
  request.mutable_source()->set_node_id("du-0");
  request.mutable_source()->set_node_type(mailbox::DU);
  request.mutable_destination()->set_node_id("ru-0");
  request.mutable_destination()->set_node_type(mailbox::RU);
  request.set_message_type(mailbox::HELLO);
  request.set_payload("HELLO FROM DU");
  request.mutable_metadata()->set_timestamp("2026-09-23T10:00:00Z");
  request.mutable_metadata()->set_correlation_id("corr-1");
  request.mutable_metadata()->set_version("1.0");
  return request;
}
} // namespace

TEST(ValidatorTest, FactoryStyleRequestPasses) {
  BaseValidator validator;
  const auto request = makeValidRequest();
  EXPECT_TRUE(validator.validate(request).isSuccess());
}

TEST(ValidatorTest, MalformedTimestampRejectedAsInvalidFormat) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_metadata()->set_timestamp("not-a-time");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_FORMAT);
}

TEST(ValidatorTest, OutOfRangeTimestampRejected) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_metadata()->set_timestamp("2026-13-40T99:99:99Z");
  EXPECT_FALSE(validator.validate(request).isSuccess());
}

TEST(ValidatorTest, UnsupportedVersionRejectedAsInvalidMetadata) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_metadata()->set_version("2.0");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_METADATA);
}

TEST(ValidatorTest, UnknownMessageTypeRejected) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.set_message_type(mailbox::MSG_UNKNOWN);
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_MESSAGE_TYPE);
}

TEST(ValidatorTest, EmptySourceNodeIdRejectedAsMissingField) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_source()->set_node_id("");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::MISSING_REQUIRED_FIELD);
}

TEST(ValidatorTest, UnknownSourceNodeTypeRejected) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_source()->set_node_type(mailbox::NODE_UNKNOWN);
  EXPECT_FALSE(validator.validate(request).isSuccess());
}

TEST(ValidatorTest, EmptyDestinationNodeIdRejected) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_destination()->set_node_id("");
  EXPECT_FALSE(validator.validate(request).isSuccess());
}

TEST(ValidatorTest, UnknownDestinationNodeTypeRejected) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_destination()->set_node_type(mailbox::NODE_UNKNOWN);
  EXPECT_FALSE(validator.validate(request).isSuccess());
}

TEST(ValidatorTest, EmptyTimestampReportedAsMissingField) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_metadata()->set_timestamp("");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::MISSING_REQUIRED_FIELD);
}

TEST(ValidatorTest, ImpossibleCalendarDateRejectedAsInvalidFormat) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_metadata()->set_timestamp("2026-02-30T10:00:00Z");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_FORMAT);
}

TEST(ValidatorTest, LeapDayAcceptedInLeapYear) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_metadata()->set_timestamp("2024-02-29T10:00:00Z");
  EXPECT_TRUE(validator.validate(request).isSuccess());
}

TEST(ValidatorTest, RequestIdWithIllegalCharsRejectedAsInvalidFormat) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.set_request_id("req 1!");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_FORMAT);
}

TEST(ValidatorTest, CorrelationIdWithIllegalCharsRejectedAsInvalidMetadata) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_metadata()->set_correlation_id("corr 1!");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_METADATA);
}

TEST(ValidatorTest, MissingMetadataBlockRejectedAsMissingField) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.clear_metadata();
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::MISSING_REQUIRED_FIELD);
}

TEST(ValidatorTest, OutOfRangeSourceNodeTypeRejectedAsInvalidMetadata) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_source()->set_node_type(static_cast<mailbox::NodeType>(99));
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_METADATA);
}

TEST(ValidatorTest, OutOfRangeDestinationNodeTypeRejectedAsInvalidMetadata) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_destination()->set_node_type(
      static_cast<mailbox::NodeType>(99));
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_METADATA);
}

TEST(ValidatorTest, OutOfRangeMessageTypeRejectedAsInvalidMessageType) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.set_message_type(static_cast<mailbox::MessageType>(99));
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_MESSAGE_TYPE);
}

TEST(ValidatorTest, EmptyRequestIdRejectedAsMissingField) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.set_request_id("");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::MISSING_REQUIRED_FIELD);
}

TEST(ValidatorTest, MaxLengthRequestIdAccepted) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.set_request_id(std::string(64, 'a'));
  EXPECT_TRUE(validator.validate(request).isSuccess());
}

TEST(ValidatorTest, OverlongRequestIdRejectedAsInvalidFormat) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.set_request_id(std::string(65, 'a'));
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_FORMAT);
}

TEST(ValidatorTest, EmptyPayloadRejectedAsMissingField) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.set_payload("");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::MISSING_REQUIRED_FIELD);
}

TEST(ValidatorTest, MaxSizePayloadAccepted) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.set_payload(std::string(4096, 'x'));
  EXPECT_TRUE(validator.validate(request).isSuccess());
}

TEST(ValidatorTest, OversizePayloadRejectedAsInvalidFormat) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.set_payload(std::string(4097, 'x'));
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_FORMAT);
}

TEST(ValidatorTest, EmptyCorrelationIdRejectedAsMissingField) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_metadata()->set_correlation_id("");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::MISSING_REQUIRED_FIELD);
}

TEST(ValidatorTest, MaxLengthCorrelationIdAccepted) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_metadata()->set_correlation_id(std::string(64, 'c'));
  EXPECT_TRUE(validator.validate(request).isSuccess());
}

TEST(ValidatorTest, OverlongCorrelationIdRejectedAsInvalidMetadata) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_metadata()->set_correlation_id(std::string(65, 'c'));
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_METADATA);
}

TEST(ValidatorTest, EmptyVersionRejectedAsMissingField) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_metadata()->set_version("");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::MISSING_REQUIRED_FIELD);
}

TEST(ValidatorTest, SourceNodeIdWithNewlineRejectedAsInvalidMetadata) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_source()->set_node_id("evil\n[RU] Forged");
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_METADATA);
}

TEST(ValidatorTest, MaxLengthSourceNodeIdAccepted) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_source()->set_node_id(std::string(64, 'n'));
  EXPECT_TRUE(validator.validate(request).isSuccess());
}

TEST(ValidatorTest, OverlongDestinationNodeIdRejectedAsInvalidMetadata) {
  BaseValidator validator;
  auto request = makeValidRequest();
  request.mutable_destination()->set_node_id(std::string(65, 'n'));
  const auto result = validator.validate(request);
  EXPECT_FALSE(result.isSuccess());
  EXPECT_EQ(result.getErrorCode(), mailbox::INVALID_METADATA);
}
