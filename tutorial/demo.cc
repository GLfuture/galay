#define _GNU_SOURCE
#define __STDC_FORMAT_MACROS

#include <stdio.h>
#include <errno.h>
#include <libaio.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#define TEST_FILE   "aio_test_file"
#define TEST_FILE_SIZE  (127 * 1024)
#define NUM_EVENTS  128
#define ALIGN_SIZE  512
#define RD_WR_SIZE  1024

struct custom_iocb
{
    struct iocb iocb;
    int nth_request;
};

void aio_callback(io_context_t ctx, struct iocb *iocb, long res, long res2)
{
    struct custom_iocb *iocbp = (struct custom_iocb *)iocb;
    printf("nth_request: %d, request_type: %s, offset: %lld, length: %lu, res: %ld, res2: %ld\n", 
            iocbp->nth_request, (iocb->aio_lio_opcode == IO_CMD_PREAD) ? "READ" : "WRITE",
            iocb->u.c.offset, iocb->u.c.nbytes, res, res2);
}

int main(int argc, char *argv[])
{
    int efd, fd, epfd;
    io_context_t ctx;
    struct timespec tms;
    struct io_event events[NUM_EVENTS];
    struct custom_iocb iocbs[NUM_EVENTS];
    struct iocb *iocbps[NUM_EVENTS];
    struct custom_iocb *iocbp;
    int i, j, r;
    void *buf;
    struct epoll_event epevent;

    efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (efd == -1) {
        perror("eventfd");
        return 2;
    }

    fd = open(TEST_FILE, O_RDWR | O_CREAT | O_DIRECT, 0644);
    if (fd == -1) {
        perror("open");
        return 3;
    }
    ftruncate(fd, TEST_FILE_SIZE);
    
    ctx = 0;
    if (io_setup(8192, &ctx)) {
        perror("io_setup");
        return 4;
    }

    if (posix_memalign(&buf, ALIGN_SIZE, RD_WR_SIZE)) {
        perror("posix_memalign");
        return 5;
    }
    printf("buf: %p\n", buf);

    for (i = 0, iocbp = iocbs; i < NUM_EVENTS; ++i, ++iocbp) {
        iocbps[i] = &iocbp->iocb;
        io_prep_pread(&iocbp->iocb, fd, buf, RD_WR_SIZE, i * RD_WR_SIZE);
        io_set_eventfd(&iocbp->iocb, efd);
        io_set_callback(&iocbp->iocb, aio_callback);
        iocbp->nth_request = i + 1;
    }

    if (io_submit(ctx, NUM_EVENTS, iocbps) != NUM_EVENTS) {
        perror("io_submit");
        return 6;
    }

    epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        return 7;
    }

    epevent.events = EPOLLIN | EPOLLET;
    epevent.data.ptr = NULL;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &epevent)) {
        perror("epoll_ctl");
        return 8;
    }

    i = 0;
    while (i < NUM_EVENTS) {
        uint64_t finished_aio;

        if (epoll_wait(epfd, &epevent, 1, -1) != 1) {
            perror("epoll_wait");
            return 9;
        }

        if (read(efd, &finished_aio, sizeof(finished_aio)) != sizeof(finished_aio)) {
            perror("read");
            return 10;
        }

        printf("finished io number: %"PRIu64"\n", finished_aio);
    
        while (finished_aio > 0) {
            tms.tv_sec = 0;
            tms.tv_nsec = 0;
            r = io_getevents(ctx, 1, NUM_EVENTS, events, &tms);
            if (r > 0) {
                for (j = 0; j < r; ++j) {
                    ((io_callback_t)(events[j].data))(ctx, events[j].obj, events[j].res, events[j].res2);
                }
                i += r;
                finished_aio -= r;
            }
        }
    }
    
    close(epfd);
    free(buf);
    io_destroy(ctx);
    close(fd);
    close(efd);
    remove(TEST_FILE);

    return 0;
}
