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
#include <dirent.h>
#include <sys/wait.h>

#define LENGTH 151
#define SIZE 1


// ########## Errors Handle Functions ##########
void closeErrorMes() {
    if(write(1, "Error in: close", 15) == -1) {}
}
void closeDirErrorMes() {
    if(write(1, "Error in: closedir", 18) == -1) {}
}
int closeFile(int fd) {
    if(close(fd) == -1) {
        closeErrorMes();
        return -1;
    }
    return 1;
}
int closeDirFunc(DIR* dir) {
    if(closedir(dir) == -1) {
        closeDirErrorMes();
        return -1;
    }
    return 1;
}
void openErrorMes() {
    if(write(1, "Error in: open", 14) == -1) {}
}
void openDirErrorMes() {
    if(write(1, "Error in: opendir", 17) == -1) {}
}
void readDirErrorMes() {
    if(write(1, "Error in: readdir", 17) == -1) {}
}
void statErrorMes() {
    if(write(1, "Error in: stat", 14) == -1) {}
}
void readErrorMes() {
    if(write(1, "Error in: read", 14) == -1) {}
}
void readError(int fd) {
    readErrorMes();
    closeFile(fd);
}
void forkErrorMes() {
    if(write(1, "Error in: fork", 14) == -1) {}
}
void waitErrorMes() {
    if(write(1, "Error in: waitpid", 17) == -1) {}
}
void execErrorMes() {
    if(write(1, "Error in: execvp", 16) == -1) {}
}
void removeErrorMes() {
    if(write(1, "Error in: remove", 16) == -1) {}
}


// ########## Read Paths lines from config file. ##########
int takePathsLines(char* configFile, char* mainDir, char* inputFile, char* outputFile) {
    int fd;
    if ((fd = open(configFile, O_RDONLY)) == -1) {
        openErrorMes();
        return -1;
    }

    char buf[1] = {'\0'};
    int index = 0, i = 0;
    char* temp = mainDir;
    size_t reading = SIZE;

    // Just 3 lines.
    while (index < 3) {
        while ((buf[0] != '\n') && (reading == SIZE)) {
            if((reading = read(fd, buf, SIZE)) == -1) {
                readError(fd);
                return -1;
            }

            if ((buf[0] != '\n') && (reading == SIZE)) {
                temp[i] = buf[0];
                i++;
            }
        }
        temp[i] = '\0';
        buf[0] = '\0';
        i = 0;
        index++;
        if (index == 1) {
            temp = inputFile;
        } else if (index == 2) {
            temp = outputFile;
        }
    }

    if(closeFile(fd) == -1) {
        return -1;
    }

    return 1;
}

// ########## Check valid\exist paths. ##########
int checkValidPaths(char* mainDir, char* inputFile, char* outputFile) {
    struct stat stat_p;

    if(stat(mainDir, &stat_p) == -1) {
        // Not exist.
        if (errno == ENOENT) {
            if(write(1, "Not a valid directory\n", 22) == -1) {}
        } else {
            statErrorMes();
        }
        return -1;
    } else if (!S_ISDIR(stat_p.st_mode)) {
        // Not directory.
        if(write(1, "Not a valid directory\n", 22) == -1) {}
        return -1;
    }

    if(stat(inputFile, &stat_p) == -1) {
        // Not exist.
        if (errno == ENOENT) {
            if(write(1, "Input file not exist\n", 21) == -1) {}
        } else {
            statErrorMes();
        }
        return -1;
    }

    if(stat(outputFile, &stat_p) == -1) {
        // Not exist.
        if (errno == ENOENT) {
            if(write(1, "Output file not exist\n", 22) == -1) {}
        } else {
            statErrorMes();
        }
        return -1;
    }
    return 1;
}


// ########## Run the executable file that created. ##########
int executeFile(char* inputFile, int fdErrors) {
    int fdOut, fdIn;
    if((fdOut = open("tempOutput.txt", O_CREAT | O_RDWR | O_APPEND | O_TRUNC, 0644)) == -1) {
        openErrorMes();
        return -1;
    }
    if((fdIn = open(inputFile, O_RDONLY, 0644)) == -1) {
        openErrorMes();
        return -1;
    }

    pid_t pid = fork();
    if(pid < 0) {
        forkErrorMes();
        return -1;
    } else if(pid == 0) {
        dup2(fdIn, 0);
        dup2(fdOut, 1);
        dup2(fdErrors, 2);

        char* arguments[2];
        arguments[0] = "./tempF.out";
        arguments[1] = NULL;
        if(execvp(arguments[0], arguments) == -1) {
            exit(-1);
        }
    } else {
        int stat;
        if(waitpid(pid, &stat, 0) == -1) {
            waitErrorMes();
            if(close(fdOut) == -1) {
                closeErrorMes();
            }
            if(close(fdIn) == -1) {
                closeErrorMes();
            }
            return -1;
        }
        // 255 is unsigned of -1.
        if((WIFEXITED(stat)) && (WEXITSTATUS(stat) == 255)) {
            execErrorMes();
            if(close(fdOut) == -1) {
                closeErrorMes();
            }
            if(close(fdIn) == -1) {
                closeErrorMes();
            }
            return -1;
        }
    }

    if(close(fdOut) == -1) {
        closeErrorMes();
        return -1;
    }
    if(close(fdIn) == -1) {
        closeErrorMes();
        return -1;
    }
    return 1;
}

