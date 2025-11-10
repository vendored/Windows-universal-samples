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

#include "pch.h"
#include "Scenario2_Publisher.g.h"
#include "MainPage.h"

using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
using namespace winrt::Windows::UI::Xaml;

namespace winrt::SDKTemplate::implementation
{
    struct Scenario2_Publisher : Scenario2_PublisherT<Scenario2_Publisher>
    {
    public:
        Scenario2_Publisher();

        fire_and_forget OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const&);
        void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const&);
        void RunButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
        void StopButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);

    private:
        // A pointer back to the main page is required to display status messages.
        SDKTemplate::MainPage rootPage{ MainPage::Current() };

        // The Bluetooth LE advertisement publisher class is used to control and customize Bluetooth LE advertising.
        BluetoothLEAdvertisementPublisher publisher;
        bool isPublisherStarted = false;

        // Capability of the Bluetooth radio adapter.
        bool supportsAdvertisingInDifferentPhy = false;

        event_token appSuspendEventToken{};
        event_token appResumeEventToken{};
        event_token publisherStatusChangedToken{};

        void App_Suspending(Windows::Foundation::IInspectable const&, Windows::ApplicationModel::SuspendingEventArgs const&);
        void App_Resuming(Windows::Foundation::IInspectable const&, Windows::Foundation::IInspectable const&);
        void UpdateButtons();
        fire_and_forget OnPublisherStatusChanged(Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementPublisher const&,
            Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementPublisherStatusChangedEventArgs e);
    };
}

namespace winrt::SDKTemplate::factory_implementation
{
    struct Scenario2_Publisher : Scenario2_PublisherT<Scenario2_Publisher, implementation::Scenario2_Publisher>
    {
    };
}
