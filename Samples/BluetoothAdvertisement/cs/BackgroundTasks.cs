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

using System;
using System.Collections.Generic;
using System.Text;
using Windows.ApplicationModel.Background;
using Windows.Devices.Bluetooth;
using Windows.Devices.Bluetooth.Advertisement;
using Windows.Devices.Bluetooth.Background;
using Windows.Storage;
using Windows.Storage.Streams;

namespace SDKTemplate
{
    // An out-of-process background task always implements the IBackgroundTask interface.
    // An in-process background task does not need to implement the IBackgroundTask interface,
    // but we do it here to make it easier to convert the task between in-process and out-of-process.
    public sealed class AdvertisementWatcherTask : IBackgroundTask
    {
        private IBackgroundTaskInstance backgroundTaskInstance;

        /// <summary>
        /// The entry point of a background task.
        /// </summary>
        /// <param name="taskInstance">The current background task instance.</param>
        public void Run(IBackgroundTaskInstance taskInstance)
        {
            backgroundTaskInstance = taskInstance;

            var details = (BluetoothLEAdvertisementWatcherTriggerDetails)taskInstance.TriggerDetails;

            // In this example, the background task simply constructs a message communicated
            // to the App. For more interesting applications, a notification can be sent from here instead.
            StringBuilder builder = new StringBuilder();

            // If the background watcher stopped unexpectedly, an error will be available here.
            var error = details.Error;
            if (error != BluetoothError.Success)
            {
                builder.Append($"Error: {error}, ");
            }

            // The Advertisements property is a list of all advertisement events received
            // since the last task triggered. The list of advertisements here might be valid even if
            // the Error status is not Success since advertisements are stored until this task is triggered
            IReadOnlyList<BluetoothLEAdvertisementReceivedEventArgs> advertisements = details.Advertisements;
            builder.Append($"EventCount: {advertisements.Count}, ");

            // The signal strength filter configuration of the trigger is returned such that further
            // processing can be performed here using these values if necessary. They are read-only here.
            var rssiFilter = details.SignalStrengthFilter;

            builder.Append($"HighDBm: {FormatOptionalInt16(rssiFilter.InRangeThresholdInDBm)}, ");
            builder.Append($"LowDBm: {FormatOptionalInt16(rssiFilter.OutOfRangeThresholdInDBm)}, ");
            builder.Append($"Timeout (ms): {FormatOptionalTimeSpanMs(rssiFilter.OutOfRangeTimeout)}, ");
            builder.Append($"Sampling (ms): {FormatOptionalTimeSpanMs(rssiFilter.SamplingInterval)}");

            // Advertisements can contain multiple events that were aggregated, each represented by
            // a BluetoothLEAdvertisementReceivedEventArgs object.
            foreach (var advertisementEventArgs in advertisements)
            {
                builder.Append($"\n[{advertisementEventArgs.Timestamp:T}] [{advertisementEventArgs.AdvertisementType}]: Rssi={advertisementEventArgs.RawSignalStrengthInDBm} dBm");
                if (!string.IsNullOrEmpty(advertisementEventArgs.Advertisement.LocalName))
                {
                    builder.Append($", localName={advertisementEventArgs.Advertisement.LocalName}");
                }

                // Check if there are any manufacturer-specific sections.
                // If there is, print the raw data of the first manufacturer section (if there are multiple).
                var manufacturerData = advertisementEventArgs.Advertisement.ManufacturerData;
                if (manufacturerData.Count > 0)
                {
                    // Print the first one of the list
                    builder.Append($", manufacturerData=[{Utilities.FormatManufacturerData(manufacturerData[0])}]");
                }
            }

            // Store the message in a local settings indexed by this task's name so that the foreground App
            // can display this message.
            ApplicationData.Current.LocalSettings.Values[taskInstance.Task.Name] = builder.ToString();
        }

        private static string FormatOptionalInt16(short? number)
        {
            return number.HasValue ? number.Value.ToString() : "none";
        }

        private static string FormatOptionalTimeSpanMs(TimeSpan? span)
        {
            return span.HasValue ? span.Value.TotalMilliseconds.ToString() : "none";
        }

    }

    // An out-of-process background task always implements the IBackgroundTask interface.
    // An in-process background task does not need to implement the IBackgroundTask interface,
    // but we do it here to make it easier to convert the task between in-process and out-of-process.
    public sealed class AdvertisementPublisherTask : IBackgroundTask
    {
        private IBackgroundTaskInstance backgroundTaskInstance;

        /// <summary>
        /// The entry point of a background task.
        /// </summary>
        /// <param name="taskInstance">The current background task instance.</param>
        public void Run(IBackgroundTaskInstance taskInstance)
        {
            backgroundTaskInstance = taskInstance;

            var details = (BluetoothLEAdvertisementPublisherTriggerDetails)taskInstance.TriggerDetails;

            // In this example, the background task simply constructs a message communicated
            // to the App. For more interesting applications, a notification can be sent from here instead.
            StringBuilder builder = new StringBuilder();

            // If the background publisher stopped unexpectedly, an error will be available here.
            BluetoothError error = details.Error;
            if (error != BluetoothError.Success)
            {
                builder.Append($"Error: {error}, ");
            }

            // The status of the publisher is useful to determine whether the advertisement payload is being serviced
            // It is possible for a publisher to stay in a Waiting state while radio resources are in use.
            BluetoothLEAdvertisementPublisherStatus status = details.Status;
            builder.Append($"Publisher status: {status}");

            // Store the message in a local settings indexed by this task's name so that the foreground App
            // can display this message.
            ApplicationData.Current.LocalSettings.Values[taskInstance.Task.Name] = builder.ToString();
        }
    }
}
