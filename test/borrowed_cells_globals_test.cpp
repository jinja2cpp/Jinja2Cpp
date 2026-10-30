// The links of a global's adapters to the cells of a render (docs/tasks/0346): a global's link is
// listed in its cell only while the thread's snapshot of the globals reaches its adapter, so that
// Finish can detach it; any other adapter of a global's data (an item the block of the global's
// adapter let go of, a map item lent afresh) owns its cell, and the host may drop it on any thread
#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace jinja2;

namespace
{
// A list of `count` lists [0]
ValuesList ListOfLists(int count)
{
    ValuesList lists;
    lists.reserve(static_cast<size_t>(count));
    for (int i = 0; i != count; ++i)
    {
        lists.emplace_back(ValuesList{ 0 });
    }
    return lists;
}

std::string RenderWithGlobal(const std::string& source, Value global)
{
    TemplateEnv env;
    env.AddGlobal("g", std::move(global));
    Template tpl(&env);
    auto loaded = tpl.Load(source);
    EXPECT_TRUE(!!loaded) << (loaded ? std::string() : loaded.error().ToString());
    auto result = tpl.RenderAsString({});
    EXPECT_TRUE(!!result) << (result ? std::string() : result.error().ToString());
    return result ? result.value() : std::string();
}

std::string Repeat(const std::string& text, int count)
{
    std::string repeated;
    for (int i = 0; i != count; ++i)
    {
        repeated += text;
    }
    return repeated;
}

// Renders `source` `renders` times with the global `g` and a callable `keep` whose argument a
// second thread drops while the render still runs (TSan: the shape of 0340 for a global's data).
// `lag`: the argument is dropped only once `keep` was called again, after the template let go
// of it, so that the host holds the last reference
void RenderWhileAnotherThreadDrops(const std::string& source, const Value& global, const std::string& expected, bool lag = false)
{
    TemplateEnv env;
    env.AddGlobal("g", global);
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load(source));
    std::mutex guard;
    std::vector<Value> queue;
    std::atomic<bool> stop{ false };
    std::thread dropper([&]() {
        while (!stop.load(std::memory_order_acquire))
        {
            std::vector<Value> mine;
            {
                const std::scoped_lock lock(guard);
                mine.swap(queue);
            }
            mine.clear();
        }
    });
#ifdef JINJA2CPP_CELL_TRACE
    const int renders = 20;
#else
    const int renders = 2000;
#endif
    Value previous;
    const ValuesMap params{ { "keep", UserCallable(
                                          [&](const UserCallableParams& p) {
                                              const std::scoped_lock lock(guard);
                                              if (lag)
                                              {
                                                  queue.push_back(std::exchange(previous, p["v"]));
                                              }
                                              else
                                              {
                                                  queue.push_back(p["v"]);
                                              }
                                              return Value();
                                          },
                                          { ArgInfo{ "v" } }) } };
    for (int i = 0; i != renders; ++i)
    {
        const auto out = tpl.RenderAsString(params);
        ASSERT_TRUE(!!out) << out.error().ToString();
        EXPECT_EQ(expected, out.value());
        const std::scoped_lock lock(guard);
        queue.push_back(std::exchange(previous, Value()));
    }
    stop.store(true, std::memory_order_release);
    dropper.join();
}
} // namespace

TEST(BorrowedCellsGlobals, SubscriptItemDroppedOnAnotherThread)
{
    // g[i] of a global with no cell of its own is a new adapter at each read, which no snapshot
    // keeps: the macro's frame lets go of `x`, written (and so linked to the item's cell) and
    // kept in a list the host holds, and the host drops it while the next call links another
    // adapter of the item. A write through g[i] itself would give g a cell, whose items the cell
    // holds
    RenderWhileAnotherThreadDrops("{% macro f(i) %}{% set x = g[299] %}{% set _ = x.append(i) %}{% set _ = keep([x]) %}{% endmacro %}"
                                  "{% for i in range(100) %}{{ f(i) }}{% endfor %}{{ g[299]|length }}",
                                  ListOfLists(300), "101", true);
}

TEST(BorrowedCellsGlobals, ItemOfAnEnumeratedBlockDroppedOnAnotherThread)
{
    // A loop over a global lends its items from blocks of 256 adapters, and the global's adapter
    // lets go of a block when it reads past it: the item kept from the first block outlives it in
    // the host's list, and the host drops it while the render links other adapters of the item
    RenderWhileAnotherThreadDrops("{% for x in g %}{% if loop.first %}{% set _ = x.append(1) %}{% set _ = keep([x]) %}{% endif %}{% endfor %}"
                                  "{% for i in range(100) %}{{ g[0]|length }}{% endfor %}",
                                  ListOfLists(300), Repeat("2", 100));
}

TEST(BorrowedCellsGlobals, MapItemDroppedOnAnotherThread)
{
    // The same for a map item of a global, also a new adapter at each read
    RenderWhileAnotherThreadDrops("{% macro f(i) %}{% set x = g.m %}{% set _ = x.append(i) %}{% set _ = keep([x]) %}{% endmacro %}"
                                  "{% for i in range(100) %}{{ f(i) }}{% endfor %}{{ g.m|length }}",
                                  Value(ValuesMap{ { "m", ValuesList{ 0 } } }), "101", true);
}

TEST(BorrowedCellsGlobals, ItemsAgreeAcrossBlocks)
{
    // Every name of a global's item sees the render's writes, whichever block lent it, and the
    // next render reads the host's data again
    const std::string source = "{% set a = g[299] %}{% set _ = a.append(1) %}{{ g[0]|length }}{{ g[299]|length }}{{ a|length }}"
                               "{% set _ = g[299].append(2) %}{{ a }}{{ g[0] }}";
    const auto global = Value(ListOfLists(300));
    EXPECT_EQ("122[0, 1, 2][0]", RenderWithGlobal(source, global));
    EXPECT_EQ("122[0, 1, 2][0]", RenderWithGlobal(source, global));
}

TEST(BorrowedCellsGlobals, MapItemsAgree)
{
    const std::string source = "{% set a = g.m %}{% set _ = a.append(1) %}{{ g.m|length }}{% set _ = g.m.append(2) %}{{ a }}{{ g.m }}";
    const auto global = Value(ValuesMap{ { "m", ValuesList{ 0 } } });
    EXPECT_EQ("2[0, 1, 2][0, 1, 2]", RenderWithGlobal(source, global));
    EXPECT_EQ("2[0, 1, 2][0, 1, 2]", RenderWithGlobal(source, global));
}

TEST(BorrowedCellsGlobals, CyclesThroughAGlobalsItemsAreFreed)
{
    // The cell of an owning link holds the adapter that holds it: the collector frees the cycle
    // when the render ends (LeakSanitizer in the address sanitizer build)
    EXPECT_EQ("2", RenderWithGlobal("{% set _ = g.m.append(g.m) %}{{ g.m|length }}", Value(ValuesMap{ { "m", ValuesList{ 0 } } })));
    EXPECT_EQ("21", RenderWithGlobal("{% set _ = g[299].append(g[299]) %}{{ g[299]|length }}{{ g[0]|length }}", Value(ListOfLists(300))));
    EXPECT_EQ("12", RenderWithGlobal("{% set x = g[299] %}{{ g[0]|length }}{% set _ = x.append(x) %}{{ x|length }}", Value(ListOfLists(300))));
}
