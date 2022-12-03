#pragma once

#include "../../plugin.h"
#include "../../color.h"
#include "../../string-sizes.h"

static CLAP_CONSTEXPR const char CLAP_EXT_TRACK_INFO[] = "clap.track-info.draft/0";

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clap_track_info {
   clap_id      id;
   int32_t      index;
   char         name[CLAP_NAME_SIZE];
   char         path[CLAP_PATH_SIZE]; // Like "/group1/group2/drum-machine/drum-pad-13"
   int32_t      channel_count;
   const char  *audio_port_type;
   clap_color_t color;
   bool         is_return_track;
} clap_track_info_t;

typedef struct clap_plugin_track_info {
   // [main-thread]
   void(CLAP_ABI *changed)(const clap_plugin_t *plugin);
} clap_plugin_track_info_t;

typedef struct clap_host_track_info {
   // Get info about the track the plugin belongs to.
   // [main-thread]
   bool(CLAP_ABI *get)(const clap_host_t *host, clap_track_info_t *info);
} clap_host_track_info_t;

#ifdef __cplusplus
}
#endif
