#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#define rep(i, n) for (int i = 0; i < (int)(n); i++)
#define Wid 10
#define Hei 20
#define Buf 20
extern int errno;
struct termios otty, ntty;
void game();
int kbhit();
int getch();
void tinit();
void dsleep(double wait);
void prBlc(int b);
void prMino(int kind, int y, int x);
void prTet(int n);
void prFrame();
void ghost(int n);
int mkNext();
int set(int m);
void ctrl(int n);
void hold();
void down();
void LR(int n);
void dLR(int n);
void tet(int level, int *levelcount, int *BtoB, int *ren);
int blackCount();
void prInf(int n);
void finish();
int board[Hei+Buf+1][Wid+2] = {{0}};
int score, turn, holded, lines, renM;
int next[14],fly = 0, sp = 0, mode;
typedef struct Mino {
    int type, dir;
    int block[4][2], ghost[4][2];
    // [0]:core, [1~3]:else
    // [][0]:y, [][1]:x
} Mino;
Mino mino;
typedef struct Enemy {
    char symbol[3];
    int HP, AtCount;
} Enemy;

int main() {
    // start
    tinit();
    srand((unsigned)time(NULL));
    while (1) {
        score= turn= holded= lines= renM= 0;
        prFrame();
        prMino(holded, 4, 1);
        // select mode
        prInf(0);
        mode = -1;
        while (!('0' <= mode && mode <= '3')) mode = getch();
        prFrame();
        game(mode);
        // play again
        int again = -1;
        struct timeval start_time, now_time;
        gettimeofday(&start_time, NULL);
        while (1) {
            if (kbhit()) again = getch();
            gettimeofday(&now_time, NULL);
            double duration = now_time.tv_sec  -start_time.tv_sec
                            +(now_time.tv_usec -start_time.tv_usec )/1000000.;
            if (duration > 1) break;
        }
        while (!kbhit()) continue;
        prInf(20);
        again = -1;
        while (!('0' <= again && again <= '1')) again = getch();
        if (again == '0') break;
        else continue;
    }
    finish();
}

void game() {
    // make frame of board
    rep(i, Hei+Buf+1) rep(j, Wid+2) board[i][j]   = 100;
    rep(i, Hei+Buf  ) rep(j, Wid  ) board[i][j+1] =   0;
    int c = Wid/2;
    switch (mode) {
        case '0':
        case '3':
            break;
        case '1':
            rep(i, Hei+Buf) rep(j, Wid) board[i][j+1  ] = 80;
            rep(i, Hei+Buf) rep(j, 4  ) board[i][j+c-1] =  0;
            board[Hei+Buf-2][c-1] = 80;
            board[Hei+Buf-1][c-1] = 80;
            board[Hei+Buf-1][c  ] = 80;
            break;
        case '2':
            rep(i, Hei*Wid/5){
                int y = Hei+Buf-1 -rand()%(Hei-5);
                int x = rand()%Wid +1;
                if (board[y][x]) i--;
                else board[y][x] = 80;
            }
            blackCount(1);
            break;
    }
    prTet(0);
    // declaration
    struct timeval start_time, now_time;
    int level = 1, levelcount = 0, BtoB = 0, ren = 0, lockLimit = 15;
    double thold = 1., duration;
    
    while (1) { // loop of locking mino

        int lockCount = 0, holdOK = 1;
        // set
        mino.type = mkNext();
        mino.dir = 0;
        int boty = 0;
        // finish
        if (set(mino.type)) {
            prInf(10);
            break;
        }
        if (mode != '2' && score >= 99999) {
            prInf(11);
            break;
        }
        if (mode == '\"') {
            prInf(12); 
            break;
        }
        // level up
        if (levelcount >= (level +4) *3) {
            levelcount -= (level +4) *3;
            if (level < 15) level++;
            if (level >= 10) lockLimit--;
            thold = pow((0.8 -(level-1) *0.007), level -1);
            printf("\x1b[21;1f       %2d", level);
        }

        while (1) { // loop of fall causing thold

            double thold2 = thold;
            if (fly == 2) {
                thold2 = (sqrt(thold) +1) /2;
                if (!lockCount) lockCount++;
            }
            gettimeofday(&start_time, NULL);

            while (1) {
                // check input
                if (kbhit()) {
                    int c = getch();
                    switch (c) {
                        case 'w': case 'A': while (fly) down(2); break;
                        case 's': case 'B': if (fly == 1) down(1); break;
                        case 'd': case 'C': LR( 1); break;
                        case 'a': case 'D': LR(-1); break;
                        case 'e': dLR( 1); break;
                        case 'q': dLR(-1); break;
                        case 'z': 
                            if (holdOK) {
                                hold();
                                holdOK--;
                                lockCount = 0;
                            } break;
                        case 'p': 
                            prInf(5);
                            while (!kbhit()) continue;
                            gettimeofday(&now_time, NULL);
                            thold2 = now_time.tv_sec  -start_time.tv_sec
                                   +(now_time.tv_usec -start_time.tv_usec) /1000000.
                                   + thold + 0.5;
                            prTet(0);
                            break;
                        case 27: prInf(10); return;
                        default: break;
                    }
                    if (fly == 2 && !lockCount) lockCount++;
                    // lockdown
                    if (lockCount && lockCount <= lockLimit) {
                        if (mino.block[0][0] > boty) {
                            boty = mino.block[0][0];
                            lockCount = 1;
                        }
                        lockCount++;
                        gettimeofday(&now_time, NULL);
                        thold2 = now_time.tv_sec  -start_time.tv_sec
                               +(now_time.tv_usec -start_time.tv_usec) /1000000.
                               + 5./(25-lockLimit);
                    }
                    if (fly == 2) {
                        int filled = 0;
                        rep(i, 4){
                            int y = mino.block[i][0], x = mino.block[i][1];
                            filled += board[y+1][x] /10;
                        }
                        if (!filled) {
                            fly = 1;
                            thold2 = 0;
                        }
                    }
                }
                // update time
                gettimeofday(&now_time, NULL);
                duration = now_time.tv_sec  -start_time.tv_sec
                         +(now_time.tv_usec -start_time.tv_usec )/1000000.;
                if (!fly) break; else
                if (duration > thold2) {
                    down(0);
                    while (lockCount > 15 && fly) down(0);
                    break;
                }
            }
            if (!fly) break;
        }
        tet(level, &levelcount, &BtoB, &ren); levelcount++; turn++;
        if (mode == '2') if (!blackCount(1)) mode = '\"';
    }
}

