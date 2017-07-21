package iconv

import (
	"bytes"
	"reflect"
	"testing"
)

// TestConv tests a simple conversion.
func TestSimple(t *testing.T) {
	utf8ToCP932, err := Open("cp932", "utf-8")
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer func() {
		if err := utf8ToCP932.Close(); err != nil {
			t.Fatalf("Close failed: %v", err)
		}
	}()

	utf8Str := "あいうえお"
	cp932, err := utf8ToCP932.Conv([]byte(utf8Str))
	if err != nil {
		t.Fatalf("Conv failed: utf8_str = %#v: %v", utf8Str, err)
	}

	cp932ToUTF8, err := Open("UTF-8", "CP932")
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer func() {
		if err := cp932ToUTF8.Close(); err != nil {
			t.Fatalf("Close failed: %v", err)
		}
	}()

	utf8, err := cp932ToUTF8.Conv(cp932)
	if err != nil {
		t.Fatalf("Conv failed: utf8_str = %#v: %v", utf8Str, err)
	}

	if string(utf8) != utf8Str {
		t.Fatalf("unreversible conversion: dest = %#v, src = %#v",
			utf8Str, string(utf8))
	}
}

// TestSafe tests the safeness of Conv.
func TestSafe(t *testing.T) {
	h, err := Open("CP932", "UTF-8")
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer func() {
		if err := h.Close(); err != nil {
			t.Fatalf("Close failed: %v", err)
		}
	}()

	srcA := []byte("あいうえお")
	srcB := []byte("かきくけこ")
	destA, err := h.Conv(srcA)
	if err != nil {
		t.Fatalf("Conv failed: srcA = %#v: %v", string(srcA), err)
	}
	destB, err := h.Conv(srcB)
	if err != nil {
		t.Fatalf("Conv failed: srcB = %#v: %v", string(srcB), err)
	}
	if reflect.DeepEqual(destA, destB) {
		t.Fatalf("unexpected match")
	}
}

// TestUnsafe tests the unsafeness of ConvUnsafe.
func TestUnsafe(t *testing.T) {
	h, err := Open("CP932", "UTF-8")
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer func() {
		if err := h.Close(); err != nil {
			t.Fatalf("Close failed: %v", err)
		}
	}()

	srcA := []byte("あいうえお")
	srcB := []byte("かきくけこ")
	destA, err := h.ConvUnsafe(srcA)
	if err != nil {
		t.Fatalf("ConvUnsafe failed: srcA = %#v: %v", string(srcA), err)
	}
	destB, err := h.ConvUnsafe(srcB)
	if err != nil {
		t.Fatalf("ConvUnsafe failed: srcB = %#v: %v", string(srcB), err)
	}
	if !reflect.DeepEqual(destA, destB) {
		t.Fatalf("unexpected unmatch")
	}
}

// TestLong tests conversion of long sequences.
func TestLong(t *testing.T) {
	h, err := Open("CP932", "UTF-8")
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer func() {
		if err := h.Close(); err != nil {
			t.Fatalf("Close failed: %v", err)
		}
	}()

	var buf bytes.Buffer
	for i := 0; i < 128; i++ {
		for j := 0; j < i; j++ {
			buf.WriteString("あいうえお")
		}
		src := buf.Bytes()
		_, err := h.ConvUnsafe(src)
		if err != nil {
			t.Fatalf("ConvUnsafe failed: len(src) = %#v: %v", len(src), err)
		}
	}
}

// TestInvalid tests an invalid sequence.
func TestInvalid(t *testing.T) {
	h, err := Open("UTF-8", "CP932")
	if err != nil {
		t.Skipf("Open failed: %v", err)
	}
	defer func() {
		if err := h.Close(); err != nil {
			t.Fatalf("Close failed: %v", err)
		}
	}()

	src := []byte{133, 150}
	_, err = h.ConvUnsafe(src)
	if err == nil {
		t.Fatalf("ConvUnsafe wrongly succeeded: src = %#v")
	}
}

// TestIgnore tests //IGNORE.
func TestIgnore(t *testing.T) {
	h, err := Open("UTF-8//IGNORE", "CP932")
	if err != nil {
		t.Skipf("Open failed: %v", err)
	}
	defer func() {
		if err := h.Close(); err != nil {
			t.Fatalf("Close failed: %v", err)
		}
	}()

	src := []byte{133, 150}
	_, err = h.ConvUnsafe(src)
	if err != nil {
		t.Fatalf("ConvUnsafe failed: src = %#v: %v", src, err)
	}
}
