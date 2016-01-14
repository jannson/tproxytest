/*
 * # iptables -t mangle -N DIVERT
 * # iptables -t mangle -A PREROUTING -p tcp -m socket -j DIVERT
 * # iptables -t mangle -A DIVERT -j MARK --set-mark 1
 * # iptables -t mangle -A DIVERT -j ACCEPT
 * # ip rule add fwmark 1 lookup 100
 * # ip route add local 0.0.0.0/0 dev lo table 100
 * # iptables -t mangle -A PREROUTING -p tcp --dport 80 -j TPROXY --tproxy-mark 0x1/0x1 --on-port 9401
 *
 */
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <linux/netfilter_ipv4.h>

int handle_client (int c, struct sockaddr_in *clntaddr);
int tunnel_transparently (int c, struct sockaddr_in *clntaddr, struct sockaddr_in *dstaddr);

int main (int argc, char **argv)
{
        int                     s;
        int                     c;
        short int               port;
        struct sockaddr_in      servaddr;
        struct sockaddr_in      clntaddr;
        int                     n;
        int                     ret;
        struct msghdr           msg;
        char                    cntrlbuf[64];
        struct iovec            iov[1];
        char                    *endptr;

        if (argc < 2)
        {
                printf ("usage: %s <port>\n", argv[0]);
                return -1;
        }

        port = strtol (argv[1], &endptr, 0);
        if (*endptr || port <= 0)
        {
                fprintf (stderr, "invalid port number %s.\n", argv[1]);
                return -2;
        }

        if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0)
        {
                fprintf (stderr, "error creating listening socket.\n");
                return -3;
        }

        n=1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));
        setsockopt(s, SOL_SOCKET, SO_BROADCAST, &n, sizeof(n));

        /* Enable TPROXY IP preservation */
        n=1;
        ret = setsockopt (s, SOL_IP, IP_TRANSPARENT, &n, sizeof(int));
        if (ret != 0)
        {
                fprintf (stderr, "error setting transparency for listening socket. err (#%d %s)\n", errno, strerror(errno));
                close (s);
                return -4;
        }

        memset (&servaddr, 0, sizeof (servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
        servaddr.sin_port = htons (port);


        if (bind (s, (struct sockaddr *) &servaddr, sizeof (servaddr)) < 0)
        {
                fprintf (stderr, "error calling bind()\n");
                return -6;
        }

        listen (s, 1024);

        while (1)
        {
                n=sizeof(clntaddr);
                if ((c = accept (s, (struct sockaddr *)&clntaddr, &n)) < 0)
                {
                        fprintf (stderr, "error calling accept()\n");
                        break;
                }

                handle_client (c, &clntaddr);
        }

        close (s);

        return 0;
}

int handle_client (int c, struct sockaddr_in *clntaddr)
{
        struct sockaddr_in      dstaddr={0,};
        int                     ret;
        int                     n;

        /* get original destination address */
        n=sizeof(struct sockaddr_in);
        ret = getsockopt (c, SOL_IP, IP_ORIGDSTADDR, &dstaddr, &n); // IP_ORIGDSTADDR = 20
        //ret = getsockopt (c, SOL_IP, SO_ORIGINAL_DST, &dstaddr, &n); // SO_ORIGINAL_DST = 80

        if (ret != 0)
        {
                fprintf (stderr, "error getting original destination address. err (#%d %s)\n", errno, strerror(errno));
                close (c);
                return -1;
        }

        dstaddr.sin_family = AF_INET;
        inet_aton("192.168.1.1", &dstaddr.sin_addr.s_addr);
        dstaddr.sin_port = htons(8080);
        printf ("original destination address %X:%d\n", dstaddr.sin_addr.s_addr, dstaddr.sin_port);

        ret = tunnel_transparently (c, clntaddr, &dstaddr);
        if (ret <= 0)
        {
                printf("ret=%d\n", ret);
                close (c);
                return -2;
        }

        close (c);
        return 0;
}

int tunnel_transparently (int c, struct sockaddr_in *clntaddr, struct sockaddr_in *dstaddr)
{
        int     d;
        int     n;
        int     ret;

        if (clntaddr == NULL || dstaddr == NULL)
        {
                return -1;
        }

        d = socket (AF_INET, SOCK_STREAM, 0);
        if (d == -1)
        {
                fprintf (stderr, "error creating socket (#%d %s)\n", errno, strerror(errno));
                return -2;
        }

        n=1;
        ret = setsockopt (d, SOL_IP, IP_TRANSPARENT, &n, sizeof(int));
        if (ret != 0)
        {
                fprintf (stderr, "error setting transparency towards destination. err (#%d %s)\n", errno, strerror(errno));
                close (d);
                return -3;
        }

        ret = bind (d, (struct sockaddr *)clntaddr, sizeof (struct sockaddr_in));
        if (ret != 0)
        {
                fprintf (stderr, "error binding to client . err (#%d %s)\n", errno, strerror(errno));
                close (d);
                return -4;
        }

        ret = connect (d, (struct sockaddr *)dstaddr, sizeof (*dstaddr));
        if (ret != 0)
        {
                fprintf (stderr, "error connecting to detination. err (#%d %s)\n", errno, strerror(errno));
                close (d);
                return -5;
        }
        // TODO: send / recv
        //

        close (d);

        return 0;
}
