// There are a couple of symbols that need to be #defined before
// #including all the headers.

#ifndef _PARAMS_H_
#define _PARAMS_H_

#include "structs.h"

// The FUSE API has been changed a number of times.  So, our code
// needs to define the version of the API that we assume.  As of this
// writing, the most current API version is 26
#define FUSE_USE_VERSION 26
#include <fuse.h>


// need this to get pwrite().  I have to use setvbuf() instead of
// setlinebuf() later in consequence.
#define _XOPEN_SOURCE 500

// maintain bbfs state in here
#include <limits.h>
#include <stdio.h>
struct bb_state {
    FILE *logfile;
    char *rootdir;
    super_block_t * super_blk;
    FILE * fs;
    table_entry_t ** openFileTable;
};
#define BB_DATA ((struct bb_state *) fuse_get_context()->private_data)

#endif