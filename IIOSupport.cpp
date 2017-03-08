// Copyright (c) 2016 Fiach Antaw
// SPDX-License-Identifier: BSL-1.0

#include "IIOSupport.hpp"
#include <Pothos/Framework.hpp>
#include <Poco/Error.h>
#include <cassert>

IIOContextRaw::IIOContextRaw(void)
{
    this->raw_ptr = iio_create_local_context();
    if (!this->raw_ptr)
    {
        throw Pothos::SystemException("IIOContextRaw::IIOContextRaw()", "iio_create_local_context: " + Poco::Error::getMessage(Poco::Error::last()));
    }
}

IIOContextRaw::~IIOContextRaw(void)
{
    iio_context_destroy(this->raw_ptr);
}

IIOContext::IIOContext(void) : ctx(new IIOContextRaw()) {}

IIOContext& IIOContext::get()
{
    static Poco::SingletonHolder<IIOContext> sh;
    return *sh.get();
}

std::string IIOContext::version(void)
{
    int ret;
    unsigned int major, minor;
    char git_tag[8];

    ret = iio_context_get_version(this->ctx->raw_ptr, &major, &minor, git_tag);
    if (ret)
    {
        throw Pothos::SystemException("IIOContext::getVersion()", "iio_context_get_version: " + Poco::Error::getMessage(-ret));
    }
    return std::to_string(major) + "." + std::to_string(minor) + " (" + git_tag + ")";
}

std::string IIOContext::name(void)
{
    return std::string(iio_context_get_name(this->ctx->raw_ptr));
}

std::string IIOContext::description(void)
{
    return std::string(iio_context_get_description(this->ctx->raw_ptr));
}

std::vector<IIODevice> IIOContext::devices(void)
{
    auto device_count = iio_context_get_devices_count(this->ctx->raw_ptr);
    std::vector<IIODevice> d;
    for (unsigned int i = 0; i < device_count; ++i) {
        struct iio_device *device = iio_context_get_device(this->ctx->raw_ptr, i);
        assert(device);
        d.push_back(IIODevice(this->ctx, device));
    }
    return d;
}

IIODevice::IIODevice(std::shared_ptr<IIOContextRaw> ctx, const struct iio_device *device)
    : ctx(ctx), device(device) {}

std::string IIODevice::id(void)
{
    return std::string(iio_device_get_id(this->device));
}

std::string IIODevice::name(void)
{
    const char *name = iio_device_get_name(this->device);
    if (!name) {
        name = "<unnamed>";
    }
    return std::string(name);
}

std::vector<IIOChannel> IIODevice::channels(void)
{
    auto channel_count = iio_device_get_channels_count(this->device);
    std::vector<IIOChannel> c;
    for (unsigned int i = 0; i < channel_count; ++i) {
        struct iio_channel *channel = iio_device_get_channel(this->device, i);
        assert(channel);
        c.push_back(IIOChannel(this->ctx, channel));
    }
    return c;
}

IIODevice IIODevice::trigger(void)
{
    const struct iio_device *trigger;
    int ret = iio_device_get_trigger(this->device, &trigger);
    if (ret)
    {
        throw Pothos::SystemException("IIODevice::trigger()", "iio_device_get_trigger: " + Poco::Error::getMessage(-ret));
    }
    if (!trigger)
    {
        throw Pothos::NotFoundException("IIODevice::trigger()", "Trigger not set");
    }

    return IIODevice(this->ctx, trigger);
}

void IIODevice::setTrigger(IIODevice *trigger)
{
    int ret = iio_device_set_trigger(this->device, trigger->device);
    if (ret)
    {
        throw Pothos::SystemException("IIODevice::setTrigger()", "iio_device_set_trigger: " + Poco::Error::getMessage(-ret));
    }
}

bool IIODevice::isTrigger(void)
{
    return iio_device_is_trigger(this->device);
}

void IIODevice::setKernelBuffersCount(unsigned int nb_buffers)
{
    int ret = iio_device_set_kernel_buffers_count(this->device, nb_buffers);
    if (ret)
    {
        throw Pothos::SystemException("IIODevice::setKernelBuffersCount()", "iio_device_set_kernel_buffers_count: " + Poco::Error::getMessage(-ret));
    }
}

IIOBuffer IIODevice::createBuffer(size_t samples_count, bool cyclic)
{
    return IIOBuffer(this->ctx, this, samples_count, cyclic);
}

