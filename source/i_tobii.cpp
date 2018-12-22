//
// The Eternity Engine
// Copyright (C) 2017 James Haley et al.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
// Additional terms and conditions compatible with the GPLv3 apply. See the
// file COPYING-EE for details.
//
// Purpose: Tobii eye tracker Stream Engine support
// Authors: Ioan Chera
//

#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>
#include "z_zone.h"
#include "c_io.h"
#include "d_main.h"
#include "doomstat.h"
#include "hal/i_platform.h"
#include "i_tobii.h"
#include "v_misc.h"

#if EE_CURRENT_PLATFORM == EE_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

enum
{
   CONN_RETRY_COOLDOWN = TICRATE,
   TIMESYNC_SUCCESS_COOLDOWN = 30 * TICRATE,
   TIMESYNC_FAILURE_COOLDOWN = 5 * TICRATE,
   MONITOR_CHECK_COOLDOWN = TICRATE
};

#if EE_CURRENT_PLATFORM == EE_PLATFORM_WINDOWS
//
// Used for main window lookup
//
struct handledata_t
{
   unsigned long processId;
   HWND bestHandle;
};

static HWND g_mainWindow;  // the main window (used for getting coordinates)
static HMONITOR g_mainMonitor;   // the main window's monitor
#endif

//
// The eyetracking event, observed by the game
//
static struct
{
   double x;
   double y;
   bool fired;
} g_gazepoint;

//
// If present
//
static struct
{
   bool yes;   // present?
   bool fired; // event is fired and valid
} g_presence;

static tobii_api_t *api;      // The Stream Engine API
static tobii_device_t *dev;   // The loaded device
static int retryCooldown;     // Time when to retry device
static int timesyncCooldown;  // Time between time syncs
static int monitorCooldown;   // monitor check cooldown

//
// Allocator callback
//
inline static void *I_alloc(void *mem_context, size_t size)
{
   return emalloc(void *, size);
}

//
// Freeing callback
//
inline static void I_free(void *mem_context, void *ptr)
{
   efree(ptr);
}

//
// Logging callback
//
static void I_log(void *log_context, tobii_log_level_t level, char const *text)
{
#if 0
   switch(level)
   {
   case TOBII_LOG_LEVEL_ERROR:
      C_Printf(FC_ERROR "Stream Engine Error: %s\n", text);
      break;
   case TOBII_LOG_LEVEL_WARN:
      C_Printf(FC_ERROR "Stream Engine Warning: %s\n", text);
      break;
#ifdef _DEBUG  // only spam in debug mode
   case TOBII_LOG_LEVEL_INFO:
      C_Printf("Stream Engine Info: %s\n", text);
      break;
   case TOBII_LOG_LEVEL_DEBUG:
      C_Printf("Stream Engine Debug: %s\n", text);
      break;
   case TOBII_LOG_LEVEL_TRACE:
      C_Printf("Stream Engine Trace: %s\n", text);
      break;
#endif
   default:
      break;
   }

   // Keep them only to debug because they're really dense.
#ifdef _DEBUG
   char c = level == TOBII_LOG_LEVEL_ERROR ? 'E' :
      level == TOBII_LOG_LEVEL_WARN ? 'W' :
      level == TOBII_LOG_LEVEL_INFO ? 'I' :
      level == TOBII_LOG_LEVEL_DEBUG ? 'D' : 'T';
   printf("Stream(%c): %s\n", c, text);
#endif
#endif
}

//
// Helper function to get whether a hwnd is main
//
static bool I_isMainWindow(HWND handle)
{
   return GetWindow(handle, GW_OWNER) == 0 && IsWindowVisible(handle);
}

//
// For window getting
//
static BOOL CALLBACK I_enumWindowsCallback(HWND handle, LPARAM lParam)
{
   handledata_t &data = *(handledata_t *)lParam;
   unsigned long processId = 0;
   GetWindowThreadProcessId(handle, &processId);
   if(data.processId != processId || !I_isMainWindow(handle))
      return TRUE;

   data.bestHandle = handle;
   g_mainWindow = data.bestHandle;
   return FALSE;
}

//
// Gaze callback
//
static void I_gazePointCallback(const tobii_gaze_point_t *gaze_point, void *user_data)
{
   if(gaze_point->validity != TOBII_VALIDITY_VALID)
      return;
   g_gazepoint.x = gaze_point->position_xy[0];
   g_gazepoint.y = gaze_point->position_xy[1];
   g_gazepoint.fired = true;
}

//
// Presence detection
//
static void I_userPresenceCallback(const tobii_user_presence_status_t status,
   int64_t timestamp_us, void *user_data)
{
   if(status == TOBII_USER_PRESENCE_STATUS_UNKNOWN)
      return;
   g_presence.yes = status == TOBII_USER_PRESENCE_STATUS_PRESENT;
   g_presence.fired = true;
}

//
// Tries to find a device if none available
//
static bool I_findDevice(tobii_error_t *outErr = nullptr)
{
   if(dev)
      return false;  // already in
   tobii_error_t err = tobii_device_create(api, nullptr, &dev);
   if(err != TOBII_ERROR_NO_ERROR)
   {
      if(outErr)
         *outErr = err;
      return false;
   }
   return true;
}

