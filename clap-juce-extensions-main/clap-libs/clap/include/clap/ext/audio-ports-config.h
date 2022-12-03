#pragma once

#include "../string-sizes.h"
#include "../plugin.h"

/// @page Audio Ports Config
///
/// This extension provides a way for the plugin to describe possible port configurations, for
/// example mono, stereo, surround, ... and a way for the host to select a configuration.
///
/// After the plugin initialization, the host may scan the list of configurations and eventually
/// select one that fits the plugin context. The host can only select a configuration if the plugin
/// is deactivated.
///
/// A configuration is a very simple description of the audio ports:
/// - it describes the main input and output ports
/// - it has a name that can be displayed to the user
///
/// The idea behind the configurations, is to let the user choose one via a menu.
///
/// Plugins with very complex configuration possibilities should let the user configure the ports
/// from the plugin GUI, and call @ref clap_host_audio_ports.rescan(CLAP_AUDIO_PORTS_RESCAN_ALL).

static CLAP_CONSTEXPR const char CLAP_EXT_AUDIO_PORTS_CONFIG[] = "clap.audio-ports-config";

#ifdef __cplusplus
extern "C" {
#endif

// Minimalistic description of ports configuration
typedef struct clap_audio_ports_config {
   clap_id id;
   char    name[CLAP_NAME_SIZE];

   uint32_t input_port_count;
   uint32_t output_port_count;

   // main input info
   bool        has_main_input;
   uint32_t    main_input_channel_count;
   const char *main_input_port_type;

   // main output info
   bool        has_main_output;
   uint32_t    main_output_channel_count;
   const char *main_output_port_type;
} clap_audio_ports_config_t;

// The audio ports config scan has to be done while the plugin is deactivated.
typedef struct clap_plugin_audio_ports_config {
   // gets the number of available configurations
   // [main-thread]
   uint32_t(CLAP_ABI *count)(const clap_plugin_t *plugin);

   // gets information about a configuration
   // [main-thread]
   bool(CLAP_ABI *get)(const clap_plugin_t       *plugin,
                       uint32_t                   index,
                       clap_audio_ports_config_t *config);

   // selects the configuration designated by id
   // returns true if the configuration could be applied
   // [main-thread,plugin-deactivated]
   bool(CLAP_ABI *select)(const clap_plugin_t *plugin, clap_id config_id);
} clap_plugin_audio_ports_config_t;

typedef struct clap_host_audio_ports_config {
   // Rescan the full list of configs.
   // [main-thread]
   void(CLAP_ABI *rescan)(const clap_host_t *host);
} clap_host_audio_ports_config_t;

#ifdef __cplusplus
}
#endif
