/*
 * Simple Echo pseudo-device KLD
 *
 * Murray Stokely
 * Søren (Xride) Straarup
 * Eitan Adler
 */

#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h>  /* uprintf */
#include <sys/param.h>  /* defines used in kernel.h */
#include <sys/kernel.h> /* types used in module initialization */
#include <sys/conf.h>   /* cdevsw struct */
#include <sys/uio.h>    /* uio struct */
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/device.h>
#define BUFFERSIZE 255

/* Function prototypes */
static d_open_t     echo_open;
static d_close_t    echo_close;
static d_read_t     echo_read;
static d_write_t    echo_write;
static d_kqfilter_t	echo_kqfilter;


/* Character device entry points */
static struct dev_ops echo_cdevsw = {
		.head = {"echo"},
		.d_open = echo_open,
		.d_close = echo_close,
		.d_read = echo_read,
		.d_write = echo_write,
		.d_kqfilter = echo_kqfilter,
};


struct s_echo {
	char msg[BUFFERSIZE + 1];
	int len;
};

/* vars */
static cdev_t echo_dev;
static struct s_echo *echomsg;

MALLOC_DECLARE(M_ECHOBUF);
MALLOC_DEFINE(M_ECHOBUF, "echobuffer", "buffer for echo module");

/*
 * This function is called by the kld[un]load(2) system calls to
 * determine what actions to take when a module is loaded or unloaded.
 */
static int
echo_loader(struct module *m __unused, int what, void *arg __unused)
{
	int error = 0;
	
	switch (what) {
	case MOD_LOAD:                /* kldload */
		echo_dev = make_dev(&echo_cdevsw,
		    0,
		    UID_ROOT,
		    GID_WHEEL,
		    0600,
		    "echo");
		reference_dev(echo_dev);
		echomsg = kmalloc(sizeof(*echomsg), M_ECHOBUF, M_WAITOK |
		    M_ZERO);
		uprintf("Echo device loaded.\n");
		break;
	case MOD_UNLOAD:
	case MOD_SHUTDOWN:
		destroy_dev(echo_dev);
		kfree(echomsg, M_ECHOBUF);
		uprintf("Echo device unloaded.\n");
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}
	return (error);
}

//static int echo_open(struct cdev *dev __unused, int oflags __unused, int devtype __unused, struct thread *td __unused)
static int echo_open(struct dev_open_args* t)
{
	int error = 0;

	uprintf("Opened device \"echo\" successfully.\n");
	return (error);
}

static int
echo_close(struct dev_close_args* t)
{

	uprintf("Closing device \"echo\".\n");
	return (0);
}

/*
 * The read function just takes the buf that was saved via
 * echo_write() and returns it to userland for accessing.
 * uio(9)
 */
//static int echo_read(struct cdev *dev __unused, struct uio *uio, int ioflag __unused)
static int echo_read(struct dev_read_args* t)
{
	size_t amt;
	int error;

	/*
	 * How big is this read operation?  Either as big as the user wants,
	 * or as big as the remaining data.  Note that the 'len' does not
	 * include the trailing null character.
	 */
	amt = MIN(t->a_uio->uio_resid, t->a_uio->uio_offset >= echomsg->len + 1 ? 0 :
	    echomsg->len + 1 - t->a_uio->uio_offset);

	if ((error = uiomove(echomsg->msg, amt, t->a_uio)) != 0)
		uprintf("uiomove failed!\n");

	return (error);
}

/*
 * echo_write takes in a character string and saves it
 * to buf for later accessing.
 */
static int
echo_write(struct dev_write_args* t)
{
	size_t amt;
	int error;

	/*
	 * We either write from the beginning or are appending -- do
	 * not allow random access.
	 */
	if (t->a_uio->uio_offset != 0 && (t->a_uio->uio_offset != echomsg->len))
		return (EINVAL);

	/* This is a new message, reset length */
	if (t->a_uio->uio_offset == 0)
		echomsg->len = 0;

	/* Copy the string in from user memory to kernel memory */
	amt = MIN(t->a_uio->uio_resid, (BUFFERSIZE - echomsg->len));

	error = uiomove(echomsg->msg + t->a_uio->uio_offset, amt, t->a_uio);

	/* Now we need to null terminate and record the length */
	echomsg->len = t->a_uio->uio_offset;
	echomsg->msg[echomsg->len] = 0;

	if (error != 0)
		uprintf("Write failed: bad address!\n");
	return (error);
}
static void filt_echodetach(struct knote *kn);
static int filt_echoread(struct knote *kn, long hint);
static int filt_echowrite(struct knote *kn, long hint);

static struct filterops echoread_filtops =
	{ FILTEROP_ISFD ,
	  NULL, filt_echodetach, filt_echoread };

static struct filterops echowrite_filtops =
	{ FILTEROP_ISFD ,
	  NULL, filt_echodetach, filt_echowrite };

static int
echo_kqfilter(struct dev_kqfilter_args *t)
{
	struct knote *kn = t->a_kn;

	t->a_result = 0;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &echoread_filtops;
		break;
	case EVFILT_WRITE:
		kn->kn_fop = &echowrite_filtops;
		break;
	default:
		t->a_result = EOPNOTSUPP;
		return (0);
	}
	kn->kn_hook = (caddr_t)t->a_head.a_dev;
		
	return (0);
}

static void
filt_echodetach(struct knote *kn) { uprintf("filter gone\n"); }

static int
filt_echoread(struct knote *kn, long hint)
{
	cdev_t dev = (cdev_t)kn->kn_hook;
	int revents = 0;

	if(kn->kn_sfflags & NOTE_OLDAPI)
	{
		if (seltrue(dev, POLLIN | POLLRDNORM)){
			if (echomsg->len > 0) {
				revents |= seltrue(dev, POLLIN | POLLRDNORM);
				uprintf("poll.Have smth\n");
			}
			else 
				uprintf("poll. No deal\n");
		}
	}
	return revents;


}

static int
filt_echowrite(struct knote *kn, long hint)
{
	cdev_t dev = (cdev_t)kn->kn_hook;
	int revents = 0;

	if(kn->kn_sfflags & NOTE_OLDAPI)
	{
		if (seltrue(dev,POLLOUT | POLLWRNORM)){
			revents |= seltrue(dev, POLLOUT | POLLWRNORM);
			uprintf("poll\n");
		}
	}
	return revents;

}

DEV_MODULE(echo, echo_loader, NULL);
