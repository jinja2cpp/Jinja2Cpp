#include "out_stream.h"

#include "recursion_guard.h"

#include <jinja2cpp/template.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

// The pieces of a render that writes each piece (RenderOptions::writeEachPiece). Apart from out_stream.cpp: in
// one file with the usual sink path, the compiler takes the gathering sink as the likely target of the host
// sink's virtual Write, and every sink write pays the check (RenderSink benchmarks)
namespace jinja2
{
inline namespace v2
{
// The piece a render that writes each piece (RenderOptions::writeEachPiece) has gathered so far, and the bytes of
// it charged to the render's memory budget. It is the stream's sink until the piece ends, so that a piece
// reaches the host's sink in one Write however long it is, as Jinja2's generate() yields it. The stream counts
// the bytes as written when they are gathered, as the limit on the output (Settings::Limits::maxOutputSize)
// and the offsets of the generation spans read the count
class OutStream::HeldPiece final
    : public OutputSink
    , public OutputSinkW
{
public:
    explicit HeldPiece(void* sink)
        : m_sink(sink)
    {
    }
    HeldPiece(const HeldPiece&) = delete;
    HeldPiece(HeldPiece&&) = delete;
    HeldPiece& operator=(const HeldPiece&) = delete;
    HeldPiece& operator=(HeldPiece&&) = delete;
    // Gives the budget back what a render that failed left gathered
    ~HeldPiece() { ReleaseRenderMemory(m_charged); }

    void Write(std::string_view text) override { Gather(text.data(), text.size()); }
    void Write(std::wstring_view text) override { Gather(text.data(), text.size() * sizeof(wchar_t)); }

    // The host's sink, OutputSink or OutputSinkW by the stream's character size
    [[nodiscard]] void* Sink() const { return m_sink; }
    // The gathered piece, which the held piece no longer holds or charges
    std::string Take()
    {
        ReleaseRenderMemory(std::exchange(m_charged, 0));
        return std::exchange(m_bytes, std::string());
    }
    [[nodiscard]] bool Empty() const { return m_bytes.empty(); }

private:
    // Charged before the piece grows: a piece past the budget fails the render, the last one too
    void Gather(const void* ptr, size_t bytes)
    {
        if (RenderMemoryCounted())
        {
            ChargeRenderMemory(bytes);
            m_charged += bytes;
        }
        m_bytes.append(static_cast<const char*>(ptr), bytes);
    }

    void* m_sink;
    std::string m_bytes;
    std::uint64_t m_charged = 0;
};

void OutStream::HeldPieceDelete::operator()(HeldPiece* held) const noexcept
{
    delete held;
}

OutStream::HeldPieceOwner OutStream::WriteEachPiece()
{
    m_eachPiece = true;
    // Only a sink gets pieces: a string target takes the output as it comes
    if (!m_sink)
    {
        return {};
    }
    HeldPieceOwner held(new HeldPiece(m_sink));
    m_held = held.get();
    m_sink = m_charSize == 1 ? static_cast<void*>(static_cast<OutputSink*>(m_held)) : static_cast<void*>(static_cast<OutputSinkW*>(m_held));
    return held;
}

void OutStream::FlushStatement()
{
    Flush();
    if (m_held && !m_holding)
    {
        EndPiece(false);
    }
}

void OutStream::EndPieces()
{
    if (m_held)
    {
        EndPiece(false);
    }
}

void OutStream::FlushPiece()
{
    if (m_writer || m_holding)
    {
        return;
    }
    FlushBuffer();
    if (m_held)
    {
        EndPiece(true);
    }
}

// The piece gathered goes to the host's sink in one Write; an empty one only when `writeEmpty`. Counted when
// gathered
void OutStream::EndPiece(bool writeEmpty)
{
    if (m_held->Empty() && !writeEmpty)
    {
        return;
    }
    const std::string piece = m_held->Take();
    if (m_charSize == 1)
    {
        static_cast<OutputSink*>(m_held->Sink())->Write(piece);
        return;
    }
    // Copied out of the byte string, whose buffer need not be aligned for wchar_t (libc++'s short string)
    std::wstring wide(piece.size() / sizeof(wchar_t), L'\0');
    std::memcpy(wide.data(), piece.data(), wide.size() * sizeof(wchar_t));
    static_cast<OutputSinkW*>(m_held->Sink())->Write(wide);
}

} // namespace v2
} // namespace jinja2
