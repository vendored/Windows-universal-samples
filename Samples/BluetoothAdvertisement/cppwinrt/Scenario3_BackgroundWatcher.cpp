#include "pch.h"
#include "Scenario3_BackgroundWatcher.h"
#include "Scenario3_BackgroundWatcher.g.cpp"
#include "MainPage.h"
#include "SampleConfiguration.h"

using namespace std::literals::chrono_literals;

namespace winrt
{
    using namespace winrt::Windows::ApplicationModel;
    using namespace winrt::Windows::ApplicationModel::Background;
    using namespace winrt::Windows::Devices::Bluetooth;
    using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Storage;
    using namespace winrt::Windows::UI::Xaml;
    using namespace winrt::Windows::UI::Xaml::Controls;
    using namespace winrt::Windows::UI::Xaml::Navigation;
}

namespace winrt::SDKTemplate::implementation
{
    Scenario3_BackgroundWatcher::Scenario3_BackgroundWatcher()
    {
        InitializeComponent();

        // Configure the advertisement filter to look for the data advertised by the publisher in Scenario 2 or 4.
        // You need to run Scenario 2 on another Windows platform within proximity of this one for Scenario 3 to
        // take effect.

        // Unlike the APIs in Scenario 1 which operate in the foreground. This API allows the developer to register a background
        // task to process advertisement packets in the background. It has more restrictions on valid filter configuration.
        // For example, exactly one single matching filter condition is allowed (no more or less) and the sampling interval

        // For determining the filter restrictions programmatically across APIs, use the following properties:
        //      MinSamplingInterval, MaxSamplingInterval, MinOutOfRangeTimeout, MaxOutOfRangeTimeout

        // Part 1A: Configuring the advertisement filter to watch for a particular advertisement payload

        // First, let create a manufacturer data section we wanted to match for. These are the same as the one
        // created in Scenario 2 and 4. Note that in the background only a single filter pattern is allowed per watcherTrigger.
        BluetoothLEManufacturerData manufacturerData;

        // Then, set the company ID for the manufacturer data. Here we picked an unused value: 0xFFFE
        manufacturerData.CompanyId(0xFFFE);

        // Finally set the data payload within the manufacturer-specific section
        // Here, use a 16-bit UUID: 0x1234 -> {0x34, 0x12} (little-endian)
        uint16_t uuidData = 0x1234;

        // Make sure that the buffer length can fit within an advertisement payload. Otherwise you will get an exception.
        manufacturerData.Data(Utilities::ToBuffer(uuidData));

        // Add the manufacturer data to the advertisement filter on the watcherTrigger:
        watcherTrigger.AdvertisementFilter().Advertisement().ManufacturerData().Append(manufacturerData);

        // Part 1B: Configuring the signal strength filter for proximity scenarios

        // Configure the signal strength filter to only propagate events when in-range
        // Please adjust these values if you cannot receive any advertisement
        // Set the in-range threshold to -70dBm. This means advertisements with RSSI >= -70dBm
        // will start to be considered "in-range".
        watcherTrigger.SignalStrengthFilter().InRangeThresholdInDBm(-70);

        // Set the out-of-range threshold to -75dBm (give some buffer). Used in conjunction with OutOfRangeTimeout
        // to determine when an advertisement is no longer considered "in-range"
        watcherTrigger.SignalStrengthFilter().OutOfRangeThresholdInDBm(-75);

        // Set the out-of-range timeout to be 2 seconds. Used in conjunction with OutOfRangeThresholdInDBm
        // to determine when an advertisement is no longer considered "in-range"
        watcherTrigger.SignalStrengthFilter().OutOfRangeTimeout(TimeSpan{2s});

        // By default, the sampling interval is set to be disabled, or the maximum sampling interval supported.
        // The sampling interval set to MaxSamplingInterval indicates that the event will only watcherTrigger once after it comes into range.
        // Here, set the sampling period to 1 second, which is the minimum supported for background.
        watcherTrigger.SignalStrengthFilter().SamplingInterval(TimeSpan{1s});
    }

