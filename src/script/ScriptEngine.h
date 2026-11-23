#pragma once

namespace fowe {

using ScriptLogCallback = void (*)(const char* message, bool isError);

// Lua 5.3 + sol2 scripting engine with a queued event bus.
// Events are queued by C++ (or Lua) and dispatched once per frame.
class ScriptEngine
{
public:
    static ScriptEngine* Create();
    static void          Destroy(ScriptEngine* engine);

    bool Init();
    void Shutdown();

    bool RunFile(const char* path);
    bool RunString(const char* code);

    void Emit(const char* eventName, const char* payload);
    void Dispatch();

    void SetLogCallback(ScriptLogCallback callback);
    int  HandlerCount() const;

private:
    ScriptEngine()  = default;
    ~ScriptEngine() = default;

    struct Impl;
    Impl* mImpl = nullptr;
};

} // namespace fowe
