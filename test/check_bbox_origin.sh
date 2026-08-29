#!/bin/sh

set -eu

ACTTOOL=$1
base=runs/bbox-origin-base
offset=runs/bbox-origin-offset
invalid=runs/bbox-origin-invalid
trap 'rm -f "$base".* "$offset".* "$invalid".* _0_0cell_0_0g0x0.rect welltap_svt.rect' EXIT

rm -f "$base".* "$offset".* "$invalid".*
"$ACTTOOL" -cnf=m.conf -p 'test<>' -c cells.act \
  -B 100,100 -X 0 -Y 0 -o "$base" 0.act >/dev/null 2>"$base.stderr"
"$ACTTOOL" -cnf=m.conf -p 'test<>' -c cells.act \
  -B 100,100 -X 600 -Y 900 -o "$offset" 0.act >/dev/null 2>"$offset.stderr"

parse_diearea()
{
    sed -n 's/^DIEAREA ( \([-0-9][0-9]*\) \([-0-9][0-9]*\) ) ( \([-0-9][0-9]*\) \([-0-9][0-9]*\) ) ;$/\1 \2 \3 \4/p' "$1"
}

set -- $(parse_diearea "$base.def")
base_llx=$1
base_lly=$2
base_urx=$3
base_ury=$4
set -- $(parse_diearea "$offset.def")
offset_llx=$1
offset_lly=$2
offset_urx=$3
offset_ury=$4

if [ "$base_llx" -ne 0 ] || [ "$base_lly" -ne 0 ]; then
    echo "bounding-box origin fixture did not preserve requested base origin" >&2
    exit 1
fi
if [ "$offset_llx" -ne 600 ] || [ "$offset_lly" -ne 900 ]; then
    echo "bounding-box origin fixture returned ($offset_llx,$offset_lly)" >&2
    exit 1
fi
if [ $((base_urx - base_llx)) -ne $((offset_urx - offset_llx)) ] || \
   [ $((base_ury - base_lly)) -ne $((offset_ury - offset_lly)) ]; then
    echo "bounding-box offset changed snapped dimensions" >&2
    exit 1
fi

if "$ACTTOOL" -cnf=m.conf -p 'test<>' -c cells.act \
  -X 600 -o "$invalid" 0.act >/dev/null 2>"$invalid.stderr"; then
    echo "bounding-box origin options were accepted without -B" >&2
    exit 1
fi

echo "bounding-box origin fixture passed: origin and snapped dimensions are stable"
