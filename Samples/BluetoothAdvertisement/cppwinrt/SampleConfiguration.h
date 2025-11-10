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

#pragma once
namespace winrt::SDKTemplate
{
    // This function is called by App::OnBackgroundActivated in the samples template
    void App_OnBackgroundActivated(Windows::ApplicationModel::Activation::BackgroundActivatedEventArgs const&);

    struct FeatureDetection
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
        // All of these features are available as a group.
        //
        static bool AreExtendedAdvertisingPhysAndScanParametersSupported();
    };

    struct Utilities
    {
        template<typename T>
        static std::optional<T> FromBuffer(Windows::Storage::Streams::IBuffer const& buffer)
        {
            static_assert(std::is_trivially_copyable_v <T>, "FromBuffer can be used only with memcpy'able types");
            uint32_t length = buffer.Length();
            if (length == sizeof(T))
            {
                T result;
                memcpy_s(&result, sizeof(result), buffer.data(), length);
                return result;
            }
            return std::nullopt;
        }

        template<typename T>
        static Windows::Storage::Streams::IBuffer ToBuffer(T const& value)
        {
            static_assert(std::is_trivially_copyable_v <T>, "ToBuffer can be used only with memcpy'able types");
            Windows::Storage::Streams::Buffer buffer(sizeof(value));
            memcpy_s(buffer.data(), sizeof(value), &value, sizeof(value));
            buffer.Length(sizeof(value));
            return buffer;
        }

        static hstring FormatManufacturerData(Windows::Devices::Bluetooth::Advertisement::BluetoothLEManufacturerData const& manufacturerData);
    };
}

namespace winrt
{
    hstring to_hstring(Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementType advertisementType);
    hstring to_hstring(Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementPublisherStatus status);
    hstring to_hstring(Windows::Devices::Bluetooth::BluetoothError error);
}
