#define _GNU_SOURCE
#include<errno.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/un.h>
#include<sys/epoll.h>
#include<sys/prctl.h>
#include<sys/socket.h>
#include"list.h"
#include"getopt.h"
#include"logger.h"
#include"system.h"
#include"output.h"
#include"confd_internal.h"
#include"proctitle.h"
#define TAG "confd"

static char*sock=DEFAULT_CONFD;
static bool clean=false,protect=false;
static int efd=-1;

static void ctl_fd(int op,int fd){
	static struct epoll_event ev;
	ev.events=EPOLLIN,ev.data.fd=fd;
	epoll_ctl(efd,op,fd,&ev);
}

static void confd_cleanup(int s __attribute__((unused))){
	if(clean)return;
	clean=true;
	unlink(sock);
}

static void signal_handler(int s,siginfo_t*info,void*c __attribute__((unused))){
	if(info->si_pid<=1&&protect)return;
	tlog_info("initconfd exiting");
	confd_cleanup(s);
	exit(0);
}

static int confd_read(int fd){
	if(fd<0)ERET(EINVAL);
	errno=0;
	struct confd_msg msg;
	int e=confd_internal_read_msg(fd,&msg);
	if(e<0)return e;
	else if(e==0)return 0;
	struct confd_msg ret;
	confd_internal_init_msg(&ret,CONF_OK);
	int retdata=0;
	switch(msg.action){
		// command response
		case CONF_OK:case CONF_FAIL:break;

		// terminate confd
		case CONF_QUIT:
			tlog_notice("receive exit request");
			e=-4;
		break;

		// dump config store
		case CONF_DUMP:
			conf_dump_store();
		break;

		// delete item
		case CONF_DELETE:
			conf_del(msg.path);
		break;

		// get item type
		case CONF_GET_TYPE:
			ret.data.type=conf_get_type(msg.path);
		break;

		// get item as string
		case CONF_GET_STRING:{
			size_t s=msg.data.data_len;
			char*data=malloc(s+1);
			if(!data){
				if(s>0)lseek(fd,s,SEEK_CUR);
				break;
			}
			memset(data,0,s+1);
			if((size_t)read(fd,data,s)!=s)break;
			char*re=conf_get_string(msg.path,data);
			if(!re)re="";
			ret.data.data_len=strlen(re);
			confd_internal_send(fd,&ret);
			write(fd,re,ret.data.data_len);
			free(data);
		}return e;

		// get item as integer
		case CONF_GET_INTEGER:
			ret.data.integer=conf_get_integer(msg.path,msg.data.integer);
		break;

		// get item as boolean
		case CONF_GET_BOOLEAN:
			ret.data.boolean=conf_get_boolean(msg.path,msg.data.boolean);
		break;

		// put item as string
		case CONF_SET_STRING:{
			size_t s=msg.data.data_len,r;
			char*data=malloc(s+1);
			if(!data){
				if(s>0)lseek(fd,s,SEEK_CUR);
				break;
			}
			memset(data,0,s+1);
			do{errno=0;r=(size_t)read(fd,data,s);}while(errno==EAGAIN);
			if(r!=s){
				free(data);
				break;
			}
			retdata=-conf_set_string(msg.path,data);
			if(retdata!=0)free(data);
		}break;

		// put item as integer
		case CONF_SET_INTEGER:
			retdata=-conf_set_integer(msg.path,msg.data.integer);
		break;

		// put item as boolean
		case CONF_SET_BOOLEAN:
			retdata=-conf_set_boolean(msg.path,msg.data.boolean);
		break;

		// unknown
		default:telog_warn(
			"action %s(0x%X) not implemented",
			confd_action2name(msg.action),msg.action
		);
	}
	if(retdata==0&&errno!=0)retdata=errno;
	ret.code=retdata;
	confd_internal_send(fd,&ret);
	return e;
}

