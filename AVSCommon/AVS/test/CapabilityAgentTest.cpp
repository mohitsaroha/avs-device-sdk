/*
 * CapabilityAgentTest.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <string>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AVSCommon/AVS/CapabilityAgent.h"
#include "AVSCommon/AttachmentManagerInterface.h"

using namespace testing;

namespace alexaClientSDK {
namespace avsCommon {
namespace test{

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;

/// Namespace for SpeechRecognizer.
static const std::string NAMESPACE_SPEECH_RECOGNIZER("SpeechRecognizer");

/// Name for directive to SpeechRecognizer.
static const std::string NAME_STOP_CAPTURE("StopCapture");

/// Name for SpeechRecognizer state.
static const std::string NAME_RECOGNIZE("Recognize");

/// Message Id key.
static const std::string MESSAGE_ID("messageId");

/// Message Id for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// Dialog request Id Key.
static const std::string DIALOG_REQUEST_ID("dialogRequestId");

/// DialogRequestId for testing.
static const std::string DIALOG_REQUEST_ID_TEST("DialogRequestId_Test");

/// Payload key
static const std::string PAYLOAD("payload");

/// A speech recognizer payload for testing
static const std::string PAYLOAD_TEST("payload_Test");

/// A payload for testing
static const std::string PAYLOAD_SPEECH_RECOGNIZER =
        "{"
            "\"profile\":\"CLOSE_TALK\","
            "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\""
        "}";

/// A context for testing
static const std::string CONTEXT_TEST =
        "{"
            "\"context\":["
                "{"
                    "\"header\":{"
                        "\"namespace\":\"SpeechSynthesizer\","
                        "\"name\":\"SpeechState\""
                    "},"
                    "\"payload\":{"
                        "\"playerActivity\":\"FINISHED\","
                        "\"offsetInMilliseconds\":0,"
                        "\"token\":\"\""
                    "}"
                "}"
            "]"
        "}";

/**
 * TestEvents for testing. Tuple contains test event string to compare the result string with, the dialogRequestID and
 * the context to be passed as arguments.
 */
static const std::tuple<std::string, std::string, std::string> testEventWithDialogReqIdAndContext = {
    std::make_tuple(
        /// Event with context and dialog request id.
        "{"
            "\"context\":["
                "{"
                    "\"header\":{"
                        "\"namespace\":\"SpeechSynthesizer\","
                        "\"name\":\"SpeechState\""
                    "},"
                    "\"payload\":{"
                        "\"playerActivity\":\"FINISHED\","
                        "\"offsetInMilliseconds\":0,"
                        "\"token\":\"\""
                    "}"
                "}"
            "],"
            "\"event\":{"
                "\"header\":{"
                        "\"namespace\":\"SpeechRecognizer\","
                        "\"name\":\"Recognize\","
                        "\"messageId\":\""+MESSAGE_ID_TEST+"\","
                        "\"dialogRequestId\":\""+DIALOG_REQUEST_ID_TEST+"\""
                "},"
                "\"payload\":{"
                    "\"profile\":\"CLOSE_TALK\","
                    "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\""
                "}"
            "}"
        "}", DIALOG_REQUEST_ID_TEST, CONTEXT_TEST)};

static const std::tuple<std::string, std::string, std::string> testEventWithDialogReqIdNoContext = {
    std::make_tuple(
        /// An event with no context.
        "{"
            "\"event\":{"
                "\"header\":{"
                        "\"namespace\":\"SpeechRecognizer\","
                        "\"name\":\"Recognize\","
                        "\"messageId\":\""+MESSAGE_ID_TEST+"\","
                        "\"dialogRequestId\":\""+DIALOG_REQUEST_ID_TEST+"\""
                "},"
                "\"payload\":{"
                    "\"profile\":\"CLOSE_TALK\","
                    "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\""
                "}"
            "}"
        "}",DIALOG_REQUEST_ID_TEST, "")};

