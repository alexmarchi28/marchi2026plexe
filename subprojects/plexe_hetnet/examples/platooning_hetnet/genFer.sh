./run -u Cmdenv -c InterferenceScenarioNoGui -r 0
cd analysis
make InterferenceScenarioFER.Rdata InterferenceScenarioEvents.Rdata InterferenceScenarioDynamics.Rdata InterferenceScenarioHandovers.Rdata
cd dynamics-plots
Rscript plot-fer.R
