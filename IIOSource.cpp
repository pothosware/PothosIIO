// Copyright (c) 2016 Fiach Antaw
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Poco/Error.h>
#include <iio.h>

/***********************************************************************
 * |PothosDoc IIO Source
 *
 * The IIO source forwards an IIO input device to an output sample stream.
 *
 * |category /IIO
 * |category /Sources
 * |keywords iio industrial io adc sdr
 *
 * |param deviceName[Device Name] The name of an IIO device on the system,
 * or the integer index of an IIO device on the system.
 * |widget StringEntry()
 * |default ""
 *
 * |param channelName[Channel Name] The name of a channel on the IIO device,
 * or the integer index of a channel on the IIO device.
 * |widget StringEntry()
 * |default ""
 *
 * |factory /iio/source(deviceName, channelName)
 **********************************************************************/
class IIOSource : public Pothos::Block
{
private:
    struct iio_context *ctx;
    struct iio_channel *chn;
    struct iio_buffer *buf;
public:
    IIOSource(const std::string &deviceName, const std::string &channelName)
    {
        //create libiio context
        this->ctx = iio_create_default_context();
        if (!this->ctx)
        {
            throw Pothos::SystemException("IIOSource::IIOSource()", "iio_create_default_context: " + Poco::Error::getMessage(Poco::Error::last()));
        }
        
        //find iio device
        struct iio_device *dev = iio_context_find_device(this->ctx, deviceName.c_str());
        if (!dev)
        {
            throw Pothos::SystemException("IIOSource::IIOSource()", "iio_context_find_device: device not found");
        }

        //disable all channels
        for (unsigned int i = 0; i < iio_device_get_channels_count(dev); i++)
        {
            iio_channel_disable(iio_device_get_channel(dev, i));
        }

        //find and enable chosen iio channel
        this->chn = iio_device_find_channel(dev, channelName.c_str(), false);
        if (!this->chn)
        {
            throw Pothos::SystemException("IIOSource::IIOSource()", "iio_device_find_channel: channel not found");
        }
        iio_channel_enable(this->chn);

        //create sample buffer (default to 4096 samples per buffer, for now)
        this->buf = iio_device_create_buffer(dev, 4096, false);
        if (!this->buf)
        {
            throw Pothos::SystemException("IIOSource::IIOSource()", "iio_device_create_buffer: " + Poco::Error::getMessage(Poco::Error::last()));
        }

        //set up output port
        this->setupOutput(0, typeid(unsigned int));
    }

    ~IIOSource(void)
    {
        iio_context_destroy(this->ctx);
    }

    static Block *make(const std::string &deviceName, const std::string &channelName)
    {
        return new IIOSource(deviceName, channelName);
    }

    void work(void)
    {
        //get new samples from iio device
        int byte_count = iio_buffer_refill(this->buf);

        //calculate number of samples in buffer
        int sample_count = byte_count / iio_buffer_step(this->buf);

        //get output buffer
        auto outputPort0 = this->output(0);
        auto outputBuffer = outputPort0->getBuffer(sample_count);

        //copy samples into output buffer
        iio_channel_read(this->chn, this->buf, (void*)outputBuffer.address, outputBuffer.length);

        //push output buffer
        outputPort0->postBuffer(outputBuffer);
    }
};

static Pothos::BlockRegistry registerIIOSource(
    "/iio/source", &IIOSource::make);
