// Copyright 2018 The OPA Authors.  All rights reserved.
// Use of this source code is governed by an Apache2
// license that can be found in the LICENSE file.

package ir

import (
	"fmt"
	"io"
	"strings"
)

// Pretty writes a human-readable representation of an IR object to w.
func Pretty(w io.Writer, x any) error {

	pp := &prettyPrinter{
		depth: -1,
		w:     w,
	}
	return Walk(pp, x)
}

type prettyPrinter struct {
	depth int
	w     io.Writer
}

func (pp *prettyPrinter) Before(_ any) {
	pp.depth++
}

func (pp *prettyPrinter) After(_ any) {
	pp.depth--
}

func (pp *prettyPrinter) Visit(x any) (Visitor, error) {
	pp.writeIndent("%T %+v", x, x)
	return pp, nil
}

func (pp *prettyPrinter) writeIndent(f string, a ...any) {
	pad := strings.Repeat("| ", pp.depth)
	fmt.Fprintf(pp.w, pad+f+"\n", a...)
}
