#include "font.h"
#include "toys/debug.h"

#include <fontconfig/fontconfig.h>

char *get_system_font_path(const char *name)
{
    FcChar8 *font_path = NULL;

    FcConfig *config = FcInitLoadConfigAndFonts();
    if (!config) return NULL;

    FcPattern *pattern = FcNameParse((const FcChar8 *)name);
    if (!pattern) {
        FcFini();
        return NULL;
    }

    FcConfigSubstitute(config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result;
    FcPattern *match = FcFontMatch(config, pattern, &result);

    if (match) {
        if (FcPatternGetString(match, FC_FILE, 0, &font_path) == FcResultMatch) {
            LOG_I("Resolved '%s' to %s", name, (char *)font_path);
        }
        FcPatternDestroy(match);
    }

    FcPatternDestroy(pattern);
    FcFini();

    return (char *)font_path;
}
