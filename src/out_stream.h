#ifndef OUT_STREAM_H
#define OUT_STREAM_H

#include "internal_value.h"
#include <functional>

namespace jinja2
{
class OutStream
{
public:
    OutStream(std::function<void (size_t, size_t)> chunkWriter, std::function<void (const InternalValue& val)> valueWriter)
        : m_chunkWriter(std::move(chunkWriter))
        , m_valueWriter(valueWriter)
    {
    }

    void WriteChunk(size_t from, size_t length)
    {
        m_chunkWriter(from, length);
    }

    void WriteValue(const InternalValue& val)
    {
        m_valueWriter(val);
    }

private:
    std::function<void (size_t, size_t)> m_chunkWriter;
    std::function<void (const InternalValue& value)> m_valueWriter;
};
} // jinja2

#endif // OUT_STREAM_H
