#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

static char *mapped = NULL;
static size_t mapped_size = 0;

void sigbus_handler(int sig, siginfo_t *si, void *ctx) {
    void *addr = si->si_addr;
    int in_mmap = (mapped && addr >= (void*)mapped && addr < (void*)(mapped + mapped_size));

    fprintf(stderr, "SIGBUS Address: %p; code: %d\n", addr, si->si_code);

    if (in_mmap) {
        fprintf(stderr, "Inside mmap\n");
    } else {
        fprintf(stderr, "Outside mmap\n");
    }

    exit(EXIT_FAILURE);
}

void test_mmap_bus_error() {
    int fd = open("testfile", O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) { perror("open"); exit(1); }

    size_t page = sysconf(_SC_PAGESIZE);
    mapped_size = 2 * page;

    if (ftruncate(fd, mapped_size) == -1) { perror("ftruncate"); close(fd); exit(1); }

    mapped = mmap(NULL, mapped_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) { perror("mmap"); close(fd); exit(1); }

    if (ftruncate(fd, page / 2) == -1) { perror("ftruncate smaller"); }

    close(fd);

    printf("Calling outside of nmap...\n");
    mapped[page] = 'X';

    munmap(mapped, mapped_size);
}

int main() {
    struct sigaction sa = {.sa_flags = SA_SIGINFO, .sa_sigaction = sigbus_handler};
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGBUS, &sa, NULL) == -1) { perror("sigaction"); exit(1); }

    test_mmap_bus_error();

    printf("no SIGBUS\n");
    return 0;
}
