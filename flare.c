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
#include <sys/thread.h>

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

/*driver control block */
struct echo_ff
{
	struct kqinfo ffread;
};
/* end of driver control block */

/* vars */
static cdev_t echo_dev;
static struct s_echo *echomsg;


MALLOC_DECLARE(M_ECHOBUF);
MALLOC_DEFINE(M_ECHOBUF, "echobuffer", "buffer for echo module");

MALLOC_DECLARE(M_ECHODEVICE);
MALLOC_DEFINE(M_ECHODEVICE, "echodevice", "echo device");

/*
 * This function is called by the kld[un]load(2) system calls to
 * determine what actions to take when a module is loaded or unloaded.
 */
static int
echo_loader(struct module *m __unused, int what, void *arg __unused)
{
	int error = 0;
	struct echo_ff *tr;
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
		tr = kmalloc(sizeof(*tr), M_ECHODEVICE, M_WAITOK |
		    M_ZERO);
		echo_dev->si_drv1 = tr;
		uprintf("Echo device loaded.\n");
		break;
	case MOD_UNLOAD:
	case MOD_SHUTDOWN:
		tr = echo_dev->si_drv1;
		destroy_dev(echo_dev);
		kfree(echomsg, M_ECHOBUF);
		kfree(tr, M_ECHODEVICE);
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
	bool ter = 0;
	int error;
	cdev_t dev = t->a_head.a_dev;
	struct echo_ff *tr = dev->si_drv1;
	struct klist *klist = &tr->ffread.ki_note;	
	/*
	 * We either write from the beginning or are appending -- do
	 * not allow random access.
	 */
	if (t->a_uio->uio_offset != 0 && (t->a_uio->uio_offset != echomsg->len))
		return (EINVAL);

	/* This is a new message, reset length */
	if (t->a_uio->uio_offset == 0)
	{
		echomsg->len = 0;
		if (t->a_uio->uio_resid != 0) ter = 1;
	}

	/* Copy the string in from user memory to kernel memory */
	amt = MIN(t->a_uio->uio_resid, (BUFFERSIZE - echomsg->len));

	error = uiomove(echomsg->msg + t->a_uio->uio_offset, amt, t->a_uio);

	/* Now we need to null terminate and record the length */
	echomsg->len = t->a_uio->uio_offset;
	echomsg->msg[echomsg->len] = 0;
	if ( ter == 1) KNOTE(klist ,0);
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
filt_echodetach(struct knote *kn) 
{ 
	cdev_t dev = (cdev_t)kn->kn_hook;
	struct echo_ff *tr = dev->si_drv1;
	struct klist *klist = &tr->ffread.ki_note;
	struct knote *kn_b = NULL;

	lwkt_getpooltoken(klist);
	if( !SLIST_EMPTY(klist) ) 
		SLIST_FOREACH(kn_b, klist, kn_next)
			if( kn_b == kn) break;
	if( kn_b == kn ) knote_remove(klist, kn);
	lwkt_relpooltoken(klist);

	uprintf("filter gone\n"); 
}

static int
filt_echoread(struct knote *kn, long hint)
{
	cdev_t dev = (cdev_t)kn->kn_hook;	
	struct echo_ff *tr = dev->si_drv1;
	struct klist *klist = &tr->ffread.ki_note;

	if(kn->kn_sfflags & NOTE_OLDAPI)
	{
		if (echomsg->len > 0) {
			uprintf("poll.Have smth\n");
			return 1;
		}
		else { // wait
			lwkt_getpooltoken(klist);
			knote_insert(klist, kn);
			lwkt_relpooltoken(klist);
			
			uprintf("poll. No deal\n");
		}
	}
	return 0;


}

static int
filt_echowrite(struct knote *kn, long hint)
{
	cdev_t dev = (cdev_t)kn->kn_hook;

	if(kn->kn_sfflags & NOTE_OLDAPI)
	{
		uprintf("poll\n");
		return 1;
	}
	return 0;

}

DEV_MODULE(echo, echo_loader, NULL);
