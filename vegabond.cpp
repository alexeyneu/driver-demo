#include <iostream>
#include <sys/poll.h>


int main(int,char**)
{
	struct pollfd pfd[1];
	int nready;
	FILE *f = fopen("/dev/echo","r");
	if(f == NULL) { std::cout << "no device" << std::endl; return 0; }
	pfd[0].fd = fileno(f);
	pfd[0].events = POLLIN;
	nready = poll(pfd, 1, 15 * 1000);

	if (nready == -1)
		std::cout << "no poll" << std::endl;
	if (nready == 0)
		std::cout << "time out" << std::endl;
	if ((pfd[0].revents & (POLLERR|POLLNVAL)))
		std::cout << "bad fd " << pfd[0].fd;
	if ((pfd[0].revents & (POLLIN|POLLHUP))) {/* can read*/};
}

