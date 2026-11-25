/* quengle: magnetic opens its game, graphics and hint files with stdio, but emglken builds have no filesystem
   (FILESYSTEM=0): files come through remglk's glk streams, served by the host (host.js bocfelDialog). this header,
   force-included into the magnetic sources, maps the few stdio calls they use onto glk streams. read only, plus
   writes for the named saves glk.c never makes here */
#ifndef QUENGLE_MAGSTDIO_H
#define QUENGLE_MAGSTDIO_H

#include <stdio.h>
#include <stdlib.h>
#include "glk.h"
#include "glkstart.h"

typedef struct quengle_file { strid_t s; } quengle_file;

static inline quengle_file *quengle_fopen(const char *name, const char *mode)
{
    glui32 fmode = (mode[0] == 'r') ? filemode_Read : filemode_Write;
    frefid_t fref = glkunix_fileref_create_by_name_uncleaned(fileusage_Data | fileusage_BinaryMode, name, 0);
    strid_t s = NULL;
    quengle_file *f;
    if (!fref) return NULL;
    if (fmode == filemode_Write || glk_fileref_does_file_exist(fref))
        s = glk_stream_open_file(fref, fmode, 0);
    glk_fileref_destroy(fref);
    if (!s || !(f = malloc(sizeof *f))) {
        if (s) glk_stream_close(s, NULL);
        return NULL;
    }
    f->s = s;
    return f;
}

static inline size_t quengle_fread(void *p, size_t size, size_t n, quengle_file *f)
{
    if (!size || !n) return 0;
    return glk_get_buffer_stream(f->s, (char *)p, (glui32)(size * n)) / size;
}

static inline size_t quengle_fwrite(const void *p, size_t size, size_t n, quengle_file *f)
{
    glk_put_buffer_stream(f->s, (char *)p, (glui32)(size * n));
    return n;
}

static inline int quengle_fseek(quengle_file *f, long off, int whence)
{
    glk_stream_set_position(f->s, (glsi32)off,
        whence == SEEK_SET ? seekmode_Start : whence == SEEK_CUR ? seekmode_Current : seekmode_End);
    return 0;
}

static inline int quengle_fclose(quengle_file *f)
{
    glk_stream_close(f->s, NULL);
    free(f);
    return 0;
}

#define FILE quengle_file
#define fopen quengle_fopen
#define fread quengle_fread
#define fwrite quengle_fwrite
#define fseek quengle_fseek
#define fclose quengle_fclose

#endif
