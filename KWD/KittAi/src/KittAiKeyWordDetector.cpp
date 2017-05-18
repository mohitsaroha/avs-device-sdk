/*
 * KittAiKeyWordDetector.cpp
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSUtils/Logging/Logger.h>
#include <AVSUtils/Memory/Memory.h>

#include "KittAi/KittAiKeyWordDetector.h"

#include <memory>
#include <sstream>

namespace alexaClientSDK {
namespace kwd {

using namespace avsCommon;
using namespace avsCommon::sdkInterfaces;

/// The number of hertz per kilohertz.
static const size_t HERTZ_PER_KILOHERTZ = 1000;

/// The timeout to use for read calls to the SharedDataStream.
const std::chrono::milliseconds TIMEOUT_FOR_READ_CALLS = std::chrono::milliseconds(1000);

/// The delimiter for Kitt.ai engine constructor parameters
static const std::string KITT_DELIMITER = ",";

/// The Kitt.ai compatible audio encoding of LPCM.
static const avsCommon::AudioFormat::Encoding KITT_AI_COMPATIBLE_ENCODING = avsCommon::AudioFormat::Encoding::LPCM;

/// The Kitt.ai compatible endianness which is little endian.
static const avsCommon::AudioFormat::Endianness KITT_AI_COMPATIBLE_ENDIANNESS = 
    avsCommon::AudioFormat::Endianness::LITTLE;

/// Kitt.ai returns -2 if silence is detected.
static const int KITT_AI_SILENCE_DETECTION_RESULT = -2;

/// Kitt.ai returns -1 if an error occurred.
static const int KITT_AI_ERROR_DETECTION_RESULT = -1;

/// Kitt.ai returns 0 if no keyword was detected but audio has been heard.
static const int KITT_AI_NO_DETECTION_RESULT = 0;

std::unique_ptr<KittAiKeyWordDetector> KittAiKeyWordDetector::create(
            std::shared_ptr<AudioInputStream> stream, 
            AudioFormat audioFormat, 
            std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers, 
            std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> 
                keyWordDetectorStateObservers,
            const std::string& resourceFilePath,
            const std::vector<KittAiConfiguration> kittAiConfigurations,
            float audioGain,
            bool applyFrontEnd,
            std::chrono::milliseconds msToPushPerIteration) {
    if (!stream) {
        avsUtils::Logger::log("Kitt.ai key word detector must be initialized with a valid stream");
        return nullptr;
    }
    // TODO: ACSDK-249 - Investigate cpu usage of converting bytes between endianness and if it's not too much, do it.
    if (isByteswappingRequired(audioFormat)) {
        avsUtils::Logger::log("Audio data endianness must match system endianness");
        return nullptr;
    }
    std::unique_ptr<KittAiKeyWordDetector> detector(
        new KittAiKeyWordDetector(
            stream,
            audioFormat, 
            keyWordObservers, 
            keyWordDetectorStateObservers,
            resourceFilePath,
            kittAiConfigurations,
            audioGain,
            applyFrontEnd,
            msToPushPerIteration
        )
    );
    if (!detector->init(audioFormat)) {
        avsUtils::Logger::log("Unable to initialize Kitt.ai detector");
        return nullptr;
    }
    return detector;
}

KittAiKeyWordDetector::~KittAiKeyWordDetector() { 
    m_isShuttingDown = true;
    if (m_detectionThread.joinable()) {
        m_detectionThread.join();
    }
}

KittAiKeyWordDetector::KittAiKeyWordDetector(
        std::shared_ptr<AudioInputStream> stream, 
        avsCommon::AudioFormat audioFormat,  
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers, 
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> 
            keyWordDetectorStateObservers,
        const std::string& resourceFilePath,
        const std::vector<KittAiConfiguration> kittAiConfigurations,
        float audioGain,
        bool applyFrontEnd,
        std::chrono::milliseconds msToPushPerIteration) : 
    AbstractKeywordDetector(keyWordObservers, keyWordDetectorStateObservers),
    m_stream{stream}, 
    m_maxSamplesPerPush{(audioFormat.sampleRateHz / HERTZ_PER_KILOHERTZ) * msToPushPerIteration.count()} {

    std::stringstream sensitivities;
    std::stringstream modelPaths;
    for (unsigned int i = 0; i < kittAiConfigurations.size(); ++i) {
        modelPaths << kittAiConfigurations.at(i).modelFilePath;
        sensitivities << kittAiConfigurations.at(i).sensitivity;
        m_detectionResultsToKeyWords[i+1] = kittAiConfigurations.at(i).keyword;
        if (kittAiConfigurations.size() - 1 != i) {
            modelPaths << KITT_DELIMITER;
            sensitivities << KITT_DELIMITER;
        }
    }
    m_kittAiEngine = avsUtils::memory::make_unique<snowboy::SnowboyDetect>(resourceFilePath, modelPaths.str());
    m_kittAiEngine->SetSensitivity(sensitivities.str());
    m_kittAiEngine->SetAudioGain(audioGain);
    m_kittAiEngine->ApplyFrontend(applyFrontEnd);
}

bool KittAiKeyWordDetector::init(avsCommon::AudioFormat audioFormat) {
    if (!isAudioFormatCompatibleWithKittAi(audioFormat)) {
        return false;
    }
    m_streamReader = m_stream->createReader(AudioInputStream::Reader::Policy::BLOCKING);
    if (!m_streamReader) {
        avsUtils::Logger::log("Unable to create Stream reader");
        return false;
    }
    m_isShuttingDown = false;
    m_detectionThread = std::thread(&KittAiKeyWordDetector::detectionLoop, this);
    return true;
}

bool KittAiKeyWordDetector::isAudioFormatCompatibleWithKittAi(avsCommon::AudioFormat audioFormat) {
    if (audioFormat.numChannels != m_kittAiEngine->NumChannels()) {
        avsUtils::Logger::log(
            "Audio data number of channels does not meet Kitt.ai requirements of " + 
            std::to_string(m_kittAiEngine->NumChannels()));
        return false;
    }
    if (audioFormat.sampleRateHz != m_kittAiEngine->SampleRate()) {
        avsUtils::Logger::log(
            "Audio data sample rate does not meet Kitt.ai requirements of " +
            std::to_string(m_kittAiEngine->SampleRate()));
        return false;
    }
    if (audioFormat.sampleSizeInBits != m_kittAiEngine->BitsPerSample()) {
        avsUtils::Logger::log(
            "Audio data bits per sample does not meet Kitt.ai requirements of " + 
            std::to_string(m_kittAiEngine->BitsPerSample()));
        return false;
    }
    if (audioFormat.endianness != KITT_AI_COMPATIBLE_ENDIANNESS) {
        avsUtils::Logger::log("Audio data fed to Kitt.ai must be little endian");
        return false;
    }
    if (audioFormat.encoding != KITT_AI_COMPATIBLE_ENCODING) {
        avsUtils::Logger::log("Audio data fed to Kitt.ai must be LPCM encoded");
        return false;
    }
    return true;
}

void KittAiKeyWordDetector::detectionLoop() {
    notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
    int16_t audioDataToPush[m_maxSamplesPerPush];
    ssize_t wordsRead;
    while (!m_isShuttingDown) {
        bool didErrorOccur;
        wordsRead = readFromStream(
                m_streamReader, 
                m_stream, 
                audioDataToPush, 
                m_maxSamplesPerPush, 
                TIMEOUT_FOR_READ_CALLS, 
                &didErrorOccur);
        if (didErrorOccur) {
            break;
        } else if (wordsRead > 0) {
            // Words were successfully read.
            notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
            int detectionResult = m_kittAiEngine->RunDetection(audioDataToPush, wordsRead);
            if (detectionResult > 0) {
                // > 0 indicates a keyword was found
                if (m_detectionResultsToKeyWords.find(detectionResult) == m_detectionResultsToKeyWords.end()) {
                    avsUtils::Logger::log("Unable to get keyword that was detected");
                    notifyKeyWordDetectorStateObservers(
                        KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ERROR);
                    break;
                } else {
                    notifyKeyWordObservers(
                        m_stream, 
                        m_detectionResultsToKeyWords[detectionResult], 
                        KeyWordObserverInterface::UNSPECIFIED_INDEX, 
                        m_streamReader->tell());
                }
            } else {
                switch (detectionResult) {
                    case KITT_AI_ERROR_DETECTION_RESULT:
                        avsUtils::Logger::log("Error occurred with KittAi Engine");
                        notifyKeyWordDetectorStateObservers(
                            KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ERROR);
                        didErrorOccur = true;
                        break;
                    case KITT_AI_SILENCE_DETECTION_RESULT:
                        break;
                    case KITT_AI_NO_DETECTION_RESULT:
                        break;
                    default:
                        avsUtils::Logger::log(
                            "Unexpected negative return from KittAi Engine: " + std::to_string(detectionResult));
                        notifyKeyWordDetectorStateObservers(
                            KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ERROR);
                        didErrorOccur = true;
                        break;                         
                }
                if (didErrorOccur) {
                    break;
                }
            }
        }
    }
    m_streamReader->close();
}

} // namespace kwd
} // namespace alexaClientSDK