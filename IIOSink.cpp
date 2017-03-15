// Copyright (c) 2016 Fiach Antaw
// SPDX-License-Identifier: BSL-1.0

#include <Poco/Error.h>
#include <poll.h>
#include <algorithm>
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include "IIOSupport.hpp"

/***********************************************************************
 * |PothosDoc IIO Sink
 *
 * The IIO source forwards an input sample stream to an IIO output device.
 *
 * |category /IIO
 * |category /Sinks
 * |keywords iio industrial io adc sdr
 *
 * |param deviceId[Device ID] The ID of an IIO device on the system.
 * |widget StringEntry()
 * |default ""
 *
 * |param channelIds[Channel IDs] The IDs of channels to enable.
 * If no IDs are specified, all channels will be enabled.
 * |preview disable
 * |default []
 *
 * |factory /iio/sink(deviceId, channelIds)
 **********************************************************************/
class IIOSink : public Pothos::Block
{
private:
    std::unique_ptr<IIODevice> dev;
    std::unique_ptr<IIOBuffer> buf;
public:
    IIOSink(const std::string &deviceId, const std::vector<std::string> &channelIds)
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
            throw Pothos::SystemException("IIOSink::IIOSink()", "device not found");
        }

        //set up probes/setters for device attributes 
        for (auto a : this->dev->attributes())
        {
            Pothos::Callable attrGetter(&IIOSink::getDeviceAttribute);
            Pothos::Callable attrSetter(&IIOSink::setDeviceAttribute);
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
            if (!c.isOutput())
                continue;
            std::string cId = c.id();
            if (channelIds.size() > 0 && std::none_of(channelIds.begin(), channelIds.end(),
                    [cId](std::string s){ return s == cId; }))
                continue;

            c.enable();

            //set up input ports for scannable input channels
            if (c.isScanElement())
            {
                this->setupInput(c.id(), c.dtype());
                have_scan_elements = true;
            }

            //set up probes/setters for channel attributes
            for (auto a : c.attributes())
            {
                Pothos::Callable attrGetter(&IIOSink::getChannelAttribute);
                Pothos::Callable attrSetter(&IIOSink::setChannelAttribute);
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

        //create sample buffer if we've got any scan elements
        //buffer size defaults to 4096 samples per buffer, for now
        if (have_scan_elements) {
            this->buf = std::unique_ptr<IIOBuffer>(new IIOBuffer(std::move(this->dev->createBuffer(4096, false))));
            if (!this->buf)
            {
                throw Pothos::SystemException("IIOSink::IIOSink()", "buffer creation failed");
            }
            this->buf->setBlockingMode(false);
        }
    }

    static Block *make(const std::string &deviceId, const std::vector<std::string> &channelIds)
    {
        return new IIOSink(deviceId, channelIds);
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
        auto sample_count = this->workInfo().minInElements;
 
        if (this->buf) {
            //wait for samples
            struct pollfd pfd = {
                .fd = this->buf->fd(),
                .events = POLLOUT,
                .revents = 0
            };
            struct timespec ts = {
                .tv_sec = static_cast<time_t>(this->workInfo().maxTimeoutNs/10000000),
                .tv_nsec = static_cast<long int>(this->workInfo().maxTimeoutNs % 10000000)
            };
            int ret = ppoll(&pfd, 1, &ts, NULL);
            if (ret < 0)
                throw Pothos::SystemException("IIOSource::work()", "ppoll failed: " + Poco::Error::getMessage(-ret));
            else if (ret == 0)
                return this->yield();

            //write samples to outputs
            for (auto c : this->dev->channels())
            {
                if (this->allInputs().count(c.id()) > 0) {
                    auto inputPort = this->input(c.id());
                    auto inputBuffer = inputPort->buffer();

                    c.write(*this->buf, (void*)inputBuffer.address, sample_count);
                    inputPort->consume(sample_count);
                }
            }

            //push new samples to iio device
            this->buf->push(sample_count);
        }
    }
};

static Pothos::BlockRegistry registerIIOSink(
    "/iio/sink", &IIOSink::make);