//
// Initializes Stream Engine
//
bool I_EyeInit()
{
   if(api)
      return false;

   tobii_version_t ver;
   tobii_get_api_version(&ver);
   usermsg("I_EyeInit: Stream Engine version %d.%d.%d.%d.", ver.major, ver.minor,
      ver.revision, ver.build);

   static const tobii_custom_alloc_t allocator = { nullptr, I_alloc, I_free };
//   static const tobii_custom_log_t logger = { nullptr, I_log };

   tobii_error_t err = tobii_api_create(&api, &allocator, nullptr);
   if(err != TOBII_ERROR_NO_ERROR)
   {
      usermsg("I_EyeInit: Failed initializing Stream Engine. %s",
         tobii_error_message(err));
      return false;
   }

   if(!I_findDevice(&err))
   {
      usermsg("I_EyeInit: Device not found. %s Will attempt again during gameplay.",
         tobii_error_message(err));
   }

   retryCooldown = gametic + CONN_RETRY_COOLDOWN;
   timesyncCooldown = gametic + TIMESYNC_SUCCESS_COOLDOWN;

   startupmsg("I_EyeInit", "Loaded Stream Engine.");

   return true;
}

//
// Called when window is updated. Gets the Eternity window so it can update the
// relative coordinates
//
void I_EyeAttachToWindow()
{
   // FIXME: when SDL2 is used, replace all this with library calls.
#if EE_CURRENT_PLATFORM == EE_PLATFORM_WINDOWS
   handledata_t data;
   data.processId = GetCurrentProcessId();
   data.bestHandle = 0;
   if(!EnumWindows(I_enumWindowsCallback, (LPARAM)&data))
   {
      return;
   }

   g_mainWindow = data.bestHandle;
#else
#error Not implemented on non-Windows systems!
#endif
}

//
// Check the gaze event
//
static bool I_checkGazeEvent(double &x, double &y)
{
   if(!g_gazepoint.fired)
      return false;
   g_gazepoint.fired = false;

#if EE_CURRENT_PLATFORM == EE_PLATFORM_WINDOWS
   RECT rect;
   if(!g_mainWindow || !GetWindowRect(g_mainWindow, &rect))
      return false;
   if(rect.right == rect.left || rect.bottom == rect.top)
      return false;

   //   printf("%g %g\n", gaze_point->position_xy[0], gaze_point->position_xy[1]);

   static int screenw, screenh;
   if(gametic >= monitorCooldown || !screenw || !screenh)
   {
      static HMONITOR monitor;
      monitorCooldown = gametic + MONITOR_CHECK_COOLDOWN;
      monitor = MonitorFromWindow(g_mainWindow, MONITOR_DEFAULTTONEAREST);
      if(!monitor)
         return false;
      MONITORINFO info = { sizeof(info) };
      if(!GetMonitorInfo(monitor, &info))
         return false;
      screenw = info.rcMonitor.right - info.rcMonitor.left;
      screenh = info.rcMonitor.bottom - info.rcMonitor.top;
      if(!screenw || !screenh)
         return false;
   }
   g_gazepoint.x *= screenw;
   g_gazepoint.y *= screenh;

   x = (g_gazepoint.x - rect.left) / (rect.right - rect.left);
   y = (g_gazepoint.y - rect.top) / (rect.bottom - rect.top);

   return true;
#endif

   return false;
}

//
// Check presence
//
static bool I_checkPresenceEvent(bool &presence)
{
   presence = g_presence.yes;
   return g_presence.fired;
}

//
// Gets event data
//
void I_EyeGetEvent(double &x, double &y, bool &presence, unsigned &eventGot)
{
   eventGot = 0;
   if(!dev)
   {
      if(gametic >= retryCooldown && !I_findDevice())
         retryCooldown = gametic + TICRATE;
      return;
   }

   tobii_gaze_point_subscribe(dev, I_gazePointCallback, nullptr); // make sure it's on
   tobii_user_presence_subscribe(dev, I_userPresenceCallback, nullptr);

   tobii_error_t err;
   if(gametic >= timesyncCooldown)
   {
      err = tobii_update_timesync(dev);
      if(err == TOBII_ERROR_OPERATION_FAILED)
         timesyncCooldown = gametic + TIMESYNC_FAILURE_COOLDOWN;
      else
         timesyncCooldown = gametic + TIMESYNC_SUCCESS_COOLDOWN;
   }
   if(tobii_device_process_callbacks(dev) == TOBII_ERROR_CONNECTION_FAILED &&
      tobii_device_reconnect(dev) == TOBII_ERROR_CONNECTION_FAILED)
   {
      tobii_device_destroy(dev);
      dev = nullptr;
      retryCooldown = gametic + TICRATE;
      return;  // try later
   }

   if(I_checkGazeEvent(x, y))
      eventGot |= EYE_EVENT_GAZE;
   if(I_checkPresenceEvent(presence))
      eventGot |= EYE_EVENT_PRESENCE;
}

//
// Clears stream engine
//
void I_EyeShutdown()
{
   tobii_user_presence_unsubscribe(dev);
   tobii_gaze_point_unsubscribe(dev);
   tobii_device_destroy(dev);
   dev = nullptr;
   tobii_api_destroy(api);
   api = nullptr;
}

// EOF
