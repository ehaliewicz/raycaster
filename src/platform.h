#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef PLATFORM_WEB
#include "platform_web.h"
#else
#include "platform_win.h"
#endif

#endif 