// ########## Compare created output to correct output (by comp.out). ##########
int cmpOutputs(char* outputFile, int fdErrors) {
    pid_t pid = fork();
    if(pid < 0) {
        forkErrorMes();
        return -1;
    } else if(pid == 0) {
        char* arguments[4];
        arguments[0] = "./comp.out";
        arguments[1] = "tempOutput.txt";
        arguments[2] = outputFile;
        arguments[3] = NULL;
        dup2(fdErrors, 2);
        if(execvp(arguments[0], arguments) == -1) {
            exit(-1);
        }
    } else {
        int stat, result;
        if(waitpid(pid, &stat, 0) == -1) {
            waitErrorMes();
            return -1;
        }
        result = 0;
        if(WIFEXITED(stat)) {
            result = WEXITSTATUS(stat);
        }
        // 255 is unsigned of -1. exec or comp.out failed.
        if((WIFEXITED(stat)) && (WEXITSTATUS(stat) == 255)) {
            execErrorMes();
            return -1;
        }
        if((result >= 1) && (result <= 3)) {
            // return the result of comp.out.
            return result;
        }
    }
    return -1;
}

// ########## Check the current C file, and return appropriate grade. ##########
int cFileFunction(char* pathFile, char* inputFile, char* outputFile, int fdErrors) {
    pid_t pid = fork();
    if(pid < 0) {
        forkErrorMes();
        return -1;
    } else if(pid == 0) {
        char* arguments[5];
        arguments[0] = "gcc";
        arguments[1] = pathFile;
        arguments[2] = "-o";
        arguments[3] = "tempF.out";
        arguments[4] = NULL;
        dup2(fdErrors, 2);
        if(execvp(arguments[0], arguments) == -1) {
            exit(-1);
        }
    } else {
        int stat;
        if(waitpid(pid, &stat, 0) == -1) {
            waitErrorMes();
            return -1;
        }
        // 255 is unsigned of -1.
        if((WIFEXITED(stat)) && (WEXITSTATUS(stat) == 255)) {
            execErrorMes();
            return -1;
        } else if((WIFEXITED(stat)) && (WEXITSTATUS(stat) == 1)) {
            // gcc return 1 if failed - Compilation error.
            return 10;
        } else {
            // Success Compilation.
            // run the executable file.
            if(executeFile(inputFile, fdErrors) == -1) {
                if(remove("tempF.out") == -1) {
                    removeErrorMes();
                    //return -1;
                }
                return -1;
            } else {
                int resultCmp;
                if((resultCmp = cmpOutputs(outputFile, fdErrors)) == -1) {
                    if((remove("tempF.out") == -1) || (remove("tempOutput.txt") == -1)) {
                        removeErrorMes();
                        //return -1;
                    }
                    return -1;
                } else {
                    if((remove("tempF.out") == -1) || (remove("tempOutput.txt") == -1)) {
                        removeErrorMes();
                        //return -1;
                    }
                    if(resultCmp == 1) {
                        return 100;
                    } else if(resultCmp == 2) {
                        return 50;
                    } else if (resultCmp == 3) {
                        return 75;
                    }
                }
            }

        }
    }

    return -1;
}

// ########## Scan sub directory. ##########
int subScanFunction(char* pathSubDir, char* nameSubDir, char* inputFile, char* outputFile, int fdErrors) {
    DIR* pDir;
    struct dirent* pDirent;
    struct stat stat_p;

    if ((pDir = opendir(pathSubDir)) == NULL) {
        openDirErrorMes();
        return -1;
    }

    errno = 0;
    while ((pDirent = readdir(pDir)) != NULL) {
        char pathFile[LENGTH] = {0};
        strcat(strcat(strcat(pathFile,pathSubDir), "/"), pDirent->d_name);
        if((strcmp(pDirent->d_name, ".") != 0) && (strcmp(pDirent->d_name, "..") != 0)) {
            if(stat(pathFile, &stat_p) == -1) {
                statErrorMes();
                return -1;
            } else if ((S_ISREG(stat_p.st_mode)) && (strlen(pDirent->d_name) > 3)) {
                // Search regular c file.
                if((pDirent->d_name[strlen(pDirent->d_name) - 1] == 'c') &&
                (pDirent->d_name[strlen(pDirent->d_name) - 2] == '.')) {

                    int result = cFileFunction(pathFile, inputFile, outputFile, fdErrors);
                    if(result == -1) {
                        closeDirFunc(pDir);
                        return -1;
                    } else {
                        if(closeDirFunc(pDir) == -1) {
                            return -1;
                        }
                        return result;
                    }

                }
            }

        }
    }

    // If errno changed and get out from loop -> was readdir error.
    if(errno != 0) {
        readDirErrorMes();
        closeDirFunc(pDir);
        return -1;
    }

    if(closeDirFunc(pDir) == -1) {
        return -1;
    }
    // Go out the loop -> Have no c file.
    // No c file grade.
    return 0;
}

