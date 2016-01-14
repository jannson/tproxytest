//iptables -t mangle -A PREROUTING -p udp --dport 80 -j TPROXY  --tproxy-mark 0x1/0x1 --on-port 8000
//ip rule add fwmark 1 lookup 100
//ip route add local 0.0.0.0/0 dev eth1 table 100
//sudo iptables -t mangle -A PREROUTING -p udp --dport 9999 -j TPROXY  --tproxy-mark 0x1/0x1 --on-port 8000

#include <netinet/in.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <string.h>
 #include <stdio.h>

 #define TARGET "10.1.1.90"
 #define TPORT  9999
 #define LOCAL  "0.0.0.0"
 #define LPORT  8000

 int main()
 {
 	struct sockaddr_in name, client[1], server;
 	int value = 1, opt=1;
 	char buffer[1024];
 	int len, ret;
 	len = sizeof(struct sockaddr_in);


 	int fd1;
 	fd1 = socket(AF_INET, SOCK_DGRAM, 0);
 	if (fd1 < 0){
 		perror("error");
 	}

 	ret = setsockopt(fd1, SOL_IP, IP_TRANSPARENT, &value, sizeof(value));
 	if (ret < 0){
 		perror("error");
 	}

 	name.sin_family = AF_INET;
 	name.sin_port = htons(LPORT);
 	name.sin_addr.s_addr = htonl(INADDR_ANY);
 	ret = bind(fd1, (struct sockaddr *)&name, sizeof(name));
 	if (ret < 0){
 		perror("error");
 	}


 	int fd2;
 	fd2 = socket(AF_INET, SOCK_DGRAM, 0);
 	if (fd2 < 0){
 		perror("error");
 	}

 	ret = setsockopt(fd2, SOL_IP, IP_TRANSPARENT, &value, sizeof(value));
 	if (ret < 0){
 		perror("error");
 	}

 	server.sin_family=AF_INET;
 	server.sin_port=htons(TPORT);
 	server.sin_addr.s_addr=inet_addr(TARGET);

 	ret = bind(fd2, (struct sockaddr *)&server, sizeof(server));
 	if (ret < 0){
 		perror("error");
 	}


 	int fd;
 	while(1){
 		memset(buffer, 0, sizeof(buffer));
 		ret = recvfrom(fd1,buffer,sizeof(buffer),0,(struct sockaddr*)&name,&len);
 		if (ret < 0){
 			perror("error");
 		}

 		if(name.sin_addr.s_addr != server.sin_addr.s_addr){
 			fd = fd1;
 			client[0] = name;
 			name = server;
 			printf("get the buffer from client: %s\n", buffer);
 		}
 		else{
 			fd = fd2;
 			name = client[0];
 			printf("get the buffer from server: %s\n", buffer);
 		}

 		ret = sendto(fd,buffer,strlen(buffer),0,(struct sockaddr *)&name,len);
 		if (ret < 0){
 			perror("error");
 		}
 	}
 }
