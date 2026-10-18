/* User interface text. The language is chosen when building: make LANG=en (default tr); lang/<code>.h holds the text. */
#ifndef YARROW_STRINGS_H
#define YARROW_STRINGS_H

#include "uidata.h"

#ifndef YARROW_LANG_HEADER
#define YARROW_LANG_HEADER "lang/tr.h"
#endif
#include YARROW_LANG_HEADER

/* emergency guide page */
typedef struct {
    const char *title;
    const char *lines[7];
} guide_page_t;

#endif
