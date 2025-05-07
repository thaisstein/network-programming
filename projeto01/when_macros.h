#ifndef WHEN_MACROS_H
#define WHEN_MACROS_H

#define when_true_ret(cond, retval, fmt, ...)                                  \
  do {                                                                         \
    if (cond) {                                                                \
      fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__);                         \
      return retval;                                                           \
    }                                                                          \
  } while (0)

#define when_true_jmp(cond, label, fmt, ...)                                   \
  do {                                                                         \
    if (cond) {                                                                \
      fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__);                         \
      goto label;                                                              \
    }                                                                          \
  } while (0)

#define when_false_ret(cond, retval, fmt, ...)                                 \
  when_true_ret(!(cond), retval, fmt, __VA_ARGS__)
#define when_null_ret(cond, retval, fmt, ...)                                  \
  when_true_ret((cond) == NULL, retval, fmt, __VA_ARGS__)
#define when_false_jmp(cond, label, fmt, ...)                                  \
  when_true_jmp(!(cond), label, fmt, __VA_ARGS__)
#define when_null_jmp(cond, label, fmt, ...)                                   \
  when_true_jmp((cond) == NULL, label, fmt, __VA_ARGS__)

#endif // !WHEN_MACROS_H
