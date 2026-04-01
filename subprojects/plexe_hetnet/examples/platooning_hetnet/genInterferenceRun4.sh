#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$SCRIPT_DIR/results"
ANALYSIS_DIR="$SCRIPT_DIR/analysis"
PLOT_SCRIPT="$ANALYSIS_DIR/dynamics-plots/plot-single-interference-run.R"

RUN_ID=4
SCENARIO_ID=8
TEMP_LEADER=0
RESULT_BASENAME="InterferenceScenario_${SCENARIO_ID}_${TEMP_LEADER}_${RUN_ID}"
WORK_DIR="$RESULTS_DIR/.interference-run-${RUN_ID}"
PLOTS_DIR="$ANALYSIS_DIR/dynamics-plots/interference-run-${RUN_ID}"

cleanup() {
    rm -rf "$WORK_DIR"
}

trap cleanup EXIT

mkdir -p "$RESULTS_DIR" "$PLOTS_DIR"
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"

echo "Running InterferenceScenarioNoGui, run ${RUN_ID}..."
if [[ "${SKIP_SIM:-0}" != "1" ]]; then
    (
        cd "$SCRIPT_DIR"
        ./run -u Cmdenv -c InterferenceScenarioNoGui -r "$RUN_ID"
    )
else
    echo "SKIP_SIM=1, reusing existing simulation outputs."
fi

VEC_FILE="$RESULTS_DIR/$RESULT_BASENAME.vec"
VCI_FILE="$RESULTS_DIR/$RESULT_BASENAME.vci"

if [[ ! -f "$VEC_FILE" ]]; then
    echo "Missing vector file: $VEC_FILE" >&2
    exit 1
fi

if [[ ! -f "$VCI_FILE" ]]; then
    echo "Indexing ${RESULT_BASENAME}.vec..."
    opp_scavetool index "$VEC_FILE"
fi

ln -sf "$VEC_FILE" "$WORK_DIR/$RESULT_BASENAME.vec"
ln -sf "$VCI_FILE" "$WORK_DIR/$RESULT_BASENAME.vci"

echo "Parsing only run ${RUN_ID}..."
(
    cd "$ANALYSIS_DIR"
    Rscript generic-parser.R "$WORK_DIR/$RESULT_BASENAME.vec" map-config events iev Rdata
    Rscript generic-parser.R "$WORK_DIR/$RESULT_BASENAME.vec" map-config dynamics idn Rdata
    Rscript generic-parser.R "$WORK_DIR/$RESULT_BASENAME.vec" map-config handover iho Rdata
    Rscript generic-parser.R "$WORK_DIR/$RESULT_BASENAME.vec" map-config fer inw Rdata

    Rscript merge.R "$WORK_DIR/" "iev.InterferenceScenario" "InterferenceScenarioEvents.Rdata" map-config events Rdata
    Rscript merge.R "$WORK_DIR/" "idn.InterferenceScenario" "InterferenceScenarioDynamics.Rdata" map-config dynamics Rdata
    Rscript merge.R "$WORK_DIR/" "iho.InterferenceScenario" "InterferenceScenarioHandovers.Rdata" map-config handover Rdata
    Rscript merge.R "$WORK_DIR/" "inw.InterferenceScenario" "InterferenceScenarioFER.Rdata" map-config fer Rdata
)

echo "Generating PDFs in $PLOTS_DIR..."
Rscript "$PLOT_SCRIPT" "$WORK_DIR" "$PLOTS_DIR" "$RUN_ID"

echo "Done."
echo "Created:"
echo "  $PLOTS_DIR/interference-distance-run-${RUN_ID}.pdf"
echo "  $PLOTS_DIR/interference-acceleration-run-${RUN_ID}.pdf"
echo "  $PLOTS_DIR/interference-leader-fer-run-${RUN_ID}.pdf"
echo "  $PLOTS_DIR/interference-front-fer-run-${RUN_ID}.pdf"