int kbhit(void){
    int ret;
    fd_set rfd;
    struct timeval timeout = {0,0};
    FD_ZERO(&rfd);
    FD_SET(0, &rfd); //0:stdin
    ret = select(1, &rfd, NULL, NULL, &timeout);
    if (ret == 1) return 1;
    else return 0;
}

int getch(void) {
    unsigned char c;
    int n;
    while ((n = read(0, &c, 1)) < 0 && errno == EINTR);
    if (n == 0) return -1;
    else return (int)c;
}

static void onsignal(int sig) {
    signal(sig, SIG_IGN);
    switch(sig){
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
        case SIGHUP:
        exit(1);
        break;
    }
}

void tinit(void){
    if (tcgetattr(1, &otty) < 0)
    ntty = otty;
    ntty.c_iflag &= ~(INLCR|ICRNL|IXON|IXOFF|ISTRIP);
    ntty.c_oflag &= ~OPOST;
    ntty.c_lflag &= ~(ICANON|ECHO);
    ntty.c_cc[VMIN] = 1;
    ntty.c_cc[VTIME] = 0;
    tcsetattr(1, TCSADRAIN, &ntty);
    signal(SIGINT, onsignal);
    signal(SIGQUIT, onsignal);
    signal(SIGTERM, onsignal);
    signal(SIGHUP, onsignal);
    printf("\x1b[?25l"); printf("\x1b[2J");
}

void dsleep(double wait) {
  if(wait != 0){
    double fWait = (double)clock()/CLOCKS_PER_SEC + wait;
    double time = (double)clock()/CLOCKS_PER_SEC;
    while (time <= fWait) {
      time = (double)clock()/CLOCKS_PER_SEC;
    }
  }
}

void prBlc(int b) {
    if (b >= 10) b /= 10;
    if (b) printf("\x1b[%dm  ", 40+(b%8));
    else printf("\x1b[0m  ");
}

