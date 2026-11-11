// Structure-aware mutation for the template fuzz targets (docs/tasks/0095).
//
// Byte mutants of a template mostly break its syntax and stop in the lexer or the parser's error paths. This mutator
// splits the template into text, `{{ }}`, `{% %}` and `{# #}` pieces and mutates at that level: it duplicates, swaps,
// moves and deletes whole tags, replaces an expression with another (from the input, the crossover partner or a
// pool), wraps a run of pieces in a statement, inserts statements and closes the ones left open. Part of the time it
// hands a single piece, or the whole input, to the byte mutator, so byte-level coverage is kept.
//
// The fuzz entry points define LLVMFuzzerCustomMutator and LLVMFuzzerCustomCrossOver with
// JINJA2CPP_FUZZ_TEMPLATE_MUTATOR. The byte mutator is passed in (LLVMFuzzerMutate under libFuzzer), so this file
// builds into jinja2cpp_fuzz_replay too, whose --mutate mode runs the mutator under every compiler.
#ifndef JINJA2CPP_FUZZ_TEMPLATE_MUTATOR_H
#define JINJA2CPP_FUZZ_TEMPLATE_MUTATOR_H

#include <cstddef>
#include <cstdint>

namespace jinja2_fuzz
{

// How a target lays out its input: `prefixLength` bytes before the template, the settings byte (fuzz_common.h
// settings_byte, which picks the delimiters) at `settingsOffset`, or none when it is negative
struct InputLayout
{
    std::size_t prefixLength = 0;
    int settingsOffset = -1;
};

constexpr InputLayout PlainTemplate{ 0, -1 };
constexpr InputLayout SettingsPrefix{ 1, 0 };
constexpr InputLayout LoaderPrefix{ 2, 0 };
constexpr InputLayout AllocFailurePrefix{ 3, 2 };

// LLVMFuzzerMutate's signature: mutates data[0, size) in place within maxSize bytes, returns the new size
using ByteMutator = std::size_t (*)(std::uint8_t* data, std::size_t size, std::size_t maxSize);

// The libFuzzer custom mutator: mutates data[0, size) in place within maxSize bytes and returns the new size
std::size_t MutateTemplate(std::uint8_t* data, std::size_t size, std::size_t maxSize, unsigned seed, const InputLayout& layout, ByteMutator bytes);

// The libFuzzer custom crossover: writes a mix of the two inputs to out (at most maxOutSize bytes), returns its size
// or 0 when there is nothing to mix
std::size_t CrossOverTemplates(const std::uint8_t* data1,
                               std::size_t size1,
                               const std::uint8_t* data2,
                               std::size_t size2,
                               std::uint8_t* out,
                               std::size_t maxOutSize,
                               unsigned seed,
                               const InputLayout& layout);

// JINJA2CPP_FUZZ_MUTATOR=bytes turns the structure-aware mutation off (MutateTemplate calls the byte mutator only and
// CrossOverTemplates leaves crossover to libFuzzer's own), for comparing the two in one time budget
bool StructureMutationEnabled();

} // namespace jinja2_fuzz

// Defines the libFuzzer custom mutator and crossover of a target whose input has the given InputLayout
#define JINJA2CPP_FUZZ_TEMPLATE_MUTATOR(layout) \
    extern "C" std::size_t LLVMFuzzerMutate(std::uint8_t* data, std::size_t size, std::size_t maxSize); \
    extern "C" std::size_t LLVMFuzzerCustomMutator(std::uint8_t* data, std::size_t size, std::size_t maxSize, unsigned int seed) \
    { \
        return jinja2_fuzz::MutateTemplate(data, size, maxSize, seed, layout, &LLVMFuzzerMutate); \
    } \
    extern "C" std::size_t LLVMFuzzerCustomCrossOver( \
        const std::uint8_t* data1, std::size_t size1, const std::uint8_t* data2, std::size_t size2, std::uint8_t* out, std::size_t maxOutSize, \
        unsigned int seed) \
    { \
        return jinja2_fuzz::CrossOverTemplates(data1, size1, data2, size2, out, maxOutSize, seed, layout); \
    }

#endif // JINJA2CPP_FUZZ_TEMPLATE_MUTATOR_H
