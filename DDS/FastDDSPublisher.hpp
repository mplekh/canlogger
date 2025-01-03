#pragma once

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>

#include "LogEntryPubSubTypes.hpp"
#include "LogEntry.hpp"

struct PubListener : public eprosima::fastdds::dds::DataWriterListener {
    int matched{};

    void on_publication_matched(
            eprosima::fastdds::dds::DataWriter*,
            const eprosima::fastdds::dds::PublicationMatchedStatus& info) override
    {
        if (info.current_count_change == 1) {
            matched = info.current_count;
            std::cout << "Publisher matched." << std::endl;
        } else if (info.current_count_change == -1) {
            matched = info.current_count;
            std::cout << "Publisher unmatched." << std::endl;
        } else {
            std::cout << info.current_count_change
                      << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
        }
        std::cout << "matched count: " << matched << std::endl;
    }
};
