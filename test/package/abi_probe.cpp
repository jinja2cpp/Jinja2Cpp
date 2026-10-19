// Negative link check for the inline ABI namespace (docs/tasks/0076). No Jinja2C++ header is
// included: the declaration below is what a program compiled against the 1.x headers refers
// to. Built with PROBE_V2 it names jinja2::v2::operator== and must link and run; without it,
// it names the 1.x jinja2::operator==, which the 2.x library must not define, so the link
// has to fail (test/package/CMakeLists.txt).
#ifdef PROBE_V2
namespace jinja2
{
inline namespace v2
{
class Value;
bool operator==(const Value& lhs, const Value& rhs);
} // namespace v2
} // namespace jinja2
#else
namespace jinja2
{
class Value;
bool operator==(const Value& lhs, const Value& rhs);
} // namespace jinja2
#endif

int main()
{
    // volatile keeps the reference to the library symbol in the object file
    bool (*volatile compare)(const jinja2::Value&, const jinja2::Value&) = &jinja2::operator==;
    return compare ? 0 : 1;
}
