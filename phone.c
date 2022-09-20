#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <arpa/inet.h>

#include <pthread.h>

// クライアントの方
struct struct_client {
    FILE *fp_client;
    int s_client;
};

void *client_read(void *arg) {
    char buf_client[1];
    struct struct_client *f = (struct struct_client *)arg;
//    fprintf(stderr, "ここまでは来た");
    while (fread(buf_client, 1, 1, f->fp_client) == 1) {
        int test = send(f->s_client, buf_client, 1, 0);
        if (test == -1) {
            perror("send");
            exit(1);
        }
    }
    return NULL;
}


// クライアントの方
struct struct_server {
    FILE *fp_server;
    int s_server;
};


void *server_read(void *arg) {
    char buf_server[1];
    struct struct_server *f = (struct struct_server *) arg;
    while (fread(buf_server, 1, 1, f->fp_server) == 1) {
        int test = send(f->s_server, buf_server, 1, 0);
        if (test == -1) {
            perror("send");
            exit(1);
        }
    }
    return NULL;
}


// 接続をacceptしてからrecを起動する方法をとった。
int main(int argc, char **argv) {
    if (argc == 3) { // クライアント側になります。
//        printf("CLIENT");
//        fprintf(stderr, "%d",argc);

        // ソケット作成
        int s_client = socket(PF_INET, SOCK_STREAM, 0);

        // コネクト（指定したIPアドレスとポート番号に接続する）
        struct sockaddr_in addr_client;
        addr_client.sin_family = AF_INET;
        inet_aton(argv[1], &addr_client.sin_addr);
        addr_client.sin_port = htons(atoi(argv[2]));
        int ret = connect(s_client, (struct sockaddr *) &addr_client, sizeof(addr_client));

        if (ret == -1) {
            perror("connect");
            exit(1);
        }

        FILE *fp_client = NULL;
        // char *cmdline_client = "/bin/ls";
        char *cmdline_client = "rec -t raw -b 16 -c 1 -e s -r 44100 -";
        if ((fp_client = popen(cmdline_client, "r")) == NULL) {
            err(EXIT_FAILURE, "%s", cmdline_client);
        }

        // スレッドを１つ作る、こっちで送る
        pthread_t thread_client;
        struct struct_client client;
        client.s_client = s_client;
        client.fp_client = fp_client;
        pthread_create(&thread_client,NULL,(void *) client_read,&client);

        // 受け取る
        unsigned char data[1];
        while (1) {
            int n = recv(s_client, data, 1, 0);
            if (n == -1) {
                perror("read");
                exit(1);
            }
            if (n == 0) {
                break;
            }
            // printf("%ld %d \n",cnt, data[0]);
            write(STDOUT_FILENO, data, 1);
        }









    } else if (argc == 2){ // サーバー側になります。
//        printf("SERVER");
//        fprintf(stderr, "%d",argc);

        // ソケット作成
        int ss_server = socket(PF_INET, SOCK_STREAM, 0);

        // bindの部分
        struct sockaddr_in addr_server;
        addr_server.sin_family = AF_INET;
        addr_server.sin_port = htons(atoi(argv[1]));
        addr_server.sin_addr.s_addr = INADDR_ANY;
        if (bind(ss_server, (struct sockaddr *) &addr_server, sizeof(addr_server)) != 0) {
            perror("bind");
            exit(1);
        }

        // listenの部分
        int po = listen(ss_server, 10);
        if (po != 0) {
            perror("listen");
            exit(1);
        }

        // acceptの部分
        struct sockaddr_in addr;
        socklen_t len = sizeof(struct sockaddr_in);
        int s_server = accept(ss_server, (struct sockaddr *) &addr, &len);

        if (s_server == -1) {
            perror("accept");
            exit(1);
        }
        FILE *fp_server = NULL;
       char *cmdline_server = "rec -t raw -b 16 -c 1 -e s -r 44100 -";
        // char *cmdline_server = "/bin/ls";

        if ((fp_server = popen(cmdline_server, "r")) == NULL) {
            err(EXIT_FAILURE, "%s", cmdline_server);
        }

        // スレッドを１つ作る、こっちで送る
        pthread_t thread_server;
        struct struct_server server;
        server.s_server = s_server;
        server.fp_server = fp_server;
        pthread_create(&thread_server,NULL,(void *) server_read,&server);


        // 受け取る
        unsigned char data[1];
        while (1) {
            int n = recv(s_server, data, 1, 0);
            if (n == -1) {
                perror("read");
                exit(1);
            }
            if (n == 0) {
                break;
            }
            // printf("%ld %d \n",cnt, data[0]);
            write(STDOUT_FILENO, data, 1);
        }
    }

}