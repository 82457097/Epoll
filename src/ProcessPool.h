#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

class Process {
public:
    process() : m_pid(-1) {}

public:
    pid_t m_pid;
    int m_pipefd[2];
};

template<typename T> 
class ProcessPool {
private:
    ProcessPool(int listenfd, int process_number = 8);
public:
    static ProcessPool* create(int listenfd, int Process_number = 8) {
        if(!m_instance) {
            m_instance = new ProcessPool(listenfd, Process_number);
        }
        return m_instance;
    }

    ~ProcessPool() {
        delete [] m_sub_process;
    }

    void run();

private:
    void setup_sig_pipe();
    void run_parent();
    void run_child();

private:
    static const int MAX_PROCESS_NUMBER = 16;
    static const int USER_PER_PROCESS = 65535;
    static const int MAX_EVENT_NUMBER = 10000;
    int m_process_number;
    int m_idx;
    int m_epollfd;
    int m_listenfd;
    int m_stop;
    Process* m_sub_process;
    static ProcessPool* m_instance;
};

template<typename T>
ProcessPool* ProcessPool::m_instance = nullptr;

static int sig_pipefd[2];
static int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NOBLOCK;
    fcntl(fd, F_SETFL, new_option);
    
    return old_option;
}

static void addfd(int epollfd, int fd) {
    epoll_evnet event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

static void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

static void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

static void addsig(int sig, void(handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if(restart) {
        sa.sa_flag |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

template<typename T>
ProcessPool::ProcessPool(int listenfd, int process_number) : m_listenfd(listenfd),
    m_process_number(Process_number), m_idx(-1), m_stop(false) {
    assert((process_number > 0) && (process_number <= MAX_EVENT_NUMBER));
    m_sub_process = new Process[process_number];
    assert(m_sub_process);

    for(int i = 0; i < process_number; ++i) {
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
        assert(ret == 0);

        m_sub_process[i].m_pid = fork();
        assert(m_sub_process[i].m_pid >= 0);
        if(m_sub_process[i].m_pid > 0) {
            close(m_sub_process[i].m_pipefd[1]);
            continue;
        } else {
            close(m_sub_process[i].m_pipefd[0]);
            m_idx = i;
            break;
        }
    }
}

template<typename T>
void ProcessPool::run() {
    if(m_idx != -1) {
        run_child();
        return ;
    }
    run_parent();
}

template<typename T>
void ProcessPool::run_child() {
    setup_sig_pipe();
    int pipefd = m_sub_process[m_idx].m_pipefd[1];
    addfd(m_epollfd, pipefd);

    epoll_event events[MAX_EVENT_NUMBER];
    T* users = new T [USER_PER_PROCESS];
    assert(users);
    int number = 0;
    int ret = -1;

    while(!m_stop) {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if((number < 0) && (errno != EINTR)) {
            printf("epoll failed\n");
            break;
        }
    }
}


