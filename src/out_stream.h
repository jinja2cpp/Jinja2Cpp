#ifndef JINJA2CPP_SRC_OUT_STREAM_H
#define JINJA2CPP_SRC_OUT_STREAM_H

#include "internal_value.h"

#include <cstddef>
#include <memory>
#include <utility>

namespace jinja2
{
class OutStream
{
public:
    struct StreamWriter
    {
        virtual ~StreamWriter() = default;

        virtual void WriteBuffer(const void* ptr, size_t length) = 0;
        virtual void WriteValue(const InternalValue& val) = 0;
    };

    // Writes through a writer that outlives the stream
    explicit OutStream(StreamWriter* writer)
        : m_writer(writer)
    {}
    // Writes through a writer the stream (and its copies) own
    explicit OutStream(std::shared_ptr<StreamWriter> writer)
        : m_writer(writer.get())
        , m_ownedWriter(std::move(writer))
    {}

    void WriteBuffer(const void* ptr, size_t length) { m_writer->WriteBuffer(ptr, length); }

    void WriteValue(const InternalValue& val) { m_writer->WriteValue(val); }

private:
    StreamWriter* m_writer;
    std::shared_ptr<StreamWriter> m_ownedWriter;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_OUT_STREAM_H