void prMino(int kind, int y, int x) {
    printf("\x1b[%d;%df", y, x);
    switch (kind) {
        case 0: prBlc(0); prBlc(0); prBlc(0); prBlc(0); break;
        case 1: prBlc(1); prBlc(1); prBlc(0); prBlc(0); break;
        case 2: prBlc(0); prBlc(2); prBlc(2); prBlc(0); break;
        case 3: prBlc(0); prBlc(3); prBlc(3); prBlc(0); break;
        case 4: prBlc(4); prBlc(0); prBlc(0); prBlc(0); break;
        case 5: prBlc(0); prBlc(5); prBlc(0); prBlc(0); break;
        case 6: prBlc(6); prBlc(6); prBlc(6); prBlc(6); break;
        case 7: prBlc(0); prBlc(0); prBlc(7); prBlc(0); break;
        default: break;
    }
    printf("\x1b[%d;%df", y+1, x);
    switch (kind) {
        case 0: prBlc(0); prBlc(0); prBlc(0); prBlc(0); break;
        case 1: prBlc(0); prBlc(1); prBlc(1); prBlc(0); break;
        case 2: prBlc(2); prBlc(2); prBlc(0); prBlc(0); break;
        case 3: prBlc(0); prBlc(3); prBlc(3); prBlc(0); break;
        case 4: prBlc(4); prBlc(4); prBlc(4); prBlc(0); break;
        case 5: prBlc(5); prBlc(5); prBlc(5); prBlc(0); break;
        case 6: prBlc(0); prBlc(0); prBlc(0); prBlc(0); break;
        case 7: prBlc(7); prBlc(7); prBlc(7); prBlc(0); break;
        default: break;
    }
    printf("\x1b[0m");
}

void prTet(int n) {
    if (n) {
        printf("\x1b[%d;13f", n+1);
        rep(i, Wid) prBlc(board[n+Buf][i+1]);
        printf("\x1b[0m\n");
    } else {
        printf("\x1b[2;13f");
        rep(i, Hei){
            rep(j, Wid) prBlc(board[i+Buf][j+1]);
            printf("\x1b[0m\n\x1b[13G");
        }
    }
}

void prFrame() {
    printf("\x1b[0m\x1b[7;2f      \x1b[9;2f      \n\x1b[2G      \x1b[12;2f      ");
    printf("\x1b[2;9f");
    rep (i, Hei) {
        printf("  ＊");
        rep(j, Wid) printf("  ");
        printf("＊  \n\x1b[9G");
    }
    printf("  ");
    rep(i, Wid+2) printf("＊");
    printf("  ");
    printf("\x1b[2;3fHOLD");
    printf("\x1b[2;%dfNEXT", 2*Wid +19);
    if (mode == '2') printf("\x1b[14;1fLEFT : ");
    else printf("\x1b[14;1fSCORE :");
    printf("\n\x1b[G        0");
    printf("\x1b[17;1fLINES :");
    printf("\n\x1b[G        0");
    printf("\x1b[20;1fLEVEL :");
    printf("\n\x1b[G        1");
    printf("\n");
}

void ghost(int n) {
    // 0 : set+print
    // 1 : clear+set+print
    // 2 : print
    // 3 : clear
    if (n % 2) {
        rep(i, 4){
            printf("\x1b[0m");
            int y = mino.ghost[i][0], x = mino.ghost[i][1];
            printf("\x1b[%d;%df", y -Buf +1, x*2 +11);
            if ((y >= Buf+1) && !board[y-1][x]) printf("  ");
        }
    }
    if (n < 2) {
        int d = 0, f = 1;
        while (1) {
            rep(i, 4){
                int y = mino.block[i][0], x = mino.block[i][1];
                if (board[y+d][x] /10) {f--; break;}
            }
            if (f) d++;
            else break;
        }
        rep(i, 4){
            mino.ghost[i][0] = mino.block[i][0] +d;
            mino.ghost[i][1] = mino.block[i][1];
        }
    }
    if (n < 3) {
        rep(i, 4){
            printf("\x1b[0m");
            int y = mino.ghost[i][0], x = mino.ghost[i][1];
            if (board[y-1][x]) printf("\x1b[7m");
            printf("\x1b[%d;%df\x1b[%dm", y -Buf +1, x*2 +11, 30+mino.type);
            if (y >= Buf +1) printf("囗");
        }
        printf("\x1b[0m\n");
    }
}

