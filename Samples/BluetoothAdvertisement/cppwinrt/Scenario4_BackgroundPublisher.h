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
#include "Scenario4_BackgroundPublisher.g.h"
#include "MainPage.h"

//using namespace winrt;
//using namespace Windows::Devices::Bluetooth;
//using namespace Windows::Foundation;
//using namespace Windows::UI::Xaml;
//using namespace Windows::UI::Xaml::Navigation;
//using namespace Windows::ApplicationModel::Background;

namespace winrt::SDKTemplate::implementation
{
    struct Scenario4_BackgroundPublisher : Scenario4_BackgroundPublisherT<Scenario4_BackgroundPublisher>
    {
        Scenario4_BackgroundPublisher();

        fire_and_forget OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const& /*e*/);
        void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const& /*e*/);
        fire_and_forget RunButton_Click(IInspectable const& /*sender*/, Windows::UI::Xaml::RoutedEventArgs const& /*e*/);
        void StopButton_Click(IInspectable const& /*sender*/, Windows::UI::Xaml::RoutedEventArgs const& /*e*/);

    private:
        // A pointer back to the main page is required to display status messages.
        SDKTemplate::MainPage rootPage = MainPage::Current();

        // The background task registration for the background advertisement publisher
        Windows::ApplicationModel::Background::IBackgroundTaskRegistration taskRegistration;

        // The watcherTrigger used to configure the background task registration
        Windows::ApplicationModel::Background::BluetoothLEAdvertisementPublisherTrigger publisherTrigger;

        // Capability of the Bluetooth radio adapter.
        bool supportsAdvertisingInDifferentPhy = false;

        // A name is given to the task in order for it to be identifiable across context.
        static constexpr std::wstring_view taskName = winrt::name_of<AdvertisementPublisherTask>();

        event_token taskCompletedRegistrationToken;
        event_token appSuspendEventToken;
        event_token appResumeEventToken;

        void App_Suspending(IInspectable const& sender, Windows::ApplicationModel::SuspendingEventArgs const& e);
        void App_Resuming(IInspectable const& sender, IInspectable const& e);
        fire_and_forget OnBackgroundTaskCompleted(Windows::ApplicationModel::Background::BackgroundTaskRegistration const& /*task*/, Windows::ApplicationModel::Background::BackgroundTaskCompletedEventArgs const& /*e*/);


        void FindTaskRegistration();
        void UpdateButtons();
    };
}

namespace winrt::SDKTemplate::factory_implementation
{
    struct Scenario4_BackgroundPublisher : Scenario4_BackgroundPublisherT<Scenario4_BackgroundPublisher, implementation::Scenario4_BackgroundPublisher>
    {
    };
}