static const std::tuple<std::string, std::string, std::string> testEventWithoutDialogReqIdOrContext = {
    std::make_tuple(
        /// An event with no dialog request id and no context for testing.
        "{"
            "\"event\":{"
                "\"header\":{"
                        "\"namespace\":\"SpeechRecognizer\","
                        "\"name\":\"Recognize\","
                        "\"messageId\":\""+MESSAGE_ID_TEST+"\""
                "},"
                "\"payload\":{"
                    "\"profile\":\"CLOSE_TALK\","
                    "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\""
                "}"
            "}"
        "}", "", "")};

static const std::tuple<std::string, std::string, std::string> testEventWithContextAndNoDialogReqId = {
    std::make_tuple(
        /// An event with no dialog request id for testing.
        "{"
            "\"context\":["
                "{"
                    "\"header\":{"
                        "\"namespace\":\"SpeechSynthesizer\","
                        "\"name\":\"SpeechState\""
                    "},"
                    "\"payload\":{"
                        "\"playerActivity\":\"FINISHED\","
                        "\"offsetInMilliseconds\":0,"
                        "\"token\":\"\""
                    "}"
                "}"
            "],"
            "\"event\":{"
                "\"header\":{"
                        "\"namespace\":\"SpeechRecognizer\","
                        "\"name\":\"Recognize\","
                        "\"messageId\":\""+MESSAGE_ID_TEST+"\""
                "},"
                "\"payload\":{"
                    "\"profile\":\"CLOSE_TALK\","
                    "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\""
                "}"
            "}"
        "}", "", CONTEXT_TEST)};

/// MockAttachmentManager implementation.
class MockAttachmentManager : public AttachmentManagerInterface {
public:
    std::future<std::shared_ptr<std::iostream>> createAttachmentReader(const std::string& attachmentId) override;
    void createAttachment(const std::string& attachmentId, std::shared_ptr<std::iostream> attachment) override;
    void releaseAttachment(const std::string& attachmentId) override;
};

std::future<std::shared_ptr<std::iostream>> MockAttachmentManager::createAttachmentReader
        (const std::string& attachmentId) {
    std::promise<std::shared_ptr<std::iostream>> attachment;
    return attachment.get_future();
}

void MockAttachmentManager::createAttachment(const std::string& attachmentId,
        std::shared_ptr<std::iostream> attachment) {
    // default no-op
}

void MockAttachmentManager::releaseAttachment(const std::string& attachmentId) {
    // default no-op
}

/// Mock @c DirectiveHandlerResultInterface implementation.
class MockResult : public DirectiveHandlerResultInterface {
public:
     void setCompleted() override;

     void setFailed(const std::string& description) override;
};

void MockResult::setCompleted() {
    // default no-op
}

void MockResult::setFailed(const std::string& description) {
    // default no-op
}

class MockCapabilityAgent: public CapabilityAgent {
public:
    /**
     * Creates an instance of the @c MockCapabilityAgent.
     *
     * @param nameSpace The namespace of the Capability Agent.
     * @return A shared pointer to an instance of the @c MockCapabilityAgent.
     */
    static std::shared_ptr<MockCapabilityAgent> create(const std::string nameSpace);

    MockCapabilityAgent(const std::string& nameSpace);

    ~MockCapabilityAgent() override;

    enum class FunctionCalled {
        NONE,

        HANDLE_DIRECTIVE_IMMEDIATELY,

        PREHANDLE_DIRECTIVE,

        HANDLE_DIRECTIVE,

        CANCEL_DIRECTIVE
    };

    void handleDirectiveImmediately(const DirectiveAndResultInterface& directiveAndResultInterface) override;

    void preHandleDirective(const DirectiveAndResultInterface& directiveAndResultInterface) override;

    void handleDirective(const DirectiveAndResultInterface& directiveAndResultInterface) override;

    void cancelDirective(const DirectiveAndResultInterface& directiveAndResultInterface) override;

    FunctionCalled waitForFunctionCalls(const std::chrono::milliseconds duration = std::chrono::milliseconds(400));

