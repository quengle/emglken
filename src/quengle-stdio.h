/* quengle: scottfree and level9 read their files with stdio (fscanf, fgetc, fread, ftell), but emglken builds have
   no filesystem (FILESYSTEM=0): files come through remglk's glk streams, served by the host (host.js bocfelDialog).
   this header, force-included into their sources, reads a file whole through glk and hands back an fmemopen
   stream, so musl's stdio does the rest unchanged. read only: writes (c64decrunch's debug dump) just fail */
#ifndef QUENGLE_STDIO_H
#define QUENGLE_STDIO_H

#include <stdio.h>
#include <stdlib.h>
#include "glk.h"
#include "glkstart.h"

static inline FILE *quengle_fopen(const char *name, const char *mode)
{
    frefid_t fref;
    strid_t s;
    size_t cap = 65536, len = 0;
    char *buf, *grown;
    FILE *f;

    if (mode[0] != 'r')
        return NULL;
    fref = glkunix_fileref_create_by_name_uncleaned(fileusage_Data | fileusage_BinaryMode, name, 0);
    if (!fref)
        return NULL;
    s = glk_fileref_does_file_exist(fref) ? glk_stream_open_file(fref, filemode_Read, 0) : NULL;
    glk_fileref_destroy(fref);
    if (!s || !(buf = malloc(cap))) {
        if (s) glk_stream_close(s, NULL);
        return NULL;
    }
    for (;;) {
        len += glk_get_buffer_stream(s, buf + len, (glui32)(cap - len));
        if (len < cap)
            break;
        if (!(grown = realloc(buf, cap * 2))) {
            free(buf);
            glk_stream_close(s, NULL);
            return NULL;
        }
        buf = grown;
        cap *= 2;
    }
    glk_stream_close(s, NULL);
    /* the buffer outlives the stream (fmemopen doesn't own it): a game file is opened once or twice per run */
    f = len ? fmemopen(buf, len, "rb") : NULL;
    if (!f)
        free(buf);
    return f;
}

#define fopen quengle_fopen

#endif