int mkNext() {
    int set[7] = {1,2,3,4,5,6,7};
    if (turn == 0) {
        rep(i, 7) {
            int j = rand() %7;
            int k = set[i]; set[i] = set[j]; set[j] = k;
        }
        rep(i, 7) next[i] = set[i];
    }
    if (turn %7 == 0) {
        rep(i, 7) {
            int j = rand() %7;
            int k = set[i]; set[i] = set[j]; set[j] = k;
        }
        rep(i, 7) next[7+i] = set[i];
    }
    int ret = next[0];
    rep(i, 13) next[i] = next[i+1];
    rep(i, (Hei-1)/3) prMino(next[i], 3*i +4, 2*Wid +17);
    return ret;
}

int set(int m) {
    int c = Wid/2;
    switch (m) {
    case 1: // Z
        if (board[Buf-1][c-1] + board[Buf-1][c] + board[Buf][c] + board[Buf][c+1]) return 1;
        mino.block[1][0] = Buf-1; mino.block[1][1] = c-1;
        mino.block[2][0] = Buf-1; mino.block[2][1] = c;
        mino.block[0][0] = Buf;   mino.block[0][1] = c;
        mino.block[3][0] = Buf;   mino.block[3][1] = c+1;
        break;
    case 2: // S
        if (board[Buf-1][c] + board[Buf-1][c+1] + board[Buf][c-1] + board[Buf][c]) return 1;
        mino.block[1][0] = Buf-1; mino.block[1][1] = c;
        mino.block[2][0] = Buf-1; mino.block[2][1] = c+1;
        mino.block[3][0] = Buf;   mino.block[3][1] = c-1;
        mino.block[0][0] = Buf;   mino.block[0][1] = c;
        break;
    case 3: // O
        if (board[Buf-1][c] + board[Buf-1][c+1] + board[Buf][c] + board[Buf][c+1]) return 1;
        mino.block[1][0] = Buf-1; mino.block[1][1] = c;
        mino.block[2][0] = Buf-1; mino.block[2][1] = c+1;
        mino.block[0][0] = Buf;   mino.block[0][1] = c;
        mino.block[3][0] = Buf;   mino.block[3][1] = c+1;
        break;
    case 4: // J
        if (board[Buf-1][c-1] + board[Buf][c-1] + board[Buf][c] + board[Buf][c+1]) return 1;
        mino.block[1][0] = Buf-1; mino.block[1][1] = c-1;
        mino.block[2][0] = Buf;   mino.block[2][1] = c-1;
        mino.block[0][0] = Buf;   mino.block[0][1] = c;
        mino.block[3][0] = Buf;   mino.block[3][1] = c+1;
        break;
    case 5: // T
        if (board[1][c] + board[Buf][c-1] + board[Buf][c] + board[Buf][c+1]) return 1;
        mino.block[1][0] = Buf-1; mino.block[1][1] = c;
        mino.block[2][0] = Buf;   mino.block[2][1] = c-1;
        mino.block[0][0] = Buf;   mino.block[0][1] = c;
        mino.block[3][0] = Buf;   mino.block[3][1] = c+1;
        break;
    case 6: // I
        if (board[Buf-1][c] + board[Buf-1][c-1] + board[Buf-1][c] + board[Buf-1][c+1]) return 1;
        mino.block[1][0] = Buf-1; mino.block[1][1] = c-1;
        mino.block[0][0] = Buf-1; mino.block[0][1] = c;
        mino.block[2][0] = Buf-1; mino.block[2][1] = c+1;
        mino.block[3][0] = Buf-1; mino.block[3][1] = c+2;
        break;
    case 7: // L
        if (board[Buf-1][c-1] + board[Buf][c] + board[Buf][c+1] + board[Buf][c+2]) return 1;
        mino.block[1][0] = Buf-1; mino.block[1][1] = c+1;
        mino.block[2][0] = Buf;   mino.block[2][1] = c-1;
        mino.block[0][0] = Buf;   mino.block[0][1] = c;
        mino.block[3][0] = Buf;   mino.block[3][1] = c+1;
        break;
    }
    fly = 1;
    ghost(0);
    ctrl(1);
    return 0;
}

void ctrl(int n) {
    rep(i, 4){
        int y = mino.block[i][0], x = mino.block[i][1];
        board[y][x] = mino.type *n;
        if (y >= Buf) {
            printf("\x1b[%d;%df\x1b", y -Buf +2, x*2 +11);
            prBlc(board[y][x]);
            printf("\x1b[0m\n");
        }
    }
}

void hold() {
    ctrl(0);
    int c = mino.type; mino.type = holded; holded = c;
    if (!mino.type) {
        turn++;
        mino.type = mkNext();
    }
    prMino(holded, 4, 1);
    ghost(3);
    set(mino.type);
}

