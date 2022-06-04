//Ben Levi 318811304

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/types.h>

#define SIZE 1


// ########## Errors Handle Functions ##########
void closeErrorMes() {
    if(write(1, "Error in: close", 15) == -1) {}
}
void readErrorMes() {
    if(write(1, "Error in: read", 14) == -1) {}
}
void openErrorMes() {
    if(write(1, "Error in: open", 14) == -1) {}
}
int closeFiles(int fd1, int fd2) {
    if(close(fd1) == -1) {
        closeErrorMes();

        if(close(fd2) == -1){
            closeErrorMes();
        }

        return -1;
    }
    if(close(fd2) == -1) {
        closeErrorMes();
        return -1;
    }
    return 1;
}
void readError(int fd1, int fd2) {
    readErrorMes();
    closeFiles(fd1, fd2);
}


// ########## Compare Files Function ##########
int checkFiles(char* path1, char* path2) {
    int fd1, fd2;

    if ((fd1 = open(path1, O_RDONLY)) == -1) {
        openErrorMes();
        return -1;
    }
    if ((fd2 = open(path2, O_RDONLY)) == -1) {
        openErrorMes();
        return -1;
    }

    char buf1[SIZE] = {'\n'};
    char buf2[SIZE] = {'\n'};
    size_t lastReading1;
    size_t lastReading2;
    bool equalFlag = true;

    do {
        buf1[0] = '\n';
        buf2[0] = '\n';

        // ##### Read character from the files. #####
        if((lastReading1 = read(fd1, buf1, SIZE)) == -1) {
            readError(fd1, fd2);
            return -1;
        }
        if((lastReading2 = read(fd2, buf2, SIZE)) == -1) {
            readError(fd1, fd2);
            return -1;
        }

        // ##### Character not equal, Or file shorter than the other. #####
        if (((lastReading1 == SIZE) && (lastReading2 == SIZE) && (buf1[0] != buf2[0])) ||
            ((lastReading1 == SIZE) && (lastReading2 != SIZE)) ||
            ((lastReading1 != SIZE) && (lastReading2 == SIZE))) {
            equalFlag = false;
        }

        if (equalFlag == false) {
            // ##### Advance to regular character. #####
            while((lastReading1 == SIZE) && ((buf1[0] == '\n') || (buf1[0] == ' '))) {
                if((lastReading1 = read(fd1, buf1, SIZE)) == -1) {
                    readError(fd1, fd2);
                    return -1;
                }
            }

            while((lastReading2 == SIZE) && ((buf2[0] == '\n') || (buf2[0] == ' '))) {
                if((lastReading2 = read(fd2, buf2, SIZE)) == -1) {
                    readError(fd1, fd2);
                    return -1;
                }

            }

            // ##### Regular character not equal -> check if similar. #####
            if ((buf1[0] != '\n') && (buf1[0] != ' ') && (buf2[0] != '\n') && (buf2[0] != ' ')) {
                if (toupper(buf1[0]) != toupper(buf2[0])) {
                    return 2;
                }
            }
        }

    } while ((lastReading1 == SIZE) && (lastReading2 == SIZE));


    if(closeFiles(fd1, fd2) == -1) {
        return -1;
    }


    // ##### If stay regular character in only one file. #####
    // Caused from the loop condition.
    if (!((lastReading1 != SIZE) && (lastReading2 != SIZE))) {
        return 2;
    }

    if(equalFlag) {
        return 1;
    }
    return 3;
}


int main(int argc, char* argv[]) {
    char* path1 = argv[1];
    char* path2 = argv[2];
    return checkFiles(path1, path2);
}
