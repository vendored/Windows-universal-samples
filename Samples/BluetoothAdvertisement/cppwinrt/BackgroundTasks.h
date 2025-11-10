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

#include "AdvertisementWatcherTask.g.h"
#include "AdvertisementPublisherTask.g.h"

namespace winrt::SDKTemplate::implementation
{
    struct AdvertisementWatcherTask : AdvertisementWatcherTaskT<AdvertisementWatcherTask>
    {
        AdvertisementWatcherTask() = default;

        void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance const& taskInstance);
        Windows::ApplicationModel::Background::IBackgroundTaskInstance watcherTaskInstance{ nullptr };
    };

    struct AdvertisementPublisherTask : AdvertisementPublisherTaskT<AdvertisementPublisherTask>
    {
        AdvertisementPublisherTask() = default;

        void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance const& taskInstance);
        Windows::ApplicationModel::Background::IBackgroundTaskInstance publisherTaskInstance{ nullptr };
    };
}

namespace winrt::SDKTemplate::factory_implementation
{
    struct AdvertisementWatcherTask : AdvertisementWatcherTaskT<AdvertisementWatcherTask, implementation::AdvertisementWatcherTask>
    {
    };

    struct AdvertisementPublisherTask : AdvertisementPublisherTaskT<AdvertisementPublisherTask, implementation::AdvertisementPublisherTask>
    {
    };
}
