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
using System.Runtime.InteropServices;
using Windows.ApplicationModel.Activation;
using Windows.ApplicationModel.Background;
using Windows.Devices.Bluetooth.Advertisement;
using Windows.Devices.Bluetooth.GenericAttributeProfile;
using Windows.Foundation.Metadata;
using Windows.Security.Cryptography;
using Windows.Storage.Streams;
using Windows.UI.Xaml.Controls;

namespace SDKTemplate
{
    public partial class MainPage : Page
    {
        public const string FEATURE_NAME = "Bluetooth Low Energy Advertisement";

        List<Scenario> scenarios = new List<Scenario>
        {
            new Scenario()
            {
                Title = "Foreground watcher",
                ClassType = typeof(Scenario1_Watcher)
            },
            new Scenario()
            {
                Title = "Foreground publisher",
                ClassType = typeof(Scenario2_Publisher)
            },
            new Scenario()
            {
                Title = "Background watcher",
                ClassType = typeof(Scenario3_BackgroundWatcher)
            },
            new Scenario()
            {
                Title = "Background publisher",
                ClassType = typeof(Scenario4_BackgroundPublisher)
            },
        };
    }

    public class Scenario
    {
        public string Title { get; set; }
        public Type ClassType { get; set; }
    }

    sealed partial class App : Windows.UI.Xaml.Application
    {
        /// <summary>
        /// Override the Application.OnBackgroundActivated method to handle background activation in
        /// the main process. This entry point is used when BackgroundTaskBuilder.TaskEntryPoint is
        /// not set during background task registration.
        /// </summary>
        /// <param name="args">Object that describes the background task being activated.</param>
        protected override void OnBackgroundActivated(BackgroundActivatedEventArgs args)
        {
            // Use the args.TaskInstance.Task.Name and/or args.TaskInstance.InstanceId to determine
            // which background task to run. In our case, the Name is sufficient.
            IBackgroundTaskInstance taskInstance = args.TaskInstance;
            string name = taskInstance.Task.Name;

            if (name == nameof(AdvertisementWatcherTask))
            {
                new AdvertisementWatcherTask().Run(taskInstance);
            }
            else if (name == nameof(AdvertisementPublisherTask))
            {
                new AdvertisementPublisherTask().Run(taskInstance);
            }
        }
    }

    static class FeatureDetection
    {
        // Reports whether the extended advertising and scanning features are supported:
        //
        // BluetoothAdapter.IsLowEnergyUncoded2MPhySupported property
        // BluetoothAdapter.IsLowEnergyCodedPhySupported property
        // BluetoothLEAdvertisementReceivedEventArgs.PrimaryPhy property
        // BluetoothLEAdvertisementReceivedEventArgs.SecondaryPhy property
        // BluetoothLEAdvertisementPublisher.PrimaryPhy property
        // BluetoothLEAdvertisementPublisher.SecondaryPhy property
        // BluetoothLEAdvertisementPublisherTrigger.PrimaryPhy property
        // BluetoothLEAdvertisementPublisherTrigger.SecondaryPhy property
        // BluetoothLEAdvertisementScanParameters class
        // BluetoothLEAdvertisementWatcher.UseUncoded1MPhy property
        // BluetoothLEAdvertisementWatcher.UseCodedPhy property
        // BluetoothLEAdvertisementWatcher.ScanParameters property
        // BluetoothLEAdvertisementWatcher.UseHardwareFilter property
        // BluetoothLEAdvertisementWatcherTrigger.UseUncoded1MPhy property
        // BluetoothLEAdvertisementWatcherTrigger.UseCodedPhy property
        // BluetoothLEAdvertisementWatcherTrigger.ScanParameters property
        // GattServiceProvider.UpdateAdvertisingParameters method
        // GattServiceProviderConnection.UpdateAdvertisingParameters method
        // GattServiceProviderAdvertisingParameters.UseLowEnergyUncoded1MPhyAsSecondaryPhy property
        // GattServiceProviderAdvertisingParameters.UseLowEnergyUncoded2MPhyAsSecondaryPhy property
        //
        // All of these features are available as a group, so testing one of them is sufficient to
        // check for the presence of all.
        public static bool AreExtendedAdvertisingPhysAndScanParametersSupported => LazyAreExtendedAdvertisingPhysAndScanParametersSupported.Value;

        private static Lazy<bool> LazyAreExtendedAdvertisingPhysAndScanParametersSupported = new Lazy<bool>(DetectExtendedAdvertisingPhysAndScanParameters);

        // Declare a dummy version of IGattServiceProviderAdvertisingParameters3 because all we need is the UUID.
        [ComImport]
        [System.Runtime.InteropServices.Guid("A23546B2-B216-5929-9055-F1313DD53E2A")]
        [InterfaceType(ComInterfaceType.InterfaceIsIInspectable)]
        private interface IGattServiceProviderAdvertisingParameters3
        {
        };
        private static bool DetectExtendedAdvertisingPhysAndScanParameters()
        {
            // We will use GattServiceProviderAdvertisingParameters.UseLowEnergyUncoded1MPhyAsSecondaryPhy
            // to detect this feature group.
            bool isPresentInMetadata = ApiInformation.IsPropertyPresent(
                typeof(GattServiceProviderAdvertisingParameters).FullName,
                nameof(GattServiceProviderAdvertisingParameters.UseLowEnergyUncoded1MPhyAsSecondaryPhy));
            if (!isPresentInMetadata)
            {
                return false;
            }

            // The feature is present in metadata. See if it is available at runtime.
            // During the initial rollout of the feature, it may be unavailable at runtime
            // despite being declared in metadata.
            return (object)(new GattServiceProviderAdvertisingParameters()) is IGattServiceProviderAdvertisingParameters3;
        }
    }

    static class Utilities
    {
        public static int? Int32FromBuffer(IBuffer buffer)
        {
            CryptographicBuffer.CopyToByteArray(buffer, out byte[] data);
            if (data.Length != sizeof(int))
            {
                return null;
            }

            return BitConverter.ToInt32(data, 0);
        }

        public static IBuffer BufferFromUInt16(UInt16 value)
        {
            return CryptographicBuffer.CreateFromByteArray(BitConverter.GetBytes(value));
        }

        public static IBuffer BufferFromInt32(int value)
        {
            return CryptographicBuffer.CreateFromByteArray(BitConverter.GetBytes(value));
        }

        public static string FormatManufacturerData(BluetoothLEManufacturerData manufacturerData)
        {
            // 0x####: zzzzzz
            // where #### is the company ID as a four-digit hex value, and
            // zzzzzz is the manufacturer data as a hex-encoded byte string
            return $"0x{manufacturerData.CompanyId:X4}: {CryptographicBuffer.EncodeToHexString(manufacturerData.Data)}";
        }
    }
}