void down(int s) {
    ctrl(0);
    if (fly == 2) {ctrl(10); fly = 0; return;}
    rep(i, 4){
        int y = mino.block[i][0], x = mino.block[i][1];
        if (board[y+1][x] /10) {ctrl(1); ghost(2); fly = 2; return;} 
    }
    rep(i, 4) mino.block[i][0]++;
    ctrl(1);
    ghost(2);
    score += s;
    if (mode != '2') printf("\x1b[0m\x1b[15;1f   %6d", score);
}

void LR(int n) {
    rep(i, 4){
        int y = mino.block[i][0], x = mino.block[i][1];
        if (board[y][x+n] /10) return;
    }
    ctrl(0); 
    rep(i, 4) mino.block[i][1] += n;
    ctrl(1);
    ghost(1);
}

void dLR(int n) {
    if (mino.type == 3 || !fly) return; 
    int filled[5] = {0}, vec[4][2], check[4][2], SRS[5][2] = {{0}}, spin = 1;
    rep(i, 4){ 
        rep(j, 2) vec[i][j] = mino.block[i][j] - mino.block[0][j];
        int d = vec[i][0];
        vec[i][0] = vec[i][1] *n;
        vec[i][1] = -d *n;
        rep(j, 2) check[i][j] = mino.block[0][j] + vec[i][j];
    }
    if (mino.type == 6) {
        int pm  = mino.dir %2 *-2 +1;
        int pm2 = mino.dir /2 *-2 +1;
        int pm3 = pm * pm2;
        switch (pm *n) {
            case 1:
                                    SRS[0][1] =  pm3;
                                    SRS[1][1] = -pm3;
                                    SRS[2][1] =  pm3*2;
                SRS[3][0] =  pm3;   SRS[3][1] = -pm3;
                SRS[4][0] = -pm3*2; SRS[4][1] =  pm3*2;
            break;
            case -1:
                SRS[0][0] =  pm2;
                SRS[1][0] =  pm2;   SRS[1][1] = -pm2;
                SRS[2][0] =  pm2;   SRS[2][1] =  pm2*2;
                SRS[3][0] = -pm2;   SRS[3][1] = -pm2;
                SRS[4][0] =  pm2*2; SRS[4][1] =  pm2*2;
            break;
        } 
    } else {
        int pmx = mino.dir /2 *2 -1;
        switch (mino.dir %2) {
            case 0:
                                SRS[1][1] =  n*pmx;
                SRS[2][0] = -1; SRS[2][1] =  n*pmx;
                SRS[3][0] =  2;
                SRS[4][0] =  2; SRS[4][1] =  n*pmx;
                break;
            case 1:
                                SRS[1][1] = -1*pmx;
                SRS[2][0] =  1; SRS[2][1] = -1*pmx;
                SRS[3][0] = -2;
                SRS[4][0] = -2; SRS[4][1] = -1*pmx;
                break;
        }
    }
    rep(i, 4){
        int y = check[i][0], x = check[i][1];
        rep(j, 5) filled[j] += board[y+SRS[j][0]][x+SRS[j][1]] /10;
    }
    ctrl(0);
    rep(i, 4) {
             if (!filled[0]) rep(j, 2) mino.block[i][j] = check[i][j] + SRS[0][j];
        else if (!filled[1]) rep(j, 2) mino.block[i][j] = check[i][j] + SRS[1][j];
        else if (!filled[2]) rep(j, 2) mino.block[i][j] = check[i][j] + SRS[2][j];
        else if (!filled[3]) rep(j, 2) mino.block[i][j] = check[i][j] + SRS[3][j];
        else if (!filled[4]) rep(j, 2) mino.block[i][j] = check[i][j] + SRS[4][j];
        else spin = 0;
    }
    ctrl(1);
    ghost(1);
    if (spin) {
        mino.dir += n;
        if (mino.dir == -1) mino.dir = 3;
        if (mino.dir ==  4) mino.dir = 0;
    }
    if (mino.type == 5 && spin) {
        int y = mino.block[0][0], x = mino.block[0][1];
        if (board[y-1][x-1] /10) {sp +=2; sp += (mino.dir +3) %4 /2;}
        if (board[y-1][x+1] /10) {sp +=2; sp += (mino.dir +2) %4 /2;}
        if (board[y+1][x+1] /10) {sp +=2; sp += (mino.dir +1) %4 /2;}
        if (board[y+1][x-1] /10) {sp +=2; sp += (mino.dir   ) %4 /2;}
        if (sp >= 8) sp = 2; else
        if (sp == 7) sp = 1; else
                     sp = 0;
    }
}

