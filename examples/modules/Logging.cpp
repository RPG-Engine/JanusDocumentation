// Module example: logging.
//
// A console program: it opens no window and does not start the rest of the engine.
// Output goes to the console and to <exe dir>/Logs/.
//
// Shows: DECLARE_LOGGER, DECLARE_LOGGER_WITH_PARAMS, a hand-built SLogger,
// the generic logger, std::format-style arguments, log levels, and
// starting/stopping the Log module.

#include <Log/Module.hpp>

//! [declare]
// One logger per source file (or per module). Messages below "debug" are dropped.
DECLARE_LOGGER(Inventory, debug)
//! [declare]

//! [declare_params]
// File only, info and above, written to <exe dir>/Logs/Network.log.
DECLARE_LOGGER_WITH_PARAMS(Network, info, Log::ELogOutputOptions::LOG_FILE, "Network")
//! [declare_params]

//! [manual]
// A logger that is not tied to a namespace.
// Log through it with the Log::Generic functions.
static Log::SLogger AudioLogger({
    .LoggerNamespace = "LogAudio",
    .LogLevel = Log::LogLevel::warn,
    .LogFile = "",                                  // unused: no file output
    .LogOptions = Log::ELogOutputOptions::LOG_CONSOLE });
//! [manual]

int main() {
    //! [module]
    // Engine::InitializePlugins() normally does this.
    // A program that only logs can call it directly.
    Log::Module();
    //! [module]

    //! [levels]
    Log::Inventory::Error("Item {} is missing from the database", 42);
    Log::Inventory::Warning("Inventory is {}% full", 90);
    Log::Inventory::Info("Picked up {} x {}", 3, "apple");
    Log::Inventory::Debug("Slots used: {}/{}", 7, 20);
    Log::Inventory::Verbose("Not shown: below the global debug level");
    //! [levels]

    //! [other_loggers]
    Log::Network::Info("Connected to {}:{}", "127.0.0.1", 7777);   // file only
    Log::Network::Debug("Not written: below this logger's info level");

    Log::Generic::Info("The generic logger needs no declaration");
    Log::Generic::Warning(AudioLogger, "Sound {} is missing", "jump.wav");
    Log::Generic::Info(AudioLogger, "Not shown: below this logger's warn level");
    //! [other_loggers]

    //! [teardown]
    Log::Flush();            // write anything still buffered
    Log::TeardownModule();   // also copies each log file to a timestamped name
    //! [teardown]
    return 0;
}
