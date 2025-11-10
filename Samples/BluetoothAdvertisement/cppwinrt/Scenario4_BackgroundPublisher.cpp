#include "pch.h"
#include "Scenario4_BackgroundPublisher.h"
#include "Scenario4_BackgroundPublisher.g.cpp"
#include "SampleConfiguration.h"

namespace winrt
{
    using namespace Windows::UI::Xaml;
    using namespace Windows::UI::Xaml::Navigation;
    using namespace Windows::ApplicationModel;
    using namespace Windows::ApplicationModel::Background;
    using namespace Windows::Devices::Bluetooth;
    using namespace Windows::Devices::Bluetooth::Advertisement;
    using namespace Windows::Storage;
}

namespace winrt::SDKTemplate::implementation
{
    Scenario4_BackgroundPublisher::Scenario4_BackgroundPublisher()
    {
        InitializeComponent();

        // We need to add some payload to the advertisement. A publisher without any payload
        // or with invalid ones cannot be started. We only need to configure the payload once
        // for any publisher.

        // Add a manufacturer-specific section:
        // First, create a manufacturer data section
        BluetoothLEManufacturerData manufacturerData;

        // Then, set the company ID for the manufacturer data. Here we picked an unused value: 0xFFFE
        manufacturerData.CompanyId(0xFFFE);

        // Finally set the data payload within the manufacturer-specific section
        // Here, use a 16-bit UUID: 0x1234 -> {0x34, 0x12} (little-endian)
        uint16_t uuidData = 0x1234;

        // Make sure that the buffer length can fit within an advertisement payload. Otherwise you will get an exception.
        manufacturerData.Data(Utilities::ToBuffer(uuidData));

        // Add the manufacturer data to the advertisement publisher:
        publisherTrigger.Advertisement().ManufacturerData().Append(manufacturerData);

        // Display the information about the published payload
        PublisherPayloadBlock().Text(L"Published payload information: " + Utilities::FormatManufacturerData(manufacturerData));

        // Reset the displayed status of the publisher
        PublisherStatusBlock().Text(L"");
    }

