// A failed CRT assertion, abort() or an unhandled crash in an MSVC Debug test run opens a dialog box that no one
// answers on a CI runner, so the job hangs until its time limit with no output. Report them on stderr instead and
// let the process fail, so the log names the check (the MSVC Debug jobs of train #973)
#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#include <cstdlib>
#include <windows.h>

namespace
{
[[maybe_unused]] const bool g_reportsOnStderr = [] {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    const int types[] = { _CRT_WARN, _CRT_ERROR, _CRT_ASSERT };
    for (const int type : types)
    {
        _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
    }
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    return true;
}();
} // namespace
#endif
