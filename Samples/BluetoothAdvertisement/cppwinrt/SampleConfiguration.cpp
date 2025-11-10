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
#include "MainPage.h"
#include "SampleConfiguration.h"
#include <sstream>
#include <iomanip>

namespace winrt
{
    using namespace winrt::SDKTemplate;
    using namespace winrt::Windows::ApplicationModel::Background;
    using namespace winrt::Windows::Devices::Bluetooth;
    using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
    using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::Foundation::Metadata;
    using namespace winrt::Windows::Security::Cryptography;
}

namespace winrt::SDKTemplate
{
    hstring implementation::MainPage::FEATURE_NAME()
    {
        return L"BluetoothAdvertisement C++/WinRT Sample";
    }

    IVector<Scenario> implementation::MainPage::scenariosInner = winrt::single_threaded_observable_vector<Scenario>(
    {
        Scenario{ L"Foreground Watcher", xaml_typename<SDKTemplate::Scenario1_Watcher>() },
        Scenario{ L"Foreground Publisher", xaml_typename<SDKTemplate::Scenario2_Publisher>() },
        Scenario{ L"Background Watcher", xaml_typename<SDKTemplate::Scenario3_BackgroundWatcher>() },
        Scenario{ L"Background Publisher", xaml_typename<SDKTemplate::Scenario4_BackgroundPublisher>() },
    });

    /// <summary>
    /// Called from App::OnBackgroundActivated to handle background activation in
    /// the main process. This entry point is used when BackgroundTaskBuilder.TaskEntryPoint is
    /// not set during background task registration.
    /// </summary>
    /// <param name="args">Object that describes the background task being activated.</param>
    void App_OnBackgroundActivated(Windows::ApplicationModel::Activation::BackgroundActivatedEventArgs const& args)
    {
        // Use the args.TaskInstance().Task().Name() and/or args.()TaskInstance.InstanceId() to determine
        // which background task to run. In our case, the Name is sufficient.
        IBackgroundTaskInstance taskInstance = args.TaskInstance();
        hstring name = taskInstance.Task().Name();

        if (name == winrt::name_of<AdvertisementWatcherTask>())
        {
            AdvertisementWatcherTask().Run(taskInstance);
        }
        else if (name == winrt::name_of<AdvertisementPublisherTask>())
        {
            AdvertisementPublisherTask().Run(taskInstance);
        }
    }

    bool FeatureDetection::AreExtendedAdvertisingPhysAndScanParametersSupported()
    {
        static bool supported = []
        {
            // We will use GattServiceProviderAdvertisingParameters.UseLowEnergyUncoded1MPhyAsSecondaryPhy
            // to detect this feature group.
            bool isPresentInMetadata = ApiInformation::IsPropertyPresent(
                winrt::name_of<GattServiceProviderAdvertisingParameters>(),
                L"UseLowEnergyUncoded1MPhyAsSecondaryPhy");
            if (!isPresentInMetadata)
            {
                return false;
            }

            // The feature is present in metadata. See if it is available at runtime.
            // During the initial rollout of the feature, it may be unavailable at runtime
            // despite being declared in metadata.
            return GattServiceProviderAdvertisingParameters().try_as<IGattServiceProviderAdvertisingParameters3>() != nullptr;
        }();

        return supported;
    }

    hstring Utilities::FormatManufacturerData(BluetoothLEManufacturerData const& manufacturerData)
    {
        // 0x####: zzzzzz
        // where #### is the company ID as a four-digit hex value, and
        // zzzzzz is the manufacturer data as a hex-encoded byte string
        std::wostringstream ss;
        ss << L"0x" << std::hex << std::setw(4) << std::setfill(L'0') << manufacturerData.CompanyId() << L": ";
        ss << CryptographicBuffer::EncodeToHexString(manufacturerData.Data());
        return hstring(ss.str());
    }
}

namespace
{
    template<typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
    winrt::hstring unknown_code_to_hstring(T value)
    {
        return L"Unknown (" + winrt::to_hstring(static_cast<uint32_t>(value)) + L")";
    }
}
namespace winrt
{
    hstring to_hstring(BluetoothLEAdvertisementType advertisementType)
    {
        switch (advertisementType)
        {
        case BluetoothLEAdvertisementType::ConnectableUndirected:
            return L"Connectable Undirected";
        case BluetoothLEAdvertisementType::ConnectableDirected:
            return L"Connectable Directed";
        case BluetoothLEAdvertisementType::ScannableUndirected:
            return L"Scannable Undirected";
        case BluetoothLEAdvertisementType::NonConnectableUndirected:
            return L"Nonconnectable Undirected";
        case BluetoothLEAdvertisementType::ScanResponse:
            return L"Scan Response";
        default: return unknown_code_to_hstring(advertisementType);
        }
    };

    hstring to_hstring(BluetoothLEAdvertisementPublisherStatus status)
    {
        switch (status)
        {
        case BluetoothLEAdvertisementPublisherStatus::Created: return L"Created";
        case BluetoothLEAdvertisementPublisherStatus::Waiting: return L"Waiting";
        case BluetoothLEAdvertisementPublisherStatus::Started: return L"Started";
        case BluetoothLEAdvertisementPublisherStatus::Stopping: return L"Stopping";
        case BluetoothLEAdvertisementPublisherStatus::Stopped: return L"Stopped";
        case BluetoothLEAdvertisementPublisherStatus::Aborted: return L"Aborted";
        default: return unknown_code_to_hstring(status);
        }
    };

    hstring to_hstring(BluetoothError error)
    {
        switch (error)
        {
        case BluetoothError::Success: return L"Success";
        case BluetoothError::RadioNotAvailable: return L"RadioNotAvailable";
        case BluetoothError::ResourceInUse: return L"ResourceInUse";
        case BluetoothError::DeviceNotConnected: return L"DeviceNotConnected";
        case BluetoothError::OtherError: return L"OtherError";
        case BluetoothError::DisabledByPolicy: return L"DisabledByPolicy";
        case BluetoothError::NotSupported: return L"NotSupported";
        case BluetoothError::DisabledByUser: return L"DisabledByUser";
        case BluetoothError::ConsentRequired: return L"ConsentRequired";
        case BluetoothError::TransportNotSupported: return L"TransportNotSupported";
        default: return unknown_code_to_hstring(error);
        }
    };
}
