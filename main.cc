#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <iostream>
#include <list>

#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "agent.h"
#include "connection.h"
#include "main.h"
#include "map.h"
#include "rawio.h"

int tick_id = 0;

int agent_id = 0;
std::list<class agent *> agents;
int client_n = 0;

static void
drop_agents(void)
{
	client_n = 0;
	for (std::list<class agent *>::iterator agent = agents.begin(); agent != agents.end(); agent++)
	{
next_agent:
		if (!(*agent)->conn && !(*agent)->tile) {
			delete *agent;
			agent = agents.erase(agent);
			if (agent != agents.end())
				goto next_agent;
		} else if ((*agent)->conn) {
			client_n++;
		}
	}
}

static void
clear(void)
{
	printf("\033[H\033[J");
	fflush(stdout);
}

int
main(int argc, char *argv[])
{
	class map map(argc < 2 ? 40 : atoi(argv[1]), argc < 3 ? 20 : atoi(argv[2]));
	int herbs = argc < 4 ? map.w * map.h / world::herb_rate : atoi(argv[3]);

	#ifdef RAWIO
		if(rawio_cfg(&map)==-1)
			std::cout<<"rawio_cfg: Cannost open cfg file"<<std::endl;
	#endif
	
	srandom(time(NULL));

	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(27754);
	sin.sin_addr.s_addr = INADDR_ANY;
	int optval = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	bind(lfd, (struct sockaddr *) &sin, sizeof(sin));
	listen(lfd, 10);

	signal(SIGPIPE, SIG_IGN);
	int flags = fcntl(lfd, F_GETFL, 0);
	fcntl(lfd, F_SETFL, flags | O_NONBLOCK);

	/* Spawn herbs. */

	for (int i = 0; i < herbs; i++) {
		class agent *a = new class herb(agent_id++, map);
		agents.push_back(a);
		a->spawn();
	}

	/* Main tick loop. */

	while (true) {
		clear();
		std::cout << "tick " << tick_id << "; agents " << agents.size() << "; clients " << client_n << '\n';

		/* Drop disconnected agents. */

		drop_agents();

		/* Accept new agents. */

		int cfd = accept(lfd, NULL, NULL);
		if (cfd >= 0) {
			class connection *conn = new class connection(cfd);
			class agent *a = new class agent(agent_id++, conn, map);
			agents.push_back(a);
		}

		/* Collect and take actions. */

		for (std::list<class agent *>::iterator agent = agents.begin(); agent != agents.end(); agent++)
			(*agent)->on_action_takes();
		drop_agents(); /* Some agents might have died. */

		/* Run on_tick everywhere. */

		map.on_tick();
		for (std::list<class agent *>::iterator agent = agents.begin(); agent != agents.end(); agent++)
			(*agent)->on_tick();
		drop_agents(); /* Some agents might have died. */

		/* Update agents' senses. */

		for (std::list<class agent *>::iterator agent = agents.begin(); agent != agents.end(); agent++)
			(*agent)->on_senses_update();

		/* Finish a tick. */

		map.print_map();
		#ifdef RAWIO
			if(rawio_map(&map)==-1)
				std::cout<<"Rawio_map: Cannot open map file"<<std::endl;
		#endif
		std::cout << '\n';
		usleep(200000);
		tick_id++;
	}

	return 0;
}
