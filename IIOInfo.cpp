// Copyright (c) 2015-2016 Josh Blum
// Copyright (c) 2016 Fiach Antaw
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Plugin.hpp>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Error.h>
#include <string>
#include "IIOSupport.hpp"

#include <typeinfo>

#include <json.hpp>
using json = nlohmann::json;

static json getIIOChannelInfo(IIOChannel chn)
{
    json infoObject;

    // Channel info
    infoObject["ID"] = chn.id();
    infoObject["Name"] = chn.name();
    infoObject["Is Scan Element"] = chn.isScanElement() ? "true" : "false";
    infoObject["Direction"] = chn.isOutput() ? "Output" : "Input";

    // Channel attributes
    auto &attrArray = infoObject["Attributes"];
    for (auto a : chn.attributes())
    {
        json attrObject;
        attrObject["Name"] = a.name();
        attrObject["Value"] = a.value();
        attrArray.push_back(attrObject);
    }

    return infoObject;
}

static json getIIODeviceInfo(IIODevice dev)
{
    json infoObject = new Poco::JSON::Object();

    // Device info
    infoObject["Device ID"] = dev.id();
    infoObject["Device Name"] = dev.name();
    infoObject["Is Trigger"] = dev.isTrigger() ? "true" : "false";

    // Device attributes
    auto &attrArray = infoObject["Attributes"];
    for (auto a : dev.attributes())
    {
        json attrObject;
        attrObject["Name"] = a.name();
        attrObject["Value"] = a.value();
        attrArray.push_back(attrObject);
    }

    // Channels
    auto &chanArray = infoObject["Channels"];
    for (auto c : dev.channels())
    {
        chanArray.push_back(getIIOChannelInfo(c));
    }

    return infoObject;
}

static std::string enumerateIIODevices(void)
{
    json topObject;
    IIOContext& ctx = IIOContext::get();

    auto &devicesArray = topObject["IIO Devices"];
    for (auto d : ctx.devices())
    {
        devicesArray.push_back(getIIODeviceInfo(d));
    }

    topObject["IIO Version"] = ctx.version();
    topObject["IIO Context Name"] = ctx.name();
    topObject["IIO Context Description"] = ctx.description();

    return topObject.dump();
}

pothos_static_block(registerIIOInfo)
{
    Pothos::PluginRegistry::addCall(
        "/devices/iio/info", &enumerateIIODevices);
}