static int listen_confd_socket(){
	int fd,er;
	struct sockaddr_un un={.sun_family=AF_UNIX};
	if(strlen(sock)>=sizeof(un.sun_path))return trlog_error(-ENAMETOOLONG,"invalid socket path");
	strcpy(un.sun_path,sock);
	if(access(un.sun_path,F_OK)==0)return trlog_error(-EEXIST,"socket %s exists",un.sun_path);
	else if(errno!=ENOENT)return terlog_error(-errno,"failed to access %s",un.sun_path);
	if((fd=socket(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK,0))<0)return terlog_error(-errno,"cannot create socket");
	if(bind(fd,(struct sockaddr*)&un,sizeof(un))<0){
		telog_error("cannot bind socket");
		goto fail;
	}
	if(listen(fd,1)<0){
		telog_error("cannot listen socket");
		goto fail;
	}
	tlog_info("listen socket %s as %d",sock,fd);
	return fd;
	fail:
	er=errno;
	close(fd);
	unlink(un.sun_path);
	ERET(er);
}

int confd_thread(int cfd){
	static size_t es=sizeof(struct epoll_event);
	int r,e=0,fd;
	open_socket_logfd_default();
	tlog_info("confd start with pid %d",getpid());
	if((fd=listen_confd_socket())<0)return -1;
	struct epoll_event*evs;
	setproctitle("confd");
	prctl(PR_SET_NAME,"Config Daemon",0,0,0);
	action_signals(
		(int[]){SIGINT,SIGHUP,SIGQUIT,SIGTERM},
		4,signal_handler
	);
	if((efd=epoll_create(64))<0)
		return terlog_error(-errno,"epoll_create failed");
	if(!(evs=malloc(es*64))){
		telog_error("malloc failed");
		e=-errno;
		goto ex;
	}
	memset(evs,0,es*64);
	ctl_fd(EPOLL_CTL_ADD,fd);
	if(cfd>=0){
		confd_internal_send_code(cfd,CONF_OK,0);
		close(cfd);
	}
	while(1){
		r=epoll_wait(efd,evs,64,-1);
		if(r==-1){
			if(errno==EINTR)continue;
			telog_error("epoll failed");
			e=-1;
			goto ex;
		}else if(r==0)continue;
		else for(int i=0;i<r;i++){
			int f=evs[i].data.fd;
			if(f==fd){
				int n=accept(f,NULL,NULL);
				if(n<0){
					ctl_fd(EPOLL_CTL_DEL,fd);
					continue;
				}
				fcntl(n,F_SETFL,O_RDWR|O_NONBLOCK);
				ctl_fd(EPOLL_CTL_ADD,n);
			}else{
				int x=confd_read(f);
				if(x==EOF)ctl_fd(EPOLL_CTL_DEL,f);
				else if(x==-4)goto ex;
			}
		}
	}
	ex:
	confd_cleanup(0);
	exit(e);
}

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: iniconfd [OPTION]...\n"
		"Start Init Config daemon.\n\n"
		"Options:\n"
		"\t-s, --socket <SOCKET>  Listen custom control socket (default is %s)\n"
		"\t-d, --daemon           Run in daemon\n"
		"\t-h, --help             Display this help and exit\n",
		DEFAULT_CONFD
	);
}

int initconfd_main(int argc __attribute__((unused)),char**argv __attribute__((unused))){
	static const struct option lo[]={
		{"help",    no_argument,       NULL,'h'},
		{"daemon",  no_argument,       NULL,'d'},
		{"socket",  required_argument, NULL,'s'},
		{NULL,0,NULL,0}
	};
	int o;
	bool daemon=false;
	while((o=b_getlopt(argc,argv,"hqdD:s:",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		case 'd':daemon=true;break;
		case 's':sock=b_optarg;break;
		default:return 1;
	}
	if(daemon){
		chdir("/");
		switch(fork()){
			case 0:break;
			case -1:return re_err(1,"fork");
			default:_exit(0);
		}
		if(setsid()<0)return re_err(1,"setsid");
		switch(fork()){
			case 0:break;
			case -1:return re_err(1,"fork");
			default:_exit(0);
		}
	}
	return confd_thread(-1);
}

int start_confd(char*tag,pid_t*p){
	int fds[2],r;
	if(confd>=0)ERET(EEXIST);
	if(pipe(fds)<0)return -errno;
	pid_t pid=fork();
	switch(pid){
		case -1:return -errno;
		case 0:
			close_all_fd((int[]){fds[1]},1);
			protect=true;
			r=confd_thread(fds[1]);
			exit(r);
	}
	close(fds[1]);
	struct confd_msg msg;
	do{if(confd_internal_read_msg(fds[0],&msg)<0)ERET(EIO);}
	while(msg.action!=CONF_OK);
	if(p)*p=pid;
	close(fds[0]);
	return open_default_confd_socket(tag);
}