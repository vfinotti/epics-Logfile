/* SPDX-License-Identifier: MIT */
#include "EPICSLog.h"

#include <epicsExport.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#include <iocsh.h>
#pragma GCC diagnostic pop

/* logAddFileStream */
static const iocshArg logAddFileStreamArg0 = {"file name", iocshArgString};
static const iocshArg *logAddFileStreamArgs[] = {&logAddFileStreamArg0};
static const iocshFuncDef logAddFileStreamFuncDef = {
      "logAddFileStream", 1 /* # parameters */, logAddFileStreamArgs};

static void logAddFileStreamCallFunc(const iocshArgBuf *_args) {
   LogfileAddFileStream(_args[0].sval);
} /* logAddFileStreamCallFunc */

/* logAddStompStream */
static const iocshArg logAddStompStreamArg0 = {
      "application name", iocshArgString};
static const iocshArg logAddStompStreamArg1 = {"server name", iocshArgString};
static const iocshArg *logAddStompStreamArgs[] = {
      &logAddStompStreamArg0, &logAddStompStreamArg1};
static const iocshFuncDef logAddStompStreamFuncDef = {
      "logAddStompStream", 2 /* # parameters */, logAddStompStreamArgs};

static void logAddStompStreamCallFunc(const iocshArgBuf *_args) {
   LogfileAddStompStream(_args[0].sval, _args[1].sval);
} /* logAddStompStreamCallFunc */

/* logDump */
static const iocshFuncDef logDumpFuncDef = {
      "logDump", 0 /* # parameters */, NULL};

static void logDumpCallFunc(const iocshArgBuf *_args __attribute__((unused))) {
   LogfileDump();
} /* logDumpCallFunc */

/* logSetMinLevel */
static const iocshArg logSetMinLevelArg0 = {"stream name", iocshArgString};
static const iocshArg logSetMinLevelArg1 = {"level name", iocshArgString};
static const iocshArg *logSetMinLevelArgs[] = {
      &logSetMinLevelArg0, &logSetMinLevelArg1};
static const iocshFuncDef logSetMinLevelFuncDef = {
      "logSetMinLevel", 2 /* # parameters */, logSetMinLevelArgs};

static void logSetMinLevelCallFunc(const iocshArgBuf *_args) {
   LogfileSetMinLevel(_args[0].sval, _args[1].sval);
} /* logSetMinLevelCallFunc */

/* logSetSubsystemMinLevel */
static const iocshArg logSetSubsystemMinLevelArg0 = {
      "subsystem name", iocshArgString};
static const iocshArg logSetSubsystemMinLevelArg1 = {
      "level name", iocshArgString};
static const iocshArg *logSetSubsystemMinLevelArgs[] = {
      &logSetSubsystemMinLevelArg0, &logSetSubsystemMinLevelArg1};
static const iocshFuncDef logSetSubsystemMinLevelFuncDef = {
      "logSetSubsystemMinLevel", 2 /* # parameters */,
      logSetSubsystemMinLevelArgs};

static void logSetSubsystemMinLevelCallFunc(const iocshArgBuf *_args) {
   LogfileSetSubsystemMinLevel(_args[0].sval, _args[1].sval);
} /* logSetSubsystemMinLevelCallFunc */

static void logRegisterCommands(void) {
   static int firstTime = 1;
   if (firstTime) {
      iocshRegister(&logAddFileStreamFuncDef, logAddFileStreamCallFunc);
      iocshRegister(&logAddStompStreamFuncDef, logAddStompStreamCallFunc);
      iocshRegister(&logDumpFuncDef, logDumpCallFunc);
      iocshRegister(&logSetMinLevelFuncDef, logSetMinLevelCallFunc);
      iocshRegister(
            &logSetSubsystemMinLevelFuncDef, logSetSubsystemMinLevelCallFunc);
      firstTime = 0;
   } /* if */
} /* logRegisterCommands */

#pragma clang diagnostic ignored "-Wmissing-variable-declarations"

epicsExportRegistrar(logRegisterCommands);
