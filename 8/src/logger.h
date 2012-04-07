#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "params.h"

#include <fuse.h>
#include <stdio.h>
#include "structs.h"
#include "config.h"

void dump_file_table(table_entry_t ** tableptr);

//  macro to log fields in structs.
#define log_struct(st, field, format, typecast)                         \
    log_msg("    " #field " = " #format "\n", typecast st->field)

FILE *log_open(void);
void log_fi (struct fuse_file_info *fi);
void log_stat(struct stat *si);
void log_statvfs(struct statvfs *sv);
void log_utime(struct utimbuf *buf);

void log_msg(const char *format, ...);

#endif //_LOGGER_H_
