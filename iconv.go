// Package iconv provides an interface for character set conversion with iconv.
package iconv

// #cgo darwin LDFLAGS: -liconv
// #include <limits.h>
// #include <stdlib.h>
// #include "iconv_cgo.h"
import "C"

import (
	"errors"
	"reflect"
	"unsafe"
)

// Handle is a handle for character set conversion.
//
// Handle is not thread-safe and each thread should use each Handle.
type Handle struct {
	h C.iconv_cgo_h
}

// Open opens a handle for conversion from fromCode to toCode.
//
// The following is an example of opening a handle for coversion from CP932 to
// UTF-8.
//
//  h, err := iconv.Open("UTF-8", "CP932")
//  if err != nil {
//    log.Fatalf("iconv.Open failed: %v", err)
//  }
//  defer h.Close()
//
// "//TRANSLIT" and "//IGNORE" may be supported as special suffixes of toCode.
// It depends on iconv.
func Open(toCode, fromCode string) (*Handle, error) {
	cToCode := C.CString(toCode)
	cFromCode := C.CString(fromCode)
	h, err := C.iconv_cgo_open(cToCode, cFromCode)
	if h == nil {
		C.free(cToCode)
		C.free(cFromCode)
		return nil, err
	}
	return &Handle{h}, nil
}

// Close closes a handle.
//
// Note that a slice acquired by ConvUnsafe gets lost.
func (h *Handle) Close() error {
	if h == nil || h.h == nil {
		return errors.New("invalid handle")
	}
	result, err := C.iconv_cgo_close(h.h)
	h.h = nil
	if result == -1 {
		return err
	}
	return nil
}

// ConvUnsafe performs character set conversion.
//
// Note that a slice acquired by ConvUnsafe will be broken by the next call of
// Conv, ConvUnsafe and Close with the same handle.
// This is because ConvUnsafe writes a result into an internal C buffer and
// returns a slice which refers to it.
func (h *Handle) ConvUnsafe(src []byte) ([]byte, error) {
	if h == nil || h.h == nil {
		return nil, errors.New("invalid handle")
	}
	if len(src) == 0 {
		return []byte{}, nil
	}
	in := (*C.char)(unsafe.Pointer(&src[0]))
	inSize := C.size_t(len(src))
	var out *C.char
	var outSize C.size_t
	result, err := C.iconv_cgo_conv(h.h, in, inSize, &out, &outSize)
	if result == -1 {
		return nil, err
	}
	var dest []byte
	destHeader := (*reflect.SliceHeader)(unsafe.Pointer(&dest))
	destHeader.Data = uintptr(unsafe.Pointer(out))
	destHeader.Len = int(outSize)
	destHeader.Cap = int(outSize)
	return dest, nil
}

// Conv performs character set conversion.
//
// Conv returns a copy of a slice acquired by ConvUnsafe so that it will not be
// broken by the next call of Conv, ConvUnsafe and Close with the same handle.
//
// Note that a slice acquired by ConvUnsafe gets lost.
func (h *Handle) Conv(src []byte) ([]byte, error) {
	unsafeDest, err := h.ConvUnsafe(src)
	if err != nil {
		return nil, err
	}
	dest := make([]byte, len(unsafeDest))
	copy(dest, unsafeDest)
	return dest, nil
}
