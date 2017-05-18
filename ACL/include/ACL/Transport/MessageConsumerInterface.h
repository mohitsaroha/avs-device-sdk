/*
 * MessageConsumerInterface.h
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_CONSUMER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_CONSUMER_INTERFACE_H_

#include <memory>

#include <AVSCommon/AVS/Message.h>

namespace alexaClientSDK {
namespace acl {

/**
 * An interface which allows a derived class to consume a Message from AVS.
 */
class MessageConsumerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MessageConsumerInterface() = default;

    /**
     * Called when a message has been received from AVS.
     * @param message The message.
     */
    virtual void consumeMessage(std::shared_ptr<avsCommon::avs::Message> message) = 0;
};

} // acl
} // alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_CONSUMER_INTERFACE_H_
