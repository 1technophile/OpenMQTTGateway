/*
  OpenMQTTGateway - Home Assistant Discovery Logging Stub
  
  Provides stub logging macros to break circular dependencies with TheengsCommon.h
  This allows the library to compile independently while maintaining API compatibility.
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

// Stub logging macros - replace external dependencies
#ifndef THEENGS_LOG_TRACE
#  define THEENGS_LOG_TRACE(...)   ((void)0)
#  define THEENGS_LOG_VERBOSE(...) ((void)0)
#  define THEENGS_LOG_NOTICE(...)  ((void)0)
#  define THEENGS_LOG_WARNING(...) ((void)0)
#  define THEENGS_LOG_ERROR(...)   ((void)0)
#endif

// Arduino compatibility macros
#ifndef F
#  define F(x) (x)
#endif

#ifndef CR
#  define CR "\n"
#endif
