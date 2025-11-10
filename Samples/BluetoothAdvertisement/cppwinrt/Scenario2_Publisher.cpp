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
#include "Scenario2_Publisher.h"
#include "Scenario2_Publisher.g.cpp"
#include "SampleConfiguration.h"

namespace winrt
{
    using namespace winrt::Windows::ApplicationModel;
    using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
    using namespace winrt::Windows::Devices::Enumeration;
    using namespace winrt::Windows::UI::Xaml;
    using namespace winrt::Windows::UI::Xaml::Navigation;
}

namespace winrt::SDKTemplate::implementation
{
    Scenario2_Publisher::Scenario2_Publisher()
    {
        InitializeComponent();

        // We need to add some payload to the advertisement. A publisher without any payload
        // or with invalid ones cannot be started. We only need to configure the payload once
        // for any publisher.

        // Add a manufacturer-specific section:
        // First, let create a manufacturer data section
        auto manufacturerData = BluetoothLEManufacturerData();

        // Then, set the company ID for the manufacturer data. Here we picked an unused value: 0xFFFE
        manufacturerData.CompanyId(0xFFFE);

        // Finally set the data payload within the manufacturer-specific section
        // Here, use a 16-bit UUID: 0x1234 -> {0x34, 0x12} (little-endian)
        uint16_t uuidData = 0x1234;

        // Make sure that the buffer length can fit within an advertisement payload. Otherwise you will get an exception.
        manufacturerData.Data(Utilities::ToBuffer(uuidData));

        // Add the manufacturer data to the advertisement publisher:
        publisher.Advertisement().ManufacturerData().Append(manufacturerData);

        // Display the information about the published payload
        PublisherPayloadBlock().Text(L"Published payload information: " + Utilities::FormatManufacturerData(manufacturerData));

        /// Display the current status of the publisher
        PublisherStatusBlock().Text(L"Published Status: " + to_hstring(publisher.Status()));
    }

    /// <summary>
    /// Invoked when this page is about to be displayed in a Frame.
    ///
    /// We will enable/disable parts of the UI if the device doesn't support it.
    /// </summary>
    /// <param name="e">Event data that describes how this page was reached. The Parameter
    /// property is typically used to configure the page.</param>
    fire_and_forget Scenario2_Publisher::OnNavigatedTo(NavigationEventArgs const& /*e*/)
    {
         auto lifetime = get_strong();

       //  Attach a event handler to monitor the status of the publisher, which
       //  can tell us whether the advertising has been serviced or is waiting to be serviced
       //  due to lack of resources. It will also inform us of unexpected errors such as the Bluetooth
       //  radio being turned off by the user.
        publisherStatusChangedToken = publisher.StatusChanged({ get_weak(), &Scenario2_Publisher::OnPublisherStatusChanged });

        // Attach handlers for suspension to stop the watcher when the App is suspended.
        appSuspendEventToken = Application::Current().Suspending({ get_weak(), &Scenario2_Publisher::App_Suspending });
        appResumeEventToken = Application::Current().Resuming({ get_weak(), &Scenario2_Publisher::App_Resuming });

        rootPage.NotifyUser(L"Press Run to start publisher.", NotifyType::StatusMessage);

        // Determine whether the default Bluetooth adapter supports 2M and Coded PHY.
        if (FeatureDetection::AreExtendedAdvertisingPhysAndScanParametersSupported())
        {
            BluetoothAdapter adapter = co_await BluetoothAdapter::GetDefaultAsync();
            if (adapter != nullptr)
            {
                supportsAdvertisingInDifferentPhy = adapter.IsLowEnergyUncoded2MPhySupported() && adapter.IsLowEnergyCodedPhySupported();
            }
            if (!supportsAdvertisingInDifferentPhy)
            {
                Publisher2MAndCodedPhysReasonRun().Text(L"(Not supported by default Bluetooth adapter)");
            }
        }
        else
        {
            Publisher2MAndCodedPhysReasonRun().Text(L"(Not supported by this version of Windows)");
        }

        UpdateButtons();
    }

    /// <summary>
    /// Invoked immediately after the Page is unloaded and is no longer the current source of a parent Frame.
    /// </summary>
    /// <param name="e">
    /// Event data that can be examined by overriding code. The event data is representative
    /// of the navigation that has unloaded the current Page.
    /// </param>
    void Scenario2_Publisher::OnNavigatedFrom(NavigationEventArgs const& /*e*/)
    {
        auto lifetime = get_strong();

        // Remove local suspension handlers from the App since this page is no longer active.
        Application::Current().Suspending(appSuspendEventToken);
        Application::Current().Resuming(appResumeEventToken);

        // Make sure to stop the publisher when leaving the context. Even if the publisher is not stopped,
        // advertising will be stopped automatically if the publisher is destroyed.
        publisher.Stop();

        // Always unregister the handlers to release the resources to prevent leaks.
        publisher.StatusChanged(publisherStatusChangedToken);
    }

