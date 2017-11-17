Logfile Library
===============

Introduction
------------
The library provides an easy method to generate and organize textual log
messages from an application. The main design criteria are reliability and
interoperability in mixed C++/Java environments.

Within the application, you can define several log sources ("subsystems"),
and destinations ("sinks"). Log messages can be suppressed by independently
setting minimum log levels for subsystems or sinks.

This document briefly explains how to use the main features of the library.

Features
--------
- Fully threadsafe design.
- Runtime configurable message routing to various kinds of sinks.
- JMS-compatible STOMP log sink.
- No code generated for discarded messages in release builds that exclude low
  log levels.
- Automatic backtrace on crashing applications.
- The actual log output is done in a separate thread in order not to delay the
  execution of the main program.
- When a log sink is unavailable, log messages are queued for an automatic
  retry. They expire after a given time.
- Interface to control the logging from within an EPICS IOC.

Installation
------------
The installation process uses *cmake* for generating the required files and
installing them. This is done by running the following commands on the top level
directory:

```sh
$ mkdir build
$ cd build
$ cmake ../
$ make
$ sudo make install
```

In some cases, it may be required to use the *-std=c++11* flag on
CMAKE_CXX_FLAGS. This can be done by running ```ccmake ./``` after
```cmake ../```, pressing "*t*", setting the flag on CMAKE_CXX_FLAGS and
pressing "*c*" to configure the environment.

Minimum Log Levels
------------------
There are two kinds of minimum log levels:
- A compile-time minimum. All calls to the logging library that create messages
  below this level are completely removed from the code. This allows to have a
  release-build mode without overhead for debug messages.

  To change the compile-time minimum, define SuS_LOG_MINLEVEL to the desired
  value when compiling a file containing calls to the logging macros.
- Run-time minimums.

Adding Subsystems
-----------------
Within the application, several subsystems can be defined. This feature can be
used to group messages by user-defined topics (e.g. "GUI", "I/O"). Especially
during development it is then useful to have different minimum log levels for
different subsystems to print debug messages only for the module under
development.

It is desirable to know the full list of subsystems right from the start of
the application, e.g. so that they can be listed in a GUI. Also, setting a log
level on a subsystem that hasn't been defined yet is not possible. Therefore,
subsystems can be registered on application start by using the
subsystem_registrator helper class.

Typically this is used to initialize a variable in file-global scope, or a
static variable of a class:
    namespace {
        subsystem_registrator log_id("my_subsystem");
    }
The subsystem id required for the logging macros is then available as
`log_id()`.

The same subsystem name can be registered several times (e.g. by several
file-local subsystem_registrators), and all definitions will automatically
be merged.

Adding and Removing Sinks
-------------------------
When the logging library is initialized, a sink outputting to stdout is
automatically added in order to make the most common use case easy.
This default sink can be removed at any time by calling
    SuS::logfile::logger::instance()->remove_output_stream("stdout");
