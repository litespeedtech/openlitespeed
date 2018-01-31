/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2017  LiteSpeed Technologies, Inc.                 *
*                                                                            *
*    This program is free software: you can redistribute it and/or modify    *
*    it under the terms of the GNU General Public License as published by    *
*    the Free Software Foundation, either version 3 of the License, or       *
*    (at your option) any later version.                                     *
*                                                                            *
*    This program is distributed in the hope that it will be useful,         *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
*    GNU General Public License for more details.                            *
*                                                                            *
*    You should have received a copy of the GNU General Public License       *
*    along with this program. If not, see http://www.gnu.org/licenses/.      *
*****************************************************************************/
#ifndef LS_THREADCHECK_H
#define LS_THREADCHECK_H

#if !defined(__has_feature)
# define __has_feature(x) 0
#endif

#if defined(USE_THREADCHECK) && defined(__has_feature) && __has_feature(thread_sanitizer)
#include <lsr/dynamic_annotations.h>

#define LS_TH_CREATE(a) ANNOTATE_RWLOCK_CREATE(a)
#define LS_TH_DESTROY(a) ANNOTATE_RWLOCK_DESTROY(a)
#define LS_TH_LOCKED(a,b) ANNOTATE_RWLOCK_ACQUIRED(a,b)
#define LS_TH_UNLOCKED(a,b) ANNOTATE_RWLOCK_RELEASED(a,b)
#define LS_TH_NEWMEM(a,b) ANNOTATE_NEW_MEMORY(a,b)
#define LS_TH_BEFORE(a) ANNOTATE_HAPPENS_BEFORE(a)
#define LS_TH_AFTER(a) ANNOTATE_HAPPENS_AFTER(a)
#define LS_TH_PCQ_CREATE(a) ANNOTATE_PCQ_CREATE(a)
#define LS_TH_PCQ_DESTROY(a) ANNOTATE_PCQ_DESTROY(a)
#define LS_TH_PCQ_PUT(a) ANNOTATE_PCQ_PUT(a)
#define LS_TH_PCQ_GET(a) ANNOTATE_PCQ_GET(a)
#define LS_TH_IGN_RD_BEG() ANNOTATE_IGNORE_READS_BEGIN()
#define LS_TH_IGN_RD_END() ANNOTATE_IGNORE_READS_END()
#define LS_TH_IGN_SYNC_BEG() ANNOTATE_IGNORE_SYNC_BEGIN()
#define LS_TH_IGN_SYNC_END() ANNOTATE_IGNORE_SYNC_END()
#define LS_TH_PURE_BEFORE_MUTEX(mu) ANNOTATE_PURE_HAPPENS_BEFORE_MUTEX(mu)
#define LS_TH_NOT_BEFORE_MUTEX(mu) ANNOTATE_NOT_HAPPENS_BEFORE_MUTEX(mu)
#define LS_TH_BENIGN(a,b) ANNOTATE_BENIGN_RACE(a,b)
#define LS_TH_EXPECT(a,b) ANNOTATE_EXPECT_RACE(a,b)
#define LS_TH_FLUSH() ANNOTATE_FLUSH_EXPECTED_RACES() 

#else /* no checking or no thread sanitizer */

#define LS_TH_CREATE(a)
#define LS_TH_LOCKED(a, b)
#define LS_TH_UNLOCKED(a, b)
#define LS_TH_NEWMEM(a,b)
#define LS_TH_BEFORE(a)
#define LS_TH_AFTER(a)
#define LS_TH_PCQ_CREATE(a)
#define LS_TH_PCQ_DESTROY(a)
#define LS_TH_PCQ_PUT(a)
#define LS_TH_PCQ_GET(a)
#define LS_TH_IGN_RD_BEG()
#define LS_TH_IGN_RD_END()
#define LS_TH_IGN_SYNC_BEG()
#define LS_TH_IGN_SYNC_END()
#define LS_TH_PURE_BEFORE_MUTEX(mu)
#define LS_TH_NOT_BEFORE_MUTEX(mu)
#define LS_TH_BENIGN(a,b)
#define LS_TH_EXPECT(a,b)
#define LS_TH_FLUSH()

#endif /* no checking or no thread sanitizer */

#endif /* LS_THREADCHECK_H */
