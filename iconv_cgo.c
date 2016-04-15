#include "iconv_cgo.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define ICONV_CGO_MIN_BUF_SIZE 256
#define ICONV_CGO_MAX_BUF_SIZE ((size_t)1 << (sizeof(size_t) * 8 - 1))

#define ICONV_CGO_SUFFIX_TRANSLIT_STR "//TRANSLIT"
#define ICONV_CGO_SUFFIX_IGNORE_STR   "//IGNORE"

enum {
  ICONV_CGO_SUFFIX_NONE,
  ICONV_CGO_SUFFIX_TRANSLIT,
  ICONV_CGO_SUFFIX_IGNORE,
};

typedef struct iconv_cgo_t {
  iconv_t cd;         /* Conversion descriptor */
  char    *to_code;   /* Destination character code (to be freed) */
  char    *from_code; /* Source character code (to be freed) */
  int     suffix;     /* Suffix type of to_code */
  char    *buf;       /* Buffer for output */
  size_t  buf_size;   /* Buffer size */
} iconv_cgo_t;

/*
 * iconv_cgo_init() initializes an instance of iconv_cgo_t.
 *
 * The caller must ensure that h is valid.
 */
static void iconv_cgo_init(iconv_cgo_h h) {
  h->cd = (iconv_t)-1;
  h->to_code = NULL;
  h->from_code = NULL;
  h->suffix = ICONV_CGO_SUFFIX_NONE;
  h->buf = NULL;
  h->buf_size = 0;
}

/*
 * iconv_cgo_new() creates an instance of iconv_cgo_t and returns a pointer
 * to the instance.
 *
 * On failure, iconv_cgo_fin() sets errno and returns NULL.
 */
static iconv_cgo_h iconv_cgo_new(void) {
  iconv_cgo_h h = (iconv_cgo_h)malloc(sizeof(iconv_cgo_t));
  if (h == NULL) {
    return NULL;
  }
  iconv_cgo_init(h);
  return h;
}

/*
 * iconv_cgo_fin() finalizes an instance of iconv_cgo_t.
 *
 * The caller must ensure that h is valid.
 *
 * On success, iconv_cgo_fin() returns 0.
 * On failure, iconv_cgo_fin() sets errno and returns -1.
 */
static int iconv_cgo_fin(iconv_cgo_h h) {
  int result = 0;
  if (h->buf != NULL) {
    free(h->buf);
    h->buf = NULL;
    h->buf_size = 0;
  }
  if (h->from_code != NULL) {
    free(h->from_code);
    h->from_code = NULL;
  }
  if (h->to_code != NULL) {
    free(h->to_code);
    h->to_code = NULL;
  }
  if (h->cd != (iconv_t)-1) {
    /* On failure, iconv_close() sets errno. */
    iconv_close(h->cd);
    h->cd = (iconv_t)-1;
    result = -1;
  }
  return result;
}

/*
 * iconv_cgo_del() deletes an instance of iconv_cgo_t.
 *
 * The caller must ensure that h is valid.
 *
 * On success, iconv_cgo_del() returns 0.
 * On failure, iconv_cgo_del() sets errno and returns -1.
 */
static int iconv_cgo_del(iconv_cgo_h h) {
  int result = iconv_cgo_fin(h);
  free(h);
  return result;
}

/*
 * iconv_cgo_set_suffix() checks h->to_code and sets h->suffix.
 */
static void iconv_cgo_set_suffix(iconv_cgo_h h) {
  char *suffix = strstr(h->to_code, "//");
  h->suffix = ICONV_CGO_SUFFIX_NONE;
  if (suffix == NULL) {
    return;
  }
  if (strcmp(suffix, ICONV_CGO_SUFFIX_TRANSLIT_STR) == 0) {
    h->suffix = ICONV_CGO_SUFFIX_TRANSLIT;
  } else if (strcmp(suffix, ICONV_CGO_SUFFIX_IGNORE_STR) == 0) {
    h->suffix = ICONV_CGO_SUFFIX_IGNORE;
  }
}

/*
 * iconv_cgo_alloc() allocates memory to store a conversion result.
 *
 * The caller must ensure that h is valid.
 *
 * On success, iconv_cgo_alloc() returns 0.
 * On failure, iconv_cgo_alloc() sets errno and returns -1.
 */
static int iconv_cgo_alloc(iconv_cgo_h h, size_t in_size) {
  char *buf;
  size_t buf_size;
  size_t out_size = in_size * 2;
  if (out_size < in_size) {
    errno = EINVAL;
    return -1;
  }
  if (out_size > ICONV_CGO_MAX_BUF_SIZE) {
    errno = ENOMEM;
    return -1;
  }
  if (h->buf_size >= out_size) {
    return 0;
  }
  buf_size = h->buf_size != 0 ? h->buf_size * 2 : ICONV_CGO_MIN_BUF_SIZE;
  while (buf_size < out_size) {
    buf_size *= 2;
  }
  buf = (char *)realloc(h->buf, buf_size);
  if (buf == NULL) {
    return -1;
  }
  h->buf = buf;
  h->buf_size = buf_size;
  return 0;
}

/*
 * iconv_cgo_realloc() reallocates memory to store a conversion result.
 *
 * The caller must ensure that h is valid.
 *
 * On success, iconv_cgo_realloc() returns 0.
 * On failure, iconv_cgo_realloc() sets errno and returns -1.
 */