    /// <summary>
    /// Invoked when this page is about to be displayed in a Frame.
    ///
    /// We will enable/disable parts of the UI if the device doesn't support it.
    /// </summary>
    /// <param name="e">Event data that describes how this page was reached. The Parameter
    /// property is typically used to configure the page.</param>
    fire_and_forget Scenario3_BackgroundWatcher::OnNavigatedTo(NavigationEventArgs const& /*e*/)
    {
        auto lifetime = get_strong();

        // If there is an existing task registration, subscribe to its completion event.
        FindTaskRegistration();
        if (taskRegistration != nullptr)
        {
            taskCompletedRegistrationToken = taskRegistration.Completed({ get_weak(), &Scenario3_BackgroundWatcher::OnBackgroundTaskCompleted });
        }
        UpdateButtons();

        // Attach handlers for suspension to disconnect from the background task when the App is suspended.
        appSuspendEventToken = Application::Current().Suspending({ get_weak(), &Scenario3_BackgroundWatcher::App_Suspending });
        appResumeEventToken = Application::Current().Resuming({ get_weak(), &Scenario3_BackgroundWatcher::App_Resuming });

        // Check whether the local Bluetooth adapter supports the Coded PHYs and hardware off load.
        if (FeatureDetection::AreExtendedAdvertisingPhysAndScanParametersSupported())
        {
            BluetoothAdapter adapter = co_await BluetoothAdapter::GetDefaultAsync();

            if (adapter != nullptr)
            {
                supportsCodedPhy = adapter.IsLowEnergyCodedPhySupported();
                supportsHardwareOffloadedFilters = adapter.IsAdvertisementOffloadSupported();
            }
            if (!supportsCodedPhy)
            {
                WatcherTrigger1MAndCodedPhysReasonRun().Text(L"(Not supported by default Bluetooth adapter)");
            }
            if (!supportsHardwareOffloadedFilters)
            {
                WatcherTriggerPerformanceOptimizationsReasonRun().Text(L"(Not supported by default Bluetooth adapter)");
            }
        }
        else
        {
            WatcherTrigger1MAndCodedPhysReasonRun().Text(L"(Not supported by this version of Windows)");
            WatcherTriggerPerformanceOptimizationsReasonRun().Text(L"(Not supported by this version of Windows)");
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
    void Scenario3_BackgroundWatcher::OnNavigatedFrom(NavigationEventArgs const& /*e*/)
    {
        // Remove local suspension handlers from the App since this page is no longer active.
        Application::Current().Suspending(appSuspendEventToken);
        Application::Current().Resuming(appResumeEventToken);

        // Since the watcher is registered in the background, the background task will be triggered when the App is closed
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
    void Scenario3_BackgroundWatcher::App_Suspending(IInspectable const& /*sender*/, SuspendingEventArgs const& /*e*/)
    {
        if (taskRegistration)
        {
            // Always unregister the handlers to release the resources to prevent leaks.
            taskRegistration.Completed(taskCompletedRegistrationToken);
        }
    }


    /// <summary>
    /// Invoked when application execution is being resumed.
    /// </summary>
    /// <param name="sender">Unused</param>
    /// <param name="e">Unused</param>
    void Scenario3_BackgroundWatcher::App_Resuming(IInspectable const&, IInspectable const&)
    {
        // If there is an existing task registration, subscribe to its completion event.
        // (We unsubscribed at suspension.)
        FindTaskRegistration();

        if (taskRegistration != nullptr)
        {
            taskCompletedRegistrationToken = taskRegistration.Completed({ get_weak(), &Scenario3_BackgroundWatcher::OnBackgroundTaskCompleted });
        }
        UpdateButtons();
    }

    /// <summary>
    /// Invoked as an event handler when the Run button is pressed.
    /// </summary>
    /// <param name="sender">Instance that triggered the event.</param>
    /// <param name="e">Event data describing the conditions that led to the event.</param>
    fire_and_forget Scenario3_BackgroundWatcher::RunButton_Click(IInspectable const& /*sender*/, RoutedEventArgs const& /*e*/)
    {
        auto lifetime = get_strong();

        // Applications registering for background watcherTrigger must request for permission.
        auto backgroundAccessStatus = co_await BackgroundExecutionManager::RequestAccessAsync();
        // Here, we do not fail the registration even if the access is not granted. Instead, we allow
        // the watcherTrigger to be registered and when the access is granted for the Application at a later time,
        // the watcherTrigger will automatically start working again.

        // At this point we assume we haven't found any existing tasks matching the one we want to register
        // First, configure the watcherTrigger.

        // Default Windows will scan over the 1M PHYs.
        if (supportsCodedPhy)
        {
            if (WatcherTrigger1MAndCodedPhysCheckBox().IsChecked().Value())
            {
                // Enable the Bluetooth adapter to also scan over 2M and Coded PHYs.
                watcherTrigger.UseCodedPhy(true);
                // Required in order to specify multiple scan PHYs
                watcherTrigger.AllowExtendedAdvertisements(true);
            }
            else
            {
                // Disable scanning over 2M and Coded PHYs.
                watcherTrigger.UseCodedPhy(false);
                watcherTrigger.AllowExtendedAdvertisements(false);
            }
        }

        if (supportsHardwareOffloadedFilters)
        {
            if (WatcherTriggerPerformanceOptimizationsCheckBox().IsChecked().Value())
            {
                // Enable the Bluetooth adapter to perform a background scan with performance optimizations.
                watcherTrigger.ScanParameters(BluetoothLEAdvertisementScanParameters::CoexistenceOptimized());
            }
            else
            {
                // Disable scanning with performance optimizations and reset it to default low latency.
                watcherTrigger.ScanParameters(BluetoothLEAdvertisementScanParameters::LowLatency());
            }
        }

        // Create a background task builder with the watcherTrigger and name.
        // Omitting the task entry point results in an in-process background task.
        // See App_OnBackgroundActivated for the in-process background task entry point.
        BackgroundTaskBuilder builder;
        builder.SetTrigger(watcherTrigger);
        builder.Name(taskName);

        // Now perform the registration.
        taskRegistration = builder.Register();

        // For this scenario, attach an event handler to display the result processed from the background task
        taskCompletedRegistrationToken = taskRegistration.Completed({ get_weak(), &Scenario3_BackgroundWatcher::OnBackgroundTaskCompleted });

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
    void Scenario3_BackgroundWatcher::StopButton_Click(IInspectable const& /*sender*/, RoutedEventArgs const& /*e*/)
    {
        // Unregistering the background task will stop scanning if this is the only client requesting scan
        taskRegistration.Unregister(true);
        taskRegistration = nullptr;
        rootPage.NotifyUser(L"Background watcher unregistered.", NotifyType::StatusMessage);

        UpdateButtons();
    }

    /// <summary>
    /// If we have not already found a task registration, try to find it.
    /// </summary>
    void Scenario3_BackgroundWatcher::FindTaskRegistration()
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
    void Scenario3_BackgroundWatcher::UpdateButtons()
    {
        bool isRegistered = taskRegistration != nullptr;
        WatcherTrigger1MAndCodedPhysCheckBox().IsEnabled(supportsCodedPhy && !isRegistered);
        WatcherTriggerPerformanceOptimizationsCheckBox().IsEnabled(supportsHardwareOffloadedFilters && !isRegistered);
        RunButton().IsEnabled(!isRegistered);
        StopButton().IsEnabled(isRegistered);
    }

    /// <summary>
    /// Invoked as an event handler when the background task is completed.
    /// </summary>
    /// <param name="task">Instance of the background task that triggered the event.</param>
    /// <param name="e">Event data containing information about the background task completion.</param>
    fire_and_forget Scenario3_BackgroundWatcher::OnBackgroundTaskCompleted(BackgroundTaskRegistration const& task, BackgroundTaskCompletedEventArgs const& /*e*/)
    {
        // We get the advertisement(s) processed by the background task
        IInspectable backgroundMessage = ApplicationData::Current().LocalSettings().Values().TryLookup(taskName);
        if (backgroundMessage != nullptr)
        {
            // Serialize UI update to the main UI thread
            co_await winrt::resume_foreground(Dispatcher());

            // Display these information on the list
            ReceivedAdvertisementListBox().Items().Append(backgroundMessage);
        }
    }
}
