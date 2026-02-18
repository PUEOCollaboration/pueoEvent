#! /bin/sh

if [ "$#" -lt 3 ]; then
  echo "Usage:  pueo-convert-run rawdir rootdir run"
  exit 1;
fi

RUN=$3
INDIR=$1/run`printf %04d $RUN`
OUTDIR=$2/run$RUN
shift 3
EXTRA_ARG="$@"

mkdir -p $OUTDIR

pueo-convert "$EXTRA_ARG" header $OUTDIR/headFile$RUN.root $INDIR/
pueo-convert "$EXTRA_ARG" waveform $OUTDIR/eventFile$RUN.root $INDIR/