IIOChannel::IIOChannel(std::shared_ptr<IIOContextRaw> ctx, struct iio_channel *channel) : ctx(ctx), channel(channel) {}

IIODevice IIOChannel::device(void)
{
    return IIODevice(this->ctx, iio_channel_get_device(this->channel));
}

std::string IIOChannel::id(void)
{
    return std::string(iio_channel_get_id(this->channel));
}

std::string IIOChannel::name(void)
{
    const char *name = iio_channel_get_name(this->channel);
    if (!name) {
        name = "<unnamed>";
    }
    return std::string(name);
}

void IIOChannel::enable(void)
{
    iio_channel_enable(this->channel);
}

void IIOChannel::disable(void)
{
    iio_channel_disable(this->channel);
}

bool IIOChannel::isEnabled(void)
{
    return iio_channel_is_enabled(this->channel);
}

bool IIOChannel::isOutput(void)
{
    return iio_channel_is_output(this->channel);
}

bool IIOChannel::isScanElement(void)
{
    return iio_channel_is_scan_element(this->channel);
}

size_t IIOChannel::read(IIOBuffer &buffer, void *dst, size_t sample_count)
{
    const struct iio_data_format *format = iio_channel_get_data_format(this->channel);
    size_t len = sample_count * format->length;
    return iio_channel_read(this->channel, buffer.buffer, dst, len);
}

size_t IIOChannel::write(IIOBuffer &buffer, void *dst, size_t sample_count)
{
    const struct iio_data_format *format = iio_channel_get_data_format(this->channel);
    size_t len = sample_count * (format->length / 8);
    return iio_channel_write(this->channel, buffer.buffer, dst, len);
}

Pothos::DType IIOChannel::dtype(void)
{
    const struct iio_data_format *format = iio_channel_get_data_format(this->channel);

    switch(format->length) {
        case 8:
            if (format->is_signed) {
                return Pothos::DType(typeid(int8_t));
            } else {
                return Pothos::DType(typeid(uint8_t));
            }
        case 16:
            if (format->is_signed) {
                return Pothos::DType(typeid(int16_t));
            } else {
                return Pothos::DType(typeid(uint16_t));
            }
        case 32:
            if (format->is_signed) {
                return Pothos::DType(typeid(int32_t));
            } else {
                return Pothos::DType(typeid(uint32_t));
            }
        case 64:
            if (format->is_signed) {
                return Pothos::DType(typeid(int64_t));
            } else {
                return Pothos::DType(typeid(uint64_t));
            }
        default:
            return Pothos::DType(typeid(char), format->length / 8);
    }
}

IIOBuffer::IIOBuffer(std::shared_ptr<IIOContextRaw> ctx, IIODevice *device, size_t samples_count, bool cyclic)
    : ctx(ctx)
{
    this->buffer = iio_device_create_buffer(device->device, samples_count, cyclic);
    if (!this->buffer)
    {
        throw Pothos::SystemException("IIOBuffer::IIOBuffer()", "iio_device_create_buffer: " + Poco::Error::getMessage(Poco::Error::last()));
    }
}

IIOBuffer::IIOBuffer(IIOBuffer&& other)
    : ctx(std::move(other.ctx))
{
    this->buffer = other.buffer;
    other.buffer = nullptr;
}

IIOBuffer::~IIOBuffer(void)
{
    if (this->buffer) {
        iio_buffer_destroy(this->buffer);
    }
}

size_t IIOBuffer::refill(void)
{
    ssize_t ret = iio_buffer_refill(this->buffer);
    if (ret < 0)
    {
        throw Pothos::SystemException("IIOBuffer::refill()", "iio_buffer_refill: " + Poco::Error::getMessage(-ret));
    }
    return (size_t)ret;
}

size_t IIOBuffer::push(size_t samples_count)
{
    ssize_t ret = iio_buffer_push_partial(this->buffer, samples_count);
    if (ret < 0)
    {
        throw Pothos::SystemException("IIOBuffer::push()", "iio_buffer_push_partial: " + Poco::Error::getMessage(-ret));
    }
    return (size_t)ret;
}

void * IIOBuffer::start(void)
{
    return iio_buffer_start(this->buffer);
}

void * IIOBuffer::end(void)
{
    return iio_buffer_end(this->buffer);
}

ptrdiff_t IIOBuffer::step(void)
{
    return iio_buffer_step(this->buffer);
}
