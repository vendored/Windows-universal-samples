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
#include "Scenario1_Watcher.g.h"
#include "MainPage.h"

namespace winrt::SDKTemplate::implementation
{
    struct Scenario1_Watcher : Scenario1_WatcherT<Scenario1_Watcher>
    {
        public:
            Scenario1_Watcher();

            fire_and_forget OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const& e);
            void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const& e);
            void RunButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& args);
            void StopButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& args);

    private:
            // A pointer back to the main page is required to display status messages.
            SDKTemplate::MainPage rootPage{ MainPage::Current() };

            // For pretty-printing timestamps.
            Windows::Globalization::DateTimeFormatting::DateTimeFormatter formatter{ L"longtime" };

            // The Bluetooth LE advertisement watcher class is used to control and customize Bluetooth LE scanning.
            Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher watcher;
            bool isWatcherStarted = false;

            // Capability of the Bluetooth radio adapter.
            bool supportsCodedPhy = false;
            bool supportsHardwareOffloadedFilters = false;

            event_token appSuspendEventToken{};
            event_token appResumeEventToken{};
            event_token watcherReceivedEventToken{};
            event_token watcherStoppedEventToken{};

            void App_Suspending(Windows::Foundation::IInspectable const& sender, Windows::ApplicationModel::SuspendingEventArgs const& e);
            void App_Resuming(Windows::Foundation::IInspectable const& sender, Windows::Foundation::IInspectable const& e);

            void UpdateButtons();

            fire_and_forget OnAdvertisementReceived(Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher const& /*sender*/, Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs const& e);
            void OnAdvertisementWatcherStopped(Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher const& /*sender*/, Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcherStoppedEventArgs const& e);
    };

}

namespace winrt::SDKTemplate::factory_implementation
{
    struct Scenario1_Watcher : Scenario1_WatcherT<Scenario1_Watcher, implementation::Scenario1_Watcher>
    {
    };
}