    const std::string callBuildJsonEventString(const std::string& eventName,
        const std::string& dialogRequestIdValue, const std::string& jsonPayloadValue, const std::string& jsonContext);

private:
    /// flag to indicate which function has been called.
    FunctionCalled m_functionCalled;

    /// Condition variable to wake the @c waitForFunctionCalls.
    std::condition_variable m_wakeTrigger;

    /// mutex to protect @c m_contextAvailable.
    std::mutex m_mutex;

};

std::shared_ptr<MockCapabilityAgent> MockCapabilityAgent::create(const std::string nameSpace) {
    return std::make_shared<MockCapabilityAgent>(nameSpace);
}

MockCapabilityAgent::MockCapabilityAgent(const std::string& nameSpace) :
        CapabilityAgent(nameSpace),
        m_functionCalled{FunctionCalled::NONE} {
}

MockCapabilityAgent::~MockCapabilityAgent() {

}

void MockCapabilityAgent::handleDirectiveImmediately(const DirectiveAndResultInterface& directiveAndResultInterface) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_functionCalled = FunctionCalled::HANDLE_DIRECTIVE_IMMEDIATELY;
    m_wakeTrigger.notify_one();
}

void MockCapabilityAgent::preHandleDirective(const DirectiveAndResultInterface& directiveAndResultInterface){
    std::lock_guard<std::mutex> lock(m_mutex);
    m_functionCalled = FunctionCalled::PREHANDLE_DIRECTIVE;
    m_wakeTrigger.notify_one();
}

void MockCapabilityAgent::handleDirective(const DirectiveAndResultInterface& directiveAndResultInterface) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_functionCalled = FunctionCalled::HANDLE_DIRECTIVE;
    m_wakeTrigger.notify_one();
}

void MockCapabilityAgent::cancelDirective(const DirectiveAndResultInterface& directiveAndResultInterface) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_functionCalled = FunctionCalled::CANCEL_DIRECTIVE;
    m_wakeTrigger.notify_one();
}

MockCapabilityAgent::FunctionCalled MockCapabilityAgent::waitForFunctionCalls
            (const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTrigger.wait_for(lock, duration, [this]() { return (m_functionCalled != FunctionCalled::NONE); }))
    {
        return FunctionCalled::NONE;
    }
    return m_functionCalled;
}

const std::string MockCapabilityAgent::callBuildJsonEventString(const std::string& eventName,
        const std::string& dialogRequestIdValue, const std::string& jsonPayloadValue, const std::string& jsonContext) {
    return CapabilityAgent::buildJsonEventString(eventName, dialogRequestIdValue, jsonPayloadValue, jsonContext);
}

class CapabilityAgentTest : public ::testing::Test {
public:
    /**
     * Find the pattern in the @c searchString and if found, return the substring from the beginning of the searchString
     * to the position of where pattern is found.
     *
     * @param pattern The string pattern to look for in the searchString.
     * @param searchString The search string in which to look for the string pattern.
     * @return The substring from the beginning of the searchString to the first occurrence of pattern appears. It there
     * is no pattern in the searchString return an empty string.
     */
    std::string findStringFromStart(std::string pattern, std::string searchString);

    /**
     * Find the pattern in the searchString and if found, return the substring from the position of where pattern is
     * found till the end of the string.
     *
     * @param pattern The string pattern to look for in the searchString.
     * @param searchString The search string in which to look for the string pattern.
     * @param pos Position in the search string from which to start the search for the pattern.
     * @return The substring of the searchstring starting from where the first occurrence of pattern appears. It there
     * is no pattern in the searchString return an empty string.
     */
    std::string findStringTillEnd(std::string pattern, std::string searchString, size_t pos = 0);

