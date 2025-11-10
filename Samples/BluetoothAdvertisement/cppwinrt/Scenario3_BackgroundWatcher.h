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
#include "Scenario3_BackgroundWatcher.g.h"

namespace winrt::SDKTemplate::implementation
{
    struct Scenario3_BackgroundWatcher : Scenario3_BackgroundWatcherT<Scenario3_BackgroundWatcher>
    {
    public:
        Scenario3_BackgroundWatcher();

        fire_and_forget OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const& /*e*/);
        void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const& /*e*/);

        fire_and_forget RunButton_Click(IInspectable const& /*sender*/, Windows::UI::Xaml::RoutedEventArgs const& /*e*/);
        void StopButton_Click(IInspectable const& /*sender*/, Windows::UI::Xaml::RoutedEventArgs const& /*e*/);

    private:
        // A pointer back to the main page is required to display status messages.
        SDKTemplate::MainPage rootPage = MainPage::Current();

        // The background task registration for the background advertisement watcher
        Windows::ApplicationModel::Background::IBackgroundTaskRegistration taskRegistration =nullptr;

        // The watcherTrigger used to configure the background task registration
        Windows::ApplicationModel::Background::BluetoothLEAdvertisementWatcherTrigger watcherTrigger;

        // Capability of the Bluetooth radio adapter.
        bool supportsCodedPhy = false;
        bool supportsHardwareOffloadedFilters = false;

        // A name is given to the task in order for it to be identifiable as the background task for this scenario.
        static constexpr std::wstring_view taskName = winrt::name_of<AdvertisementWatcherTask>();

        event_token taskCompletedRegistrationToken{};
        event_token appSuspendEventToken{};
        event_token appResumeEventToken{};

        void App_Suspending(IInspectable const& /*sender*/, Windows::ApplicationModel::SuspendingEventArgs const& /*e*/);
        void App_Resuming(IInspectable const& /*sender*/, IInspectable const& /*e*/);
        fire_and_forget OnBackgroundTaskCompleted(Windows::ApplicationModel::Background::BackgroundTaskRegistration const& task, Windows::ApplicationModel::Background::BackgroundTaskCompletedEventArgs const& eventArgs);

        void FindTaskRegistration();
        void UpdateButtons();
    };
}

namespace winrt::SDKTemplate::factory_implementation
{
    struct Scenario3_BackgroundWatcher : Scenario3_BackgroundWatcherT<Scenario3_BackgroundWatcher, implementation::Scenario3_BackgroundWatcher>
    {
    };
}
