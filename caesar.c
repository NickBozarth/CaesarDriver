// TODO make return values parenthesized
// return (whatever_you_return);
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>

MALLOC_DECLARE(M_CAESAR);
MALLOC_DEFINE(M_CAESAR, "caesar_cipher", "Caesar encrypt a a message buffer");

#define BUFFER_SIZE 1024

struct caesar_buffer {
    char buffer[BUFFER_SIZE];
    size_t length;
};

static struct caesar_buffer *buf;
static struct cdev *sdev;


static d_open_t     d_open;
static d_close_t    d_close;
static d_read_t     d_read;
static d_write_t    d_write;



static struct cdevsw caesar_cdevsw = {
    .d_version  = D_VERSION,
    .d_open     = d_open,
    .d_close    = d_close,
    .d_read     = d_read,
    .d_write    = d_write,
    .d_name     = "caesar",
};

static void in_place_caesar_cipher(struct caesar_buffer *buf, int caesar_offset) {
    char* c = buf->buffer;
    int case_offset = 0;
    for (int i = 0; i < buf->length; i++, c++) {
        if      (*c >= 'a' && *c <= 'z') case_offset = 'a';
        else if (*c >= 'A' && *c <= 'Z') case_offset = 'A';
        else if (*c == '\0') break;
        else continue;

        *c = (*c - case_offset + caesar_offset) % 26 + case_offset;
    }
}


static int d_open(struct cdev *dev, int oflags, int devtype, struct thread *td) {
    return 0;
}

static int d_close(struct cdev *dev, int fflag, int devtype, struct thread *td) {
    return 0;
}

// move data from kernel buffer to user space
// KERNEL_BUFFER -> CAESAR_SHIFT -> CHECK IF CAESAR OFFSET WAS ALREADY USED
//  if false: transfer shifted cipher to the user
//  else: return an EOF to stop cat from reading
// NOTE THAT THIS IS NOT A THREAD SAFE OPERATION, IT IS JUST AN EXAMPLE
// NOTE THAT THIS IS INTENDED TO BE USED WITH THE CAT COMMAND
static int is_searching = 1;
static int d_read(struct cdev *dev, struct uio *uio, int ioflag) {
    int error;

    if (buf->length == 0)
        return 0;

    // send an EOF when caesar cipher has fully looped
    if (!is_searching) {
        is_searching = 1;
        return 0;
    }

    // display all caesar ciphers that are reachable with a given CAESAR_OFFSET_DELTA
    static int current_caesar_offset = 0;
    const int CAESAR_OFFSET_DELTA = 10;
    in_place_caesar_cipher(buf, CAESAR_OFFSET_DELTA);
    current_caesar_offset = (current_caesar_offset + CAESAR_OFFSET_DELTA) % 26;
    if (current_caesar_offset == 0)
        is_searching = 0;


    error = uiomove(buf->buffer, buf->length, uio);
    if (error != 0) {
        uprintf("Caesar driver error: uiomove failed\n");
        buf->length = 0;
        return 0;
    }

    return error;
}

// move data from user space to kernel buffer
static int d_write(struct cdev *dev, struct uio *uio, int ioflag) {
    int error;

    buf->length = MIN(uio->uio_resid, BUFFER_SIZE-1);

    error = uiomove(buf->buffer, buf->length, uio);
    if (error != 0) {
        uprintf("Caesar driver error: uiomove failed\n");
        buf->length = 0;
        return error;
    }

    buf->buffer[buf->length] = '\0';
    return 0;
}


// handle kernel interactions with driver
static int d_modevent(module_t mod, int type, void *arg) {
    int error = 0;

    switch (type) {
    case MOD_LOAD:
        // prealloc the r/w buffer
        buf = malloc(sizeof(*buf), M_CAESAR, M_WAITOK | M_ZERO);
        if (buf == NULL) {
            error = ENOMEM;
            uprintf("Driver failed to load. Failed to malloc\n");
        } else {
            sdev = make_dev(&caesar_cdevsw, 0, UID_ROOT, GID_WHEEL, 0666, "caesar");
            uprintf("Driver loaded into kernel\n");
        }
        break;
    case MOD_UNLOAD:
        free(buf, M_CAESAR);

        destroy_dev(sdev);
        uprintf("Driver unloaded successfully\n");
        break;
    default:
        error = EOPNOTSUPP;
        break;
    }

    return error;
}


DEV_MODULE(caesar, d_modevent, NULL);
