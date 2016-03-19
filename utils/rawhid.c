#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/ioctl.h>
#include <termios.h>
#elif defined(OS_WINDOWS)
#include <conio.h>
#include <Windows.h>
#endif

#include <hid.h>


static char get_keystroke(void);


int main()
{
	int i, r, num, k;
	char c, buf[64];

	// C-based example is 16C0:0480:FFAB:0200
found_device:
    r = -1;	
	while (r <= 0) {
        r = rawhid_open(1, 0x10C4, 0x8468, 0xFF00, 0x0001);
        if (r > 0) {
            k = 0;
            while (1) {
                memset(buf, 0, sizeof(buf));
                // check if any Raw HID packet has arrived
                num = rawhid_recv(0, buf, 64, 220);
                if (num < 0) {
                    printf("\nerror reading, device went offline\n");
                    rawhid_close(0);
                    Sleep(100);
                    goto found_device;
                }
                if (num > 0) {
                    if (buf[0] > 0) {
                        for (i = 0; i < buf[0]; ++i) {
                            printf("%c", buf[i + 1] & 255);
                        }
                    }
                    //			printf("\nrecv %d bytes:\n", num);
                    //			for (i=0; i<num; i++) {
                    //                char ch = buf[i] & 255;
                    //                if (ch < 32) {
                    //                    ch = '.';
                    //                } else if (ch == 0x7f) {
                    //                    ch = '.';
                    //                }
                    //				printf("%02X[%c] ", buf[i] & 255, ch);
                    //				if (i % 16 == 15 && i < num-1) printf("\n");
                    //			}
                    //			printf("\n");
                } else {
                    printf(".");
                    if (k > 2) {
                        k = 0;
                        printf("\n");
                        memset(buf, 0, sizeof(buf));
                        for (i = 0; i<64; i++) {
                            buf[i] = 0;
                        }
                        buf[0] = 0x02;
                        buf[1] = 0x06;
                        buf[2] = 0x51;
                        buf[3] = 0x32;
                        buf[4] = 0x44;
                        buf[5] = 0x0d;
                        buf[6] = 0x44;
                        buf[7] = 0x34;
                        rawhid_send(0, buf, 64, 100);
                    }
                    ++k;
                }
                // check if any input on stdin
                while ((c = get_keystroke()) >= 32) {
                    if (c == 'c' || c == 'C')
                        return EXIT_SUCCESS;
                }
            }
        }
        Sleep(100);
	}
	printf("found rawhid device\n");

   
}

#if defined(OS_LINUX) || defined(OS_MACOSX)
// Linux (POSIX) implementation of _kbhit().
// Morgan McGuire, morgan@cs.brown.edu
static int _kbhit() {
	static const int STDIN = 0;
	static int initialized = 0;
	int bytesWaiting;

	if (!initialized) {
		// Use termios to turn off line buffering
		struct termios term;
		tcgetattr(STDIN, &term);
		term.c_lflag &= ~ICANON;
		tcsetattr(STDIN, TCSANOW, &term);
		setbuf(stdin, NULL);
		initialized = 1;
	}
	ioctl(STDIN, FIONREAD, &bytesWaiting);
	return bytesWaiting;
}
static char _getch(void) {
	char c;
	if (fread(&c, 1, 1, stdin) < 1) return 0;
	return c;
}
#endif


static char get_keystroke(void)
{
	if (_kbhit()) {
		char c = _getch();
		if (c >= 32) return c;
	}
	return 0;
}