void tet(int level, int *levelcount, int *BtoB, int *ren) {
    // erase 
    printf("\x1b[0m\x1b[7;2f      \x1b[9;2f      \n\x1b[2G      \x1b[12;2f      ");
    // check clears
    int arrs[20] = {0}, count = 0;
    rep(i, Hei){
        long check = 1;
        rep(j, Wid) check *= board[i+Buf][j+1];
        if (check != 0) {
            arrs[count] = i;
            count++;
        }
    }
    int count2 = count, scoreAdd = 0;
    // display
    if (count) {
        *levelcount += count*2;
        lines += count;
        printf("\x1b[18;1f    %5d", lines);
    }
    // special clears
    // T-Spin
    if (sp) {
        printf("\x1b[9;2f\x1b[1m\x1b[7m\x1b[35m");
        if (sp == 2) printf("T-Spin\n\x1b[2G");
        if (sp == 1) printf("Mini-T\n\x1b[2G");
        switch (count) {
            case 0: *ren = 0; break;
            case 1: printf("single"); break;
            case 2: printf("double"); break;
            case 3: printf("triple"); break;
        }
        printf("\x1b[0m\n");
        if (sp == 2 || count) *BtoB += 1;
        count += sp;
        sp = 0;
    // Tetris
    } else if (count == 4) {
        printf("\x1b[9;2f\x1b[1m\x1b[7m\x1b[36mTETRIS\x1b[0m");
        *BtoB += 1;
    } else if (count) *BtoB = 0;
    // Back to Back
    if (count) {
        if (*BtoB >= 2) {
            printf("\x1b[12;2f\x1b[1m\x1b[7m");
            printf("\x1b[31m \x1b[33mB\x1b[32mt\x1b[36mo\x1b[34mB\x1b[35m ");
            printf("\x1b[0m");
            count++;
        }
    // add score
        if (mode != '2') scoreAdd = Wid *5 *(count *(count+1) +*ren*2) *(1. +(level-1)*level/20.);
        if (mode == '1') scoreAdd *= 4./Wid;
        if (scoreAdd) {
            score += scoreAdd;
            printf("\x1b[15;1f   +%5d", scoreAdd);
        }
    // ren
        *ren += 1;
        if (*ren >= 10) printf("\x1b[7;2f\x1b[1m\x1b[7m%dren!\x1b[0m", *ren); else
        if (*ren >=  2) printf("\x1b[7;2f\x1b[1m\x1b[7m %dren \x1b[0m",*ren);
        if (*ren > renM) renM = *ren;
    } else *ren = 0;
    // animation
    if (count) {
        rep(i, Wid){
            rep(j, count2) {
                board[arrs[j]+Buf][i+1] = mino.type;
                printf("\x1b[%d;%df\x1b", arrs[j] +2, i*2 +13);
                prBlc(mino.type);
            }
            printf("\x1b[0m\n");
            dsleep(0.15 / (Wid+level));
        }
        dsleep(1.5 / (Wid+level));
        rep(i, Wid){
            rep(j, count2) {
                board[arrs[j]+Buf][i+1] = 0;
                printf("\x1b[%d;%df\x1b", arrs[j] +2, i*2 +13);
                prBlc(0);
            }
            printf("\x1b[0m\n");
            dsleep(0.15 / (Wid+level));
        }
        rep(i, count2){
            rep(j, Wid) {
                for(int k = arrs[i] +Buf; k > 0; k--)
                board[k][j+1] = board[k-1][j+1];
                if (mode != '1') board[0][j+1] = 0;
            }
            dsleep(1.5 / (Wid+level)); prTet(0);
        }
    }
    // all clear
    rep(i, Hei) if(board[Hei+Buf-1][i+1]) return;
    scoreAdd = (1000+(level-1)*level*50) *Wid;
    score += scoreAdd;
    *levelcount += (level +4) *3;
    rep(i, 3){
        printf("\x1b[6;%df\x1b[1m\x1b[7m", Wid +10);
        printf("\x1b[31m \x1b[33mA\x1b[32ml\x1b[36ml\x1b[34m \x1b[35m ");
        printf("\x1b[7;%df", Wid +10);
        printf("\x1b[31m \x1b[33mC\x1b[32ml\x1b[36me\x1b[34ma\x1b[35mr");
        printf("\x1b[0m");
        printf("\x1b[15;1f  +%6d\n", scoreAdd);
        dsleep(0.2);
        printf("\x1b[6;%df      ", Wid +10);
        printf("\x1b[7;%df      ", Wid +10);
        printf("\x1b[15;1f         \n");
        dsleep(0.2);
    }
}

