﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include "pch.h"
#include <wil/win32_helpers.h>
#include <iostream>
#include <winrt/Windows.ApplicationModel.Background.h>
#include <testdef.h>
#include "Helpers.h"

using namespace winrt::Microsoft::Windows::AppLifecycle;
using namespace winrt::Microsoft::Windows::PushNotifications;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::ApplicationModel::Background; // BackgroundTask APIs
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;

winrt::Windows::Foundation::IAsyncOperation<PushNotificationChannel> RequestChannelAsync()
{
    // To obtain an AAD RemoteIdentifier for your app,
    // follow the instructions on https://docs.microsoft.com/azure/active-directory/develop/quickstart-register-app
    auto channelOperation = PushNotificationManager::CreateChannelAsync(
        winrt::guid("ccd2ae3f-764f-4ae3-be45-9804761b28b2"));

    // Setup the inprogress event handler
    channelOperation.Progress(
        [](auto&& sender, auto&& args)
        {
            if (args.status == PushNotificationChannelStatus::InProgress)
            {
                // This is basically a noop since it isn't really an error state
                std::cout << "Channel request is in progress." << std::endl << std::endl;
            }
            else if (args.status == PushNotificationChannelStatus::InProgressRetry)
            {
                LOG_HR_MSG(
                    args.extendedError,
                    "The channel request is in back-off retry mode because of a retryable error! Expect delays in acquiring it. RetryCount = %d",
                    args.retryCount);
            }
        });

    auto result = co_await channelOperation;

    if (result.Status() == PushNotificationChannelStatus::CompletedSuccess)
    {
        auto channelUri = result.Channel().Uri();

        std::cout << "channelUri: " << winrt::to_string(channelUri.ToString()) << std::endl << std::endl;

        auto channelExpiry = result.Channel().ExpirationTime();

        // Register Push Event for Foreground
        winrt::event_token token = result.Channel().PushReceived([](const auto&, PushNotificationReceivedEventArgs const& args)
            {
                auto payload = args.Payload();

                // Do stuff to process the raw payload
                std::string payloadString(payload.begin(), payload.end());
                std::cout << "Push notification content received from FOREGROUND: " << payloadString << std::endl << std::endl;
                args.Handled(true);
            });
        // Caller's responsibility to keep the channel alive
        co_return result.Channel();
    }
    else if (result.Status() == PushNotificationChannelStatus::CompletedFailure)
    {
        LOG_HR_MSG(result.ExtendedError(), "We hit a critical non-retryable error with channel request!");
        co_return nullptr;
    }
    else
    {
        LOG_HR_MSG(result.ExtendedError(), "Some other failure occurred.");
        co_return nullptr;
    }

};

winrt::Microsoft::Windows::PushNotifications::PushNotificationChannel RequestChannel()
{
    auto task = RequestChannelAsync();
    if (task.wait_for(std::chrono::seconds(300)) != AsyncStatus::Completed)
    {
        task.Cancel();
        return nullptr;
    }

    auto result = task.GetResults();
    return result;
}

int main()
{
    RETURN_IF_FAILED(BootstrapInitialize());

    PushNotificationChannel channel = RequestChannel();
    printf("Press 'Enter' at any time to exit App.");
    std::cin.ignore();

    BootstrapShutdown();
    return 0;
}