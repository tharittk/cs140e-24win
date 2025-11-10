#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libunix.h"

// allocate buffer, read entire file into it, return it.   
// buffer is zero padded to a multiple of 4.
//
//  - <size> = exact nbytes of file.
//  - for allocation: round up allocated size to 4-byte multiple, pad
//    buffer with 0s. 
//
// fatal error: open/read of <name> fails.
//   - make sure to check all system calls for errors.
//   - make sure to close the file descriptor (this will
//     matter for later labs).
// 
void *read_file(unsigned *size, const char *name) {
    // How: 
    //    - use stat() to get the size of the file.
    //    - round up to a multiple of 4.
    //    - allocate a buffer
    //    - zero pads to a multiple of 4.
    //    - read entire file into buffer (read_exact())
    //    - fclose() the file descriptor
    //    - make sure any padding bytes have zeros.
    //    - return it.   
    char* buf;
    struct stat* s = (struct stat*) malloc (sizeof(struct stat));
    stat(name, s);
    *size = s->st_size;

    unsigned padded_size = 4 * ((s->st_size + 3) / 4);
    buf = (char*) calloc (padded_size, sizeof(char));
    if (!buf){
        trace("calloc error for buf \n");
        exit(1);
    }
    // trace("Read %s, unpadded %llu, padded size %u \n", name, s->st_size, padded_size);
    int fd = open(name, O_RDONLY);
    if (fd == -1){
        trace("Error opening file %s \n", name);
        exit(1);
    }
    read(fd, buf, *size);
    close(fd);
    free(s);

    return buf;


}
