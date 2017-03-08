// Copyright (c) 2015-2016 Josh Blum
// Copyright (c) 2016 Fiach Antaw
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Plugin.hpp>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Error.h>
#include <string>
#include "IIOSupport.hpp"

static Poco::JSON::Object::Ptr getIIOChannelInfo(IIOChannel chn)
{
    Poco::JSON::Object::Ptr infoObject = new Poco::JSON::Object();

    // Channel info
    infoObject->set("ID", chn.id());
    infoObject->set("Name", chn.name());
    infoObject->set("Is Scan Element", chn.isScanElement() ? "true" : "false");
    infoObject->set("Direction", chn.isOutput() ? "Output" : "Input");

    return infoObject;
}

static Poco::JSON::Object::Ptr getIIODeviceInfo(IIODevice dev)
{
    Poco::JSON::Object::Ptr infoObject = new Poco::JSON::Object();

    // Device info
    infoObject->set("Device ID", dev.id());
    infoObject->set("Device Name", dev.name());
    infoObject->set("Is Trigger", dev.isTrigger() ? "true" : "false");

    // Channels
    Poco::JSON::Array::Ptr chanArray = new Poco::JSON::Array();
    infoObject->set("Channels", chanArray);
    for (auto c : dev.channels())
    {
        chanArray->add(getIIOChannelInfo(c));
    }

    return infoObject;
}

static Poco::JSON::Object::Ptr enumerateIIODevices(void)
{
    Poco::JSON::Object::Ptr topObject = new Poco::JSON::Object();
    IIOContext& ctx = IIOContext::get();

    Poco::JSON::Array::Ptr devicesArray = new Poco::JSON::Array();
    topObject->set("IIO Devices", devicesArray);
    for (auto d : ctx.devices())
    {
        devicesArray->add(getIIODeviceInfo(d));
    }

    topObject->set("IIO Version", ctx.version());
    topObject->set("IIO Context Name", ctx.name());
    topObject->set("IIO Context Description", ctx.description());

    return topObject;
}

pothos_static_block(registerIIOInfo)
{
    Pothos::PluginRegistry::addCall(
        "/devices/iio/info", &enumerateIIODevices);
}
