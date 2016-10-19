// Copyright (c) 2015-2016 Josh Blum
// Copyright (c) 2016 Fiach Antaw
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Plugin.hpp>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Error.h>
#include <string>
#include <iio.h>

static std::string getIIOVersion(const struct iio_context* ctx)
{
    int ret;
    unsigned int major, minor;
    char git_tag[8];
    
    ret = iio_context_get_version(ctx, &major, &minor, git_tag);
    if (ret)
    {
        throw Pothos::SystemException("getIIOVersion()", "iio_context_get_version: " + Poco::Error::getMessage(-ret));
    }
    return std::to_string(major) + "." + std::to_string(minor) + " (" + git_tag + ")";
}

static int readAllChnAttr_cb(struct iio_channel *, const char *attr, const char *value, size_t, void *d)
{
    Poco::JSON::Array* attrArray = (Poco::JSON::Array*)d;
    Poco::JSON::Object::Ptr attrObject = new Poco::JSON::Object();

    attrObject->set("Name", std::string(attr));
    attrObject->set("Value", std::string(value));

    attrArray->add(attrObject);
    return 0;
}

static Poco::JSON::Object::Ptr getIIOChannelInfo(struct iio_channel* chn)
{
    Poco::JSON::Object::Ptr infoObject = new Poco::JSON::Object();

    // Channel info
    infoObject->set("ID", iio_channel_get_id(chn));
    auto chn_name = iio_channel_get_name(chn);
    if (chn_name)
    {
        infoObject->set("Name", chn_name);
    }
    infoObject->set("Type", std::to_string(iio_channel_get_type(chn)));
    infoObject->set("Modifier Type", std::to_string(iio_channel_get_modifier(chn)));
    infoObject->set("Is Enabled", iio_channel_is_enabled(chn));
    infoObject->set("Is Scan Element", iio_channel_is_scan_element(chn));
    infoObject->set("Direction", iio_channel_is_output(chn) ? "Output" : "Input");

    // Attributes
    Poco::JSON::Array::Ptr attrArray = new Poco::JSON::Array();
    infoObject->set("Attributes", attrArray);
    int ret = iio_channel_attr_read_all(chn, readAllChnAttr_cb, attrArray);
    if (ret)
    {
        throw Pothos::SystemException("getIIOChannelInfo()", "iio_channel_attr_read_all: " + Poco::Error::getMessage(-ret));
    }

    return infoObject;
}

static int readAllDevAttr_cb(struct iio_device *, const char *attr, const char *value, size_t, void *d)
{
    Poco::JSON::Array* attrArray = (Poco::JSON::Array*)d;
    Poco::JSON::Object::Ptr attrObject = new Poco::JSON::Object();

    attrObject->set("Name", std::string(attr));
    attrObject->set("Value", std::string(value));

    attrArray->add(attrObject);
    return 0;
}

static Poco::JSON::Object::Ptr getIIODeviceInfo(struct iio_device* dev)
{
    Poco::JSON::Object::Ptr infoObject = new Poco::JSON::Object();

    // Device info
    infoObject->set("Device ID", std::string(iio_device_get_id(dev)));
    auto dev_name = iio_device_get_name(dev);
    if (dev_name)
    {
        infoObject->set("Device Name", std::string(dev_name));
    }
    infoObject->set("Is Trigger", iio_device_is_trigger(dev));

    // Trigger info
    const struct iio_device *trigger_dev;
    int ret = iio_device_get_trigger(dev, &trigger_dev);
    if (!ret && trigger_dev)
    {
        infoObject->set("Trigger ID", std::string(iio_device_get_id(trigger_dev)));
    }

    // Attributes
    Poco::JSON::Array::Ptr attrArray = new Poco::JSON::Array();
    infoObject->set("Attributes", attrArray);
    ret = iio_device_attr_read_all(dev, readAllDevAttr_cb, attrArray);
    if (ret)
    {
        throw Pothos::SystemException("getIIODeviceInfo()", "iio_device_attr_read_all: " + Poco::Error::getMessage(-ret));
    }
    Poco::JSON::Array::Ptr dbgAttrArray = new Poco::JSON::Array();
    infoObject->set("Debug Attributes", dbgAttrArray);
    ret = iio_device_debug_attr_read_all(dev, readAllDevAttr_cb, dbgAttrArray);
    if (ret)
    {
        throw Pothos::SystemException("getIIODeviceInfo()", "iio_device_debug_attr_read_all: " + Poco::Error::getMessage(-ret));
    }

    // Channels
    Poco::JSON::Array::Ptr chanArray = new Poco::JSON::Array();
    infoObject->set("Channels", chanArray);
    for (unsigned int i = 0; i < iio_device_get_channels_count(dev); i++)
    {
        chanArray->add(getIIOChannelInfo(iio_device_get_channel(dev, i)));
    }

    return infoObject;
}

static Poco::JSON::Object::Ptr enumerateIIODevices(void)
{
    Poco::JSON::Object::Ptr topObject = new Poco::JSON::Object();
    struct iio_context *ctx = iio_create_default_context();
    if (!ctx)
    {
            throw Pothos::SystemException("enumerateIIODevices()", "iio_create_default_context: " + Poco::Error::getMessage(Poco::Error::last()));
    }

    Poco::JSON::Array::Ptr devicesArray = new Poco::JSON::Array();
    topObject->set("IIO Devices", devicesArray);
    for (unsigned int i = 0; i < iio_context_get_devices_count(ctx); i++)
    {
        devicesArray->add(getIIODeviceInfo(iio_context_get_device(ctx, i)));
    }

    topObject->set("IIO Version", getIIOVersion(ctx));
    topObject->set("IIO Context Name", std::string(iio_context_get_name(ctx)));
    topObject->set("IIO Context Description", std::string(iio_context_get_description(ctx)));

    iio_context_destroy(ctx);
    return topObject;
}

pothos_static_block(registerIIOInfo)
{
    Pothos::PluginRegistry::addCall(
        "/devices/iio/info", &enumerateIIODevices);
}
