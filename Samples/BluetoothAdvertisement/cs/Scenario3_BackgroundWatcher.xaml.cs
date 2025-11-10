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
using Windows.ApplicationModel;
using Windows.ApplicationModel.Background;
using Windows.Devices.Bluetooth;
using Windows.Devices.Bluetooth.Advertisement;
using Windows.Security.Cryptography;
using Windows.Storage;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;

namespace SDKTemplate
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class Scenario3_BackgroundWatcher : Page
    {
        // A pointer back to the main page is required to display status messages.
        private MainPage rootPage = MainPage.Current;

        // The background task registration for the background advertisement watcher
        private IBackgroundTaskRegistration taskRegistration = null;

        // The watcherTrigger used to configure the background task registration
        private BluetoothLEAdvertisementWatcherTrigger watcherTrigger;

        // Capabilities of the Bluetooth radio adapter.
        private bool supportsCodedPhy = false;
        private bool supportsHardwareOffloadedFilters = false;

        // A name is given to the task in order for it to be identifiable as the background task for this scenario.
        private static readonly string taskName = nameof(AdvertisementWatcherTask);

        const int E_DEVICE_NOT_AVAILABLE = unchecked((int)0x800710df); // HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_AVAILABLE)

        public Scenario3_BackgroundWatcher()
        {
            InitializeComponent();

            // Create and initialize a new watcherTrigger to configure it.
            watcherTrigger = new BluetoothLEAdvertisementWatcherTrigger();

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
            var manufacturerData = new BluetoothLEManufacturerData();

            // Then, set the company ID for the manufacturer data. Here we picked an unused value: 0xFFFE
            manufacturerData.CompanyId = 0xFFFE;

            // Finally set the data payload within the manufacturer-specific section
            // Here, use a 16-bit UUID: 0x1234 -> {0x34, 0x12} (little-endian)
            UInt16 uuidData = 0x1234;

            // Make sure that the buffer length can fit within an advertisement payload. Otherwise you will get an exception.
            manufacturerData.Data = CryptographicBuffer.CreateFromByteArray(BitConverter.GetBytes(uuidData));

            // Add the manufacturer data to the advertisement filter on the watcherTrigger:
            watcherTrigger.AdvertisementFilter.Advertisement.ManufacturerData.Add(manufacturerData);

            // Part 1B: Configuring the signal strength filter for proximity scenarios

            // Configure the signal strength filter to only propagate events when in-range
            // Please adjust these values if you cannot receive any advertisement
            // Set the in-range threshold to -70dBm. This means advertisements with RSSI >= -70dBm
            // will start to be considered "in-range".
            watcherTrigger.SignalStrengthFilter.InRangeThresholdInDBm = -70;

            // Set the out-of-range threshold to -75dBm (give some buffer). Used in conjunction with OutOfRangeTimeout
            // to determine when an advertisement is no longer considered "in-range"
            watcherTrigger.SignalStrengthFilter.OutOfRangeThresholdInDBm = -75;

            // Set the out-of-range timeout to be 2 seconds. Used in conjunction with OutOfRangeThresholdInDBm
            // to determine when an advertisement is no longer considered "in-range"
            watcherTrigger.SignalStrengthFilter.OutOfRangeTimeout = TimeSpan.FromMilliseconds(2000);

            // By default, the sampling interval is set to be disabled, or the maximum sampling interval supported.
            // The sampling interval set to MaxSamplingInterval indicates that the event will only watcherTrigger once after it comes into range.
            // Here, set the sampling period to 1 second, which is the minimum supported for background.
            watcherTrigger.SignalStrengthFilter.SamplingInterval = TimeSpan.FromMilliseconds(1000);
        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        ///
        /// We will enable/disable parts of the UI if the device doesn't support it.
        /// </summary>
        /// <param name="eventArgs">Event data that describes how this page was reached. The Parameter
        /// property is typically used to configure the page.</param>
        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            // If there is an existing task registration, subscribe to its completion event.
            FindTaskRegistration();
            if (taskRegistration != null)
            {
                taskRegistration.Completed += OnBackgroundTaskCompleted;
            }
            UpdateButtons();

            // Attach handlers for suspension to disconnect from the background task when the App is suspended.
            App.Current.Suspending += App_Suspending;
            App.Current.Resuming += App_Resuming;

            // Check whether the local Bluetooth adapter supports the Coded PHYs and hardware off load.
            if (FeatureDetection.AreExtendedAdvertisingPhysAndScanParametersSupported)
            {
                BluetoothAdapter adapter = await BluetoothAdapter.GetDefaultAsync();

                if (adapter != null)
                {
                    supportsCodedPhy = adapter.IsLowEnergyCodedPhySupported;
                    supportsHardwareOffloadedFilters = adapter.IsAdvertisementOffloadSupported;
                }
                if (!supportsCodedPhy)
                {
                    WatcherTrigger1MAndCodedPhysReasonRun.Text = "(Not supported by default Bluetooth adapter)";
                }
                if (!supportsHardwareOffloadedFilters)
                {
                    WatcherTriggerPerformanceOptimizationsReasonRun.Text = "(Not supported by default Bluetooth adapter)";
                }
            }
            else
            {
                WatcherTrigger1MAndCodedPhysReasonRun.Text = "(Not supported by this version of Windows)";
                WatcherTriggerPerformanceOptimizationsReasonRun.Text = "(Not supported by this version of Windows)";
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
        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            // Remove local suspension handlers from the App since this page is no longer active.
            App.Current.Suspending -= App_Suspending;
            App.Current.Resuming -= App_Resuming;

            // Since the watcher is registered in the background, the background task will be triggered when the App is closed
            // or in the background. To unregister the task, press the Stop button.
            if (taskRegistration != null)
            {
                // Always unregister the handlers to release the resources to prevent leaks.
                taskRegistration.Completed -= OnBackgroundTaskCompleted;
            }
        }

        /// <summary>
        /// Invoked when application execution is being suspended.  Application state is saved
        /// without knowing whether the application will be terminated or resumed with the contents
        /// of memory still intact.
        /// </summary>
        /// <param name="sender">Unused.</param>
        /// <param name="e">Details about the suspend request.</param>
        private void App_Suspending(object sender, SuspendingEventArgs e)
        {
            if (taskRegistration != null)
            {
                // Always unregister the handlers to release the resources to prevent leaks.
                taskRegistration.Completed -= OnBackgroundTaskCompleted;
            }
            rootPage.NotifyUser("App suspending.", NotifyType.StatusMessage);
        }

        /// <summary>
        /// Invoked when application execution is being resumed.
        /// </summary>
        /// <param name="sender">The source of the resume request.</param>
        /// <param name="e"></param>
        private void App_Resuming(object sender, object e)
        {
            // If there is an existing task registration, subscribe to its completion event.
            // (We unsubscribed at suspension.)
            FindTaskRegistration();
            if (taskRegistration != null)
            {
                taskRegistration.Completed += OnBackgroundTaskCompleted;
            }
            UpdateButtons();
        }

        /// <summary>
        /// Invoked as an event handler when the Run button is pressed.
        /// </summary>
        /// <param name="sender">Instance that triggered the event.</param>
        /// <param name="e">Event data describing the conditions that led to the event.</param>
        private async void RunButton_Click(object sender, RoutedEventArgs e)
        {
            // Applications registering for background watcherTrigger must request permission.
            BackgroundAccessStatus backgroundAccessStatus = await BackgroundExecutionManager.RequestAccessAsync();
            // Here, we do not fail the registration even if the access is not granted. Instead, we allow
            // the watcherTrigger to be registered and when the access is granted for the Application at a later time,
            // the watcherTrigger will automatically start working again.

            // At this point we assume we haven't found any existing tasks matching the one we want to register
            // First, configure the watcherTrigger.

            // Default Windows will scan over the 1M PHYs.
            if (supportsCodedPhy)
            {
                if (WatcherTrigger1MAndCodedPhysCheckBox.IsChecked.Value)
                {
                    // Enable the Bluetooth adapter to also scan over 2M and Coded PHYs.
                    watcherTrigger.UseCodedPhy = true;
                    // Required in order to specify multiple scan PHYs
                    watcherTrigger.AllowExtendedAdvertisements = true;
                }
                else
                {
                    // Disable scanning over 2M and Coded PHYs.
                    watcherTrigger.UseCodedPhy = false;
                    watcherTrigger.AllowExtendedAdvertisements = false;
                }
            }

            if (supportsHardwareOffloadedFilters)
            {
                if (WatcherTriggerPerformanceOptimizationsCheckBox.IsChecked.Value)
                {
                    // Enable the Bluetooth adapter to perform a background scan with performance optimizations.
                    watcherTrigger.ScanParameters = BluetoothLEAdvertisementScanParameters.CoexistenceOptimized();
                }
                else
                {
                    // Disable scanning with performance optimizations and reset it to default low latency.
                    watcherTrigger.ScanParameters = BluetoothLEAdvertisementScanParameters.LowLatency();
                }
            }

            // Create a background task builder with the watcherTrigger and name.
            // Omitting the task entry point results in an in-process background task.
            // See App.OnBackgroundActivated for the in-process background task entry point.
            var builder = new BackgroundTaskBuilder();
            builder.SetTrigger(watcherTrigger);
            builder.Name = taskName;

            // Now perform the registration. This may throw if the system does not have a Bluetooth radio.
            try
            {
                taskRegistration = builder.Register();
            }
            catch (Exception ex) when (ex.HResult == E_DEVICE_NOT_AVAILABLE)
            {
                rootPage.NotifyUser("Cannot register background task. Maybe the system does not have a Bluetooth radio.", NotifyType.ErrorMessage);
            }

            if (taskRegistration != null)
            {
                // For this scenario, attach an event handler to display the result processed from the background task
                taskRegistration.Completed += OnBackgroundTaskCompleted;

                // Even though the watcherTrigger is registered successfully, it might be blocked. Notify the user if that is the case.
                if ((backgroundAccessStatus == BackgroundAccessStatus.AlwaysAllowed) || (backgroundAccessStatus == BackgroundAccessStatus.AllowedSubjectToSystemPolicy))
                {
                    rootPage.NotifyUser("Background watcher registered.", NotifyType.StatusMessage);
                }
                else
                {
                    rootPage.NotifyUser("Background tasks may be disabled for this app", NotifyType.ErrorMessage);
                }
            }
            UpdateButtons();
        }

        /// <summary>
        /// Invoked as an event handler when the Stop button is pressed.
        /// </summary>
        /// <param name="sender">Instance that triggered the event.</param>
        /// <param name="e">Event data describing the conditions that led to the event.</param>
        private void StopButton_Click(object sender, RoutedEventArgs e)
        {
            // Unregistering the background task will stop scanning if this is the only client requesting scan
            taskRegistration.Unregister(true);
            taskRegistration = null;
            rootPage.NotifyUser("Background watcher unregistered.", NotifyType.StatusMessage);

            UpdateButtons();
        }

        /// <summary>
        /// If we have not already found a task registration, try to find it.
        /// </summary>
        private void FindTaskRegistration()
        {
            if (taskRegistration == null)
            {
                // Look among the already-registered tasks for the one with the matching name.
                foreach (var task in BackgroundTaskRegistration.AllTasks.Values)
                {
                    if (task.Name == taskName)
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
        private void UpdateButtons()
        {
            bool isRegistered = taskRegistration != null;
            WatcherTrigger1MAndCodedPhysCheckBox.IsEnabled = supportsCodedPhy && !isRegistered;
            WatcherTriggerPerformanceOptimizationsCheckBox.IsEnabled = supportsHardwareOffloadedFilters && !isRegistered;
            RunButton.IsEnabled = !isRegistered;
            StopButton.IsEnabled = isRegistered;
        }

        /// <summary>
        /// Handle background task completion.
        /// </summary>
        /// <param name="task">The task that is reporting completion.</param>
        /// <param name="e">Arguments of the completion report.</param>
        private async void OnBackgroundTaskCompleted(BackgroundTaskRegistration task, BackgroundTaskCompletedEventArgs e)
        {
            // We get the advertisement(s) processed by the background task
            if (ApplicationData.Current.LocalSettings.Values.TryGetValue(taskName, out object backgroundMessage))
            {
                // Serialize UI update to the main UI thread
                await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    // Display these information on the list
                    ReceivedAdvertisementListBox.Items.Add(backgroundMessage);
                });
            }
        }
    }
}
