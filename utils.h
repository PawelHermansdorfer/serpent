//////////////////////////////////////////////////////////////////
/// File: utils.h                                              ///
/// Date: 2023-05-05 14:55:04                                  ///
/// Creator: Pawel Hermansdorfer                               ///
/// (C) Copyright by Pawel Hermansdorfer, all rights reserved. ///
//////////////////////////////////////////////////////////////////

#ifndef UTILS_H
#define UTILS_H

static inline void hide_cursor(void);
static inline void show_cursor(void);
static inline void enter_alt_buff(void);
static inline void exit_alt_buff(void);

static void move_cursor(unsigned int x, unsigned int y);
static inline void clear(void);
static void print(char *str, int size);

static void *memory_alloc(int size);
static void memory_free(void *p, int size);

static unsigned long long get_time_ms();
static unsigned int rnd(void);

#endif // UTILS_H