int blackCount(int print) {
    int count = 0;
    rep(i, Hei) rep(j, Wid) if(board[i+Buf][j+1] == 80) count++;
    if (print) printf("\x1b[0m\x1b[15;1f      %3d", count);
    return count;
}

void prInf(int n) {
    prTet(0);
    printf("\x1b[%d;%df//            \\\\", Hei/2 -2, Wid +5);
    switch (n) {
        case 0:
            printf("\n\x1b[%dG  Select Mode   ", Wid +5);
            printf("\n\x1b[%dG                ", Wid +5);
            printf("\n\x1b[%dG  Normal   : 0  ", Wid +5);
            printf("\n\x1b[%dG  chaRENge : 1  ", Wid +5);
            printf("\n\x1b[%dG  Bl@ck    : 2  ", Wid +5);
            printf("\n\x1b[%dG                ", Wid +5);
            // printf("\n\x1b[%dG  Battle   : 3  ", Wid +5);
            break;
        case 5:
            printf("\n\x1b[%dG                ", Wid +5);
            printf("\n\x1b[%dG     PAUSE      ", Wid +5);
            printf("\n\x1b[%dG                ", Wid +5);
            printf("\n\x1b[%dG    Any Key     ", Wid +5);
            printf("\n\x1b[%dG   to Resume    ", Wid +5);
            printf("\n\x1b[%dG                ", Wid +5);
            break;
        case 10:
            printf("\n\x1b[%dG                ", Wid +5);
            printf("\n\x1b[%dG                ", Wid +5);
            printf("\n\x1b[%dG   Game Over!   ", Wid +5);
                 if (mode == '0') printf("\n\x1b[%dG    %5d pts   ",    Wid +5, score);
            else if (mode == '1') printf("\n\x1b[%dG   MAX %2d ren   ", Wid +5, renM);
            else if (mode == '2') printf("\n\x1b[%dG      %2d left   ", Wid +5, blackCount(0));
            printf("\n\x1b[%dG                ", Wid +5);
            printf("\n\x1b[%dG                ", Wid +5);
            break;
        case 11:
            if (holded) turn--;
            printf("\n\x1b[%dG   Game Clear   ", Wid +5);
            printf("\n\x1b[%dG   99999+ pts   ", Wid +5);
            printf("\n\x1b[%dG                ", Wid +5);
            printf("\n\x1b[%dG   Record :     ", Wid +5);
            printf("\n\x1b[%dG    %3d turns   ", Wid +5, turn);
                 if (mode == '0') printf("\n\x1b[%dG    %3d lines   ",  Wid +5, lines);
            else if (mode == '1') printf("\n\x1b[%dG   MAX %2d ren   ", Wid +5, renM);
            break;
        case 12:
            if (holded) turn--;
            printf("\n\x1b[%dG   Game Clear   ", Wid +5);
            printf("\n\x1b[%dG   All Bl@cks   ", Wid +5);
            printf("\n\x1b[%dG   Eliminated   ", Wid +5);
            printf("\n\x1b[%dG                ", Wid +5);
            printf("\n\x1b[%dG   Record :     ", Wid +5);
            printf("\n\x1b[%dG    %3d turns   ", Wid +5, turn);
            break;
        case 20:
            printf("\n\x1b[%dG                ", Wid +5);
            printf("\n\x1b[%dG   Play Again?  ", Wid +5);
            printf("\n\x1b[%dG                ", Wid +5);
            printf("\n\x1b[%dG    YES : 1     ", Wid +5);
            printf("\n\x1b[%dG    NO  : 0     ", Wid +5);
            printf("\n\x1b[%dG                ", Wid +5);
            break;
    }
    printf("\n\x1b[%dG\\\\            //\n", Wid +5);
}

void finish() {
    printf("\x1b[?25h");
    tcsetattr(1, TCSADRAIN, &otty);
    write(1, "\n", 1);
    printf("\x1b[%d;1f\x1b[K\n", 3+Hei);
    exit(1);
}