    /// <summary>
    /// Invoked when application execution is being suspended.  Application state is saved
    /// without knowing whether the application will be terminated or resumed with the contents
    /// of memory still intact.
    /// </summary>
    /// <param name="sender">Unused.</param>
    /// <param name="e">Details about the suspend request.</param>
    void Scenario2_Publisher::App_Suspending(IInspectable const& /*sender*/, SuspendingEventArgs const& /*e*/)
    {
        // Make sure to stop the publisher on suspend.
        publisher.Stop();
        isPublisherStarted = false;

        // Always unregister the handlers to release the resources to prevent leaks.
        publisher.StatusChanged(publisherStatusChangedToken);

        rootPage.NotifyUser(L"App suspending. Publisher stopped.", NotifyType::StatusMessage);
    }

    /// <summary>
    /// Invoked when application execution is being resumed.
    /// </summary>
    /// <param name="sender">Unused.</param>
    /// <param name="e">Unused.</param>
    void Scenario2_Publisher::App_Resuming(IInspectable const& /*sender*/, IInspectable const& /*e*/)
    {
        publisherStatusChangedToken = publisher.StatusChanged({ get_weak(), &Scenario2_Publisher::OnPublisherStatusChanged });
        UpdateButtons();
    }

    /// <summary>
    /// Invoked as an event handler when the Run button is pressed.
    /// </summary>
    /// <param name="sender">Instance that triggered the event.</param>
    /// <param name="e">Event data describing the conditions that led to the event.</param>
    void Scenario2_Publisher::RunButton_Click(IInspectable const& /*sender*/, RoutedEventArgs const& /*e*/)
    {
        // By default, the BT radio uses the 1M PHY primary/1M PHY secondary configuration,
        // which matches the Windows default configuration.
        // If both coded and 2M PHYs are supported, use the preferred configuration.
        if (supportsAdvertisingInDifferentPhy)
        {
            if (Publisher2MAndCodedPhysCheckBox().IsChecked().Value())
            {
                // Enable the Bluetooth adapter to only advertise over 2M and Coded PHYs.
                publisher.PrimaryPhy(BluetoothLEAdvertisementPhyType::CodedPhy);
                publisher.SecondaryPhy(BluetoothLEAdvertisementPhyType::Uncoded2MPhy);
                publisher.UseExtendedAdvertisement(true);
            }
            else
            {
                // Disable the Bluetooth adapter to advertise over 2M and Coded PHYs and reset it back to 1M PHYs only.
                publisher.PrimaryPhy(BluetoothLEAdvertisementPhyType::Uncoded1MPhy);
                publisher.SecondaryPhy(BluetoothLEAdvertisementPhyType::Uncoded1MPhy);
                publisher.UseExtendedAdvertisement(false);
            }
        }
        // Calling publisher start will start the advertising if resources are available to do so.
        publisher.Start();
        isPublisherStarted = true;

        rootPage.NotifyUser(L"Publisher started.", NotifyType::StatusMessage);
        UpdateButtons();
    }

    /// <summary>
    /// Invoked as an event handler when the Stop button is pressed.
    /// </summary>
    /// <param name="sender">Instance that triggered the event.</param>
    /// <param name="e">Event data describing the conditions that led to the event.</param>
    void Scenario2_Publisher::StopButton_Click(IInspectable const& /*sender*/, RoutedEventArgs const& /*e*/)
    {
        // Stopping the publisher will stop advertising the published payload
        publisher.Stop();
        isPublisherStarted = false;

        rootPage.NotifyUser(L"Publisher stopped.", NotifyType::StatusMessage);
        UpdateButtons();
    }

    /// <summary>
    /// Enable and disable buttons based on the publisher state and based on what
    /// features are supported.
    /// </summary>
    void Scenario2_Publisher::UpdateButtons()
    {
        Publisher2MAndCodedPhysCheckBox().IsEnabled(supportsAdvertisingInDifferentPhy && !isPublisherStarted);
        RunButton().IsEnabled(!isPublisherStarted);
        StopButton().IsEnabled(isPublisherStarted);
    }

    /// <summary>
    /// Invoked as an event handler when the status of the publisher changes.
    /// </summary>
    /// <param name="sender">Instance of publisher that triggered the event.</param>
    /// <param name="e">Event data containing information about the publisher status change event.</param>
    fire_and_forget Scenario2_Publisher::OnPublisherStatusChanged(
        BluetoothLEAdvertisementPublisher const& /*sender*/,
        BluetoothLEAdvertisementPublisherStatusChangedEventArgs e)
    {
        auto lifetime = get_strong();

        // This event handler can be used to monitor the status of the publisher.
        // We can catch errors if the publisher is aborted by the system

        // Must switch to UI thread to update UI content.
        co_await winrt::resume_foreground(Dispatcher());

        PublisherStatusBlock().Text(L"Published Status: " + to_hstring(e.Status()) + L", Error: " + to_hstring(e.Error()));
    }
}
