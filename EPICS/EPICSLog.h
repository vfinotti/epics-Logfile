/* SPDX-License-Identifier: MIT */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void LogfileAddFileStream(const char *_filename);

void LogfileAddStompStream(const char *_appname, const char *_server);

void LogfileDump();

void LogfileSetMinLevel(const char *_streamname, const char *_level);

void LogfileSetSubsystemMinLevel(const char *_subsystem, const char *_level);

#ifdef __cplusplus
}
#endif
