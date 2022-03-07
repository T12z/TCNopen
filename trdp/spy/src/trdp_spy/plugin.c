/*
 * When using cmake and full wireshark source, this file is auto generated.
 * However, if only building a deb package, we get away with just libwireshark-dev, but then we have to prepare and 
 * provide the plugin.c.
 *
 * Generated automatically from /home/thorsten/Projekte/debian/wireshark-{2.6.7,3.2.0}/tools/make-plugin-reg.py.
 */

#include "config.h"

#include <gmodule.h>

/* plugins are DLLs */
#define WS_BUILD_DLL
#include "ws_symbol_export.h"

#include "epan/proto.h"

#include "packet-trdp_spy.h"

void proto_register_trdp(void);
void proto_reg_handoff_trdp(void);

WS_DLL_PUBLIC_DEF const gchar plugin_version[] = PLUGIN_VERSION;
#if VERSION_MAJOR < 3
#if VERSION_MAJOR < 2 || VERSION_MINOR < 6
#warning Very old Wireshark - this plugin source will probably not work.
#endif
WS_DLL_PUBLIC_DEF const gchar plugin_release[] = VERSION_RELEASE;
#else
WS_DLL_PUBLIC_DEF const int plugin_want_major = VERSION_MAJOR;
WS_DLL_PUBLIC_DEF const int plugin_want_minor = VERSION_MINOR;
#endif

WS_DLL_PUBLIC void plugin_register(void);

void plugin_register(void)
{
    static proto_plugin plug_trdp;

    plug_trdp.register_protoinfo = proto_register_trdp;
    plug_trdp.register_handoff = proto_reg_handoff_trdp;
    proto_register_plugin(&plug_trdp);
}
