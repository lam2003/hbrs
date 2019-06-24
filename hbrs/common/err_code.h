#pragma once

#include <system_error>

enum err_code
{
    KSuccess = 0,
    KInitialized = -1,
    KSDKError = -2,
    KUnInitialized = -3,
    KParamsError = -4,
    KSystemError = -5
};
