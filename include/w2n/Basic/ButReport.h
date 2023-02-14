#ifndef W2N_BASIC_BUTREPORT_H
#define W2N_BASIC_BUTREPORT_H

#ifndef W2N_BUG_REPORT_URL
#define W2N_BUG_REPORT_URL "https://github.com/WeZZard/wasm2native/issues"
#endif

#define W2N_BUG_REPORT_MESSAGE_BASE                                      \
  "submit a bug report (" W2N_BUG_REPORT_URL ")"

#define W2N_BUG_REPORT_MESSAGE "please " W2N_BUG_REPORT_MESSAGE_BASE

#define W2N_CRASH_BUG_REPORT_MESSAGE                                     \
  "Please " W2N_BUG_REPORT_MESSAGE_BASE                                  \
  " and include the crash backtrace."

#endif // W2N_BASIC_BUTREPORT_H
