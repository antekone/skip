#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>

#define bool char
#define true 1
#define false 0

void syntax() {
    puts("Syntax:");
    puts("");
    puts("-i <file>       input file");
    puts("-o <file>       output file");
    puts("-x <offset>     read offset, 0x12345");
    puts("-y <offset>     write offset, 0x12345");
}

int f_min_int(int a, int b) {
    return a > b ? b : a;
}

int f_max_int(int a, int b) {
    return a > b ? a : b;
}

void do_process_fd(FILE* fd, FILE* fw, uint64_t offset, uint64_t endoffset) {
    uint64_t curofs;
    int i;
    struct stat st;
    uint32_t prog;

    prog = 0;

    if(fstat(fileno(fd), & st)) {
        perror("fstat");
        return;
    }

    fseek(fd, offset, SEEK_SET);
    fseek(fw, endoffset, SEEK_SET);

    uint8_t secbuf[512];
    uint8_t skipbuf[8];

    for(; offset < st.st_size; ) {
        fread(secbuf, 512, 1, fd);
        fread(skipbuf, 8, 1, fd);
        fwrite(secbuf, 512, 1, fw);

        offset += 520;
    }
}

void process_fn(char *fn, char *ofn, uint64_t offset, uint64_t endoffset) {
    FILE* fd = fopen(fn, "r"); // O_RDONLY);
    if(fd == NULL) {
        perror("open");
        return;
    }

    FILE* fw = fopen(ofn, "wb+");
    if(fw == NULL) {
        fclose(fd);
        perror("open(O_CREAT)");
        return;
    }

    char *buf = malloc(8 * 1024 * 1024);
    char *buf2 = malloc(8 * 1024 * 1024);

    setvbuf(fd, buf, _IOFBF, 8 * 1024 * 1024);
    setvbuf(fw, buf2, _IOFBF, 8 * 1024 * 1024);

    do_process_fd(fd, fw, offset, endoffset);

    free(buf2);
    free(buf);

    fclose(fd);
    fclose(fw);
}

int main(int argc, char **argv) {
    int c;
    char infile[4096];
    char outfile[4096];

    char ofsbuf[64];
    char ofs2buf[64];

    uint64_t reading_offset = 0;
    uint64_t writing_offset = 0;

    bool use_offset = false;
    bool use_offset2 = false;

    while((c = getopt(argc, argv, "i:o:hx:")) != -1) {
        switch(c) {
            case 'i':
                strncpy(infile, optarg, f_min_int(sizeof(infile), strlen(optarg)));
                break;
            case 'o':
                strncpy(outfile, optarg, f_min_int(sizeof(outfile), strlen(optarg)));
                break;
            case 'h':
                syntax();
                return 0;
            case 'x':
                use_offset = true;
                strncpy(ofsbuf, optarg, f_min_int(sizeof(ofsbuf), strlen(optarg)));
                break;
            case 'y':
                use_offset2 = true;
                strncpy(ofs2buf, optarg, f_min_int(sizeof(ofs2buf), strlen(optarg)));
                break;
            default:
                return 1;
        }
    }

    if(! strlen(infile) || ! strlen(outfile)) {
        syntax();
        return 1;
    }

    if(use_offset && ! strlen(ofsbuf)) {
        puts("invalid read offset");
        return 1;
    }

    if(use_offset2 && ! strlen(ofs2buf)) {
        puts("invaild write offset");
        return 1;
    }

    if(use_offset && (ofsbuf[0] != '0' || ofsbuf[1] != 'x')) {
        puts("invalid read offset (pls use 0xabcd format)");
        return 1;
    }

    if(use_offset2 && (ofs2buf[0] != '0' || ofs2buf[1] != 'x')) {
        puts("invalid write offset (pls use 0xabcd format)");
        return 1;
    }

    reading_offset = strtoull(& ofsbuf[2], 0, 16);
    writing_offset = strtoull(& ofs2buf[2], 0, 16);

    printf("input file: %s\n", infile);
    printf("output file: %s\n", outfile);

    printf("starting offset: %llx\n", reading_offset);
    printf("ending offset:   %llx\n", writing_offset);

    process_fn(infile, outfile, reading_offset, writing_offset);
    return 0;
}
