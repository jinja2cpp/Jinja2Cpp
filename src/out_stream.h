#ifndef OUT_STREAM_H
#define OUT_STREAM_H

#include "internal_value.h"
#include <functional>

namespace jinja2
{
class OutStream
{
public:
    OutStream(std::function<void (const void*, size_t)> chunkWriter, std::function<void (const InternalValue& val)> valueWriter)
        : m_bufferWriter(std::move(chunkWriter))
        , m_valueWriter(valueWriter)
    {
    }

    void WriteBuffer(const void* ptr, size_t length)
    {
        m_bufferWriter(ptr, length);
    }

    void WriteValue(const InternalValue& val)
    {
        m_valueWriter(val);
    }

private:
    std::function<void (const void*, size_t)> m_bufferWriter;
    std::function<void (const InternalValue& value)> m_valueWriter;
};
} // jinja2

#endif // OUT_STREAM_H