    /**
     * Tests the @c buildJsonEventString functionality. Calls @c buildJsonEventString and compares it to the test
     * string. Assert if the strings are not equal.     *
     *
     * @param testTuple A tuple of test string, dialogRequestId and context.
     * @param dialogRequestIdPresent Whether a dialogRequestId is expected or not. This helps decide which parts of the
     * string to compare.
     */
    void testBuildJsonEventString(std::tuple<std::string, std::string, std::string> testTuple,
            bool dialogRequestIdPresent);

    void SetUp() override;

    std::shared_ptr<MockCapabilityAgent> m_capabilityAgent;

    /// Mock object of AttachmentManagerInterface
    std::shared_ptr<AttachmentManagerInterface> m_mockAttachmentManager;
};

void CapabilityAgentTest::SetUp() {
    m_capabilityAgent = MockCapabilityAgent::create(NAMESPACE_SPEECH_RECOGNIZER);
    m_mockAttachmentManager = std::make_shared<MockAttachmentManager>();
}

std::string CapabilityAgentTest::findStringFromStart(std::string pattern, std::string searchString) {
    std::size_t foundPattern = searchString.find(pattern);
    if (foundPattern == std::string::npos) {
        return "";
    }
    return searchString.substr(0, foundPattern);
}

std::string CapabilityAgentTest::findStringTillEnd(std::string pattern, std::string searchString, size_t pos) {
    std::size_t foundPattern = searchString.find(pattern, pos);
    if (foundPattern == std::string::npos) {
        return "";
    }
    return searchString.substr(foundPattern);
}

void CapabilityAgentTest::testBuildJsonEventString(std::tuple<std::string, std::string, std::string> testTuple,
            bool dialogRequestIdPresent) {
    std::string testString = std::get<0>(testTuple);
    std::string jsonEventString = m_capabilityAgent->callBuildJsonEventString(NAME_RECOGNIZE,
            std::get<1>(testTuple), PAYLOAD_SPEECH_RECOGNIZER, std::get<2>(testTuple));

    ASSERT_EQ(findStringFromStart(MESSAGE_ID, testString), findStringFromStart(MESSAGE_ID, jsonEventString));

    if (dialogRequestIdPresent) {
        ASSERT_EQ(findStringTillEnd(DIALOG_REQUEST_ID, testString),
                  findStringTillEnd(DIALOG_REQUEST_ID, jsonEventString));
    } else {
        ASSERT_EQ(findStringTillEnd(PAYLOAD, testString, testString.find(MESSAGE_ID)),
                  findStringTillEnd(PAYLOAD, jsonEventString, jsonEventString.find(MESSAGE_ID)));
    }
}

/**
 * Call the @c handleDirectiveImmediately from the @c CapabilityAgent base class with a directive as the argument.
 * Expect the @c handleDirectiveImmediately with the argument of @c DirectiveAndResultInterface will be called.
 */
TEST_F(CapabilityAgentTest, testCallToHandleImmediately) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE,
            MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST,
            m_mockAttachmentManager);
    m_capabilityAgent->CapabilityAgent::handleDirectiveImmediately(directive);
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::HANDLE_DIRECTIVE_IMMEDIATELY,
            m_capabilityAgent->waitForFunctionCalls());
}

/**
 * Call the @c preHandleDirective from the @c CapabilityAgent base class with a directive as the argument.
 * Expect the @c preHandleDirective with the argument of @c DirectiveAndResultInterface will be called.
 */
TEST_F(CapabilityAgentTest, testCallToPrehandleDirective) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE,
            MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST,
            m_mockAttachmentManager);
    std::unique_ptr<MockResult> dirHandlerResult(new MockResult );
    m_capabilityAgent->CapabilityAgent::preHandleDirective(directive, std::move(dirHandlerResult));
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::PREHANDLE_DIRECTIVE, m_capabilityAgent->waitForFunctionCalls());
}

/**
 * Call the @c preHandleDirective from the @c CapabilityAgent base class with a directive. *
 * Call the @c handleDirective from the @c CapabilityAgent base class with a directive as the argument.
 * Expect the @c handleDirective with the argument of @c DirectiveAndResultInterface will be called.
 */
