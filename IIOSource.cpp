// Copyright (c) 2016 Fiach Antaw
// SPDX-License-Identifier: BSL-1.0

#include <Poco/Error.h>
#include <memory>
#include <string>
#include <cstring>
#include "IIOSupport.hpp"

/***********************************************************************
 * |PothosDoc IIO Source
 *
 * The IIO source forwards an IIO input device to an output sample stream.
 *
 * |category /IIO
 * |category /Sources
 * |keywords iio industrial io adc sdr
 *
 * |param deviceId[Device ID] The ID of an IIO device on the system.
 * |widget StringEntry()
 * |default ""
 *
 * |factory /iio/source(deviceId)
 **********************************************************************/
class IIOSource : public Pothos::Block
{
private:
    std::unique_ptr<IIODevice> dev;
    std::unique_ptr<IIOBuffer> buf;
public:
    IIOSource(const std::string &deviceId)
    {
        //get libiio context
        IIOContext& ctx = IIOContext::get();
        
        //find iio device
        for (auto d : ctx.devices())
        {
            if (d.id() == deviceId)
            {
                this->dev = std::unique_ptr<IIODevice>(new IIODevice(d));
                break;
            }
        }
        if (!this->dev)
        {
            throw Pothos::SystemException("IIOSource::IIOSource()", "device not found");
        }

        //set up probes/setters for device attributes 
        for (auto a : this->dev->attributes())
        {
            Pothos::Callable attrGetter(&IIOSource::getDeviceAttribute);
            Pothos::Callable attrSetter(&IIOSource::setDeviceAttribute);
            attrGetter.bind(std::ref(*this), 0);
            attrGetter.bind(a, 1);
            attrSetter.bind(std::ref(*this), 0);
            attrSetter.bind(a, 1);

            std::string getDeviceAttrName = "deviceAttribute[" + a.name() + "]";
            std::string setDeviceAttrName = "setdeviceAttribute[" + a.name() + "]";
            this->registerCallable(getDeviceAttrName, attrGetter);
            this->registerCallable(setDeviceAttrName, attrSetter);
            this->registerProbe(getDeviceAttrName);
        }

        //disable all output channels/enable all input channels
        bool have_scan_elements = false;
        for (auto c : this->dev->channels())
        {
            if (c.isOutput())
            {
                c.disable();
            }
            else
            {
                c.enable();

                //set up output ports for scannable input channels
                if (c.isScanElement())
                {
                    this->setupOutput(c.id(), c.dtype());
                    have_scan_elements = true;
                }

                //set up probes/setters for channel attributes
                for (auto a : c.attributes())
                {
                    Pothos::Callable attrGetter(&IIOSource::getChannelAttribute);
                    Pothos::Callable attrSetter(&IIOSource::setChannelAttribute);
                    attrGetter.bind(std::ref(*this), 0);
                    attrGetter.bind(a, 1);
                    attrSetter.bind(std::ref(*this), 0);
                    attrSetter.bind(a, 1);

                    std::string getChannelAttrName = "channelAttribute[" + c.id() + "][" + a.name() + "]";
                    std::string setChannelAttrName = "setChannelAttribute[" + c.id() + "][" + a.name() + "]";
                    this->registerCallable(getChannelAttrName, attrGetter);
                    this->registerCallable(setChannelAttrName, attrSetter);
                    this->registerProbe(getChannelAttrName);
                }
            }
        }

        //create sample buffer if we've got any scan elements
        //buffer size defaults to 4096 samples per buffer, for now
        if (have_scan_elements) {
            this->buf = std::unique_ptr<IIOBuffer>(new IIOBuffer(std::move(this->dev->createBuffer(4096, false))));
            if (!this->buf)
            {
                throw Pothos::SystemException("IIOSource::IIOSource()", "buffer creation failed");
            }
        }
    }

    static Block *make(const std::string &deviceId)
    {
        return new IIOSource(deviceId);
    }

    std::string getDeviceAttribute(IIOAttr<IIODevice> a)
    {
        return a.value();
    }

    void setDeviceAttribute(IIOAttr<IIODevice> a, Pothos::Object value)
    {
        a = value.toString();
    }

    std::string getChannelAttribute(IIOAttr<IIOChannel> a)
    {
        return a.value();
    }

    void setChannelAttribute(IIOAttr<IIOChannel> a, Pothos::Object value)
    {
        a = value.toString();
    }

    void work(void)
    {
        if (this->buf) {
            //get new samples from iio device
            auto bytes_read = this->buf->refill();
            //libiio read operations shouldn't return partial scans
            assert(bytes_read % this->buf->step() == 0);
            auto sample_count = bytes_read / this->buf->step();

            //write samples to output ports
            for (auto c : this->dev->channels())
            {
                if (this->allOutputs().count(c.id()) > 0) {
                    auto outputPort = this->output(c.id());
                    auto outputBuffer = outputPort->getBuffer(sample_count);

                    outputBuffer.length = c.read(*this->buf, (void*)outputBuffer.address, sample_count);
                    outputPort->postBuffer(outputBuffer);
                }
            }
        }
    }
};

static Pothos::BlockRegistry registerIIOSource(
    "/iio/source", &IIOSource::make);