    /// <summary>
    /// Invoked when this page is about to be displayed in a Frame.
    ///
    /// We will enable/disable parts of the UI if the device doesn't support it.
    /// </summary>
    /// <param name="e">Event data that describes how this page was reached. The Parameter
    /// property is typically used to configure the page.</param>
    fire_and_forget Scenario4_BackgroundPublisher::OnNavigatedTo(NavigationEventArgs const& e)
    {
        auto lifetime = get_strong();

        // If there is an existing task registration, subscribe to its completion event.
        FindTaskRegistration();
        if (taskRegistration != nullptr)
        {
            taskCompletedRegistrationToken = taskRegistration.Completed({ get_weak(), &Scenario4_BackgroundPublisher::OnBackgroundTaskCompleted });
        }
        UpdateButtons();

        // Attach handlers for suspension to stop the publisherTrigger when the App is suspended.
        appSuspendEventToken = Application::Current().Suspending({ get_weak(), &Scenario4_BackgroundPublisher::App_Suspending });
        appResumeEventToken = Application::Current().Resuming({ get_weak(), &Scenario4_BackgroundPublisher::App_Resuming });

        // Check whether the local Bluetooth adapter supports 2M and Coded PHY.
        if (FeatureDetection::AreExtendedAdvertisingPhysAndScanParametersSupported())
        {
            BluetoothAdapter adapter = co_await BluetoothAdapter::GetDefaultAsync();

            if (adapter != nullptr)
            {
                supportsAdvertisingInDifferentPhy = adapter.IsLowEnergyUncoded2MPhySupported() && adapter.IsLowEnergyCodedPhySupported();
            }
            if (!supportsAdvertisingInDifferentPhy)
            {
                PublisherTrigger2MAndCodedPhysReasonRun().Text(L"(Not supported by default Bluetooth adapter)");
            }
        }
        else
        {
            PublisherTrigger2MAndCodedPhysReasonRun().Text(L"(Not supported by this version of Windows)");
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
    void Scenario4_BackgroundPublisher::OnNavigatedFrom(NavigationEventArgs const& /*e*/)
    {
        // Remove local suspension handlers from the App since this page is no longer active.
        Application::Current().Suspending(appSuspendEventToken);
        Application::Current().Resuming(appResumeEventToken);

        // Since the publisher is registered in the background, the background task will be triggered when the App is closed
        // or in the background. To unregister the task, press the Stop button.
        if (taskRegistration != nullptr)
        {
            // Always unregister the handlers to release the resources to prevent leaks.
            taskRegistration.Completed(taskCompletedRegistrationToken);
        }
    }

    /// <summary>
    /// Invoked when application execution is being suspended.  Application state is saved
    /// without knowing whether the application will be terminated or resumed with the contents
    /// of memory still intact.
    /// </summary>
    /// <param name="sender">Unused.</param>
    /// <param name="e">Details about the suspend request.</param>
    void Scenario4_BackgroundPublisher::App_Suspending(IInspectable const& /*sender*/, SuspendingEventArgs const& /*e*/)
    {
        if (taskRegistration != nullptr)
        {
            taskRegistration.Completed(taskCompletedRegistrationToken);
        }
    }

    /// <summary>
    /// Invoked when application execution is being resumed.
    /// </summary>
    /// <param name="sender">Unused.</param>
    /// <param name="e">Unused.</param>
    void Scenario4_BackgroundPublisher::App_Resuming(IInspectable const& /*sender*/, IInspectable const& /*e*/)
    {
        // If there is an existing task registration, subscribe to its completion event.
        // (We unsubscribed at suspension.)
        FindTaskRegistration();
        if (taskRegistration == nullptr)
        {
            taskCompletedRegistrationToken = taskRegistration.Completed({ get_weak(), &Scenario4_BackgroundPublisher::OnBackgroundTaskCompleted });
        }
        UpdateButtons();
    }

    /// <summary>
    /// Invoked as an event handler when the Run button is pressed.
    /// </summary>
    /// <param name="sender">Instance that triggered the event.</param>
    /// <param name="e">Event data describing the conditions that led to the event.</param>
    fire_and_forget Scenario4_BackgroundPublisher::RunButton_Click(IInspectable const& /*sender*/, RoutedEventArgs const& /*e*/)
    {
        auto lifetime = get_strong();

        // Applications registering for background publisherTrigger must request for permission.
        BackgroundAccessStatus backgroundAccessStatus = co_await BackgroundExecutionManager::RequestAccessAsync();
        // Here, we do not fail the registration even if the access is not granted. Instead, we allow
        // the publisherTrigger to be registered and when the access is granted for the Application at a later time,
        // the publisherTrigger will automatically start working again.

        // First, configure the publisherTrigger.
        if (supportsAdvertisingInDifferentPhy)
        {
            // By default, the BT radio uses the 1M PHY primary/1M PHY secondary configuration,
            // which matches the Windows default configuration.
            // If both coded and 2M PHYs are supported. Use the preferred configuration.
            if (PublisherTrigger2MAndCodedPhysCheckBox().IsChecked().Value())
            {
                publisherTrigger.UseExtendedFormat(true);
                publisherTrigger.PrimaryPhy(BluetoothLEAdvertisementPhyType::CodedPhy);
                publisherTrigger.SecondaryPhy(BluetoothLEAdvertisementPhyType::Uncoded2MPhy);
            }
            else
            {
                publisherTrigger.UseExtendedFormat(false);
                publisherTrigger.PrimaryPhy(BluetoothLEAdvertisementPhyType::Uncoded1MPhy);
                publisherTrigger.SecondaryPhy(BluetoothLEAdvertisementPhyType::Uncoded1MPhy);
            }
        }

        // Create a background task builder with the watcherTrigger and name.
        // Omitting the task entry point results in an in-process background task.
        // See App_OnBackgroundActivated for the in-process background task entry point.
        BackgroundTaskBuilder builder;
        builder.SetTrigger(publisherTrigger);
        builder.Name(taskName);

        // Now perform the registration.
        taskRegistration = builder.Register();

        // For this scenario, attach an event handler to display the result processed from the background task
        taskCompletedRegistrationToken = taskRegistration.Completed({ get_weak(), &Scenario4_BackgroundPublisher::OnBackgroundTaskCompleted});

        // Even though the watcherTrigger is registered successfully, it might be blocked. Notify the user if that is the case.
        if ((backgroundAccessStatus == BackgroundAccessStatus::AlwaysAllowed) ||
            (backgroundAccessStatus == BackgroundAccessStatus::AllowedSubjectToSystemPolicy))
        {
            rootPage.NotifyUser(L"Background watcher registered.", NotifyType::StatusMessage);
        }
        else
        {
            rootPage.NotifyUser(L"Background tasks may be disabled for this app", NotifyType::ErrorMessage);
        }

        UpdateButtons();
    }

    /// <summary>
    /// Invoked as an event handler when the Stop button is pressed.
    /// </summary>
    /// <param name="sender">Instance that triggered the event.</param>
    /// <param name="e">Event data describing the conditions that led to the event.</param>
    void Scenario4_BackgroundPublisher::StopButton_Click(IInspectable const& /*sender*/, RoutedEventArgs const& /*e*/)
    {
        // Unregistering the background task will stop advertising if this is the only client requesting
        taskRegistration.Unregister(true);
        taskRegistration = nullptr;
        rootPage.NotifyUser(L"Background publisher unregistered.", NotifyType::StatusMessage);

        UpdateButtons();
    }

    /// <summary>
    /// If we have not already found a task registration, try to find it.
    /// </summary>
    void Scenario4_BackgroundPublisher::FindTaskRegistration()
    {
        if (taskRegistration == nullptr)
        {
            // Look among the already-registered tasks for the one with the matching name.
            for (auto const& entry : BackgroundTaskRegistration::AllTasks())
            {
                IBackgroundTaskRegistration task = entry.Value();
                if (task.Name() == taskName)
                {
                    taskRegistration = task;
                    break;
                }
            }
        }
    }

    /// <summary>
    /// Enable and disable buttons based on the task registration state and based on what
    /// features are supported.
    /// </summary>
    void Scenario4_BackgroundPublisher::UpdateButtons()
    {
        bool isRegistered = taskRegistration != nullptr;
        PublisherTrigger2MAndCodedPhysCheckBox().IsEnabled(supportsAdvertisingInDifferentPhy && !isRegistered);
        RunButton().IsEnabled(!isRegistered);
        StopButton().IsEnabled(isRegistered);
    }

    /// <summary>
    /// Invoked as an event handler when the background task is completed.
    /// </summary>
    /// <param name="task">Instance of the background task that triggered the event.</param>
    /// <param name="e">Event data containing information about the background task completion.</param>
    fire_and_forget Scenario4_BackgroundPublisher::OnBackgroundTaskCompleted(BackgroundTaskRegistration const& /*task*/, BackgroundTaskCompletedEventArgs const& /*e*/)
    {
        auto lifetime = get_strong();

        std::optional<hstring> backgroundMessage = ApplicationData::Current().LocalSettings().Values().TryLookup(taskName).try_as<hstring>();
        if (backgroundMessage)
        {
            co_await winrt::resume_foreground(Dispatcher());
            PublisherStatusBlock().Text(*backgroundMessage);
        }
    }
}