static int iconv_cgo_realloc(iconv_cgo_h h) {
  char *buf;
  size_t buf_size;
  if (h->buf_size >= ICONV_CGO_MAX_BUF_SIZE) {
    errno = ENOMEM;
    return -1;
  }
  buf_size = h->buf_size * 2;
  buf = (char *)realloc(h->buf, buf_size);
  if (buf == NULL) {
    return -1;
  }
  h->buf = buf;
  h->buf_size = buf_size;
  return 0;
}

/*
 * iconv_cgo_conv_last() flushes out the remaining.
 *
 * The caller must ensure that arguments are valid.
 *
 * On success, iconv_cgo_conv_last() returns 0.
 * On failure, iconv_cgo_conv_last() sets errno and returns -1.
 */
static int iconv_cgo_conv_last(iconv_cgo_h h, char **out, size_t *out_size) {
  /* On failure, iconv() sets errno and returns (size_t)-1. */
  if (iconv(h->cd, NULL, NULL, out, out_size) != (size_t)-1) {
    return 0;
  }
  if (errno == E2BIG) {
    /* Extend an internal buffer and output the remaining. */
    size_t offset = h->buf_size - *out_size;
    errno = 0;
    if (iconv_cgo_realloc(h) == -1) {
      return -1;
    }
    *out = h->buf + offset;
    *out_size = h->buf_size - offset;
    if (iconv(h->cd, NULL, NULL, out, out_size) != (size_t)-1) {
      return 0;
    }
  }
  return -1;
}

/*
 * iconv_cgo_conv_core() performs character set converion.
 *
 * The caller must ensure that arguments are valid.
 *
 * On success, iconv_cgo_conv_core() returns 0.
 * On failure, iconv_cgo_conv_core() sets errno and returns -1.
 */
static int iconv_cgo_conv_core(iconv_cgo_h h, char *in, size_t in_size,
                               char **out, size_t *out_size) {
  int result = -1;
  char *in_left = in;
  char *out_left = h->buf;
  size_t in_size_left = in_size;
  size_t out_size_left = h->buf_size;
  while (in_size_left != 0) {
    /* On failure, iconv() sets errno and returns (size_t)-1. */
    if (iconv(h->cd, &in_left, &in_size_left, &out_left, &out_size_left) !=
        (size_t)-1) {
      result = iconv_cgo_conv_last(h, &out_left, &out_size_left);
      break;
    }
    if (errno == E2BIG) {
      /* Extend an internal buffer and convert the remaining. */
      size_t offset = h->buf_size - out_size_left;
      errno = 0;
      if (iconv_cgo_realloc(h) == -1) {
        break;
      }
      out_left = h->buf + offset;
      out_size_left = h->buf_size - offset;
      continue;
    }
    if (h->suffix != ICONV_CGO_SUFFIX_IGNORE) {
      break;
    }
    if (errno == EILSEQ) {
      /* Skip an invalid multibyte sequence. */
      errno = 0;
      in_left++;
      in_size_left--;
    } else if (errno == EINVAL) {
      /* Ignore an incomplete multibyte sequence. */
      errno = 0;
      result = iconv_cgo_conv_last(h, &out_left, &out_size_left);
      break;
    } else {
      break;
    }
  }
  *out = h->buf;
  *out_size = h->buf_size - out_size_left;
  return result;
}

/*
 * =====
 *  API
 * =====
 */

iconv_cgo_h iconv_cgo_open(char *to_code, char *from_code) {
  iconv_cgo_h h;
  if (to_code == NULL || from_code == NULL) {
    errno = EINVAL;
    return NULL;
  }
  h = iconv_cgo_new();
  if (h == NULL) {
    return NULL;
  }
  h->cd = iconv_open(to_code, from_code);
  if (h->cd == (iconv_t)-1) {
    /* On failure, iconv_open() sets errno. */
    return NULL;
  }
  h->to_code = to_code;
  h->from_code = from_code;
  iconv_cgo_set_suffix(h);
  return h;
}

int iconv_cgo_close(iconv_cgo_h h) {
  if (h == NULL) {
    errno = EINVAL;
    return -1;
  }
  return iconv_cgo_del(h);
}

const char *iconv_cgo_get_to_code(iconv_cgo_h h) {
  if (h == NULL || h->to_code == NULL) {
    return "";
  }
  return h->from_code;
}

const char *iconv_cgo_get_from_code(iconv_cgo_h h) {
  if (h == NULL || h->from_code == NULL) {
    return "";
  }
  return h->from_code;
}

int iconv_cgo_conv(iconv_cgo_h h, char *in, size_t in_size,
                   char **out, size_t *out_size) {
  if (h == NULL || h->cd == (iconv_t)-1 || (in == NULL && in_size != 0) ||
      (out == NULL || out_size == NULL)) {
    errno = EINVAL;
    return -1;
  }
  *out = "";
  *out_size = 0;
  if (in_size == 0) {
    return 0;
  }
  if (h->buf != NULL) {
    if (iconv(h->cd, NULL, NULL, NULL, NULL) == (size_t)-1) {
      return -1;
    }
  }
  if (iconv_cgo_alloc(h, in_size) == -1) {
    return -1;
  }
  return iconv_cgo_conv_core(h, in, in_size, out, out_size);
}
