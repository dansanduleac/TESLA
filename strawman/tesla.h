/*
 * Copyright (c) 2012 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	TESLA_H

#ifndef	TESLA

/* If TESLA is not defined, provide macros that do nothing. */
#define tassert(X)
#define assert_previously(predicate)
#define assert_eventually(predicate)

#else	/* TESLA */

#include <stdbool.h>

/** Basic TESLA types (magic for the compiler to munge). */
typedef struct __tesla_event {} __tesla_event;
typedef struct __tesla_locality {} __tesla_locality;

/**
 * TESLA events can be serialised either with respect to the current thread
 * or, using explicit synchronisation, the global execution context.
 */
extern __tesla_locality *__tesla_global;
extern __tesla_locality *__tesla_perthread;

#define	TESLA_GLOBAL(pred)	TESLA_ASSERT(__tesla_global, pred)
#define	TESLA_PERTHREAD(pred)	TESLA_ASSERT(__tesla_perthread, pred)


/** Magic "function" representing a TESLA assertion. */
void __tesla_inline_assertion(const char *filename, int line, int count,
		__tesla_locality*, bool);

/** A more programmer-friendly version of __tesla_inline_assertion. */
#define TESLA_ASSERT(locality, predicate)				\
	__tesla_inline_assertion(					\
		__FILE__, __LINE__, __COUNTER__,			\
		locality, predicate					\
	)


/** A sequence of TESLA events. Can be combined with && or ||. */
bool __tesla_sequence(__tesla_event, ...);
#define	TSEQUENCE(x, ...)	__tesla_sequence(x, __VA_ARGS__)


/* TESLA events: */
/** Entering a function. */
__tesla_event __tesla_entered(void*);
#define	entered(f)	__tesla_entered(f)

/** Exiting a function. */
__tesla_event __tesla_leaving(void*);
#define	leaving(f)	__tesla_leaving(f)

/** Reaching the inline assertion. */
__tesla_event __tesla_now;
#define	TESLA_NOW &__tesla_now

/** The result of a function call (e.g., foo(x) == y). */
__tesla_event __tesla_call(bool);

/** A number of times to match an event: positive or "any number". */
typedef	int	__tesla_count;
#define	ANY_REP	-1

/** A repetition of events — this allows globby "?", "*", "+", etc. */
__tesla_event __tesla_repeat(__tesla_count, __tesla_count, __tesla_event);
#define	REPEAT(m, n, event)	__tesla_repeat(m, n, event)
#define	UPTO(n, event)		__tesla_repeat(0, n, event)
#define	ATLEAST(n, event)	__tesla_repeat(n, ANY_REP, event)

/** A value that could match a lot of function parameters. Maybe anything? */
void* __tesla_any();
#define ANY __tesla_any()


/** A more programmer-friendly way to write assertions about the past. */
#define since(bound, call)						\
	__tesla_sequence(						\
		bound,							\
		__tesla_call(call),					\
		__tesla_now						\
	)

/** A more programmer-friendly way to write assertions about the future. */
#define before(bound, call)						\
	__tesla_sequence(						\
		__tesla_now,						\
		__tesla_call(call),					\
		bound							\
	)


#endif /* TESLA */

#endif /* TESLA_H */