// ########## Add line to results.csv ##########
int addToResults(int fdCsv, char* name, int grade) {
    char info[20] = {0};
    char gradeStr[6] = {0};
    if(grade == 0) {
        strcpy(info, "NO_C_FILE\n");
        strcpy(gradeStr, ",0,");
    } else if(grade == 10) {
        strcpy(info, "COMPILATION_ERROR\n");
        strcpy(gradeStr, ",10,");
    } else if(grade == 50) {
        strcpy(info, "WRONG\n");
        strcpy(gradeStr, ",50,");
    } else if(grade == 75) {
        strcpy(info, "SIMILAR\n");
        strcpy(gradeStr, ",75,");
    } else if(grade == 100) {
        strcpy(info, "EXCELLENT\n");
        strcpy(gradeStr, ",100,");
    }
    if(write(fdCsv, name, strlen(name)) == -1) {
        return -1;
    }
    if(write(fdCsv, gradeStr, strlen(gradeStr)) == -1) {
        return -1;
    }
    if(write(fdCsv, info, strlen(info)) == -1) {
        return -1;
    }
    return 1;
}

// ########## Scan the directory from line 1. ##########
int scanFunction(char* mainDir, char* inputFile, char* outputFile) {
    DIR* pDir;
    struct dirent* pDirent;
    struct stat stat_p;

    if ((pDir = opendir(mainDir)) == NULL) {
        openDirErrorMes();
        return -1;
    }

    int fdErrors;
    if((fdErrors = open("errors.txt", O_CREAT | O_RDWR | O_APPEND | O_TRUNC, 0644)) == -1) {
        openErrorMes();
        closeDirFunc(pDir);
        return -1;
    }

    int fdCsv;
    if((fdCsv = open("results.csv", O_CREAT | O_RDWR | O_APPEND | O_TRUNC, 0644)) == -1) {
        openErrorMes();
        closeDirFunc(pDir);
        if(close(fdErrors) == -1) {
            closeErrorMes();
        }
        return -1;
    }

    errno = 0;
    while ((pDirent = readdir(pDir)) != NULL) {
        char pathSubDir[LENGTH] = {0};
        strcat(strcat(strcat(pathSubDir,mainDir), "/"), pDirent->d_name);
        if((strcmp(pDirent->d_name, ".") != 0) && (strcmp(pDirent->d_name, "..") != 0)) {
            if(stat(pathSubDir, &stat_p) == -1) {
                statErrorMes();
                return -1;
            } else if (S_ISDIR(stat_p.st_mode)) {
                // Search sub dirs.
                int result = subScanFunction(pathSubDir, pDirent->d_name, inputFile, outputFile, fdErrors);
                if(result == -1) {
                    closeDirFunc(pDir);
                    if(close(fdErrors) == -1) {
                        closeErrorMes();
                    }
                    if(close(fdCsv) == -1) {
                        closeErrorMes();
                    }
                    return -1;
                } else {
                    if (addToResults(fdCsv, pDirent->d_name, result) == -1) {
                        closeDirFunc(pDir);
                        if(close(fdErrors) == -1) {
                            closeErrorMes();
                        }
                        if(close(fdCsv) == -1) {
                            closeErrorMes();
                        }
                        return -1;
                    }
                }
            }

        }
    }

    // If errno changed and get out from loop -> was readdir error.
    if(errno != 0) {
        readDirErrorMes();
        closeDirFunc(pDir);
        if(close(fdErrors) == -1) {
            closeErrorMes();
        }
        if(close(fdCsv) == -1) {
            closeErrorMes();
        }
        return -1;
    }

    if(close(fdErrors) == -1) {
        closeErrorMes();
        closeDirFunc(pDir);
        if(close(fdCsv) == -1) {
            closeErrorMes();
        }
        return -1;
    }

    if(close(fdCsv) == -1) {
        closeErrorMes();
        closeDirFunc(pDir);
        return -1;
    }

    if(closeDirFunc(pDir) == -1) {
        return -1;
    }

    return 1;
}


int main(int argc, char* argv[]) {
    char* configFile = argv[1];
    char mainDir[LENGTH] = {0};
    char inputFile[LENGTH] = {0};
    char outputFile[LENGTH] = {0};

    if(takePathsLines(configFile, mainDir, inputFile, outputFile) == -1) {
        return -1;
    }

    if(checkValidPaths(mainDir, inputFile, outputFile) == -1) {
        return -1;
    }

    if (scanFunction(mainDir, inputFile, outputFile) == -1) {
        return -1;
    }

    return 0;
}