TEST_F(CapabilityAgentTest, testCallToHandleDirective) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE,
            MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST,
            m_mockAttachmentManager);
    std::unique_ptr<MockResult> dirHandlerResult(new MockResult );
    m_capabilityAgent->CapabilityAgent::preHandleDirective(directive, std::move(dirHandlerResult));
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::PREHANDLE_DIRECTIVE, m_capabilityAgent->waitForFunctionCalls());
    m_capabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::HANDLE_DIRECTIVE, m_capabilityAgent->waitForFunctionCalls());
}

/**
 * Call the @c handleDirective from the @c CapabilityAgent base class with a directive as the argument. No
 * @c preHandleDirective is called before handleDirective. Expect @c handleDirective to return @c false.
 */
TEST_F(CapabilityAgentTest, testCallToHandleDirectiveWithNoPrehandle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE,
            MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST,
            m_mockAttachmentManager);
    ASSERT_FALSE(m_capabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST));
}

/**
 * Call the @c cancelDirective from the @c CapabilityAgent base class with a directive as the argument.
 * Expect the @c cancelDirective with the argument of @c DirectiveAndResultInterface will be called.
 */
TEST_F(CapabilityAgentTest, testCallToCancelDirective) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE,
            MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST,
            m_mockAttachmentManager);
    std::unique_ptr<MockResult> dirHandlerResult(new MockResult );
    m_capabilityAgent->CapabilityAgent::preHandleDirective(directive, std::move(dirHandlerResult));
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::PREHANDLE_DIRECTIVE, m_capabilityAgent->waitForFunctionCalls());
    m_capabilityAgent->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::CANCEL_DIRECTIVE, m_capabilityAgent->waitForFunctionCalls());
}

/**
 * Call the @c cancelDirective from the @c CapabilityAgent base class with a directive as the argument. No
 * @c preHandleDirective is called before handleDirective. Expect the @c cancelDirective with the argument of
 * @c DirectiveAndResultInterface will not be called.
 */
TEST_F(CapabilityAgentTest, testCallToCancelDirectiveWithNoPrehandle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE,
            MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST,
            m_mockAttachmentManager);
    m_capabilityAgent->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::NONE, m_capabilityAgent->waitForFunctionCalls());
}

/**
 * Call the @c callBuildJsonEventString with dialogRequestID and context. Expect a json event string that matches the
 * corresponding @c testEvent. The messageId will not match since it is a random number. Verify the string before and
 * after the messageId.
 */
TEST_F(CapabilityAgentTest, testWithDialogIdAndContext) {
    testBuildJsonEventString(testEventWithDialogReqIdAndContext, true);
}

/**
 * Call the @c callBuildJsonEventString with dialogRequestId and without context. Expect a json event string that
 * matches the corresponding @c testEvent. The messageId will not match since it is a random number. Verify the string
 * before and after the messageId.
 */
TEST_F(CapabilityAgentTest, testWithDialogIdAndNoContext) {
    testBuildJsonEventString(testEventWithDialogReqIdNoContext, true);
}

/**
 * Call the @c callBuildJsonEventString  without context and without dialogRequestId. Expect a json event string
 * that matches the corresponding @c testEvent. The messageId will not match since it is a random number.
 * Verify the string before and after the messageId.
 */
TEST_F(CapabilityAgentTest, testWithoutDialogIdOrContext) {
    testBuildJsonEventString(testEventWithoutDialogReqIdOrContext, false);
}

/**
 * Call the @c callBuildJsonEventString multiple times with context and without dialogRequestId. Expect a json event
 * string that matches the corresponding @c testEvent. The messageId will not match since it is a random number.
 * Verify the string before and after the messageId.
 */
TEST_F(CapabilityAgentTest, testWithContextAndNoDialogId) {
    testBuildJsonEventString(testEventWithContextAndNoDialogReqId, false);
}

} // namespace test
} // namespace avsCommon
} // namespace alexaClientSDK
