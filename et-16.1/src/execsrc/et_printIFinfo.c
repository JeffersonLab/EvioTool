#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "etCommonNetwork.h"

static char *Sock_ntop_host(const struct sockaddr *sa, socklen_t salen);
static char *sock_ntop_host(const struct sockaddr *sa, socklen_t salen);


int main(int argc, char **argv) {

	struct ifi_info	*ifi, *ifihead;
	struct sockaddr	*sa;
	u_char *ptr;
	int i, family = AF_INET, doaliases = 1;

	printf("usage: prifinfo <inet4|inet6> <doaliases (1 or 0)>\n\n");

    if (argc > 1) {
        if (strcmp(argv[1], "inet4") == 0)
            family = AF_INET;
#ifdef    IPV6
	    else if (strcmp(argv[1], "inet6") == 0)
		    family = AF_INET6;
#endif
        else
            printf("invalid 1st arg, using inet4\n\n");
    }

    if (argc > 2) {
        doaliases = atoi(argv[2]);
    }

    for (ifihead = ifi = etNetGetInterfaceInfo(family, doaliases);
		 ifi != NULL; ifi = ifi->ifi_next) {
		printf("%s: <", ifi->ifi_name);

		if (ifi->ifi_flags & IFF_UP)            printf("UP ");
		if (ifi->ifi_flags & IFF_BROADCAST)     printf("BCAST ");
		if (ifi->ifi_flags & IFF_MULTICAST)     printf("MCAST ");
		if (ifi->ifi_flags & IFF_LOOPBACK)      printf("LOOP ");
		if (ifi->ifi_flags & IFF_POINTOPOINT)	printf("P2P ");
		printf(">\n");

		if ( (i = ifi->ifi_hlen) > 0) {
			ptr = ifi->ifi_haddr;
			do {
				printf("%s%x", (i == ifi->ifi_hlen) ? "  " : ":", *ptr++);
			} while (--i > 0);
			printf("\n");
		}

		if ( (sa = ifi->ifi_addr) != NULL) {
                  struct hostent *hptr;
                  char **pptr;
		          struct sockaddr_in *sin = (struct sockaddr_in *) sa;
                  printf("  IP addr: %s\n", Sock_ntop_host(sa, sizeof(*sa)));
                  
                  if ( (hptr = gethostbyaddr((const char *)&sin->sin_addr, sizeof(struct in_addr), AF_INET)) != NULL) {
                    printf("  name   : %s\n", hptr->h_name);
                    for (pptr= hptr->h_aliases; *pptr != NULL; pptr++) {
                      printf("  alias  : %s\n", *pptr);
                    }
                  }
                }
		if ( (sa = ifi->ifi_brdaddr) != NULL)
			printf("  broadcast addr: %s\n", Sock_ntop_host(sa, sizeof(*sa)));
		if ( (sa = ifi->ifi_dstaddr) != NULL)
			printf("  destination addr: %s\n", Sock_ntop_host(sa, sizeof(*sa)));
	}
    etNetFreeInterfaceInfo(ifihead);
	exit(0);
}

static char *sock_ntop_host(const struct sockaddr *sa, socklen_t salen) {

    static char str[128];       /* Unix domain is largest */

    switch (sa->sa_family) {
        case AF_INET: {
            struct sockaddr_in  *sin = (struct sockaddr_in *) sa;

            if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL) {
                return (NULL);
            }

            return(str);
        }

#ifdef  IPV6
        case AF_INET6: {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;

            if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL) {
                return(NULL);
            }

            return(str);
        }
#endif

        default:
            snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d", sa->sa_family, salen);
            return(str);
    }
    return (NULL);
}

static char *Sock_ntop_host(const struct sockaddr *sa, socklen_t salen) {

    char *ptr;

    if ((ptr = sock_ntop_host(sa, salen)) == NULL) {
        printf("sock_ntop_host error");    /* inet_ntop() sets errno */
    }
	return(ptr);
}
