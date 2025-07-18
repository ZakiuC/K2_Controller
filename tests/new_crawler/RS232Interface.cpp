#include "RS232Interface.h"

// 设置串口参数并打开串口
int setup_serial_port(const char *port, speed_t baudrate) {
    struct termios tty;         
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        close(fd);
        return -1;
    }

    cfsetospeed(&tty, baudrate);    
    cfsetispeed(&tty, baudrate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    // tty.c_cc[VTIME] = 20;
    tty.c_cc[VTIME] = 5;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    /*
    你在 setup_serial_port 中设置了 FNDELAY 标志，这会导致 read 在没有数据时立即返回 -1。
    如果希望 read 阻塞等待数据，可以移除 fcntl 调用。
    */
    // fcntl(fd, F_SETFL, FNDELAY);

    tcsetattr(fd, TCSANOW, &tty);

    return fd;
}