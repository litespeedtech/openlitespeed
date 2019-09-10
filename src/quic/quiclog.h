#ifndef QUICLOG_H
#define QUICLOG_H

#include <liblsquic/lsquic_logger.h>

#define LS_DBG_LC(...) do {                             \
    char cidbuf_[MAX_CID_LEN * 2 + 1];                  \
    LS_DBG_L(__VA_ARGS__);                              \
} while (0)

#define LS_DBG_MC(...) do {                             \
    char cidbuf_[MAX_CID_LEN * 2 + 1];                  \
    LS_DBG_M(__VA_ARGS__);                              \
} while (0)

#endif
