// sol2-backed Lua scripting engine with a queued event bus.
// This translation unit intentionally avoids The-Forge headers so Forge's
// allocator macros (new/delete) cannot interfere with sol2 / the STL.
#include "ScriptEngine.h"

#include <sol/sol.hpp>

#include <cstdio>
#include <string>
#include <vector>

namespace fowe {
namespace {

struct Event
{
    std::string name;
    std::string payload;
};

} // namespace

struct ScriptEngine::Impl
{
    sol::state         lua;
    sol::table         handlers;
    std::vector<Event> queue;
    ScriptLogCallback  logCallback  = nullptr;
    bool               ready        = false;
    int                handlerCount = 0;

    void Log(const std::string& message, bool isError)
    {
        if (logCallback)
            logCallback(message.c_str(), isError);
        else
            std::fprintf(stderr, "[Lua]%s %s\n", isError ? " error:" : "", message.c_str());
    }

    void BindApi()
    {
        sol::table foweTable = lua.create_named_table("fowe");

        foweTable.set_function("log", [this](const std::string& message) { Log(message, false); });

        foweTable.set_function("on", [this](const std::string& name, sol::protected_function handler) {
            handlers[name] = handler;
            ++handlerCount;
        });

        foweTable.set_function("emit", sol::overload(
            [this](const std::string& name) { queue.push_back({ name, std::string() }); },
            [this](const std::string& name, const std::string& payload) { queue.push_back({ name, payload }); }));
    }

    bool Init()
    {
        lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string, sol::lib::math,
                           sol::lib::table, sol::lib::os, sol::lib::io, sol::lib::package);

        handlers = lua.create_table();
        BindApi();
        ready = true;
        return true;
    }

    void DispatchAll()
    {
        if (!ready || queue.empty())
            return;

        std::vector<Event> events;
        events.swap(queue);

        for (const Event& event : events)
        {
            sol::object handler = handlers[event.name];
            if (handler.get_type() != sol::type::function)
                continue;

            sol::protected_function         function = handler;
            sol::protected_function_result result = function(event.payload);
            if (!result.valid())
            {
                sol::error error = result;
                Log(std::string("handler for '") + event.name + "' failed: " + error.what(), true);
            }
        }
    }
};

ScriptEngine* ScriptEngine::Create()
{
    ScriptEngine* engine = new ScriptEngine();
    engine->mImpl = new Impl();
    return engine;
}

void ScriptEngine::Destroy(ScriptEngine* engine)
{
    if (!engine)
        return;
    delete engine->mImpl;
    delete engine;
}

bool ScriptEngine::Init()
{
    if (!mImpl)
        return false;
    return mImpl->Init();
}

void ScriptEngine::Shutdown()
{
    if (!mImpl)
        return;
    mImpl->queue.clear();
    mImpl->handlers = sol::nil;
    mImpl->ready = false;
    mImpl->handlerCount = 0;
    mImpl->lua.collect_garbage();
}

bool ScriptEngine::RunFile(const char* path)
{
    if (!mImpl || !mImpl->ready)
        return false;

    sol::protected_function_result result = mImpl->lua.safe_script_file(path, sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error error = result;
        mImpl->Log(std::string("failed to run '") + path + "': " + error.what(), true);
        return false;
    }
    return true;
}

bool ScriptEngine::RunString(const char* code)
{
    if (!mImpl || !mImpl->ready)
        return false;

    sol::protected_function_result result = mImpl->lua.safe_script(code, sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error error = result;
        mImpl->Log(std::string("script error: ") + error.what(), true);
        return false;
    }
    return true;
}

void ScriptEngine::Emit(const char* eventName, const char* payload)
{
    if (!mImpl || !mImpl->ready || !eventName)
        return;
    mImpl->queue.push_back({ eventName, payload ? payload : "" });
}

void ScriptEngine::Dispatch()
{
    if (mImpl)
        mImpl->DispatchAll();
}

void ScriptEngine::SetLogCallback(ScriptLogCallback callback)
{
    if (mImpl)
        mImpl->logCallback = callback;
}

int ScriptEngine::HandlerCount() const
{
    if (!mImpl || !mImpl->ready)
        return 0;
    return mImpl->handlerCount;
}

} // namespace fowe
