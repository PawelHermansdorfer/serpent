//////////////////////////////////////////////////////////////////
/// File: utils.c                                              ///
/// Date: 2023-05-05 14:54:59                                  ///
/// Creator: Pawel Hermansdorfer                               ///
/// (C) Copyright by Pawel Hermansdorfer, all rights reserved. ///
//////////////////////////////////////////////////////////////////

static inline void
hide_cursor(void)
{
  write(1, "\033[?25l", 7);
}

static inline void
show_cursor(void)
{
  write(1, "\033[?25h", 7);
}

static inline void
enter_alt_buff(void)
{
  write(1, "\033[?1049h", 9);
}

static inline void
exit_alt_buff(void)
{
  write(1, "\033[?1049l", 9);
}

static void
move_cursor(unsigned int x, unsigned int y)
{
    // max uint32 4294967295 -> len==10
    // \033[line;columnH"

    //        \033[  x   ;    y   H
    char buffer[4 + 10 + 1 + 10 + 1] = "\033[";
    char *p = buffer + 4;

    x++;
    y++;
    for(int div = 1000000000; div > 0; div /= 10)
    {
        int a = y / div;
        y -= a * div;
        *p++ = '0' + a;
    }
    *p++ = ';';
    for(int div = 1000000000; div > 0; div /= 10)
    {
        int a = x / div;
        x -= a * div;
        *p++ = '0' + a;
    }
    *p++ = 'H';

	write(1, buffer, 26);
}

static inline void
clear(void)
{
    write(1, "\033[2J", 5);
}

static void
print(char *str, int size)
{
	write(1, str, size);
}

static void*
memory_alloc(int size)
{
    void *p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    return(p);
}

static void
memory_free(void *p, int size)
{
    munmap(p, size);
}

static unsigned long long get_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec*(unsigned long long)1000000+tv.tv_usec;
}

static unsigned int
rnd(void)
{
    static const unsigned long long M = 4093;
    static const unsigned long long N = 8191;
    static unsigned long long s = 0;

    // Get entropy
    if(!s)
    {
        getrandom(&s, 8, 0);
    }

    s = s * M + N;
    unsigned int rot = (s >> 60);
    unsigned int x = (unsigned int)s;
    unsigned int lrot = (32 - rot);
    if (lrot >= 32)
    {
        lrot = 0;
    }
    unsigned int r = (x >> rot) | (x << lrot);
    return(r);
}
