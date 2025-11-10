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
    public sealed partial class Scenario4_BackgroundPublisher : Page
    {
        // A pointer back to the main page is required to display status messages.
        private MainPage rootPage = MainPage.Current;

        // The background task registration for the background advertisement publisher
        private IBackgroundTaskRegistration taskRegistration;

        // The publisher trigger used to configure the background task registration
        private BluetoothLEAdvertisementPublisherTrigger publisherTrigger;

        // Capability of the Bluetooth radio adapter.
        private bool supportsAdvertisingInDifferentPhy = false;

        // A name is given to the task in order for it to be identifiable across context.
        private string taskName = nameof(AdvertisementPublisherTask);

        const int E_DEVICE_NOT_AVAILABLE = unchecked((int)0x800710df); // HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_AVAILABLE)

        public Scenario4_BackgroundPublisher()
        {
            InitializeComponent();

            // Create and initialize a new publisherTrigger to configure it.
            publisherTrigger = new BluetoothLEAdvertisementPublisherTrigger();

            // We need to add some payload to the advertisement. A publisher without any payload
            // or with invalid ones cannot be started. We only need to configure the payload once
            // for any publisher.

            // Add a manufacturer-specific section:
            // First, create a manufacturer data section
            var manufacturerData = new BluetoothLEManufacturerData();

            // Then, set the company ID for the manufacturer data. Here we picked an unused value: 0xFFFE
            manufacturerData.CompanyId = 0xFFFE;

            // Finally set the data payload within the manufacturer-specific section
            // Here, use a 16-bit UUID: 0x1234 -> {0x34, 0x12} (little-endian)
            UInt16 uuidData = 0x1234;

            // Make sure that the buffer length can fit within an advertisement payload. Otherwise you will get an exception.
            manufacturerData.Data = CryptographicBuffer.CreateFromByteArray(BitConverter.GetBytes(uuidData));

            // Add the manufacturer data to the advertisement publisher:
            publisherTrigger.Advertisement.ManufacturerData.Add(manufacturerData);

            // Display the information about the published payload
            PublisherPayloadBlock.Text = $"Published payload information: {Utilities.FormatManufacturerData(manufacturerData)}";

            // Reset the displayed status of the publisher
            PublisherStatusBlock.Text = "";
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
            rootPage = MainPage.Current;

            // If there is an existing task registration, subscribe to its completion event.
            FindTaskRegistration();
            if (taskRegistration != null)
            {
                taskRegistration.Completed += OnBackgroundTaskCompleted;
            }
            UpdateButtons();

            // Attach handlers for suspension to stop the publisherTrigger when the App is suspended.
            App.Current.Suspending += App_Suspending;
            App.Current.Resuming += App_Resuming;

            // Check whether the local Bluetooth adapter supports 2M and Coded PHY.
            if (FeatureDetection.AreExtendedAdvertisingPhysAndScanParametersSupported)
            {
                var adapter = await BluetoothAdapter.GetDefaultAsync();

                if (adapter != null)
                {
                    supportsAdvertisingInDifferentPhy = adapter.IsLowEnergyUncoded2MPhySupported && adapter.IsLowEnergyCodedPhySupported;
                }
                if (!supportsAdvertisingInDifferentPhy)
                {
                    PublisherTrigger2MAndCodedPhysReasonRun.Text = "(Not supported by default Bluetooth adapter)";
                }
            }
            else
            {
                PublisherTrigger2MAndCodedPhysReasonRun.Text = "(Not supported by this version of Windows)";
            }

            UpdateButtons();
        }

        /// <summary>
        /// Invoked immediately before the Page is unloaded and is no longer the current source of a parent Frame.
        /// </summary>
        /// <param name="e">
        /// Event data that can be examined by overriding code. The event data is representative
        /// of the navigation that will unload the current Page unless canceled. The
        /// navigation can potentially be canceled by setting Cancel.
        /// </param>
        protected override void OnNavigatingFrom(NavigatingCancelEventArgs e)
        {
            // Remove local suspension handlers from the App since this page is no longer active.
            App.Current.Suspending -= App_Suspending;
            App.Current.Resuming -= App_Resuming;

            // Since the publisher is registered in the background, the background task will be triggered when the App is closed
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
        /// <param name="sender">The source of the suspend request.</param>
        /// <param name="e">Details about the suspend request.</param>
        private void App_Suspending(object sender, Windows.ApplicationModel.SuspendingEventArgs e)
        {
            if (taskRegistration != null)
            {
                // Always unregister the handlers to release the resources to prevent leaks.
                taskRegistration.Completed -= OnBackgroundTaskCompleted;
            }
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
            // Applications registering for background publisherTrigger must request for permission.
            BackgroundAccessStatus backgroundAccessStatus = await BackgroundExecutionManager.RequestAccessAsync();
            // Here, we do not fail the registration even if the access is not granted. Instead, we allow
            // the publisherTrigger to be registered and when the access is granted for the Application at a later time,
            // the publisherTrigger will automatically start working again.

            // First, configure the publisherTrigger.
            if (supportsAdvertisingInDifferentPhy)
            {
                // By default, the BT radio uses the 1M PHY primary/1M PHY secondary configuration,
                // which matches the Windows default configuration.
                // If both coded and 2M PHYs are supported. Use the preferred configuration.
                if (PublisherTrigger2MAndCodedPhysCheckBox.IsChecked.Value)
                {
                    publisherTrigger.UseExtendedFormat = true;
                    publisherTrigger.PrimaryPhy = BluetoothLEAdvertisementPhyType.CodedPhy;
                    publisherTrigger.SecondaryPhy = BluetoothLEAdvertisementPhyType.Uncoded2MPhy;
                }
                else
                {
                    publisherTrigger.UseExtendedFormat = false;
                    publisherTrigger.PrimaryPhy = BluetoothLEAdvertisementPhyType.Uncoded1MPhy;
                    publisherTrigger.SecondaryPhy = BluetoothLEAdvertisementPhyType.Uncoded1MPhy;
                }
            }

            // Create a background task builder with the watcherTrigger and name.
            // Omitting the task entry point results in an in-process background task.
            // See App.OnBackgroundActivated for the in-process background task entry point.
            var builder = new BackgroundTaskBuilder();
            builder.SetTrigger(publisherTrigger);
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

                // Even though the publisherTrigger is registered successfully, it might be blocked. Notify the user if that is the case.
                if ((backgroundAccessStatus == BackgroundAccessStatus.AlwaysAllowed) || (backgroundAccessStatus == BackgroundAccessStatus.AllowedSubjectToSystemPolicy))
                {
                    rootPage.NotifyUser("Background publisher registered.", NotifyType.StatusMessage);
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
            // Unregistering the background task will stop advertising if this is the only client requesting
            taskRegistration.Unregister(true);
            taskRegistration = null;
            rootPage.NotifyUser("Background publisher unregistered.", NotifyType.StatusMessage);

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
            PublisherTrigger2MAndCodedPhysCheckBox.IsEnabled = supportsAdvertisingInDifferentPhy && !isRegistered;
            RunButton.IsEnabled = !isRegistered;
            StopButton.IsEnabled = isRegistered;
        }

        /// <summary>
        /// Handle background task completion.
        /// </summary>
        /// <param name="task">The task that is reporting completion.</param>
        /// <param name="e">Arguments of the completion report.</param>
        private async void OnBackgroundTaskCompleted(BackgroundTaskRegistration task, BackgroundTaskCompletedEventArgs eventArgs)
        {
            // We get the status changed processed by the background task
            if (ApplicationData.Current.LocalSettings.Values.TryGetValue(taskName, out object o) &&
                o is string backgroundMessage)
            {
                // Serialize UI update to the main UI thread
                await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    // Display the status change
                    PublisherStatusBlock.Text = backgroundMessage;
                });
            }
        }
    }
}
