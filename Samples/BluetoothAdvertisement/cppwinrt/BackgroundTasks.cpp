//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "pch.h"
#include "BackgroundTasks.h"
#include "AdvertisementWatcherTask.g.cpp"
#include "AdvertisementPublisherTask.g.cpp"
#include "SampleConfiguration.h"
#include <chrono>
#include <sstream>

namespace winrt
{
    using namespace winrt::Windows::ApplicationModel::Background;
    using namespace winrt::Windows::Devices::Bluetooth;
    using namespace winrt::Windows::Devices::Bluetooth::Background;
    using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::Globalization::DateTimeFormatting;
    using namespace winrt::Windows::Storage;
    using namespace winrt::Windows::Storage::Streams;
}

namespace winrt::SDKTemplate::implementation
{
    std::wstring FormatOptionalInt16(IReference<int16_t> const& value)
    {
        if (value)
        {
            return std::to_wstring(value.Value());
        }
        else
        {
            return L"none";
        }
    }

    std::wstring FormatOptionalTimeSpanMs(IReference<TimeSpan> const& span)
    {
        if (span)
        {
            return std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(span.Value()).count());
        }
        else
        {
            return L"none";
        }
    }

    /// <summary>
    /// The entry point of a background task.
    /// </summary>
    /// <param name="taskInstance">The current background task instance.</param>
    void AdvertisementWatcherTask::Run(IBackgroundTaskInstance const& taskInstance)
    {
        watcherTaskInstance = taskInstance;

        auto details = taskInstance.TriggerDetails().as<BluetoothLEAdvertisementWatcherTriggerDetails>();

        // In this example, the background task simply constructs a message communicated
        // to the App. For more interesting applications, a notification can be sent from here instead.
        std::wostringstream ss;

        // If the background watcher stopped unexpectedly, an error will be available here.
        BluetoothError error = details.Error();
        if (error != BluetoothError::Success)
        {
            ss << L"Error: " << to_hstring(error) << L", ";
        }

        // The Advertisements property is a list of all advertisement events received
        // since the last task triggered. The list of advertisements here might be valid even if
        // the Error status is not Success since advertisements are stored until this task is triggered
        IVectorView<BluetoothLEAdvertisementReceivedEventArgs> advertisements = details.Advertisements();
        ss << L"EventCount: " << advertisements.Size() << L", ";

        // The signal strength filter configuration of the trigger is returned such that further
        // processing can be performed here using these values if necessary. They are read-only here.
        auto rssiFilter = details.SignalStrengthFilter();
        ss << L"HighDBm: " << FormatOptionalInt16(rssiFilter.InRangeThresholdInDBm()) << L", ";
        ss << L"LowDBm: " << FormatOptionalInt16(rssiFilter.OutOfRangeThresholdInDBm()) << L", ";
        ss << L"Timeout (ms): " << FormatOptionalTimeSpanMs(rssiFilter.OutOfRangeTimeout()) << L", ";
        ss << L"Sampling (ms): " << FormatOptionalTimeSpanMs(rssiFilter.SamplingInterval());

        DateTimeFormatter timestampFormatter(L"longtime");

        // Advertisements can contain multiple events that were aggregated, each represented by
        // a BluetoothLEAdvertisementReceivedEventArgs object.
        for (auto const& advertisementEventArgs : advertisements)
        {
            ss << L"\n[" + timestampFormatter.Format(advertisementEventArgs.Timestamp()) + L"] ";
            ss << L"[" + to_hstring(advertisementEventArgs.AdvertisementType()) + L"]: ";
            ss << L"Rssi=" << advertisementEventArgs.RawSignalStrengthInDBm() << L"dBm";
            if (hstring name = advertisementEventArgs.Advertisement().LocalName(); !name.empty())
            {
                ss << L", localName=" << name;
            }
            // Check if there are any manufacturer-specific sections.
            // If there is, print the raw data of the first manufacturer section (if there are multiple).
            auto manufacturerData = advertisementEventArgs.Advertisement().ManufacturerData();
            if (manufacturerData.Size() > 0)
            {
                // Print the first one of the list
                ss << L", manufacturerData=[" << Utilities::FormatManufacturerData(manufacturerData.GetAt(0)) << L"]";
            }
        }

        // Store the message in a local settings indexed by this task's name so that the foreground App
        // can display this message.
        ApplicationData::Current().LocalSettings().Values().Insert(taskInstance.Task().Name(), winrt::box_value(ss.str()));
    }

    /// <summary>
    /// The entry point of a background task.
    /// </summary>
    /// <param name="taskInstance">The current background task instance.</param>
    void AdvertisementPublisherTask::Run(IBackgroundTaskInstance const& taskInstance)
    {
        publisherTaskInstance = taskInstance;

        auto details = taskInstance.TriggerDetails().as<BluetoothLEAdvertisementPublisherTriggerDetails>();

        // In this example, the background task simply constructs a message communicated
        // to the App. For more interesting applications, a notification can be sent from here instead.
        std::wostringstream ss;

        // If the background publisher stopped unexpectedly, an error will be available here.
        BluetoothError error = details.Error();
        if (error != BluetoothError::Success)
        {
            ss << L"Error: " << to_hstring(error) << L", ";
        }

        // The status of the publisher is useful to determine whether the advertisement payload is being serviced
        // It is possible for a publisher to stay in a Waiting state while radio resources are in use.
        BluetoothLEAdvertisementPublisherStatus status = details.Status();
        ss << L"Publisher status: " + to_hstring(status);

        // Store the message in a local settings indexed by this task's name so that the foreground App
        // can display this message.
        ApplicationData::Current().LocalSettings().Values().Insert(taskInstance.Task().Name(), winrt::box_value(ss.str()));
    }
}

