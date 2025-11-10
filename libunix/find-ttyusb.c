// engler, cs140e: your code to find the tty-usb device on your laptop.
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include "libunix.h"

#define _SVID_SOURCE
#include <dirent.h>
static const char *ttyusb_prefixes[] = {
    "ttyUSB",	// linux
    "ttyACM",   // linux
    "cu.SLAB_USB", // mac os
    // "cu.usbserial", // mac os
    // if your system uses another name, add it.
	0
};

static int filter(const struct dirent *d) {
    // scan through the prefixes, returning 1 when you find a match.
    // 0 if there is no match.
    // unsigned count = sizeof(ttyusb_prefixes) / sizeof(ttyusb_prefixes[0]);
    
    for (int i = 0; ttyusb_prefixes[i] != NULL; i++) {
        if (strncmp(d->d_name, ttyusb_prefixes[i], strlen(ttyusb_prefixes[i])) == 0){
            // trace("Got match .. %s \n", ttyusb_prefixes[i]);
            return 1;
        }
    }
    return 0;
}
enum OPTION {
    LAST = 0,
    FIRST,
    UNDEF
};


// Custom comparison function for scandir
int compare_mtime_less(const struct dirent **a, const struct dirent **b) {
    struct stat* stat_a = (struct stat*) malloc (sizeof(struct stat));
    struct stat* stat_b = (struct stat*) malloc (sizeof(struct stat));
    // struct stat* stat_a = (struct stat*) malloc (sizeof(struct stat));
    char path_a[PATH_MAX], path_b[PATH_MAX];

    snprintf(path_a, PATH_MAX, "./%s", (*a)->d_name);
    snprintf(path_b, PATH_MAX, "./%s", (*b)->d_name);

    if (stat(path_a, stat_a) == -1 || stat(path_b, stat_b) == -1) {
        return 0;
    }

    int ret;
    if (stat_a->st_mtime < stat_b->st_mtime) {
        ret = 1; // a is older
    } else if (stat_a->st_mtime > stat_b->st_mtime) {
        ret = -1;  // a is newer
    } else {
        ret = 0;  // Same modification time
    }

    free(stat_a);
    free(stat_b);
    return ret;
}

int compare_mtime_greater(const struct dirent **a, const struct dirent **b) {
    struct stat* stat_a = (struct stat*) malloc (sizeof(struct stat));
    struct stat* stat_b = (struct stat*) malloc (sizeof(struct stat));
    // struct stat* stat_a = (struct stat*) malloc (sizeof(struct stat));
    char path_a[PATH_MAX], path_b[PATH_MAX];

    snprintf(path_a, PATH_MAX, "./%s", (*a)->d_name);
    snprintf(path_b, PATH_MAX, "./%s", (*b)->d_name);

    if (stat(path_a, stat_a) == -1 || stat(path_b, stat_b) == -1) {
        return 0;
    }

    int ret;
    if (stat_a->st_mtime < stat_b->st_mtime) {
        ret = -1; // a is older
    } else if (stat_a->st_mtime > stat_b->st_mtime) {
        ret = 1;  // a is newer
    } else {
        ret = 0;  // Same modification time
    }

    free(stat_a);
    free(stat_b);
    return ret;
}
char* wrap_find_ttyusb(enum OPTION op){
    const char* parent_dir = "/dev";
    struct dirent **name_list;
    int n ;
    switch (op){
        case LAST:
            n = scandir(parent_dir, &name_list, filter, compare_mtime_greater);
            break;
        case FIRST:
            n = scandir(parent_dir, &name_list, filter, compare_mtime_less);
            break;
        case UNDEF:
            n = scandir(parent_dir, &name_list, filter, alphasort);
            break;
        default:
            notreached();
    } 

    if (n == 0 || n > 1) {
        if (n == 2) {
            // TODO (tharitt): maybe later for portable reason
        } else {
            panic("scandir return %d while expect only 1 \n", n);
        }
    }

    char* tty_name = name_list[0]->d_name;
    unsigned abs_len = strlen(parent_dir) + 1 + strlen(tty_name) + 1;
    char* abs_path = (char*) malloc(abs_len * sizeof(char));
    snprintf(abs_path, abs_len, "%s/%s", parent_dir,  tty_name);

    return strdupf(abs_path);
}

// find the TTY-usb device (if any) by using <scandir> to search for
// a device with a prefix given by <ttyusb_prefixes> in /dev
// returns:
//  - device name.
// error: panic's if 0 or more than 1 devices.
char *find_ttyusb(void) {
    return wrap_find_ttyusb(UNDEF);
}

// return the most recently mounted ttyusb (the one
// mounted last).  use the modification time 
// returned by state.
char *find_ttyusb_last(void) {
    return wrap_find_ttyusb(LAST);
}

// return the oldest mounted ttyusb (the one mounted
// "first") --- use the modification returned by
// stat()
char *find_ttyusb_first(void) {
    return wrap_find_ttyusb(FIRST);
}
