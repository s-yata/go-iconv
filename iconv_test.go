package iconv

import (
	"bytes"
	"reflect"
	"testing"
)

// TestConv tests a simple conversion.
func TestSimple(t *testing.T) {
	utf8_to_cp932, err := Open("cp932", "utf-8")
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer func() {
		if err := utf8_to_cp932.Close(); err != nil {
			t.Fatalf("Close failed: %v", err)
		}
	}()

	utf8_str := "あいうえお"
	cp932, err := utf8_to_cp932.Conv([]byte(utf8_str))
	if err != nil {
		t.Fatalf("Conv failed: utf8_str = %#v: %v", utf8_str, err)
	}

	cp932_to_utf8, err := Open("UTF-8", "CP932")
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer func() {
		if err := cp932_to_utf8.Close(); err != nil {
			t.Fatalf("Close failed: %v", err)
		}
	}()

	utf8, err := cp932_to_utf8.Conv(cp932)
	if err != nil {
		t.Fatalf("Conv failed: utf8_str = %#v: %v", utf8_str, err)
	}

	if string(utf8) != utf8_str {
		t.Fatalf("unreversible conversion: dest = %#v, src = %#v",
			utf8_str, string(utf8))
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
