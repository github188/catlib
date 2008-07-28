#include <sys/time.h>
#include <cat/io.h>
#include <cat/uevent.h>
#include <string.h>
#include <stdio.h>



struct ioctx {
	int		infd;
	int		outfd;
	int		len;
	struct ue_ioevent *in;
	struct ue_ioevent *out;
	char 		buf[256];
};





int incb(void *arg, struct callback *cb)
{
	struct ioctx *x;
	struct uemux *m;
	int n;

	x = cb->ctx;
	m = x->in->mux;
	n = io_try_read(x->infd, x->buf, sizeof(x->buf));
	if ( n == 0 ) { 
		ue_io_del(x->in);
		ue_io_del(x->out);
		return 0;
	} else if ( n > 0 ) {
		x->len = n;
		ue_io_cancel(x->in);
		ue_io_reg(m, x->out);
	} else {
		errsys("reading data!\n");
	}

	return 0;
}





int outcb(void *arg, struct callback *cb)
{
	struct ioctx *x;
	struct uemux *m;
	int n;

	x = cb->ctx;
	m = x->out->mux;
	n = io_try_write(x->outfd, x->buf, x->len);
	if ( n == 0 ) { 
		ue_io_del(x->in);
		ue_io_del(x->out);
		return 0;
	} else if ( n > 0 ) {
		x->len -= n;
		if ( x->len )
			memmove(x->buf, x->buf + n, x->len);
		else {
			ue_io_cancel(x->out);
			ue_io_reg(m, x->in);
		}
	} else {
		errsys("Error writing data");
	}

	return 0;
}





int percb(void *arg, struct callback *cb)
{
	printf("Periodic ping\n");
	return 0;
}




int attimecb(void *arg, struct callback *cb)
{
	printf("Attime ping\n");
	return 0;
}




int main(int argc, char *argv[])
{
	struct uemux m;
	struct timeval tv;
	unsigned long n;
	struct ioctx x;

	ue_init(&m);
	ue_tm_new(&m, UE_PERIODIC, 2000, percb, 0);
	ue_tm_new(&m, UE_TIMEOUT, 7000, attimecb, 0);

	x.infd  = 0;
	x.outfd = 1;
	x.in = ue_io_new(&m, UE_RD, 0, incb, &x);
	x.out = ue_io_new(&m, UE_WR, 1, outcb, &x);
	ue_io_cancel(x.out);

	ue_runfor(&m, 10000);

	return 0;
}
